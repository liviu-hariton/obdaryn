#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""
Validates a brand trouble-code pack (src/core/data/dtc/brands/<brand>.json).

    python3 tools/check_brand_pack.py src/core/data/dtc/brands/hyundai-kia.json

A brand pack explains manufacturer-specific codes (P1xxx, C1xxx, B1xxx, U1xxx...)
for one vehicle profile. Rules, because a wrong meaning on an airbag or ABS
code misleads people about safety systems:

  * every entry names its module and its public sources ("src": [{"u": url, "o": true if official}]);
  * "confidence" is "official" (a Hyundai/Kia document states it; at least one
    source) or "two-sources" (independent sources agree; at least two);
  * English text is required, Romanian is expected (warned when missing);
  * text is written for Obdaryn in its own words: never paste manual text.

Exits 1 on errors, so it can run in CI.
"""

import json
import re
import sys

MODULES = {"engine", "transmission", "hybrid", "abs", "airbag", "steering", "tpms",
           "body", "cluster", "climate", "gateway", "infotainment", "other"}
CODE = re.compile(r"^[PCBU][0-3][0-9A-F]{3}(@[a-z]+)?$")  # "C1604" or "C1604@steering"
FLAGS = {"mil", "emissions", "limp_mode", "warning_lamp"}
LIKELIHOOD = {"high", "medium", "low"}


def check_text(code, lang, t, errors):
    if not isinstance(t, dict) or not t.get("t"):
        errors.append(f"{code}: {lang}.t (title) missing")
        return
    if len(t.get("t", "")) > 90:
        errors.append(f"{code}: {lang}.t longer than 90 characters")
    for cause in t.get("c", []):
        if not (isinstance(cause, list) and len(cause) == 2 and cause[0] in LIKELIHOOD and cause[1]):
            errors.append(f"{code}: {lang}.c entries must be [likelihood, text]")


def main(path):
    with open(path, encoding="utf-8") as fh:
        pack = json.load(fh)
    errors, warnings = [], []
    for key in ("brand", "label", "codes"):
        if key not in pack:
            errors.append(f"top-level '{key}' missing")
    for code, e in pack.get("codes", {}).items():
        if not CODE.match(code):
            errors.append(f"{code}: not a valid trouble code")
        if e.get("module") not in MODULES:
            errors.append(f"{code}: module must be one of {sorted(MODULES)}")
        if "@" in code and code.split("@")[1] != e.get("module"):
            errors.append(f"{code}: the @module suffix must match 'module'")
        conf = e.get("confidence")
        src = e.get("src", [])
        if conf not in ("official", "two-sources"):
            errors.append(f"{code}: confidence must be 'official' or 'two-sources'")
        elif conf == "official" and len(src) < 1:
            errors.append(f"{code}: 'official' needs at least one source")
        elif conf == "two-sources" and len(src) < 2:
            errors.append(f"{code}: 'two-sources' needs at least two sources")
        urls = [r["u"] if isinstance(r, dict) else r for r in src]
        n_official = sum(1 for r in src if isinstance(r, dict) and r.get("o"))
        if conf == "official" and n_official < 1:
            errors.append(f"{code}: 'official' but no reference is flagged official (\"o\": true)")
        for url in urls:
            if not url.startswith(("http://", "https://")):
                errors.append(f"{code}: source '{url}' is not a URL")
        unknown = set(e.get("f", [])) - FLAGS
        if unknown:
            errors.append(f"{code}: unknown flags {sorted(unknown)}")
        check_text(code, "en", e.get("en"), errors)
        if "ro" in e:
            check_text(code, "ro", e["ro"], errors)
        else:
            warnings.append(f"{code}: no Romanian text")

    for w in warnings:
        print("warning:", w)
    for err in errors:
        print("error:", err)
    print(f"{len(pack.get('codes', {}))} codes, {len(errors)} errors, {len(warnings)} warnings")
    sys.exit(1 if errors else 0)


if __name__ == "__main__":
    main(sys.argv[1])
