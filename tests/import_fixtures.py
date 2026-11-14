#!/usr/bin/env python3
"""One-time fixture import; never run as part of the tests.

Usage: python3 tests/import_fixtures.py ARCHIVE PDF CATALOG_JSON CATALOG_TLE
Requires pdftotext. See fixtures/README.md for sources and pinned hashes.
Only converts existing expected vectors; never invokes a propagator.
"""
import csv
import datetime
import hashlib
import json
import math
from pathlib import Path
import re
import subprocess
import sys
import zipfile


ROOT = Path(__file__).resolve().parent / "fixtures"
ELEMENT_FIELDS = ["case_id", "satnum", "epoch", "bstar", "mean_motion_dot",
                  "mean_motion_ddot", "eccentricity", "arg_of_pericenter",
                  "inclination", "mean_anomaly", "mean_motion", "ra_of_asc_node"]
STATE_FIELDS = ["case_id", "minutes", "x_km", "y_km", "z_km",
                "vx_km_s", "vy_km_s", "vz_km_s"]


def elements(case_id, line1, line2):
    def exponent(value):
        return float(value[0] + "." + value[1:6] + "e" + value[6:8])
    year = int(line1[18:20])
    year += 2000 if year < 57 else 1900
    epoch = ((datetime.date(year, 1, 1) - datetime.date(1949, 12, 31)).days
             + float(line1[20:32]) - 1)
    radians = math.pi / 180
    rev_to_rad_min = 2 * math.pi / 1440
    return [case_id, line1[2:7], epoch, exponent(line1[53:61]),
            float(line1[33:43]) * rev_to_rad_min / 1440,
            exponent(line1[44:52]) * rev_to_rad_min / 1440**2,
            float("0." + line2[26:33]), float(line2[34:42]) * radians,
            float(line2[8:16]) * radians, float(line2[43:51]) * radians,
            float(line2[52:63]) * rev_to_rad_min,
            float(line2[17:25]) * radians]


def write_csv(name, header, rows):
    with (ROOT / name).open("w", newline="") as stream:
        writer = csv.writer(stream, lineterminator="\n")
        writer.writerow(header)
        writer.writerows(rows)


def main():
    if len(sys.argv) != 5:
        raise SystemExit(__doc__)
    archive, pdf, catalog_json, catalog_tle = map(Path, sys.argv[1:])
    ROOT.mkdir(exist_ok=True)
    expected = json.loads((ROOT / "source_hashes.json").read_text())
    for name, path in zip(("vallado-verification.zip", "vallado-rev3.pdf",
                           "sgp4CatalogResults.json", "satellite-catalog.tle"),
                          (archive, pdf, catalog_json, catalog_tle)):
        actual = hashlib.sha256(path.read_bytes()).hexdigest()
        if actual != expected[name]:
            raise SystemExit(f"Source hash mismatch for {name}: {actual}")
    with zipfile.ZipFile(archive) as source:
        tle_bytes = source.read("sgp4/cpp/testsgp4/SGP4-VER.TLE")
    (ROOT / "SGP4-VER.TLE").write_bytes(tle_bytes)
    tle = [line for line in tle_bytes.decode().splitlines()
           if line.startswith(("1 ", "2 "))]
    assert len(tle) == 66
    a_elements = [elements(f"a_{i // 2:02d}", *tle[i:i+2])
                  for i in range(0, len(tle), 2)]
    write_csv("a_elements.csv", ELEMENT_FIELDS, a_elements)
    text = subprocess.check_output(["pdftotext", "-layout", str(pdf), "-"], text=True)
    appendix = text.split("Appendix E – Test Case Results Listing", 1)[1]
    appendix = appendix.split("Appendix F – Computer Code Listing", 1)[0]
    (ROOT / "appendix_e.txt").write_text(appendix)
    states = []
    case = -1
    discarded = []
    for line in appendix.splitlines():
        header = re.fullmatch(r"\s*(\d+) xx\s*", line)
        if header:
            case += 1
            assert int(header[1]) == int(a_elements[case][1])
        elif re.match(r"^\s*-?\d+\.\d+\s+", line):
            values = line.split()[:7]
            assert len(values) == 7 and all(math.isfinite(float(x)) for x in values)
            if a_elements[case][1] == "33334":
                # Published output reuses the preceding satellite's buffers
                # after a failed initialization. This is not a valid state.
                discarded.append(values)
            else:
                states.append([a_elements[case][0], *values])
    assert case == 32 and len(states) == 666 and len(discarded) == 1
    write_csv("a_states.csv", STATE_FIELDS, states)

    catalog = json.loads(catalog_json.read_text())
    lines = catalog_tle.read_text(encoding="utf-8-sig").splitlines()
    # Deterministic samples: four normal LEO, four simplified-drag LEO,
    # four MEO, four low-inclination GEO, four eccentric half-day orbits.
    buckets = {name: [] for name in ("leo", "low_leo", "meo", "geo", "molniya")}
    subset = []
    for index in range(len(lines) // 2):
        line1, line2 = lines[2*index:2*index+2]
        result = catalog[index // 500][index % 500]
        if len(result) != 5 or any(r is None for r in result):
            continue
        n, inc, ecc = float(line2[52:63]), float(line2[8:16]), float("0." + line2[26:33])
        kind = ("leo" if 12 < n < 15 else "low_leo" if n >= 15.8 else
                "meo" if 1.8 < n < 2.2 and ecc < .1 else
                "geo" if .98 < n < 1.03 and inc < 5 else
                "molniya" if 1.8 < n < 2.2 and ecc > .5 else None)
        if kind is None or len(buckets[kind]) == 4:
            continue
        case_id = f"i_{kind}_{len(buckets[kind])}"
        buckets[kind].append(case_id)
        subset.append(dict(case_id=case_id, source_index=index, category=kind,
                           line1=line1, line2=line2, results=result))
        if all(len(bucket) == 4 for bucket in buckets.values()):
            break
    assert len(subset) == 20, buckets
    (ROOT / "i_source_subset.json").write_text(json.dumps(subset, indent=2) + "\n")
    write_csv("i_elements.csv", ELEMENT_FIELDS,
              [elements(s["case_id"], s["line1"], s["line2"]) for s in subset])
    write_csv("i_states.csv", STATE_FIELDS,
              [[s["case_id"], time, *[result[k][axis] for k in ("position", "velocity")
                                      for axis in ("x", "y", "z")]]
               for s in subset for time, result in zip((0, 360, 720, 1080, 1440), s["results"])])
    print("Imported 33 AFSPC cases / 666 states and 20 improved cases / 100 states")


if __name__ == "__main__":
    main()
