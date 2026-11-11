# How and when Obdaryn talks to the car

This guide covers what each feature asks of the car, which state the car must be in, and what to expect from the Vgate iCar Pro Wi-Fi.

## 1. What you need

- **The adapter.** A Vgate iCar Pro Wi-Fi or any ELM327-compatible Wi-Fi adapter.
- **The OBD-II socket.** On the 2014 Elantra it is under the dashboard, left of the steering column, above the pedals. Every car sold in the EU since 2001 (petrol) or 2004 (diesel) has one, within reach of the driver's seat.
- **A laptop with Obdaryn**, macOS for now.

## 2. Connecting

1. Plug the adapter into the OBD socket. It is powered from pin 16, which carries battery voltage even with the ignition off.
2. Press the adapter's button if it has one. The iCar Pro sleeps after about 20 minutes idle, and the button wakes it.
3. On the laptop, join the Wi-Fi network named **V-LINK**. It is open, with no password.
   - While you are on it, the laptop has **no internet**. That's normal.
   - macOS may jump back to your home Wi-Fi on its own. If the connection drops, check the Wi-Fi menu.
4. In Obdaryn, choose **Connect**. The default address is `192.168.0.10`, port `35000`.
   - The first time, macOS asks whether Obdaryn may use the **local network**. Allow it.
   - If you clicked "Don't allow", turn it on under **System Settings → Privacy & Security → Local Network**.
5. Only one app can use the adapter at a time. Close Car Scanner, Torque and the like on your phone first.

## 3. Which car state for which feature

| What you want | Ignition | Engine | Why |
|---|---|---|---|
| Read trouble codes (stored / pending / permanent) | ON | off or running | Modules answer as soon as they are powered |
| Freeze frame | ON | off or running | It is stored data, so the engine state doesn't matter |
| Readiness / "ready for ITP?" | ON | off or running | Status is stored and updated while driving |
| Vehicle info (VIN, calibration IDs, ECU names) | ON | off or running | |
| Live data at rest (temperatures, voltage, fuel trims at idle) | ON | running | Most sensors read 0 or are frozen with the engine off |
| Live data while driving | ON | running | A **passenger** watches the screen. Never the driver. |
| **Clear trouble codes** | **ON** | **off** | Obdaryn refuses while it sees engine speed above 0 |

"Ignition ON" means key position II, or pressing START twice without your foot on the brake on push-button cars. The dashboard lights up, but the engine does not start.

**Battery.** With the engine off and the ignition ON, the car draws several amps. Keep sessions under about 20–30 minutes, or run the engine. Obdaryn shows the battery voltage in the top strip:
- about 12.4–12.7 V with the engine off is healthy,
- about 13.8–14.6 V with the engine running means the alternator is charging,
- below about 12.0 V, stop and start the engine.

## 4. Scanning every module

Generic OBD-II only reaches the engine (and usually the transmission). Warning lights for ABS, airbag, tyre pressure or the body computer come from modules that must be asked one by one. **All modules** does that:

- **Car state.** Ignition ON. The engine may run.
- **Duration.** About 5–15 seconds; each missing module costs a short adapter timeout. Live data pauses meanwhile.
- **Which module list.** Obdaryn picks the module list from the VIN (Hyundai/Kia for your Elantra). You can choose another list if the VIN isn't reported.
- **"Unconfirmed address".** These addresses come from community lists. If such a module shows "Not found", the module may be fine but at another address on your model.
- **What each module answers.** For every module that answers you get its codes, whether each fault is present right now, whether it asks for a warning light, and its part number.
- **Clearing a module's codes.** Same rules as before: confirm, engine off, ignition ON. Clearing never repairs anything; an active ABS or airbag fault comes straight back.

## 4b. Manufacturer values (Hyundai and Kia)

