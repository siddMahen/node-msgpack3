#define BUILDING_NODE_EXTENSION

#include <node.h>
#include <string.h>
#include <v8stdint.h>
#include <node_buffer.h>

enum MSG_PACK_TYPE {
    POS_FIXNUM = 0x00,
    MAP_FIX    = 0x80,
    ARRAY_FIX  = 0x90,
    RAW_FIX    = 0xa0,
    NIL        = 0xc0,
    FALSE      = 0xc2,
    TRUE       = 0xc3,
    FLOAT      = 0xca, /* Not supported */
    DOUBLE     = 0xcb,
    UINT_8     = 0xcc,
    UINT_16    = 0xcd,
    UINT_32    = 0xce,
    UINT_64    = 0xcf, /* Not supported */
    INT_8      = 0xd0,
    INT_16     = 0xd1,
    INT_32     = 0xd2,
    INT_64     = 0xd3, /* Not supported */
    RAW_16     = 0xda,
    RAW_32     = 0xdb,
    ARRAY_16   = 0xdc,
    ARRAY_32   = 0xdd,
    MAP_16     = 0xde,
    MAP_32     = 0xdf,
    NEG_FIXNUM = 0xe0
};

using namespace v8;
using namespace node;

static Handle<Value> Pack(const Arguments& args);
static Handle<Value> Unpack(const Arguments& args);

static const uint16_t read2Bytes(const uint8_t *buf, const uint32_t offset){
    uint16_t val = 0x0000;
    val |= buf[0 + offset] << 8;
    val |= buf[1 + offset];

    return val;
}

static const uint32_t read4Bytes(const uint8_t *buf, const uint32_t offset){
    uint32_t val = 0x000000;
    val |= buf[0 + offset] << 24;
    val |= buf[1 + offset] << 16;
    val |= buf[2 + offset] << 8;
    val |= buf[3 + offset];

    return val;
}

static const uint8_t sizeof_logic(const uint8_t arg) {
  switch(arg){
    case 0xc0:
    case 0xc2:
    case 0xc3:
      return 1;
      break;
  }

  return 0;
}

static const uint8_t sizeof_number(const uint8_t arg) {
  switch(arg){
    case POS_FIXNUM:
    case NEG_FIXNUM:
      return 1;
      break;
    case UINT_8:
    case INT_8:
      return 2;
      break;
    case UINT_16:
    case INT_16:
      return 3;
      break;
    case UINT_32:
    case INT_32:
      return 5;
      break;
    case DOUBLE:
      return 9;
      break;
  }

  return 0;
}

static const uint8_t sizeof_raw(const uint8_t arg){
  switch(arg){
    case RAW_FIX:
      return 1;
      break;
    case RAW_16:
      return 3;
      break;
    case RAW_32:
      return 5;
      break;
  }

  return 0;
}

static const uint8_t sizeof_array(const uint8_t arg){
  switch(arg){
    case ARRAY_FIX:
      return 1;
      break;
    case ARRAY_16:
      return 3;
      break;
    case ARRAY_32:
      return 5;
      break;
  }

  return 0;
}

static const uint8_t sizeof_map(const uint8_t arg){
  switch(arg){
    case MAP_FIX:
      return 1;
      break;
    case MAP_16:
      return 3;
      break;
    case MAP_32:
      return 5;
      break;
  }

  return 0;
}

