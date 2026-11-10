#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""
Builds src/core/data/dtc_generic.json from the obdex database.

obdex (https://github.com/foerbsnavi/obdex) is a public-domain (CC0-1.0)
collection of 9,500+ generic OBD-II trouble codes, written independently of
the copyrighted SAE J2012 text. For every code it has a title, a plain-language
description, likely causes, symptoms, repair hints and flags.

This script keeps only what Obdaryn shows and stores it with short keys to
keep the embedded file small:

    {
      "source": {...attribution...},
      "codes": {
        "P0420": {
          "t": "title", "d": "description",
          "c": [["high", "cause"], ...],      # likely causes, most likely first
          "s": ["symptom", ...],
          "f": ["mil", "emissions", ...],     # flags that are true
          "r": {"difficulty": "medium", "diy": true, "eur": [80, 350]},
          "rel": ["P0430", ...]
        }
      }
    }

Usage:
    python3 tools/import_obdex.py                 # downloads the latest bundle
    python3 tools/import_obdex.py generic.min.json  # uses a local copy
"""

import json
import pathlib
import sys
import urllib.request

URL = "https://foerbsnavi.github.io/OBDex/generic.min.json"
OUT = pathlib.Path(__file__).resolve().parent.parent / "src/core/data/dtc_generic.json"
LIKELIHOOD_ORDER = {"high": 0, "medium": 1, "low": 2}


def load(argv):
    if len(argv) > 1:
        with open(argv[1], encoding="utf-8") as fh:
            return json.load(fh)
    with urllib.request.urlopen(URL, timeout=60) as resp:
        return json.load(resp)


def en(field):
    """Returns the English text of a {"en": ..., "de": ...} field."""
    if isinstance(field, dict):
        return (field.get("en") or "").strip()
    return (field or "").strip() if isinstance(field, str) else ""


def compact(entry):
    out = {"t": en(entry.get("title"))}
    desc = en(entry.get("description"))
    if desc:
        out["d"] = desc

    causes = sorted(entry.get("common_causes") or [],
                    key=lambda c: LIKELIHOOD_ORDER.get(c.get("likelihood"), 3))
    causes = [[c.get("likelihood", ""), en(c.get("label"))] for c in causes if en(c.get("label"))]
    if causes:
        out["c"] = causes

    symptoms = [en(s) for s in entry.get("symptoms") or [] if en(s)]
    if symptoms:
        out["s"] = symptoms

    flag_names = {
        "mil": "mil",
        "emissions_relevant": "emissions",
        "drive_cycle_required": "drive_cycle",
        "limp_mode_possible": "limp_mode",
    }
    flags = entry.get("flags") or {}
    set_flags = [short for key, short in flag_names.items() if flags.get(key)]
    if set_flags:
        out["f"] = set_flags

    repair = entry.get("repair") or {}
    if repair:
        r = {}
        if repair.get("difficulty"):
            r["difficulty"] = repair["difficulty"]
        if "diy_possible" in repair:
            r["diy"] = bool(repair["diy_possible"])
        if repair.get("estimated_cost_eur"):
            r["eur"] = repair["estimated_cost_eur"]
        if r:
            out["r"] = r

    related = entry.get("related_codes") or []
    if related:
        out["rel"] = related
    return out


def main(argv):
    data = load(argv)
    codes = {}
    for entry in data:
        code = (entry.get("code") or "").strip().upper()
        if len(code) == 5 and entry.get("title"):
            codes[code] = compact(entry)

    result = {
        "source": {
            "name": "obdex",
            "url": "https://github.com/foerbsnavi/obdex",
            "license": "CC0-1.0",
            "note": "Independently written descriptions; not SAE J2012 text.",
        },
        "codes": dict(sorted(codes.items())),
    }
    OUT.parent.mkdir(parents=True, exist_ok=True)
    with open(OUT, "w", encoding="utf-8") as fh:
        json.dump(result, fh, ensure_ascii=False, separators=(",", ":"))
    print(f"wrote {len(codes)} codes to {OUT} ({OUT.stat().st_size / 1e6:.1f} MB)")


if __name__ == "__main__":
    main(sys.argv)