Standard OBD-II live data is the same on every car. A brand's modules know much more: oil temperature, each wheel's speed, the steering angle, tyre pressures. **Live data → Hyundai values** reads them, from the community [OBDb](https://github.com/OBDb) definitions:

- **What is sent.** Read-only requests only: `21 xx` (read local data) and `22 xx xx` (read data by identifier), each to one module, after pointing the adapter at it (`AT SH`, `AT CRA`). The safety guard checks each one; the standard addressing is restored afterwards.
- **Choose the model once.** The definitions differ per model and year. The year comes from the VIN; the model you pick, and Obdaryn remembers it for this car.
- **Find values.** Obdaryn asks each module once which values it answers, which takes up to a minute (absent modules cost a short timeout each). After that it reads only what answered, a few requests at a time between the standard live data, and only while the screen is open.
- **Car state.** Ignition ON; engine running for meaningful engine values.
- **Not checked on your car.** These definitions come from other owners' cars. Compare a few values with Car Scanner before relying on them, and report wrong ones to OBDb.

## 4c. Looking for modules a brand list doesn't know (discovery)

For brands whose module addresses aren't public yet (Suzuki today), `obdaryn-probe --discover` (step 3 of `tools/first-contact.sh`) asks every 11-bit diagnostic address whether a module is there:

- **What it sends:** `AT CRA` once, so the adapter shows answers from any reply ID; then for each address from 700 to 7F7, `AT SH <address>` and TesterPresent (`3E 00`); at the end `AT FCSM0`, `AT CRA` and `AT SH 7DF` restore the standard addressing. TesterPresent reads nothing and changes nothing in a module, in any session. All of it is on the read-only allowlist; nothing new was allowed for discovery.
- **What counts as an answer:** a positive reply (`7E 00`) or a refusal (`7F 3E xx`), which still proves a module is there. Other frames on the bus are ignored.
- **What it never does:** transmit on the functional address 7DF, on the standard reply IDs 7E8–7EF, or on a reply ID found during the sweep, so it never sends with an ID another module answers with.
- **Then:** the ordinary module scan reads the codes and part numbers of everything that answered, with UDS `19 02 FF` or KWP `18 02 FF 00` as usual.
- **Car state:** ignition ON, engine off. About 240 addresses, roughly a minute.
- **Limits:** 11-bit CAN only. Modules behind an extended address (Toyota's 750 gateway) or on 29-bit CAN aren't found this way.

## 5. What clearing codes really does

Clearing (OBD-II service 04) is the only thing Obdaryn does that changes anything in the car. It is still not a write to a module's software or settings: the clear command is part of the standard. Before you confirm, know that clearing:

- **turns off the check-engine light**, but if the fault is still there, the light comes back, sometimes immediately and sometimes after a few drives;
- **erases the freeze frames**, the snapshot of sensor values taken when the fault happened. Read them first. Obdaryn's confirmation dialog reminds you;
- **resets all readiness monitors to "not complete"**. The car will **fail the ITP emissions check** until it has been driven enough for the monitors to re-run, typically 50–150 km of mixed city and highway driving over a few days. Never clear codes right before an inspection;
- **does not remove permanent codes**. Those disappear by themselves once the car has re-tested the system and found it fixed.

Clearing codes in other modules (ABS, airbag, ...) works the same way, with the same confirmation (see section 4).

## 6. What Obdaryn will never send

The safety guard (`src/core/safety/RequestGuard.cpp`) checks every single command. It only lets through:

- **OBD-II services:**
  - read services 01, 02, 03, 06, 07, 09 and 0A;
  - 04, only after you confirm.
- **Read-only manufacturer services:**
  - UDS: 0x19 (read codes), 0x22 (read data by identifier), 0x3E (keep-alive);
  - KWP2000: 0x18, 0x1A and 0x21;
  - 0x14 (clear a module's codes), only after you confirm.

Everything that could change how the car behaves is refused, whichever part of the app asks: writing settings (coding, adaptations), actuator tests, routines, security access, ECU resets, diagnostic session changes and flashing. The guard also refuses adapter commands that could push arbitrary bytes onto the car's network. ISO-TP flow control is allowed only as the standard "continue" frame (`30 00 00`), which the adapter needs to receive long answers from some brands' modules. A few read-only adapter commands (`AT IGN`, `AT CS`, `AT PPS`, `AT KW`, besides identification and voltage) are allowed so Obdaryn can tell what an adapter really supports; they never reach the car.

## 7. What the 2014 Elantra will (probably) show

- **Protocol.** ISO 15765-4 CAN, 11-bit, 500 kbit/s. Obdaryn detects it automatically.
- **Modules that answer generic OBD-II.**
  - The engine (ECM, address 7E8) answers.
  - The automatic transmission (TCM, 7E9) usually answers too.
  - ABS, airbag, body and cluster modules do **not** answer generic OBD-II. Use **All modules** to read them.
- **Live data.** Typically 25–40 standard values. On CAN, Obdaryn asks for up to 6 values per request, so the dashboard refreshes several times per second.
- **Permanent codes.** Many EU-market cars answer "no data" for permanent codes (service 0A). That's normal.

## 8. Checking Obdaryn against another app

Compare against **Car Scanner** on your phone, one app at a time, since the adapter only takes one connection:

1. Read the codes in both apps. The codes must match.
2. With the engine idling and warm, compare engine speed, coolant temperature, short and long fuel trims, and module voltage. They should agree within the refresh lag.
3. The VIN must match the one on the windshield sticker and your registration document.

If something differs, open **Adapter log** in Obdaryn and copy it: it shows every command and answer.

## 9. Desk testing without the car

- **Demo car (built in).** Choose **Try the demo car** on the connect screen. It simulates a 2014 Elantra with a few faults, live values, a freeze frame and readiness monitors.
  - In demo mode, Settings has a switch for "engine running", so you can try clearing codes.
- **ELM327-emulator (Python).** It talks over real TCP, like the adapter does:

  ```bash
  python3 -m venv tools/.venv && tools/.venv/bin/pip install ELM327-emulator
  tools/.venv/bin/elm -n 35000 -s car     # then connect Obdaryn to 127.0.0.1:35000
  ```
