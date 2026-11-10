#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""
Fills a Qt Linguist .ts file from a Python dictionary of translations.

Qt Linguist is the normal way to edit i18n/obdaryn_ro.ts. This script exists for
bulk work, e.g. after many new strings were added:

    python3 tools/fill_translations.py i18n/obdaryn_ro.ts my_translations.py

The dictionary file must define RO = {"English source": "translation", ...}.
Plural messages (qsTr("%n stored", "", n)) take a list of 3 forms for Romanian:
[one, few, other] -> ["%n memorat", "%n memorate", "%n de memorate"].

Only messages still marked "unfinished" are touched, so translations made in
Qt Linguist are never overwritten. Prints the sources that are still missing.
"""

import runpy
import sys
import xml.etree.ElementTree as ET


def main(ts_path, dict_path):
    translations = runpy.run_path(dict_path)["RO"]
    tree = ET.parse(ts_path)
    missing = []
    filled = 0
    for message in tree.getroot().iter("message"):
        source = message.find("source").text or ""
        translation = message.find("translation")
        if translation is None:
            translation = ET.SubElement(message, "translation")
        if translation.get("type") not in ("unfinished", None) or (
            translation.get("type") is None and (translation.text or list(translation))
        ):
            continue  # already translated by a human
        value = translations.get(source)
        if value is None:
            missing.append(source)
            continue
        for child in list(translation):
            translation.remove(child)
        if message.get("numerus") == "yes":
            forms = value if isinstance(value, list) else [value] * 3
            translation.text = None
            for form in forms:
                ET.SubElement(translation, "numerusform").text = form
        else:
            translation.text = value
        translation.attrib.pop("type", None)
        filled += 1

    tree.write(ts_path, encoding="utf-8", xml_declaration=True)
    print(f"filled {filled} messages; {len(missing)} still untranslated")
    for m in missing:
        print("  missing:", m)


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