static const uint8_t type(const Local<Value>& arg){
    HandleScope scope;
    if(arg->IsString()){
        const int len = arg->ToString()->Utf8Length();
        if(len < 32){
            return RAW_FIX;
        }else if(len < 0x10000){
            return RAW_16;
        }else{
            return RAW_32;
        }
    }else if(arg->IsArray()){
        Local<Array> array = Local<Array>::Cast(arg);
        const uint32_t len = array->Length();
        if(len < 16){
            return ARRAY_FIX;
        }else if(len < 0x10000){
            return ARRAY_16;
        }else{
            return ARRAY_32;
        }
    }else if(arg->IsObject()){
        Local<Object> obj = arg->ToObject();
        const uint32_t size = obj->GetOwnPropertyNames()->Length();
        if(size < 16){
            return MAP_FIX;
        }else if(size < 0x10000){
            return MAP_16;
        }else{
            return MAP_32;
        }
    }else if(arg->IsNumber()){
        if(arg->IsUint32()){
            const uint32_t val = arg->Uint32Value();
            if(val < MAP_FIX){
                return POS_FIXNUM;
            }else if(val < 0x100){
                return UINT_8;
            }else if(val < 0x10000){
                return UINT_16;
            }else{
                return UINT_32;
            }
        }else if(arg->IsInt32()){
            const int32_t val = arg->Int32Value();
            if(val >= -32){
                return NEG_FIXNUM;
            }else if(val > -0x80){
                return INT_8;
            }else if(val > -0x8000){
                return INT_16;
            }else{
                return INT_32;
            }
        }else{
            return DOUBLE;
        }
    }else if(arg->IsBoolean()){
        if(arg->BooleanValue() == true){
            return TRUE;
        }else{
            return FALSE;
        }
    }else if(arg->IsNull() || arg->IsUndefined()){
        return NIL;
    }

    ThrowException(Exception::TypeError(String::New("Unknow MessagePack Type")));
    return 0;
}

static const uint8_t reverseType(const uint8_t obj){
    if(obj <= 0x7f){
        return POS_FIXNUM;
    }else if(obj <= 0x8f){
        return MAP_FIX;
    }else if(obj <= 0x9f){
        return ARRAY_FIX;
    }else if(obj <= 0xbf){
        return RAW_FIX;
    }else if(obj >= 0xe0){
        return NEG_FIXNUM;
    }

    return obj;
}

static const uint32_t sizeof_obj(const Local<Value>& obj){
    HandleScope scope;

    uint32_t disc = type(obj);
    uint32_t size = (sizeof_number(disc) ? sizeof_number(disc) : sizeof_logic(disc));

    if(!size){
        uint8_t psize;
        if((psize = sizeof_raw(disc))){
            size += (obj->ToString()->Utf8Length()) + psize;
        }else if((psize = sizeof_array(disc))){
            Handle<Array> array = Handle<Array>::Cast(obj);

            for(uint32_t i = 0; i < array->Length(); i++){
                size += sizeof_obj(array->Get(i));
            }

            size += psize;
        }else if((psize = sizeof_map(disc))){
            Local<Object> object = obj->ToObject();
            Local<Array> keys = object->GetOwnPropertyNames();

            for(uint32_t i = 0; i < keys->Length(); i++){
                Local<Value> key = keys->Get(i);
                size += sizeof_obj(key);
                size += sizeof_obj(object->Get(key));
            }

            size += psize;
        }
    }

    return size;
}

