var msgpack = require("../"),
    assert = require("assert");

// LOGIC

assert.deepEqual(msgpack.unpack(Buffer([0xc0])), null);
assert.deepEqual(msgpack.unpack(Buffer([0xc3])), true);
assert.deepEqual(msgpack.unpack(Buffer([0xc2])), false);

// NUMBERS
assert.deepEqual(msgpack.unpack(Buffer([0x01])), 1);
assert.deepEqual(msgpack.unpack(Buffer([0xff])), -1);
assert.deepEqual(msgpack.unpack(Buffer([0xcc, 0xff])), 255);
assert.deepEqual(msgpack.unpack(Buffer([0xcc, 0x80])), 128);
assert.deepEqual(msgpack.unpack(Buffer([0xd0, 0xdf])), -33);
assert.deepEqual(msgpack.unpack(Buffer([0xcd, 0x01, 0x00])), 256);

// ARRAY

assert.deepEqual(msgpack.unpack(Buffer([0x90])), []);
assert.deepEqual(msgpack.unpack(Buffer([0x94, 0x01, 0x02, 0x03, 0x04])), [1, 2, 3, 4]);

// MAP

assert.deepEqual(msgpack.unpack(Buffer([0x80])), {});
assert.deepEqual(msgpack.unpack(Buffer([0x82, 0xa1, 0x61, 0x01, 0xa1, 0x62, 0x02])), { a: 1, b: 2 });

// RAW

assert.deepEqual(msgpack.unpack(Buffer([0xa0])), "");
assert.deepEqual(msgpack.unpack(Buffer([0xa2, 0x61, 0x62])), "ab");

console.log("Passed");
