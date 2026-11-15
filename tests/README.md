# Testing Suite

**This test suite seeks to verify the following criteria:**
1. The original library and/or future derivations of it produce accurate propagation results

2. The custom wrapper produces the same output as the propagation library.

3. The C API itself produces the correct input validation and error handling

4. Behaviour for the library is consistent for all build targets

Run from the repository root:

```sh
make test-native    # C/C++ compiler and ar
make test-wasm      # Emscripten and Node
make test           # Both targets and a direct comparison; also needs Python 3
make test-sanitize  # Native AddressSanitizer, UndefinedBehaviorSanitizer and leaks
```

## How the components work together

- `fixtures/` holds satellite inputs and saved reference results;
- `test_suite.cpp` compares pre-bridge C++ SGP4 results with those references, then
  checks that the C bridge returns the same results as the direct SGP4 calls.
- `test_api.c` and `test_modes.c/.cpp` check input validation, error handling,
  C compatibility, and operation modes within the main suite.
- `test_faults.cpp` runs separately and simulates allocation and SGP4 failures
  to check errors that are difficult to trigger with real satellite data.
- `make/tests.mk` builds and runs both suites for native and WASM targets.
  Each main suite links the corresponding production archive from `build/lib/`.
  `compare_targets.py` compares their outputs when you run `make test`.
- `make test-sanitize` runs the native suites with memory and undefined-behavior
  checks enabled.

## Coverage at a glance

| Criterion | Implementation |
| --- | --- |
| Propagator accuracy | `test_suite.cpp` checks direct Vallado calls against 666 published AFSPC vectors and 100 external improved-mode vectors. See `fixtures/README.md` for sources, units, tolerances, and limitations. Counts are pinned so accidentally dropping cases fails the suite. |
| Bridge equivalence | Each reference propagation is also run through the bridge. Repeated, negative, and out-of-order times are compared with direct calls and fresh initialization. `test_modes.cpp` additionally interleaves handles using different modes. |
| C API contract | `test_api.c` checks every element field for NaN and both infinities, range boundaries, malformed identifiers, null arguments, lifecycle, initialization errors, and unchanged output vectors on failure. Real propagation errors 1, 4 and 6 are checked in both modes, followed by successful recovery at epoch. |
| Target consistency | `compare_targets.py` runs both suites and compares all 766 actual bridge state vectors and six propagation error results, including case IDs, modes, times, and error codes. It checks positions within `1e-7` km and velocities within `1e-10` km/s. |

