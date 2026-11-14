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

All routine tests run offline. WASM embeds the same four CSV fixtures read by
the native executable. Tests link the static library, and the API callers in
`test_api.c` and `test_modes.c` are compiled as C rather than C++.

| Criterion | Implementation |
| --- | --- |
| Propagator accuracy | `test_suite.cpp` checks direct Vallado calls against 666 published AFSPC vectors and 100 external improved-mode vectors. See `fixtures/README.md` for sources, units, tolerances, and limitations. Counts are pinned so accidentally dropping cases fails the suite. |
| Bridge equivalence | Each reference propagation is also run through the bridge. Repeated, negative, and out-of-order times are compared with direct calls and fresh initialization. `test_modes.cpp` additionally interleaves handles using different modes. |
| C API contract | `test_api.c` checks every element field for NaN and both infinities, range boundaries, malformed identifiers, null arguments, lifecycle, initialization errors, and unchanged output vectors on failure. Real propagation errors 1, 4 and 6 are checked in both modes, followed by successful recovery at epoch. |
| Target consistency | `compare_targets.py` runs both suites and compares all 766 actual bridge state vectors and six propagation error results, including case IDs, modes, times, and error codes. It checks positions within `1e-7` km and velocities within `1e-10` km/s. |

`test_faults.cpp` links the real bridge against isolated SGP4 stubs and an
allocation-failure hook. It covers allocation error -2, fallback error -3, and
forwarding of SGP4 error codes 1–6, including errors reported during
initialization even when the upstream boolean return is true. These stubs are
never linked into the production archive or numerical test executable. Some
SGP4 codes are unreachable through valid inputs or unused in this upstream
version; injection tests the bridge's handling without claiming physical test
cases exist for every code.

Numerical comparisons reject non-finite results and report case, time, axis,
actual value, expected value, and tolerance on failure. Native/WASM tests cover
the supported library builds; browser-specific JavaScript loading and bindings
are outside this repository's current API.
