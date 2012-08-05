#define BUILDING_NODE_EXTENSION

#include <node.h>
#include <string.h>
#include <v8stdint.h>
#include <node_buffer.h>

enum MSG_PACK_TYPE {
    MSG_PACK_POS_FIXNUM = 0x00,
    MSG_PACK_MAP_FIX    = 0x80,
    MSG_PACK_ARRAY_FIX  = 0x90,
    MSG_PACK_RAW_FIX    = 0xa0,
    MSG_PACK_NIL        = 0xc0,
    MSG_PACK_FALSE      = 0xc2,
    MSG_PACK_TRUE       = 0xc3,
    MSG_PACK_FLOAT      = 0xca, /* Not supported */
    MSG_PACK_DOUBLE     = 0xcb,
    MSG_PACK_UINT_8     = 0xcc,
    MSG_PACK_UINT_16    = 0xcd,
    MSG_PACK_UINT_32    = 0xce,
    MSG_PACK_UINT_64    = 0xcf, /* Not supported */
    MSG_PACK_INT_8      = 0xd0,
    MSG_PACK_INT_16     = 0xd1,
    MSG_PACK_INT_32     = 0xd2,
    MSG_PACK_INT_64     = 0xd3, /* Not supported */
    MSG_PACK_RAW_16     = 0xda,
    MSG_PACK_RAW_32     = 0xdb,
    MSG_PACK_ARRAY_16   = 0xdc,
    MSG_PACK_ARRAY_32   = 0xdd,
    MSG_PACK_MAP_16     = 0xde,
    MSG_PACK_MAP_32     = 0xdf,
    MSG_PACK_NEG_FIXNUM = 0xe0
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
    default:
      return 0;
      break;
  }
}

static const uint8_t sizeof_number(const uint8_t arg) {
  switch(arg){
    case MSG_PACK_POS_FIXNUM:
    case MSG_PACK_NEG_FIXNUM:
      return 1;
      break;
    case MSG_PACK_UINT_8:
    case MSG_PACK_INT_8:
      return 2;
      break;
    case MSG_PACK_UINT_16:
    case MSG_PACK_INT_16:
      return 3;
      break;
    case MSG_PACK_UINT_32:
    case MSG_PACK_INT_32:
      return 5;
      break;
    case MSG_PACK_DOUBLE:
      return 9;
      break;
    default:
      return 0;
      break;
  }
}

static const uint8_t sizeof_raw(const uint8_t arg){
  switch(arg){
    case MSG_PACK_RAW_FIX:
      return 1;
      break;
    case MSG_PACK_RAW_16:
      return 3;
      break;
    case MSG_PACK_RAW_32:
      return 5;
      break;
    default:
      return 0;
      break;
  }
}

static const uint8_t sizeof_array(const uint8_t arg){
  switch(arg){
    case MSG_PACK_ARRAY_FIX:
      return 1;
      break;
    case MSG_PACK_ARRAY_16:
      return 3;
      break;
    case MSG_PACK_ARRAY_32:
      return 5;
      break;
    default:
      return 0;
      break;
  }
}

static const uint8_t sizeof_map(const uint8_t arg){
  switch(arg){
    case MSG_PACK_MAP_FIX:
      return 1;
      break;
    case MSG_PACK_MAP_16:
      return 3;
      break;
    case MSG_PACK_MAP_32:
      return 5;
      break;
    default:
      return 0;
      break;
  }
}

static const uint8_t type(const Local<Value>& arg){
    HandleScope scope;
    if(arg->IsString()){
        const int len = arg->ToString()->Utf8Length();
        if(len < 32){
            return MSG_PACK_RAW_FIX;
        }else if(len < 0x10000){
            return MSG_PACK_RAW_16;
        }else{
            return MSG_PACK_RAW_32;
        }
    }else if(arg->IsArray()){
        Local<Array> array = Local<Array>::Cast(arg);
        const uint32_t len = array->Length();
        if(len < 16){
            return MSG_PACK_ARRAY_FIX;
        }else if(len < 0x10000){
            return MSG_PACK_ARRAY_16;
        }else{
            return MSG_PACK_ARRAY_32;
        }
    }else if(arg->IsObject()){
        Local<Object> obj = arg->ToObject();
        const uint32_t size = obj->GetOwnPropertyNames()->Length();
        if(size < 16){
            return MSG_PACK_MAP_FIX;
        }else if(size < 0x10000){
            return MSG_PACK_MAP_16;
        }else{
            return MSG_PACK_MAP_32;
        }
    }else if(arg->IsNumber()){
        if(arg->IsUint32()){
            const uint32_t val = arg->Uint32Value();
            if(val < MSG_PACK_MAP_FIX){
                return MSG_PACK_POS_FIXNUM;
            }else if(val < 0x100){
                return MSG_PACK_UINT_8;
            }else if(val < 0x10000){
                return MSG_PACK_UINT_16;
            }else{
                return MSG_PACK_UINT_32;
            }
        }else if(arg->IsInt32()){
            const int32_t val = arg->Int32Value();
            if(val >= -32){
                return MSG_PACK_NEG_FIXNUM;
            }else if(val > -0x80){
                return MSG_PACK_INT_8;
            }else if(val > -0x8000){
                return MSG_PACK_INT_16;
            }else{
                return MSG_PACK_INT_32;
            }
        }else{
            return MSG_PACK_DOUBLE;
        }
    }else if(arg->IsBoolean()){
        if(arg->BooleanValue() == true){
            return MSG_PACK_TRUE;
        }else{
            return MSG_PACK_FALSE;
        }
    }else if(arg->IsNull() || arg->IsUndefined()){
        return MSG_PACK_NIL;
    }

    ThrowException(Exception::TypeError(String::New("Unknow MessagePack Type")));
    return 0;
}

