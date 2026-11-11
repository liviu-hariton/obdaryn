# Product

## Platform

adaptive

## Stack

Qt 6 + Qt Quick (QML) with a C++20 core, built with CMake. macOS first (14.4 and later, Apple Silicon and Intel, one universal installer); Windows, Android and iOS later from the same codebase. One Obdaryn design language across platforms, honoring each OS's native affordances (menu bar and shortcuts on desktop, safe areas and touch targets on mobile).

## Users

- **Primary: car owners who do their own diagnostics.** Technically curious, not mechanics. They plug a cheap ELM327 adapter in, want to know why the check-engine light is on, whether the car will pass the emissions inspection (in Romania, the ITP), and whether a used car they're looking at is hiding something.
- **Secondary: power users and mechanics** who want raw data, many signals at once and the adapter traffic. Served by denser views, not a separate product.

## Product Purpose

Obdaryn reads everything a car will tell an OBD-II scan tool without paying for a subscription: trouble codes from the engine and other modules with plain-language explanations, freeze frames, readiness monitors, live sensor data, and vehicle and ECU identification. It clears codes only when the user explicitly asks. Success means a user understands what their car is saying and what to do next, without a paywall, ads or guesswork.

## Positioning

- **A read-only tool by design.** A code-level safety guard refuses every request that could write to, code, actuate or flash a vehicle module, and the UI says so. Competitors that sell coding (OBDeleven, Carista) can't truthfully make that promise.
- **Free and open source (GPL-3.0).** The basics that competitors paywall are free: reading and clearing codes, freeze frame, readiness, live data and export.
- **A real desktop app**, where the market's leaders run only as phone or iPad apps.

## Operating Context

- **In the car, parked.** Laptop on the driver's or passenger seat, ignition ON and engine off, to read and clear codes and check readiness. Often in a dim garage or at night.
- **On a test drive.** A passenger watches live gauges and logs. The values must be readable at a glance.
- **Hardware.** Vgate iCar Pro Wi-Fi (ELM327-compatible). The laptop joins the adapter's Wi-Fi access point "V-LINK" (192.168.0.10:35000) and loses internet while connected. Clone adapters are common and flaky.
- **Desk work.** Without a car, the user works in the built-in demo car or with an ELM327 emulator.
- **Checking values.** Users compare readings against other apps, such as Car Scanner on a phone.

## Capabilities and Constraints

- **Allowed services.** OBD-II 01, 02, 03, 06, 07, 09 and 0A. Read-only UDS 0x19, 0x22 and 0x3E. KWP 0x18, 0x1A and 0x21. Clearing (04, 14) only after explicit user confirmation, with the engine off. Everything else is blocked in `src/core/safety/RequestGuard`.
- **Many manufacturers, not just one.** Generic OBD-II works on every car. Brand module lists and code packs are data files per brand; enhanced live data (OBDb, CC-BY-SA) comes later.
- **Trouble-code explanations.** Generic codes come from obdex (CC0, English). Manufacturer codes come from Obdaryn brand packs written from public sources and linked per code (Hyundai/Kia first, English and Romanian). Explanations are always labelled as guidance.
- **Languages.** English (default) and Romanian, switchable at runtime.
- **Clearing has side effects.** It resets readiness monitors, erases freeze frames and does not remove permanent codes. The UI must say this before the user confirms.
- **Units.** Metric, UK (mph, mpg, °C, psi) or US (mph, mpg, °F, psi) for display; everything is stored in metric and exports stay metric.
- **Not planned.** Cloud features and accounts. Obdaryn sends nothing about the user or the car online; its only internet request is the opt-in update check (the newest version number from obdaryn.eu/latest.json).

## Brand Commitments

- Name: **Obdaryn**.
- Voice: plain, direct and calm, like a knowledgeable friend explaining what the car is saying. No alarmism, no upsell.

## Evidence on Hand

- A built-in simulated car (a 2014 Hyundai Elantra profile) for demos and screenshots.
- No users, testimonials, ratings or benchmarks exist yet. Do not invent any.

## Product Principles

1. **Explain first, raw data one step away.** Every code and value has a plain meaning, and the hex is always reachable.
2. **Safety is visible.** The user always knows Obdaryn cannot change their car, and destructive-looking actions (clear) state their consequences.
3. **Honest about uncertainty.** Inspection readiness, VIN decoding and code meanings are guidance, and the UI labels them as such.
4. **Works with cheap hardware.** Connection problems get specific, fixable instructions, not generic errors.
5. **Nothing behind a paywall** that a basic scan tool should do.

## Accessibility & Inclusion

- Used in dim cars and bright garages, so contrast must hold in both light and dark themes.
- Status (MIL, monitor complete, error) is never conveyed by color alone.
- Full keyboard operation on desktop.
- Romanian strings run roughly 20–30% longer than English. Layouts must not truncate them.
