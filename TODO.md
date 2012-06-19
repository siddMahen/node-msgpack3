# TODO

## Streaming Support

A lack of streaming support.

## Speed

Currently, write speeds are hovering around 6950 ms and
read speeds are around 3300 ms.

```
msgpack pack:   6929 ms
msgpack unpack: 3288 ms
json    pack:   1294 ms
json    unpack: 612 ms

msgpack pack:   6966 ms
msgpack unpack: 3277 ms
json    pack:   1281 ms
json    unpack: 608 ms

msgpack pack:   6930 ms
msgpack unpack: 3298 ms
json    pack:   1300 ms
json    unpack: 602 ms
```

For comparison these are the results using `node-msgpack` by pgriess,
and the native msgpack lib. In this case write speeds are around 7200 ms
and read speeds are around 2100 ms.

```
msgpack pack:   7238 ms
msgpack unpack: 2139 ms
json    pack:   1303 ms
json    unpack: 603 ms

msgpack pack:   7293 ms
msgpack unpack: 2088 ms
json    pack:   1323 ms
json    unpack: 599 ms

msgpack pack:   7221 ms
msgpack unpack: 2152 ms
json    pack:   1321 ms
json    unpack: 600 ms
```

It seems as though our write speeds are almost the same as those of the
native lib (if not slightly faster), however, read speeds are over 1.5 times
slower.

All tests run on my local machine, hence subject to errors. Please
verify.

NOTE: I actually used [this](https://github.com/godsflaw/node-msgpack) branch of
node-msgpack, as the original lib didn't compile with node v0.7.11.
