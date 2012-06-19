var msgpack = require("../");

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

console.log("msgpack: " + msgpack.pack(o).length + " bytes");
console.log("json:    " + Buffer.byteLength(JSON.stringify(o)) + " bytes");
