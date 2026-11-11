# Obdaryn user guide

The same guide is in the app under **Help** (⌘?). For what each feature asks of the car and the adapter in more depth, see [CAR_INTERACTION.md](CAR_INTERACTION.md).

## Start here

Obdaryn reads what the car's computers report through an ELM327-compatible Wi-Fi adapter such as the Vgate iCar Pro. It only reads: it can't change anything in the car.

1. Plug the adapter into the OBD-II socket, under the dashboard on the driver's side.
2. Turn the ignition ON. The engine can stay off; start it for live data with the engine running.
3. Join the adapter's Wi-Fi (usually "V-LINK", no password). The computer has no internet while it's connected; that's normal.
4. Press **Connect**. The first time, macOS asks to let Obdaryn find devices on the local network: allow it.
5. Close other OBD apps first (Car Scanner, Torque): a Wi-Fi adapter takes one connection at a time.

No car nearby? Try the **demo car**, or replay a recording: every screen works with both.

## What to do in which car state

| Car state | What you can do |
|---|---|
| Ignition ON, engine off | Read trouble codes, the freeze frame, readiness and self-test results, vehicle info, and scan all modules. The only state in which codes can be cleared. |
| Engine running, parked | Everything above except clearing, plus meaningful live data: temperatures, fuel trims, charging voltage. |
| Driving | Live data, charts, the trip computer and recording a session. A passenger watches the screen, never the driver. |
| Not connected | Replay or chart recordings, export their data as CSV, and look through saved trips. |

With the ignition ON and the engine off, the car drains its battery. Keep such sessions under 20–30 minutes, or start the engine. Obdaryn shows the battery voltage on the Overview.

## What clearing codes really does

Clearing is the only thing Obdaryn does that changes anything in the car, and only after you confirm with the engine off. It doesn't repair anything:

- The warning light goes off, but comes back if the fault is still there, sometimes after a few drives.
- The freeze frame is erased. Read it, or save a report, first.
- Every readiness monitor resets. The car fails the ITP emissions check until it has been driven enough, usually 50–150 km of mixed driving over a few days. Never clear codes right before an inspection.
- Permanent codes stay. They clear themselves once the car has re-tested the system and found it fixed.

## The screens

| Screen | What it's for |
|---|---|
| **Overview** | The check-engine light, readiness, battery, codes and the main live values at a glance. **Save report…** (⌘S) makes a PDF or web page to share with a buyer or a mechanic. |
| **Trouble codes** | Codes from the engine and transmission, each explained: what it means, likely causes, whether it matters for the ITP. |
| **All modules** | Asks ABS, airbag, body and the other modules for their codes, using the module list of your car's brand. |
| **Readiness** | Which self-tests have finished since codes were last cleared. **Read test results** shows the self-test results (Mode 06) with their limits: a result close to its limit points to a part that's wearing out. |
| **Freeze frame** | The sensor values saved when a fault was stored. |
| **Live data** | Every value the car reports, with its recent history. Pin values to the Overview. |
| **Charts** | Up to six values over time, one chart each on a shared time axis, live or from a recording. Compare two recordings to see a drive before and after a repair. Hover, or focus the charts and use the arrow keys, to read every value at one moment. |
| **Trip** | Distance, fuel and consumption. On cars without an air-flow sensor, enter the engine size. Calibrate the fuel estimate with two fill-ups, and set a fuel price to see the cost. |
| **Vehicle info** | The VIN and what it tells, software versions and the modules that answer. |
| **Adapter log** | What the adapter supports, how fast it answers, and every command and answer, for troubleshooting. |
| **Help**, **Settings** | This guide; language (English, Romanian), appearance, units (metric, UK, US). |

### Recording and replaying

Press **Record** in the sidebar while connected. Obdaryn saves everything the car says to `~/Documents/Obdaryn/Recordings`. On the connect screen, **Replay a recording** plays it back as if the car were there, and **CSV** exports its live data. Charts can show a recording, or compare two. Recordings include the VIN, so check them before sharing.

## When something doesn't work

| Problem | What to do |
|---|---|
| Can't reach the adapter | Check that the computer is on the adapter's Wi-Fi; macOS sometimes jumps back to your usual network. On macOS 15 and later, allow Obdaryn under System Settings → Privacy & Security → Local Network. |
| The adapter refuses | Another app is connected to it, often a phone app. Close it and connect again. |
| The car isn't answering | Turn the ignition ON. If you switched it off, Obdaryn picks up again by itself when it's back on. |
| The connection drops | The adapter may have fallen asleep or the Wi-Fi changed. Obdaryn reconnects by itself; if it gives up, turn the ignition off and on, then press Connect again. |
| A module shows "Not found" | Your model may not have it, or uses another address. Addresses marked "unconfirmed" come from community lists. |
| Live data is slow | Cheap adapters and older cars answer slowly. Pin fewer values to the Overview; pinned and charted values refresh first. |
| A code came back | The fault is still there. Clearing only turns the light off; the cause needs fixing. |

## Words you'll see

| Term | Meaning |
|---|---|
| Check-engine light (MIL) | The engine warning lamp, turned on by the engine computer for faults that affect emissions. |
| Trouble code (DTC) | A five-character code such as P0420: P powertrain, C chassis, B body, U network. |
| Stored, pending, permanent | Stored: confirmed. Pending: seen once, stored if it happens again. Permanent: can't be cleared by a tool, clears itself once fixed. |
| Freeze frame | A snapshot of sensor values taken when a code was stored. |
| Readiness monitors | Self-tests of the emission systems. Incomplete monitors can fail an emissions inspection. |
| Self-test results (Mode 06) | The numbers behind the monitors, each with the limits the car judges it by. |
| ITP | The periodic technical inspection in Romania, which includes an emissions check. |
| VIN | The car's 17-character serial number. |

## Privacy and safety

- Obdaryn sends nothing online. It talks only to the adapter.
- Recordings and reports stay on your computer and include the VIN. Check them before sharing.
- A safety guard checks every command and refuses anything that could write to, code, reset or flash a module. The only change it allows is clearing trouble codes, after you confirm, with the engine off.
- Code explanations are guidance from public sources. Confirm with a workshop before replacing parts.
