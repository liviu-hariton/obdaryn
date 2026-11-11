# Manufacturer value packs: OBDb data

The files in this folder (`*.json`) are adapted from the signal sets of
[OBDb](https://github.com/OBDb), the open vehicle signal database, and are
licensed under the **Creative Commons Attribution-ShareAlike 4.0
International** license (CC BY-SA 4.0):
https://creativecommons.org/licenses/by-sa/4.0/

They are data files shipped alongside Obdaryn, not part of its GPL-3.0 code.

## Changes from the original

`tools/import_obdb.py` reads each model's `signalsets/v3/default.json` and:

- keeps only the commands OBDb doesn't mark as debug (unverified);
- keeps only read-only requests: services 0x21 and 0x22 to 11-bit CAN addresses;
- keeps only signals with a numeric value or a yes/no, on/off state;
- leaves out a few signals that decode to impossible values on OBDb's own
  recordings of real cars (listed with reasons in `EXCLUDED`);
- renames a few fields and drops descriptions and metadata Obdaryn doesn't use.

Every model in a pack records the OBDb repository and the commit it came from
(`repo`, `commit`), so the original data can be found and compared.

## Credits

Signal definitions © the OBDb contributors. If you improve a definition while
using Obdaryn, please contribute it back to the model's OBDb repository.