static const uint32_t pack(const Local<Value>& obj, uint8_t *data, const bool needSize){
    HandleScope scope;

    uint8_t disc = type(obj);
    uint8_t ssize;

    uint32_t offset = 0;

    if((ssize = sizeof_map(disc))){
        Local<Object> node = obj->ToObject();
        Local<Array> keys = node->GetOwnPropertyNames();
        uint32_t size = keys->Length();

        uint8_t *d;

        switch(disc){
            case MAP_FIX:{
                uint8_t len[] = { (disc | size) };
                d = len;
                break;
            }
            case MAP_16:{
                uint8_t len[] = { disc, (size >> 8), (size & 0xff) };
                d = len;
                break;
            }
            case MAP_32:{
                uint8_t len[] = { disc, (size >> 24), (size >> 16) & 0xff,
                    (size >> 8) & 0xff, (size & 0xff) };
                d = len;
                break;
            }
            default:
                ThrowException(Exception::TypeError(String::New("Not Implemented Yet")));
                break;
        }

        // Add asserts here to make sure that the offset is not over the length
        // of the initialized data and that ssize won't cause it to go over

        memcpy(data + offset, d, ssize);
        offset += ssize;

        for(uint32_t i = 0; i < keys->Length(); i++){
            Local<Value> key = keys->Get(i);
            Local<Value> val = node->Get(key);

            offset += pack(key, data + offset, true);
            offset += pack(val, data + offset, true);
        }
    }else if((ssize = sizeof_array(disc))){
        Handle<Array> node = Handle<Array>::Cast(obj);
        uint32_t size = node->Length();

        uint8_t *d;

        switch(disc){
            case ARRAY_FIX:{
                uint8_t len[] = { (disc | size) };
                d = len;
                break;
            }
            case ARRAY_16:{
                uint8_t len[] = { disc, (size >> 8), (size & 0xff) };
                d = len;
                break;
            }
            case ARRAY_32:{
                uint8_t len[] = { disc, (size >> 24), (size >> 16) & 0xff,
                    (size >> 8) & 0xff, (size & 0xff) };
                d = len;
                break;
            }
            default:
                ThrowException(Exception::TypeError(String::New("Not Implemented Yet")));
                break;
        }

        memcpy(data + offset, d, ssize);
        offset += ssize;

        for(uint32_t i = 0; i < size; i++){
            Local<Value> val = node->Get(i);
            offset += pack(val, data + offset, true);
        }
    }else if((ssize = sizeof_raw(disc))){
        Local<String> node = obj->ToString();
        uint32_t size = node->Utf8Length();

        uint8_t *d;

        switch(disc){
            case RAW_FIX:{
                uint8_t len[] = { (disc | size) };
                d = len;
                break;
            }
            case RAW_16:{
                uint8_t len[] = { disc, (size >> 8), (size & 0xff) };
                d = len;
                break;
            }
            case RAW_32:{
                uint8_t len[] = { disc, (size >> 24), (size >> 16) & 0xff,
                    (size >> 8) & 0xff, (size & 0xff) };
                d = len;
                break;
            }
            default:
                ThrowException(Exception::TypeError(String::New("Not Implemented Yet")));
                break;
        }

        memcpy(data + offset, d, ssize);
        offset += ssize;

        node->WriteUtf8((char*)data + offset, size);
    }else if((ssize = sizeof_number(disc))){
        Local<Number> node = obj->ToNumber();

        uint8_t *d;

        switch(disc){
            case POS_FIXNUM:{
                uint8_t len[] = { node->Uint32Value() & 0xff };
                d = len;
                break;
            }
            case NEG_FIXNUM:{
                uint8_t len[] = { node->Int32Value() & 0xff };
                d = len;
                break;
            }
            case UINT_8:{
                uint32_t num = node->Uint32Value();
                uint8_t len[] = { disc, (num & 0xff) };
                d = len;
                break;
            }
            case INT_8:{
                int32_t num = node->Int32Value();
                uint8_t len[] = { disc, (num & 0xff) };
                d = len;
                break;
            }
            case UINT_16:{
                uint32_t num = node->Uint32Value();
                uint8_t len[] = { disc, (num >> 8) & 0xff, num & 0xff };
                d = len;
                break;
            }
            case INT_16:{
                int32_t num = node->Int32Value();
                uint8_t len[] = { disc, (num >> 8) & 0xff, num & 0xff };
                d = len;
                break;
            }
            case UINT_32:{
                uint32_t num = node->Uint32Value();
                uint8_t len[] = { disc, num >> 24, (num >> 16) & 0xff,
                    (num >> 8) & 0xff, num & 0xff };
                d = len;
                break;
            }
            case INT_32:{
                int32_t num = node->Int32Value();
                uint8_t len[] = { disc, num >> 24, (num >> 16) & 0xff,
                    (num >> 8) & 0xff, num & 0xff };
                d = len;
                break;
            }
            case DOUBLE:{
                double num = node->Value();
                const uint8_t *c = reinterpret_cast<uint8_t *>(&num);
                uint8_t len[] = { disc, c[7], c[6], c[5], c[4], c[3], c[2], c[1], c[0] };
                d = len;
                break;
            }
            default:
                ThrowException(Exception::TypeError(String::New("Not Implemented Yet")));
                break;
        }

        memcpy(data + offset, d, ssize);
        offset += ssize;
    }else if((ssize = sizeof_logic(disc))){
        uint8_t *d;
        uint8_t len[] = { disc & 0xff };

        d = len;

        memcpy(data + offset, d, ssize);
        offset += ssize;
    }

    return (needSize == true ? sizeof_obj(obj) : 0);
}