static const uint8_t reverseType(const uint8_t obj){
    if(obj <= 0x7f){
        return MSG_PACK_POS_FIXNUM;
    }else if(obj <= 0x8f){
        return MSG_PACK_MAP_FIX;
    }else if(obj <= 0x9f){
        return MSG_PACK_ARRAY_FIX;
    }else if(obj <= 0xbf){
        return MSG_PACK_RAW_FIX;
    }else if(obj >= 0xe0){
        return MSG_PACK_NEG_FIXNUM;
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

static const uint32_t pack(const Local<Value>& obj, uint8_t *data){
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
            case MSG_PACK_MAP_FIX:{
                uint8_t len[] = { (disc | size) };
                d = len;
                break;
            }
            case MSG_PACK_MAP_16:{
                uint8_t len[] = { disc, (size >> 8), (size & 0xff) };
                d = len;
                break;
            }
            case MSG_PACK_MAP_32:{
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

            offset += pack(key, data + offset);
            offset += pack(val, data + offset);
        }
    }else if((ssize = sizeof_array(disc))){
        Handle<Array> node = Handle<Array>::Cast(obj);
        uint32_t size = node->Length();

        uint8_t *d;

        switch(disc){
            case MSG_PACK_ARRAY_FIX:{
                uint8_t len[] = { (disc | size) };
                d = len;
                break;
            }
            case MSG_PACK_ARRAY_16:{
                uint8_t len[] = { disc, (size >> 8), (size & 0xff) };
                d = len;
                break;
            }
            case MSG_PACK_ARRAY_32:{
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
            offset += pack(val, data + offset);
        }
    }else if((ssize = sizeof_raw(disc))){
        Local<String> node = obj->ToString();
        uint32_t size = node->Utf8Length();

        uint8_t *d;

        switch(disc){
            case MSG_PACK_RAW_FIX:{
                uint8_t len[] = { (disc | size) };
                d = len;
                break;
            }
            case MSG_PACK_RAW_16:{
                uint8_t len[] = { disc, (size >> 8), (size & 0xff) };
                d = len;
                break;
            }
            case MSG_PACK_RAW_32:{
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
        offset += size;
    }else if((ssize = sizeof_number(disc))){
        Local<Number> node = obj->ToNumber();

        uint8_t *d;

        switch(disc){
            case MSG_PACK_POS_FIXNUM:{
                uint8_t len[] = { node->Uint32Value() & 0xff };
                d = len;
                break;
            }
            case MSG_PACK_NEG_FIXNUM:{
                uint8_t len[] = { node->Int32Value() & 0xff };
                d = len;
                break;
            }
            case MSG_PACK_UINT_8:{
                uint32_t num = node->Uint32Value();
                uint8_t len[] = { disc, (num & 0xff) };
                d = len;
                break;
            }
            case MSG_PACK_INT_8:{
                int32_t num = node->Int32Value();
                uint8_t len[] = { disc, (num & 0xff) };
                d = len;
                break;
            }
            case MSG_PACK_UINT_16:{
                uint32_t num = node->Uint32Value();
                uint8_t len[] = { disc, (num >> 8) & 0xff, num & 0xff };
                d = len;
                break;
            }
            case MSG_PACK_INT_16:{
                int32_t num = node->Int32Value();
                uint8_t len[] = { disc, (num >> 8) & 0xff, num & 0xff };
                d = len;
                break;
            }
            case MSG_PACK_UINT_32:{
                uint32_t num = node->Uint32Value();
                uint8_t len[] = { disc, num >> 24, (num >> 16) & 0xff,
                    (num >> 8) & 0xff, num & 0xff };
                d = len;
                break;
            }
            case MSG_PACK_INT_32:{
                int32_t num = node->Int32Value();
                uint8_t len[] = { disc, num >> 24, (num >> 16) & 0xff,
                    (num >> 8) & 0xff, num & 0xff };
                d = len;
                break;
            }
            case MSG_PACK_DOUBLE:{
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

    return offset;
}

static void buffer_free(char *data, void *hint){
    delete [] data;
}

static Handle<Value> Pack(const Arguments& args){
    HandleScope scope;
    Local<Value> obj = args[0];

    const uint32_t size = sizeof_obj(obj);
    uint8_t *data = new uint8_t[size];
    pack(obj, data);

    return scope.Close((Buffer::New((char *)data, (size_t)size, &buffer_free, NULL))->handle_);
}

static Handle<Value> unpack(const uint8_t *buf, uint32_t& s){
    HandleScope scope;

    const uint8_t disc = reverseType(buf[0]);
    uint8_t ssize;

    if((ssize = sizeof_map(disc))){
        Local<Object> map = Object::New();

        uint32_t len;
        uint32_t start = ssize;
        uint32_t end = start;

        switch(disc){
            case MSG_PACK_MAP_FIX:
                len = buf[0] ^ MSG_PACK_MAP_FIX;
                break;
            case MSG_PACK_MAP_16:
                len = read2Bytes(buf, 1);
                break;
            case MSG_PACK_MAP_32:
                len = read4Bytes(buf, 1);
                break;
        }

        s += ssize;

        for(uint32_t i = 0; i < len*2; i+=2){
            Handle<Value> key = unpack(buf + start, end);
            s += end - start;
            start = end;

            Handle<Value> value = unpack(buf + start, end);
            s += end - start;
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
            case MSG_PACK_ARRAY_FIX:
                len = buf[0] ^ MSG_PACK_ARRAY_FIX;
                break;
            case MSG_PACK_ARRAY_16:
                len = read2Bytes(buf, 1);
                break;
            case MSG_PACK_ARRAY_32:
                len = read4Bytes(buf, 1);
                break;
        }

        s += ssize;

        for(uint32_t i = 0; i < len; i++){
            array->Set(i, unpack(buf + start, end));
            s += end - start;
            start = end;
        }

        return scope.Close(array);
    }else if((ssize = sizeof_raw(disc))){
        uint32_t len;

        switch(disc){
            case MSG_PACK_RAW_FIX:
                len = buf[0] ^ MSG_PACK_RAW_FIX;
                break;
            case MSG_PACK_RAW_16:
                len = read2Bytes(buf, 1);
                break;
            case MSG_PACK_RAW_32:
                len = read4Bytes(buf, 1);
                break;
        }

        s += ssize + len;

        return scope.Close(String::New((char *)(buf + ssize), len));
    }else if((ssize = sizeof_number(disc))){

        s += ssize;

        switch(disc){
            case MSG_PACK_POS_FIXNUM:
                return scope.Close(Integer::NewFromUnsigned(buf[0]));
                break;
            case MSG_PACK_NEG_FIXNUM:
                return scope.Close(Integer::New((int8_t)buf[0]));
                break;
            case MSG_PACK_UINT_8:
                return scope.Close(Integer::NewFromUnsigned((uint8_t)buf[1]));
                break;
            case MSG_PACK_INT_8:
                return scope.Close(Integer::New((int8_t)buf[1]));
                break;
            case MSG_PACK_UINT_16:
                return scope.Close(Integer::NewFromUnsigned(read2Bytes(buf, 1)));
                break;
            case MSG_PACK_INT_16:
                return scope.Close(Integer::New((int16_t)read2Bytes(buf, 1)));
                break;
            case MSG_PACK_UINT_32:
                return scope.Close(Integer::NewFromUnsigned(read4Bytes(buf, 1)));
                break;
            case MSG_PACK_INT_32:
                return scope.Close(Integer::New((int32_t)read4Bytes(buf, 1)));
                break;
            case MSG_PACK_DOUBLE:{
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

        s += ssize;

        if(disc == MSG_PACK_NIL){
            return scope.Close(Null());
        }else{
            return scope.Close(Boolean::New((disc == MSG_PACK_TRUE ? true : false)));
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

    uint32_t s = 0;
    return scope.Close(unpack((uint8_t *)Buffer::Data(buf), s));
}

void Init(Handle<Object> target) {
  target->Set(String::NewSymbol("pack"),
      FunctionTemplate::New(Pack)->GetFunction());
  target->Set(String::NewSymbol("unpack"),
      FunctionTemplate::New(Unpack)->GetFunction());
}

NODE_MODULE(msgpack3, Init)
