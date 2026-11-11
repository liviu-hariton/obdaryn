# Obdaryn roadmap

The ordering follows what DIY owners most often say existing apps lack, or keep behind paywalls (see the research notes below). Every item stays within the read-only safety rules described in [CAR_INTERACTION.md](CAR_INTERACTION.md).

## v0.1 — generic OBD-II on the desktop

- [x] Wi-Fi ELM327 connection (Vgate iCar Pro), ELM327 initialization and protocol auto-detection
- [x] Safety guard: an allowlist of read services, clearing only on confirmation, unit-tested
- [x] Trouble codes (stored / pending / permanent) from every OBD module, with plain-language explanations (obdex, 9,533 codes)
- [x] Freeze frame
- [x] Readiness monitors with an "is it ready for the ITP?" verdict
- [x] Vehicle info: VIN decoding, calibration IDs, CVNs, ECU names
- [x] Live data with multi-PID batching (up to 6 PIDs per CAN request), pinned dashboard values, sparklines
- [x] Clearing codes: refused while the engine runs, with the consequences explained first
- [x] Adapter console (raw traffic) for troubleshooting
- [x] Built-in demo car; English and Romanian with runtime switching

## v0.2 — every module, not just the engine (in progress)

- [x] **Full-vehicle scan.** Probes the known module addresses per brand and reads their codes with UDS 0x19 or KWP 0x18, falling back automatically. Covers ABS/ESC, airbag, transmission, body, cluster, steering and more.
  - Brand profiles: Hyundai/Kia/Genesis, Toyota/Lexus, Ford, the VW group (MQB), Renault/Dacia, plus a generic fallback for any car.
  - Profiles are JSON in `src/core/data/vehicles/`. Addresses not yet confirmed on a real car are labelled "unconfirmed".
  - Standard ISO-TP flow control (`AT FC SH/SM/SD 30 00 00`) for brands whose modules don't answer on request ID + 8.
  - Honest states: a module that answers generic OBD-II but not code requests is reported as such, not as "not found".
  - Each answering module's spare-part number and software version.
  - `obdaryn-probe --modules` runs the same scan from the terminal.
- [x] **Clear codes per module** (UDS/KWP 0x14), with the same confirmation flow and engine-off check.
- [ ] Confirm the Hyundai addresses on the real 2014 Elantra, then mark them verified.
- [ ] 29-bit CAN modules (Honda, some GM), Toyota's extended addressing behind gateway 750, and VW's older TP 2.0.
- [x] Brand-specific code descriptions, starting with Hyundai/Kia: 159 codes (177 module-specific entries) in English and Romanian, with sources (see `docs/brand-packs.md`).
- [x] Brand packs for Toyota/Lexus, Ford, VW group and Renault/Dacia (see `docs/brand-packs.md`).
- [x] **Suzuki,** first part: a profile recognised from the VIN (with the Santana-built `VSE` and the Toyota-built Across fixed), and a pack of 23 engine and automatic-gearbox codes for 2005–2014 Swift, SX4 and Grand Vitara.
  - [ ] Module addresses for ABS, airbag, body and the rest: none are public; to be found on a real 2010–2017 Swift (K12B) with the module discovery mode.
  - [ ] Confirm the pack's codes on the 2010–2017 Swift and later; ABS and airbag codes once their modules can be read.
- [x] **Manufacturer data packs.** OBDb signal sets for 52 Hyundai and Kia models (`tools/import_obdb.py`, CC BY-SA 4.0, a separate attributed data file): read-only `21xx` / `22xxxx` requests, verified (non-debug) commands only, filtered by model year. Live data gets a "Hyundai values" view: choose the model, find what the modules answer (remembered per VIN), then the values refresh at OBDb's suggested rates while on screen. The probe reads them too (`--model Elantra`), and `tools/first-contact.sh` asks for the model.
  - Decoder checked against OBDb's recordings of real 2012–2015 Elantras; signals that decode to impossible values there are left out.
  - [ ] Confirm the Elantra values against Car Scanner on the real car; report wrong definitions upstream to OBDb.
  - [x] Romanian names for the common values (about 120 names and templates such as "Misfires, cylinder %1", and the groups); the rest stay as OBDb names them.
  - [ ] Charting and pinning manufacturer values.
  - [ ] More brands (Toyota, Ford, VW group, Renault/Dacia) once their OBDb sets are checked.
- [ ] **Brand pack loader.** A JSON schema, a contribute-back flow, and a "record which PIDs my car answers" mode.

## v0.3 — logging and reports

