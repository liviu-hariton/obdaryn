#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""
Builds src/core/data/dtc_generic_ro.json, the Romanian layer over the obdex
generic codes: a title for every code and, for the most common codes, the full
explanation (description, causes, symptoms).

    python3 tools/build_generic_ro.py TITLES.json [FULL.json ...]

TITLES.json: {"P0420": "Eficiența catalizatorului sub prag, banc 1", ...}
FULL.json:   {"P0420": {"t": ..., "d": ..., "c": [[likelihood, text]], "s": [...]}}

Translations of obdex text (CC0) are released with Obdaryn. Checks that every
code exists in dtc_generic.json and that causes/symptoms match the English
counts, so a translation can't silently drop information.
"""

import json
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
EN = json.loads((ROOT / "src/core/data/dtc_generic.json").read_text(encoding="utf-8"))["codes"]
OUT = ROOT / "src/core/data/dtc_generic_ro.json"


def main(paths):
    codes = {}
    errors = []
    for path in paths:
        data = json.loads(pathlib.Path(path).read_text(encoding="utf-8"))
        for code, value in data.items():
            if code not in EN:
                errors.append(f"{code}: not a generic obdex code")
                continue
            entry = codes.setdefault(code, {})
            if isinstance(value, str):
                entry.setdefault("t", value.strip())
                continue
            en = EN[code]
            if len(value.get("c", [])) != len(en.get("c", [])) or len(value.get("s", [])) != len(en.get("s", [])):
                errors.append(f"{code}: causes/symptoms count differs from English; full text skipped")
                continue
            # Keep the title set's consistent style; take the full text from here.
            entry.setdefault("t", value.get("t", ""))
            entry.update({k: value[k] for k in ("d", "c", "s") if k in value})
    for err in errors:
        print("warning:", err)
    out = {"source": {"name": "obdex (Romanian translation for Obdaryn)", "license": "CC0-1.0 source; translation GPL-3.0-or-later"},
           "codes": dict(sorted(codes.items()))}
    OUT.write_text(json.dumps(out, ensure_ascii=False, separators=(",", ":")), encoding="utf-8")
    full = sum(1 for e in codes.values() if "d" in e)
    print(f"wrote {len(codes)} titles ({full} with full text) to {OUT}; {len(EN) - len(codes)} codes without Romanian")


if __name__ == "__main__":
    main(sys.argv[1:])
