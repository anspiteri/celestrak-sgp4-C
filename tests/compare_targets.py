#!/usr/bin/env python3
"""Run both suites and compare their actual bridge outputs, offline."""
import math
from pathlib import Path
import subprocess
import sys


def run(command):
    result = subprocess.run(command, text=True, capture_output=True)
    if result.returncode:
        sys.stderr.write(result.stdout + result.stderr)
        raise SystemExit(f"Failed: {' '.join(command)}")
    records = []
    for line in result.stdout.splitlines():
        if line.startswith("STATE,"):
            records.append(line.split(",")[1:])
        else:
            print(line)
    return records


def main():
    native_dir, wasm_dir, node = sys.argv[1:]
    print("Native:", flush=True)
    native = run([str(Path(native_dir) / "test_suite"), "--dump"])
    run([str(Path(native_dir) / "test_faults")])
    print("WASM:", flush=True)
    wasm = run([node, str(Path(wasm_dir) / "test_suite.js"), "--dump"])
    run([node, str(Path(wasm_dir) / "test_faults.js")])
    if len(native) != 772 or len(wasm) != 772:
        raise SystemExit(f"Expected 772 results per target; got {len(native)} and {len(wasm)}")
    worst = [0.0, 0.0]
    for a, b in zip(native, wasm):
        if a[:4] != b[:4] or len(a) != 10 or len(b) != 10:
            raise SystemExit(f"Target case/time/error mismatch: {a} / {b}")
        for i, (x, y) in enumerate(zip(map(float, a[4:]), map(float, b[4:]))):
            quantity = 0 if i < 3 else 1
            tolerance = (1e-7, 1e-10)[quantity]
            if not math.isfinite(x) or not math.isfinite(y) or abs(x - y) > tolerance:
                raise SystemExit(f"Target numerical mismatch: {a} / {b}")
            worst[quantity] = max(worst[quantity], abs(x - y))
    print(f"Cross-target: 766 states and 6 errors agree; max component difference {worst[0]:.3g} km / {worst[1]:.3g} km/s")


if __name__ == "__main__":
    main()