- [x] **Record sessions** and replay them without the car. A recording holds every request and raw answer with timing and target module (`.obdrec`, JSON Lines). Replay follows the recorded clock, rebuilds live data per value, and never replays a clear. The probe records too (`--record`, `--replay`), and `tools/first-contact.sh` saves a recording next to each log.
- [x] **Charts:** up to six values, one strip each on a shared time axis (never two y-scales), live (last 1–10 minutes, the charted values polled every cycle) or from a recording, optionally compared with a second recording (A solid, B dashed, with a legend). A crosshair reads every value at one moment; built-in and saved presets; copy as CSV. Lines are reduced to two points per pixel column so spikes survive
- [x] CSV export of a recording's live data: one row per moment, one column per value (Parquet maybe later)
- [x] **Shareable report** (PDF or web page): a summary at a glance (check-engine light, codes, other modules, ITP readiness, when the codes were last cleared, battery), every code with its explanation, the module scan, readiness monitors, freeze frame and vehicle identification. It says plainly when it was made from the demo car or a recording. Save with ⌘S or "Save report…" on the Overview.
- [x] The probe writes the same report (`--report-html`, with the self-test results and battery voltage), so first-contact runs leave one behind
- [x] **Trip and fuel statistics.** Distance from speed; fuel from the fuel-rate value, the air-flow sensor, or speed-density (manifold pressure, intake temperature, rpm and the engine size) for cars without one; fuel cut while coasting counts as zero. A per-car calibration factor learned from fill-ups (liter-weighted, refuses implausible ones). Trips of real cars are saved per car; demo and replay trips never are.
- [x] Trip statistics in the shareable report ("This drive"), and a fuel price (per liter or per gallon, any currency) for trip costs, kept with each saved trip

## v0.4 — adapter health and robustness

- [x] **Adapter check.** Identification (AT I, @1, @2) plus read-only commands every genuine v1.4+ knows (AT IGN, AT CS, AT PPS), flow-control support and voltage. A calm verdict: genuine, ELM327-compatible (including the "v1.5" that Elm Electronics never made), or unknown. Shown above the adapter log and in the probe report.
- [x] **Connection health:** answer time (average and 95th percentile), timeouts, errors on car requests, combined requests and response counts, live on the adapter page.
- [x] **Response-count digits** ("010C0D1") when the answering ECUs are known and the answer fits one CAN frame; automatic fallback for adapters that reject them. The probe benchmarks polling with and without them.
- [ ] Tune `AT ST` from measured answer times (needs the real adapter's numbers first)
- [x] **Auto-reconnect.** A dropped link or a sleeping adapter (three timeouts in a row) is reconnected with backoff (2, 4, 8, 15, then 30 s, eight tries). A car that goes silent (ignition off) puts Obdaryn in "car not answering", and it picks up again by itself when the car answers. The demo car has an ignition switch to try it.
- [x] **Connection help:** failures are classified; "unreachable" and "timeout" explain the Wi-Fi and macOS Local Network permission steps, with a button that opens the right System Settings pane; "refused" points to another app holding the adapter.

## Distribution and website

- [x] **Installer for macOS 14.4 and later,** one universal app for Apple Silicon and Intel, built with the official Qt binaries (`tools/make-release.sh`). Qt modules the app never loads are left out (77 MB → 56 MB), guarded by a link check and an "every screen opens" check.
- [x] **Releases on GitHub** with the installer and its SHA-256 (`tools/publish-release.sh`).
- [x] **obdaryn.eu,** in English and Romanian, built from the app's data: the home page explains a demo code the way the app does, a page for every trouble code the app explains (9,533 generic and 461 manufacturer explanations), supported cars, the user guide, a download page that walks through macOS's first-open step, and no cookies or trackers (`tools/deploy-site.sh`).
- [ ] Sign with an Apple Developer ID and notarize, so macOS opens the app without the Open Anyway step (needs a paid Apple Developer account).
- [ ] Run the installer on a real Intel Mac (so far only under Rosetta).
- [ ] macOS older than 14.4: needs an older Qt, or Qt built from source.
- [ ] Romanian explanations for more generic codes, which also gives them Romanian pages on the site (132 today).
- [ ] Replace the `Qt.labs.platform` menu bar with Qt Quick's native menus, which would drop QtWidgets (12 MB) from the installer.

## Later

- [ ] Windows build (MSIX or installer), then Android and iOS (Qt for mobile; BLE adapters on iOS)
- [ ] Bluetooth LE and USB serial transports
- [x] **Mode 06 (on-board monitor test results)** on CAN cars: every monitor's latest test with its limits, units from the SAE J1979 table (unknown units shown raw, still judged), verdicts passed / close to the limit (within 10 %) / failed / not run yet, a small range bar per test, and a section in the shareable report
- [x] Romanian titles for all generic codes, plus full Romanian explanations for the 132 most common
- [x] Units: metric, UK (mph, mpg, °C, psi) and US (mph, mpg, °F, psi). Values are stored in metric and converted for display; exports stay metric

## Research notes (November 2026)

- **Paywalls and subscriptions** are the most common complaint. Carista costs about $60 a year, OBDeleven charges per-action credits, and OBD Auto Doctor has a Premium tier for freeze frame, clearing and export. Car Scanner is the cheap exception.
- **Clone adapters** cause most connection instability. Don't trust `ATI`; probe the features instead.
- **Coverage beyond the engine** (ABS, airbag) is the most requested capability once the basics work.
- **Code explanations.** Users want severity, likely causes and whether it is safe to drive, not just a label.
- **No native desktop app.** None of the leading apps has one: Car Scanner runs on Macs only as an iPad app.
- **Vgate iCar Pro Wi-Fi.**
  - It is Wi-Fi only: a Hi-Flying LPT230 module bridges an ELM327-compatible chip, with access point "V-LINK" at 192.168.0.10:35000.
  - It sleeps after about 20 minutes idle.
  - It typically reports v2.1–v2.3, but the firmware is Vgate's own.
