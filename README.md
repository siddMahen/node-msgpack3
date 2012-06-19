# msgpack3

A speedy implementation of the MessagePack serialization protocol, written
from scratch.

MessagePack offers significant advantages over other serialization protocols,
such as JSON, as it encodes objects very compactly. For example, the following
object:

```
var o = {
  a: {
    b: -123,
    c: 123
  },
  d: [
    "Augustine",
    "Emma",
    "Shai",
    "Leea"
  ]
};
```

Is packed into 39 bytes by `msgpack3`, and 63 bytes by the native JSON library.
In this specific case, MessagePack was around 60% more space efficient. However,
these kinds of savings are quite typical. In general, MessagePack messages are
significantly more space efficient than JSON.

**Disclaimer**: At the moment, this implementation of MessagePack is NOT faster
than node's native JSON library.

## Installation

It's on `npm`:

```
$ npm install msgpack3
```

## Usage

`msgpack3` provides two functions, `pack` and `unpack` which, respectively,
serialize and deserialize javascript objects.

### msgpack.pack(obj)

Takes a javascript object as input and returns a buffer.

### msgpack.unpack(buf)

Takes a buffer as input and returns a javascript object.

See the source code for more details.

## Example

```
var msgpack = require("msgpack3"),
    assert = require("assert);

var a = { a: 123, b: [1, 2, 3] }

var buf = msgpack.pack(o);

var b = msgpack.unpack(buf);

assert.deepEqual(a, b);
```

## License

(The MIT License)

Copyright (C) 2012 by Siddharth Mahendraker

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
