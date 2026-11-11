# Third-party components

| Component | Used for | Licence |
|---|---|---|
| [Qt 6](https://www.qt.io) | Application framework (Core, Network, Gui, Qml, Quick, Quick Controls, Svg, Widgets for the macOS menu bar). The installer bundles the official Qt 6.12 binaries as separate frameworks inside the app, unmodified | LGPL-3.0 / GPL-3.0 (open-source) |
| [Inter](https://rsms.me/inter/) | UI typeface (`src/app/assets/fonts`), and the website's, self-hosted through the `@fontsource-variable/inter` package | SIL Open Font License 1.1 |
| [JetBrains Mono](https://www.jetbrains.com/lp/mono/) | Codes, VINs, hex and the adapter log, in the app and on the website (`@fontsource-variable/jetbrains-mono`) | SIL Open Font License 1.1 |
| [Lucide](https://lucide.dev) | Stroke icons (`src/app/assets/icons`, and the website's `site/src/components/Icon.astro`); the engine telltale icon is Obdaryn's own | ISC |
| [Astro](https://astro.build) | Builds the website in `site/` (not part of the app) | MIT |
| GitHub mark | The website's links to the GitHub repository (`site/src/components/Icon.astro`), used as [GitHub's logo guidelines](https://github.com/logos) allow | GitHub's trademark; not covered by Obdaryn's licence |
| [OBDb](https://github.com/OBDb) | Manufacturer values for Hyundai and Kia models (`src/core/data/obdb/hyundai-kia.json`, built by `tools/import_obdb.py`), and recorded Elantra answers used as test data (`tests/data/obdb_elantra_cases.json`) | CC BY-SA 4.0 (adapted: filtered and reformatted; see `src/core/data/obdb/LICENSE.md`) |
| [obdex](https://github.com/foerbsnavi/obdex) | Generic OBD-II trouble-code titles, descriptions, causes, symptoms and repair hints (`src/core/data/dtc_generic.json`, built by `tools/import_obdex.py`) | CC0-1.0 (public domain) |

The website's code pages show the same obdex texts and brand-pack entries as the app, with the same credit on each page.

The obdex descriptions are written independently and are not SAE J2012 text. We credit the project voluntarily: "OBD-II code data: obdex (https://github.com/foerbsnavi/obdex), CC0-1.0".

PID formulas follow the public SAE J1979 / ISO 15031-5 definitions and were implemented from scratch.

Module addresses in `src/core/data/vehicles/` are facts (CAN identifiers) gathered from public sources: OBDb, FORScan and DDT4All community lists, and VW community documentation. Each entry names its source. No third-party code or descriptive text is copied.

Brand trouble-code packs (`src/core/data/dtc/brands/`) are written for Obdaryn (GPL-3.0-or-later). The meanings are facts checked against public sources, mostly Hyundai/Kia documents filed with NHTSA, and each entry links them. No manual text is copied.

The OBDb data ships as separate data files under CC BY-SA 4.0, not as part of Obdaryn's GPL code. Each model in the pack records the OBDb repository and commit it came from. Signals that decode to impossible values on OBDb's own recordings are left out (listed with reasons in `tools/import_obdb.py`); fixes belong upstream in OBDb.
