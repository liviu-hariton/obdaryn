# Obdaryn architecture

```
┌──────────────────────────── src/app (Qt Quick) ────────────────────────────┐
│  QML screens  ──►  App singleton (AppController)                            │
│                     ├─ LiveDataModel   (values + sparkline history)         │
│                     ├─ DtcModel        (codes + plain-language titles)      │
│                     └─ TrafficLogModel (adapter console)                    │
└──────────────────────────────────┬─────────────────────────────────────────┘
                                   │ asynchronous calls with callbacks
┌──────────────────────────── src/core (QtCore + QtNetwork only) ────────────┐
│  ObdClient      "read codes", "read PIDs", "clear", "vehicle info"…         │
│      │          builds requests, decodes answers (Pids, Dtc, Readiness,     │
│      │          VehicleInfo, DtcDatabase)                                    │
│      ▼                                                                       │
│  ElmSession     one command at a time, waits for '>', timeouts, init        │
│      │          ── every line passes RequestGuard (read-only allowlist) ──   │
│      ▼                                                                       │
│  Transport      TcpTransport (Wi-Fi) · SimulatedTransport (demo / tests)    │
│                 ReplayTransport (recordings) · later: BLE, USB serial         │
└──────────────────────────────────────────────────────────────────────────────┘
```

## Layers

- **Transport** (`src/core/transport`) moves bytes and nothing else. Adding Bluetooth LE or USB means one new class.
- **ElmSession** (`src/core/elm327`) speaks the ELM327 text protocol.
  - **Queueing.** It keeps a queue, because the adapter handles one command at a time.
  - **Prompt and errors.** It strips echoes and `SEARCHING...`, waits for the `>` prompt, and classifies `NO DATA`, `?`, `CAN ERROR` and similar replies.
  - **Initialization.** It sends the init sequence: `ATZ`, `ATE0`, `ATL0`, `ATS1`, `ATH1`, `ATAT1`, `ATSP0`, `0100`, `ATDPN`.
  - **Timeouts.** It never sends a bare CR, because the ELM327 would repeat the last command.
- **ConnectionKeeper** (`src/core/elm327`) keeps a session alive. `ElmSession` raises `adapterUnresponsive` after three timeouts in a row and switches itself to NoVehicle when live-data requests go unanswered for 8 s (`vehicleSilent`). The keeper reopens a lost link with backoff, only after the session was Ready once and never after the user disconnected, and asks the car again every 5 s while it is silent. Its connections are queued, so it never acts in the middle of the session's reply handling.
- **AdapterCheck** (`src/core/elm327`) asks identification and a few read-only adapter commands and returns a verdict and a capability list. `ElmSession::health()` keeps answer-time statistics.
- **Response counts** (`ObdClient`): live-data requests get the ELM327's trailing response-count digit when the answering ECUs are known from the supported-PID map and the answer fits one CAN frame; a "?" turns them off for the connection.
- **RequestGuard** (`src/core/safety`) is the only gate to the car. It holds an allowlist of read services and adapter commands, and lets clear services through only with explicit confirmation. Its unit tests pin down exactly what may pass.
- **ResponseParser** (`src/core/obd`) turns reply lines into per-ECU byte messages. It handles:
  - headers on and off,
  - spaces on and off,
  - 11-bit and 29-bit CAN,
  - ISO-TP multi-frame reassembly,
  - legacy protocols with checksum bytes.
- **Decoders** (`src/core/obd`):
  - `Pids`: about 90 standard SAE J1979 parameters with their formulas.
  - `Dtc`: 2-byte codes such as P0420, and the system each one belongs to.
  - `Readiness`: the monitor status in PID 01.
  - `MonitorTests`: service 06 test records (MID, TID, unit and scaling ID, value, limits), scaled and judged.
  - `VehicleInfo`: VIN, model year, manufacturer, calibration IDs, CVNs and ECU names.
  - `DtcDatabase`: plain-language explanations from obdex.
- **Recording** (`src/core/recording`) saves and replays sessions.
  - `SessionRecorder` listens to `ElmSession::exchanged` and writes every command with its raw reply, its timing and the module it was addressed to (`AT SH`) to a `.obdrec` file (JSON Lines, flushed line by line). Answers from before Record was pressed (the handshake, the VIN) go in first as snapshot entries, so a replay can always connect.
  - `ReplayTransport` is a pretend adapter that answers from a recording, following the recording's clock and looping at the end. Requests are matched by module + command; live data (service 01) is rebuilt value by value, so any combination of values the app asks for still gets answers. Clears are never replayed.
  - `Recording::toCsv()` exports the live data, one row per moment and one column per value.
  - The same files work in the app ("Replay a recording" on the connect screen) and in the probe (`--record`, `--replay`).