static void buffer_free(char *data, void *hint){
    delete [] data;
}

static Handle<Value> Pack(const Arguments& args){
    HandleScope scope;
    Local<Value> obj = args[0];

    const uint32_t size = sizeof_obj(obj);
    uint8_t *data = new uint8_t[size];
    pack(obj, data, false);

    return scope.Close((Buffer::New((char *)data, (size_t)size, &buffer_free, NULL))->handle_);
}

static Handle<Array> sizeof_variable(const uint8_t *buf, const uint8_t type, uint32_t ssize){
    HandleScope scope;

    Local<Array> lengths = Array::New(0);
    uint32_t offset = 0;

    switch(type){
        case ARRAY_32:
        case MAP_32:
            offset += 3;
            break;
        case ARRAY_16:
        case MAP_16:
            offset += 2;
            break;
        case ARRAY_FIX:
        case MAP_FIX:
            offset += 1;
            break;
    }

    if(sizeof_map(type)){
        ssize *= 2;
    }

    for(uint32_t i = 0; i < ssize; i++){
        const uint8_t marker = reverseType(buf[offset]);
        uint32_t size = (sizeof_number(marker) ? sizeof_number(marker) : sizeof_logic(marker));

        if(!size){
            uint8_t psize;
            if((psize = sizeof_raw(marker))){
                uint32_t len;

                switch(marker){
                    case RAW_FIX:
                        len = buf[offset] ^ RAW_FIX;
                        break;
                    case RAW_16:
                        len = read2Bytes(buf, offset + 1);
                        break;
                    case RAW_32:
                        len = read4Bytes(buf, offset + 1);
                        break;
                }

                size += len + psize;
            }else if((psize = (sizeof_array(marker) || sizeof_map(marker)))){
                uint32_t len;

                switch(marker){
                    case ARRAY_FIX:
                    case MAP_FIX:
                        len = buf[offset] ^ marker;
                        break;
                    case ARRAY_16:
                    case MAP_16:
                        len = read2Bytes(buf, offset + 1);
                        break;
                    case ARRAY_32:
                    case MAP_32:
                        len = read4Bytes(buf, offset + 1);
                        break;
                }

                Handle<Array> array = sizeof_variable(&(buf[offset]), marker, len);

                for(uint32_t j = 0; j < array->Length(); j++){
                    size += array->Get(j)->Uint32Value();
                }

                size += psize;
            }
        }

        lengths->Set(i, Integer::NewFromUnsigned(size));
        offset += size;
    }

    return scope.Close(lengths);
}

