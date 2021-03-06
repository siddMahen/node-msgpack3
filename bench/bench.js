// Taken directly from [node-msgpack](https://github.com/pgriess/node-msgpack/blob/master/bench.js)

var assert = require('assert');
var msgpack = require('../');

var DATA_TEMPLATE = {'abcdef' : 1, 'qqq' : 13, '19' : [1, 2, 3, 4]};
var DATA = [];

for (var i = 0; i < 500000; i++) {
    DATA.push(JSON.parse(JSON.stringify(DATA_TEMPLATE)));
}

while (true) {
    var now = Date.now();
    var mpBuf;
    DATA.forEach(function(d) {
        mpBuf = msgpack.pack(d);
    });
    console.log('msgpack pack:   ' + (Date.now() - now) + ' ms');

    var now = Date.now();
    DATA.forEach(function(d) {
        msgpack.unpack(mpBuf);
    });
    console.log('msgpack unpack: ' + (Date.now() - now) + ' ms');

    now = Date.now();
    var jsonStr;
    DATA.forEach(function(d) {
        jsonStr = JSON.stringify(d);
    });
    console.log('json    pack:   ' + (Date.now() - now) + ' ms');

    now = Date.now();
    DATA.forEach(function(d) {
        JSON.parse(jsonStr);
    });
    console.log('json    unpack: ' + (Date.now() - now) + ' ms');

    console.log();
}