- **VehicleReport** (`src/core/report`) turns a `ReportInput` (the last scan, module scan, readiness, freeze frame, "since codes were cleared" counters) into one standalone HTML document. Every value from the car or a recording is HTML-escaped. It keeps to tables, paragraphs and simple CSS so the same document works as a web page and in Qt's rich-text engine; `Target::Pdf` drops screen-only rules and uses static system fonts, because PDF output can't use the variable Inter.
  - The app's `ReportWriter` (`src/app/report`, it needs QtGui) renders the PDF: A4, laid out at screen DPI and scaled onto the page, with a fixed dark ink (never the dark-mode palette) and a page-number footer.
- **TripComputer** (`src/core/trip`) integrates live values into trip statistics: distance from speed (trapezoids, gaps over 5 s not counted), fuel from PID 5E, from MAF (with the commanded equivalence ratio), or by speed-density (ideal-gas air density × displacement × rpm/120 × 85 % volumetric efficiency) for cars without an air-flow sensor, with zero fuel during deceleration fuel cut. `learnCalibration()` turns fill-ups into a liter-weighted correction factor. The app polls the trip's inputs on every cycle (`LiveDataModel::setRequiredPids`), banks fuel toward the per-car fill-up count every 30 s, and saves trips per car (QSettings, keyed by VIN) on disconnect, new trip or quit.
- **ModuleCatalog** (`src/core/vehicles`) loads the brand profiles from `data/vehicles/*.json`: one file per brand listing each module's category, request and response CAN IDs, preferred protocol, and whether the address is verified. A profile is picked from the VIN prefix, then from the manufacturer name, else the generic list.
- **ModuleScanner** (`src/core/obd`) runs the full-vehicle scan. For each module it:
  1. points the adapter at the module (`AT SH`, `AT CRA`, and standard flow control when the response ID isn't request ID + 8);
  2. reads codes with UDS `19 02 FF` or KWP2000 `18 02 FF 00`, falling back from one to the other;
  3. reads the part number and software version.

  It always restores functional addressing (`AT SH 7DF`) afterwards. The app pauses live polling for the duration, and waits for an in-flight poll to finish first, because the adapter's target address is shared state.
- **ObdClient** (`src/core/obd`) offers diagnostic intents as single asynchronous calls:
  - **Multi-PID batching.** On CAN it batches up to 6 PIDs per request, falling back to one per request when an ECU ignores the extras.
  - **Safe clearing.** It refuses to clear codes while the engine runs.
- **App** (`src/app`):
  - **QML singleton.** `AppController` is exposed to QML as the `App` singleton and sequences the first scan: vehicle info → readiness → codes → freeze frame → supported PIDs.
  - **Polling.** After the scan, live data is polled continuously. Pinned values refresh on every cycle; the rest refresh in rotation.

## Conventions

- **Licence header.** Every file starts with `SPDX-License-Identifier: GPL-3.0-or-later`.
- **Translatable strings.**
  - QML uses `qsTr()`.
  - C++ uses `tr()` in QObject classes, or `QT_TRANSLATE_NOOP` plus `QCoreApplication::translate()` in tables.
  - Romanian translations live in `i18n/obdaryn_ro.ts` and are edited with Qt Linguist or `tools/translate_ro.py`.
- **Callbacks.** Asynchronous APIs take a callback that is **always called exactly once**, even on errors or disconnects.
- **No GUI in core.** `obdaryn_core` must stay free of QtGui and QtQuick, so it can back a CLI or a mobile app.

## Tests

`ctest --test-dir build --output-on-failure` runs the unit tests. They need no hardware:

- `tst_requestguard`: what may and may not reach the car.
- `tst_responseparser`: recorded adapter output, including multi-frame, 29-bit and legacy.
- `tst_decoders`: PID formulas, DTC decoding, readiness, VIN and the DTC database.
- `tst_session`: end-to-end runs against the simulator (connect, scan, clear with the engine on and off, guard enforcement).
- `tst_modules`: brand profiles, UDS and KWP code decoding, and a full module scan of the simulated Hyundai, including the protocol fallback, restored addressing and a guarded per-module clear.
