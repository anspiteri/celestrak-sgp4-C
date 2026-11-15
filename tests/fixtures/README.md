# Reference fixtures

These are frozen inputs and expected outputs. Routine tests never download
data, generate expected vectors, or call another installed propagator.

## AFSPC (`'a'`), WGS72

Source: Vallado, Crawford, Hujsak and Kelso, *Revisiting Spacetrack Report #3*,
AIAA 2006-6753, Revision 3, Appendix E:

- Paper: https://celestrak.org/publications/AIAA/2006-6753/AIAA-2006-6753-Rev3.pdf
- Archive: https://celestrak.org/publications/AIAA/2006-6753/AIAA-2006-6753.zip
- Inputs: archive member `sgp4/cpp/testsgp4/SGP4-VER.TLE`, preserved verbatim here.
- `appendix_e.txt`: text extracted from the PDF with `pdftotext -layout`.
- `a_elements.csv`: all 33 input cases, converted to bridge units.
- `a_states.csv`: 666 valid state rows copied from Appendix E without
  recomputing or rounding their values.

The paper explicitly specifies `'a'` / `'72'`. Outputs are TEME of date,
position in kilometres and velocity in kilometres/second. Times are minutes
relative to the element epoch; calendar-date columns are not used. Distinct
case IDs retain the two occurrences of satellite 20413 with different epochs.

The PDF has 667 numerical rows. The epoch row for satellite 33334 repeats the
previous satellite's final output after an initialization failure. It is not
a valid reference vector and is excluded explicitly by the importer. The
test instead requires initialization error 3 for that case.

Absolute component tolerances are `1e-6` km and `1e-9` km/s. Satellite 23333
uses `1e-5` km and `3e-9` km/s: the published extreme-eccentricity example
differs by about 4.11e-6 km and 1.78e-9 km/s with both the repository version
and the separately downloaded upstream C++ version. This exception is local
to that case; other cases retain the tighter limits.

## Improved (`'i'`), WGS72

Source: Satellite.js, pinned commit
`696b363d76278c2ad6ceff6ca1da335947dbdede`:

- Results: https://raw.githubusercontent.com/shashwatak/satellite-js/696b363d76278c2ad6ceff6ca1da335947dbdede/test/propagation/sgp4CatalogResults.json
- Inputs: https://raw.githubusercontent.com/shashwatak/satellite-js/696b363d76278c2ad6ceff6ca1da335947dbdede/test/propagation/tle.txt
- Consumer: https://github.com/shashwatak/satellite-js/blob/696b363d76278c2ad6ceff6ca1da335947dbdede/test/propagation/sgp4Catalog.test.ts
- Mode selection: `src/io.ts` selects `'i'`; `src/constants.ts` uses WGS72.

`i_source_subset.json` preserves 20 selected TLE pairs, their original catalog
indices, and all five saved results per satellite. The deterministic selection
takes the first four successful examples in each category: normal LEO,
high-mean-motion LEO, nearly circular MEO, low-inclination GEO, and eccentric
half-day orbits. The exact selection predicates are in `import_fixtures.py`.
`i_elements.csv` and `i_states.csv` contain the same inputs and 100 vectors at
0, 360, 720, 1080 and 1440 minutes in the same TEME/km/km/s conventions.

This is external implementation regression data, not an authoritative published
improved-mode validation standard. Satellite.js shares Vallado/Python SGP4
ancestry, and its earlier catalog tests used snapshots. Agreement does not
provide independent physical validation of SGP4 theory.

During import, all 100 vectors were also checked against an unmodified C++
implementation extracted separately from the pinned CelesTrak archive above
(`sgp4/cpp/SGP4/SGP4/SGP4.cpp` and `SGP4.h`, version 2020-07-13), built with
WGS72 and mode `'i'`. The maximum component differences were 4.09e-9 km and
2.89e-12 km/s. The repository's 2025-12-30 implementation has the same maxima.
The bridge's creation error check is used in that verification, but the upstream
propagator sources are unmodified. Absolute test tolerances are `1e-7` km and
`1e-10` km/s.

Satellite.js is MIT licensed; its license is retained alongside this subset.
The Vallado reference source's original unrestricted-use terms are described
in `celestrak-sgp4/NOTICE`.

## Reproducing the conversion

`source_hashes.json` records SHA-256 hashes of all four downloaded originals,
retrieved on 2026-11-14. The import script requires those hashes to match
before writing fixtures. Download the linked originals to local files, then:

```sh
python3 tests/import_fixtures.py \
    /path/to/AIAA-2006-6753.zip /path/to/AIAA-2006-6753-Rev3.pdf \
    /path/to/sgp4CatalogResults.json /path/to/tle.txt
```

The script needs Python 3 and `pdftotext`. It performs format conversion only.
It converts TLE units to radians, radians/minute, SGP4 derivative conventions,
and epoch days since 1949-12-31. It does not invoke any propagator. Review
fixture changes explicitly; never regenerate expected vectors from the code
under test.

To repeat the separate upstream cross-check, extract the two C++ source files
from the archive to `/tmp/sgp4-pinned-upstream`, run `make test-native`, then:

```sh
c++ -std=c++11 -O2 -I/tmp/sgp4-pinned-upstream -I. \
    tests/test_suite.cpp tests/test_modes.cpp sgp4_bridge.cpp \
    /tmp/sgp4-pinned-upstream/SGP4.cpp \
    build/tests/native/test_api.o build/tests/native/test_modes.o \
    -o /tmp/test_sgp4_upstream
/tmp/test_sgp4_upstream
```

This runs the same tests with the independently extracted propagator in place
of the repository copy. It is a provenance check, not part of routine testing.
