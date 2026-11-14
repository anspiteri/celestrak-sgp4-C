# celestrak-sgp4-C
A bridge from the original David Vallado SGP4 satellite propagation C++ implementation to a static C library designed for use in the browser as WASM.

Select the operation mode when creating a satellite:

```c
int err;
Sgp4Satellite *sat = sgp4_create(&elements, 'a', &err);
/* 'a': AFSPC compatibility; 'i': improved mode. */
```

`sgp4_create()` requires an explicit mode on every call.
Both modes use WGS72 gravity constants. Each satellite keeps its selected mode
for its lifetime, so both modes can be used concurrently through separate
handles. Invalid mode characters return NULL with `err = -1`.
Check creation succeeded before propagation and call `sgp4_destroy(sat)` when finished.

Run `make test-native` for the native verification suite, `make test-wasm`
for the same suite in WASM under Node, or `make test` for both plus direct
comparison of their outputs. `make test-sanitize` checks the native suite with
address, undefined-behavior, and leak sanitizers. See [tests/README.md](tests/README.md)
for coverage and reference-data provenance.

The root `Makefile` loads shared settings from [make/config.mk](make/config.mk),
production library rules from [make/library.mk](make/library.mk), and test
rules from [make/tests.mk](make/tests.mk). Run all targets from the repository
root; plain `make` builds the WASM static library.

TODO: "Best practice: Add a brief comment at the top of the file—or in a vendor/README.md—noting the original repository URL, license, and commit hash you copied it from."
