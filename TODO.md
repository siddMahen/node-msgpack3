# TODO

## Streaming Support

A lack of streaming support.

## Speed

Currently, write speeds are hovering around 5220 ms and
read speeds are around 1350 ms.

```
msgpack pack:   5236 ms
msgpack unpack: 1327 ms
json    pack:   1259 ms
json    unpack: 596 ms

msgpack pack:   5200 ms
msgpack unpack: 1341 ms
json    pack:   1253 ms
json    unpack: 596 ms

msgpack pack:   5293 ms
msgpack unpack: 1347 ms
json    pack:   1241 ms
json    unpack: 597 ms
```

For comparison these are the results using `msgpack2` by JulesAU,
and the native msgpack lib. In this case write speeds are around 5420 ms
and read speeds are around 1850 ms.

```
msgpack pack:   5431 ms
msgpack unpack: 1844 ms
json    pack:   1264 ms
json    unpack: 595 ms

msgpack pack:   5422 ms
msgpack unpack: 1862 ms
json    pack:   1268 ms
json    unpack: 597 ms

msgpack pack:   5451 ms
msgpack unpack: 1863 ms
json    pack:   1286 ms
json    unpack: 594 ms
```

Write speeds are therefore only 1.083 times faster than `msgpack2`.
Similarly, read speeds are only 1.370 times faster. These numbers need to
be better. They are still nowhere close to beating JSON.

All tests run on my local machine, hence subject to errors. Please
verify. Run on node `v0.7.12`.
