var msgpack = require("../"),
    assert = require("assert");

// LOGIC

assert.deepEqual(msgpack.pack(null), Buffer([0xc0]));
assert.deepEqual(msgpack.pack(undefined), Buffer([0xc0]));
assert.deepEqual(msgpack.pack(true), Buffer([0xc3]));
assert.deepEqual(msgpack.pack(false), Buffer([0xc2]));

// NUMBERS
assert.deepEqual(msgpack.pack(1), Buffer([0x01]));
assert.deepEqual(msgpack.pack(-1), Buffer([0xff]));
assert.deepEqual(msgpack.pack(255), Buffer([0xcc, 0xff]));
assert.deepEqual(msgpack.pack(128), Buffer([0xcc, 0x80]));
assert.deepEqual(msgpack.pack(-33), Buffer([0xd0, 0xdf]));
assert.deepEqual(msgpack.pack(256), Buffer([0xcd, 0x01, 0x00]));

// ARRAY

assert.deepEqual(msgpack.pack([]), Buffer([0x90]));
assert.deepEqual(msgpack.pack([1, 2, 3, 4]), Buffer([0x94, 0x01, 0x02, 0x03, 0x04]));

// MAP

assert.deepEqual(msgpack.pack({}), Buffer([0x80]));
assert.deepEqual(msgpack.pack({ a: 1, b: 2 }), Buffer([0x82, 0xa1, 0x61, 0x01, 0xa1, 0x62, 0x02]));

// RAW

assert.deepEqual(msgpack.pack(""), Buffer([0xa0]));
assert.deepEqual(msgpack.pack("ab"), Buffer([0xa2, 0x61, 0x62]));

console.log("Passed");
