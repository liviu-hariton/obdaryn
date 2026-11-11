#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""
Imports OBDb signal sets (https://github.com/OBDb, CC BY-SA 4.0) into one
Obdaryn data pack per brand group.

    python3 tools/import_obdb.py                  # Hyundai and Kia models
    python3 tools/import_obdb.py Hyundai-Elantra  # just these repositories

Writes src/core/data/obdb/hyundai-kia.json. The output is an adaptation of
OBDb's data and stays under CC BY-SA 4.0 (see src/core/data/obdb/LICENSE.md);
it is a separate data file, not part of Obdaryn's GPL code.

What the import keeps, and why:

  * Only commands OBDb doesn't mark as debug ("dbg"): debug entries are
    unverified guesses. Commands that are debug only for some years keep that
    range ("dbg") so the app skips them for those years.
  * Only read-only requests Obdaryn's safety guard allows: service 0x21
    (ReadDataByLocalIdentifier, one byte) and 0x22 (ReadDataByIdentifier,
    two bytes), sent to an 11-bit CAN address. Anything else is dropped.
  * Only signals with a numeric value or a yes/no, on/off state. Enumerations
    ("map"), raw hex and unknown units are dropped until the app can show them
    properly.
  * Not the signals in EXCLUDED: they decode to impossible values on OBDb's
    own recordings of real cars.

Each model records the repository and the commit it came from, so the data
can be traced and refreshed.
"""

import json
import os
import re
import subprocess
import sys
import urllib.request

ORG = "OBDb"
GROUPS = {"hyundai-kia": ["Hyundai", "Kia"]}
ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
OUT_DIR = os.path.join(ROOT, "src", "core", "data", "obdb")

# Units the app understands (OBDb name -> kept as is; the app maps them).
NUMERIC_UNITS = {
    "scalar", "celsius", "fahrenheit", "kilometersPerHour", "milesPerHour", "kilometers", "miles",
    "kilopascal", "psi", "bars", "volts", "millivolts", "amps", "percent", "rpm", "degrees",
    "milliseconds", "seconds", "minutes", "hours", "liters", "newtonMeters", "hertz",
    "metersPerSecondSquared", "gravity", "kilowatts", "watts", "kilowattHours", "ampereHours",
    "kiloohms", "millimeters",
}
STATE_UNITS = {"noyes", "offon"}

# Signals left out because they decode to impossible values on OBDb's own
# recordings of real cars (its tests/test_cases). Showing them would mislead.
# Fix them upstream in OBDb, then remove them from this list.
EXCLUDED = {
    # 207 V and 81 V on running 2012-2015 Elantras: the scaling is missing.
    "ELANTRA_CONTROL_MODULE_VOLTAGE": "decodes to 81-207 V",
    "ELANTRA_CONTROL_MODULE_VOLTAGE_AFTER_IGNITION": "decodes to 81-207 V",
    # A raw counter (17,000-18,000) labelled as a voltage.
    "ELANTRA_RELATIVE_ACCELERATOR_PEDAL_POSITION_VOLTAGE": "raw value, no scaling",
    # 49.5 and 111.75 degrees of knock retard at idle: not a plausible retard.
    "ELANTRA_KNOCK_RETARD_CYLINDER_1": "decodes to up to 111 degrees",
    "ELANTRA_KNOCK_RETARD_CYLINDER_2": "decodes to up to 111 degrees",
    "ELANTRA_KNOCK_RETARD_CYLINDER_3": "decodes to up to 111 degrees",
    "ELANTRA_KNOCK_RETARD_CYLINDER_4": "decodes to up to 111 degrees",
}
FMT_KEYS = ("bix", "len", "blsb", "sign", "mul", "div", "add", "min", "max", "nullmin", "nullmax", "unit")


def get(url):
    req = urllib.request.Request(url, headers={"User-Agent": "obdaryn-import"})
    with urllib.request.urlopen(req, timeout=30) as r:
        return r.read()


def repos_for(makes):
    names = []
    for make in makes:
        data = json.loads(get(f"https://api.github.com/search/repositories?q=org:{ORG}+{make}+in:name&per_page=100"))
        # "Hyundai" alone is OBDb's make-wide fallback: every command in it is debug.
        names += [r["name"] for r in data["items"] if r["name"].startswith(make + "-")]
    return sorted(set(names))


def years(f):
    """OBDb filter -> {"from", "to", "years"} with only the keys that are set."""
    if not f:
        return None
    out = {k: f[k] for k in ("from", "to") if k in f}
    if f.get("years"):
        out["years"] = sorted(f["years"])
    return out or None


def keep_command(c):
    if c.get("dbg"):
        return False
    if not re.fullmatch(r"[0-7][0-9A-F]{2}", c.get("hdr", "")):
        return False  # 11-bit CAN only
    cmd = c.get("cmd", {})
    if len(cmd) != 1:
        return False
    service, ident = next(iter(cmd.items()))
    if service == "21":
        return re.fullmatch(r"[0-9A-F]{2}", ident) is not None
    if service == "22":
        return re.fullmatch(r"[0-9A-F]{4}", ident) is not None
    return False


def keep_signal(s):
    if s.get("id") in EXCLUDED:
        return False
    f = s.get("fmt", {})
    if "map" in f or "len" not in f:
        return False
    return f.get("unit") in NUMERIC_UNITS or f.get("unit") in STATE_UNITS


def convert(repo):
    # git, not the GitHub API: the API allows only 60 anonymous calls an hour.
    sha = subprocess.run(["git", "ls-remote", f"https://github.com/{ORG}/{repo}.git", "refs/heads/main"],
                         capture_output=True, text=True, check=True, timeout=60).stdout.split()[0]
    data = json.loads(get(f"https://raw.githubusercontent.com/{ORG}/{repo}/{sha}/signalsets/v3/default.json"))
    make, model = repo.split("-", 1)
    commands = []
    for c in data.get("commands", []):
        if not keep_command(c):
            continue
        signals = []
        for s in c["signals"]:
            if not keep_signal(s):
                continue
            fmt = {k: s["fmt"][k] for k in FMT_KEYS if k in s["fmt"]}
            fmt.setdefault("bix", 0)
            signals.append({"id": s["id"], "name": s["name"], "path": s.get("path", ""), "fmt": fmt})
        if not signals:
            continue
        service, ident = next(iter(c["cmd"].items()))
        out = {"hdr": c["hdr"], "rax": c.get("rax", "%03X" % (int(c["hdr"], 16) + 8)),
               "svc": service, "id": ident, "freq": c.get("freq", 1), "signals": signals}
        if c.get("fcm1"):
            out["fcm1"] = True
        if years(c.get("filter")):
            out["years"] = years(c.get("filter"))
        if years(c.get("dbgfilter")):
            out["dbg"] = years(c.get("dbgfilter"))
        commands.append(out)
    return {"make": make, "model": model.replace("-", " "), "repo": f"{ORG}/{repo}", "commit": sha,
            "commands": commands}


def main(argv):
    os.makedirs(OUT_DIR, exist_ok=True)
    for group, makes in GROUPS.items():
        repos = argv or repos_for(makes)
        models = []
        for repo in repos:
            try:
                m = convert(repo)
            except Exception as e:  # a repository without a v3 signal set
                print(f"  skipped {repo}: {e}")
                continue
            if m["commands"]:
                models.append(m)
                print(f"  {repo}: {len(m['commands'])} commands, "
                      f"{sum(len(c['signals']) for c in m['commands'])} values")
            else:
                print(f"  {repo}: nothing usable")
        pack = {
            "format": "obdaryn-obdb",
            "version": 1,
            "license": "CC-BY-SA-4.0",
            "attribution": "Signal definitions from OBDb (https://github.com/OBDb), CC BY-SA 4.0. "
                           "Filtered and reformatted for Obdaryn by tools/import_obdb.py.",
            "models": sorted(models, key=lambda m: (m["make"], m["model"])),
        }
        path = os.path.join(OUT_DIR, f"{group}.json")
        with open(path, "w", encoding="utf-8") as f:
            json.dump(pack, f, ensure_ascii=False, separators=(",", ":"))
            f.write("\n")
        print(f"Wrote {path} ({os.path.getsize(path) // 1024} KB, {len(models)} models)")


if __name__ == "__main__":
    main(sys.argv[1:])
