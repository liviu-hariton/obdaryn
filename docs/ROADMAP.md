# Obdaryn roadmap

The ordering follows what DIY owners most often say existing apps lack, or keep behind paywalls (see the research notes below). Every item stays within the read-only safety rules described in [CAR_INTERACTION.md](CAR_INTERACTION.md).

## v0.1 — generic OBD-II on the desktop (this release)

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
- [x] Brand-specific code descriptions, starting with Hyundai/Kia: 177 sourced entries in English and Romanian, module-aware (see `docs/brand-packs.md`).
- [x] Brand packs for Toyota/Lexus, Ford, VW group and Renault/Dacia (see `docs/brand-packs.md`).
- [ ] **Manufacturer data packs.** Import OBDb signal sets (CC-BY-SA-4.0, shipped as separate attributed data files) for enhanced live data. Hyundai/Kia first: `21xx` reads on 7E0 / 7E1 / 7D1 and similar.
- [ ] **Brand pack loader.** A JSON schema, a contribute-back flow, and a "record which PIDs my car answers" mode.

## v0.3 — logging and reports

- [ ] Record sessions: values plus raw adapter traffic, replayable through the simulator
- [ ] Charts with several signals, saved presets, and session comparison
- [ ] CSV export (Parquet maybe later)
- [ ] **Shareable report** (PDF/HTML): codes with explanations, readiness, freeze frame and VIN. For used-car checks and mechanics.
- [ ] **Trip and fuel statistics** from MAF or speed-density, with a calibration factor checked against fill-ups

## v0.4 — adapter health and robustness

- [ ] Adapter fingerprinting (clone detection by probing commands), latency and poll-rate benchmark
- [ ] Tune `AT ST` and use response-count digits for faster polling
- [ ] Auto-reconnect; detect the adapter going to sleep
- [ ] Guided macOS Local Network permission help

## Later

- [ ] Windows build (MSIX or installer), then Android and iOS (Qt for mobile; BLE adapters on iOS)
- [ ] Bluetooth LE and USB serial transports
- [ ] Mode 06 (on-board monitor test results) with readable limits
- [x] Romanian titles for all generic codes, plus full Romanian explanations for the 132 most common
- [ ] Imperial units
- [ ] Brand-specific DTC description packs (P1xxx, B, C, U codes), written in our own words

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
