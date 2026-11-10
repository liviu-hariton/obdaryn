# Third-party components

| Component | Used for | Licence |
|---|---|---|
| [Qt 6](https://www.qt.io) | Application framework (Core, Network, Gui, Qml, Quick, Quick Controls) | LGPL-3.0 / GPL-3.0 (open-source) |
| [Inter](https://rsms.me/inter/) | UI typeface (`src/app/assets/fonts`) | SIL Open Font License 1.1 |
| [JetBrains Mono](https://www.jetbrains.com/lp/mono/) | Codes, VINs, hex and the adapter log | SIL Open Font License 1.1 |
| [Lucide](https://lucide.dev) | Stroke icons (`src/app/assets/icons`); the engine telltale icon is Obdaryn's own | ISC |
| [obdex](https://github.com/foerbsnavi/obdex) | Generic OBD-II trouble-code titles, descriptions, causes, symptoms and repair hints (`src/core/data/dtc_generic.json`, built by `tools/import_obdex.py`) | CC0-1.0 (public domain) |

The obdex descriptions are written independently and are not SAE J2012 text. We credit the project voluntarily: "OBD-II code data: obdex (https://github.com/foerbsnavi/obdex), CC0-1.0".

PID formulas follow the public SAE J1979 / ISO 15031-5 definitions and were implemented from scratch.

Module addresses in `src/core/data/vehicles/` are facts (CAN identifiers) gathered from public sources: OBDb, FORScan and DDT4All community lists, and VW community documentation. Each entry names its source. No third-party code or descriptive text is copied.

## Planned

- [OBDb](https://github.com/OBDb) manufacturer signal sets, CC-BY-SA-4.0. They will ship as separate, attributed data files, and changes to them will be shared under the same licence.
