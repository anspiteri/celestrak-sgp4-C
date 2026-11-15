# celestrak-sgp4-C

This is a C bridge for the David Vallado et al. satellite propagation C++ library.
Build targets include both native machine code and WebAssembly (WASM) using Emscripten.

**The links to the original source code and research can be found here:**
- Vallado et al. SGP4 library main page: https://celestrak.org/publications/AIAA/2006-6753/
- Source Code GitHub: https://github.com/CelesTrak/fundamentals-of-astrodynamics
- Original Paper: https://celestrak.org/publications/AIAA/2006-6753/AIAA-2006-6753.pdf

## Building
Run from the repository root:

```sh
make          # Build both production static libraries
make native   # Build only the native library
make wasm     # Build only the WASM library
```

| Archive | Contents | Toolchain |
| --- | --- | --- |
| `build/lib/libsgp4_native.a` | Machine code for the build host's architecture | C++ compiler and `ar` |
| `build/lib/libsgp4_wasm.a` | WebAssembly object code | Emscripten (`em++` and `emar`) |

Override `OUT` to change the output directory, for example `make OUT=build/release`.
Toolchain overrides include `NATIVE_CXX`, `NATIVE_AR`, `WASM_CXX`, and `WASM_AR`;
the existing `COMPILER` and `AR` overrides remain defaults for the WASM toolchain.

## Licensing
The original C++ version of the SGP4 library has no restrictions on distribution.
- You can see a copy of these terms in the [NOTICE](celestrak-sgp4/NOTICE) that was included with the original source code.

This code is distributed under the MIT license, see [LICENSE](LICENSE).

## Disclaimer
Outside of the original library itself, most of the C bridging, verification, and documentation was outsourced to AI.

While having a broad understanding of the architecture of the bridge, build and test suite; as a developer who is not very familiar with C++ and only just learning the domain, I cannot say I have a strong comprehension of the implementation currently.

For the initial release, I have done my best to ensure that there is an adequate test suite and you can find rationale behind that here: [tests/README.md](tests/README.md).

Hopefully as my expertise improves, I can provide further guarantees for the correctness and usefulness of this software.