static Handle<Value> unpack(const uint8_t *buf){
    HandleScope scope;

    const uint8_t disc = reverseType(buf[0]);
    uint8_t ssize;

    if((ssize = sizeof_map(disc))){
        Local<Object> map = Object::New();

        uint32_t len;
        uint32_t start = ssize;
        uint32_t end = start;

        switch(disc){
            case MAP_FIX:
                len = buf[0] ^ MAP_FIX;
                break;
            case MAP_16:
                len = read2Bytes(buf, 1);
                break;
            case MAP_32:
                len = read4Bytes(buf, 1);
                break;
        }

        Handle<Array> lengths = sizeof_variable(buf, disc, len);

        for(uint32_t i = 0; i < len*2; i+=2){
            end += lengths->Get(i)->Uint32Value();
            Handle<Value> key = unpack(buf + start);
            start = end;

            end += lengths->Get(i+1)->Uint32Value();
            Handle<Value> value = unpack(buf + start);
            start = end;

            map->Set(key, value);
        }

        return scope.Close(map);
    }else if((ssize = sizeof_array(disc))){
        Local<Array> array = Array::New(0);

        uint32_t len;
        uint32_t start = ssize;
        uint32_t end = start;

        switch(disc){
            case ARRAY_FIX:
                len = buf[0] ^ ARRAY_FIX;
                break;
            case ARRAY_16:
                len = read2Bytes(buf, 1);
                break;
            case ARRAY_32:
                len = read4Bytes(buf, 1);
                break;
        }

        Handle<Array> lengths = sizeof_variable(buf, disc, len);

        for(uint32_t i = 0; i < len; i++){
            end += lengths->Get(i)->Uint32Value();
            array->Set(i, unpack(buf + start));
            start = end;
        }

        return scope.Close(array);
    }else if((ssize = sizeof_raw(disc))){
        uint32_t len;

        switch(disc){
            case RAW_FIX:
                len = buf[0] ^ RAW_FIX;
                break;
            case RAW_16:
                len = read2Bytes(buf, 1);
                break;
            case RAW_32:
                len = read4Bytes(buf, 1);
                break;
        }

        return scope.Close(String::New((char *)(buf + ssize), len));
    }else if((ssize = sizeof_number(disc))){
        switch(disc){
            case POS_FIXNUM:
                return scope.Close(Integer::NewFromUnsigned(buf[0]));
                break;
            case NEG_FIXNUM:
                return scope.Close(Integer::New((int8_t)buf[0]));
                break;
            case UINT_8:
                return scope.Close(Integer::NewFromUnsigned((uint8_t)buf[1]));
                break;
            case INT_8:
                return scope.Close(Integer::New((int8_t)buf[1]));
                break;
            case UINT_16:
                return scope.Close(Integer::NewFromUnsigned(read2Bytes(buf, 1)));
                break;
            case INT_16:
                return scope.Close(Integer::New((int16_t)read2Bytes(buf, 1)));
                break;
            case UINT_32:
                return scope.Close(Integer::NewFromUnsigned(read4Bytes(buf, 1)));
                break;
            case INT_32:
                return scope.Close(Integer::New((int32_t)read4Bytes(buf, 1)));
                break;
            case DOUBLE:{
                // TODO: Hacky, make this tidier
                double d;
                memcpy(&d, buf + 1, sizeof(double));
                const uint8_t *c = reinterpret_cast<uint8_t *>(&d);
                const uint8_t b[] = { c[7], c[6], c[5], c[4], c[3], c[2], c[1], c[0] };
                memcpy(&d, &b, sizeof(double));
                return scope.Close(Number::New(d));
                break;
            }
            default:
                ThrowException(Exception::TypeError(String::New("Unpack Error: Unknown number")));
                return scope.Close(Undefined());
                break;

        }
    }else if((ssize = sizeof_logic(disc))){
        if(disc == NIL){
            return scope.Close(Null());
        }else{
            return scope.Close(Boolean::New((disc == TRUE ? true : false)));
        }
    }

    ThrowException(Exception::TypeError(String::New("Unknown MessagePack object")));
    return scope.Close(Undefined());
}

static Handle<Value> Unpack(const Arguments& args){
    HandleScope scope;
    Local<Object> buf = args[0]->ToObject();

    if(!Buffer::HasInstance(buf)){
        ThrowException(Exception::TypeError(String::New("Requires a Buffer object")));
        return scope.Close(Undefined());
    }

    return scope.Close(unpack((uint8_t *)Buffer::Data(buf)));
}

void Init(Handle<Object> target) {
  target->Set(String::NewSymbol("pack"),
      FunctionTemplate::New(Pack)->GetFunction());
  target->Set(String::NewSymbol("unpack"),
      FunctionTemplate::New(Unpack)->GetFunction());
}

NODE_MODULE(msgpack3, Init)
