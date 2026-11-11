---
name: Obdaryn
description: A precise native OBD-II diagnostics app where the car speaks in its own warning-lamp language.
colors:
  window: "#F7F8FA"
  window-dark: "#0F1114"
  sidebar: "#EEF0F3"
  sidebar-dark: "#14171B"
  surface: "#FFFFFF"
  surface-dark: "#171A1F"
  hover: "#E8EBEF"
  hover-dark: "#1D2127"
  selected: "#E1E6EE"
  selected-dark: "#232831"
  hairline: "#DCE0E6"
  hairline-dark: "#262A31"
  field: "#FFFFFF"
  field-dark: "#12151A"
  field-border: "#C9CFD8"
  field-border-dark: "#323843"
  text: "#15181C"
  text-dark: "#E8EAED"
  text-muted: "#545C67"
  text-muted-dark: "#A3A9B2"
  text-faint: "#646C77"
  text-faint-dark: "#7E858F"
  telltale-red: "#C8372D"
  telltale-red-dark: "#F0564A"
  telltale-amber: "#B26700"
  telltale-amber-dark: "#F2A93B"
  telltale-green: "#1E8A50"
  telltale-green-dark: "#3CBF7A"
  telltale-blue: "#2F6FE0"
  telltale-blue-dark: "#5B97F7"
  red-ink: "#B3302A"
  red-ink-dark: "#F0564A"
  amber-ink: "#9A5800"
  amber-ink-dark: "#F2A93B"
  green-ink: "#17703F"
  green-ink-dark: "#3CBF7A"
  blue-fill: "#2F6FE0"
  blue-ink: "#2860C4"
  blue-ink-dark: "#5B97F7"
  red-fill: "#C8372D"
  amber-fill: "#9A5800"
  on-fill: "#FFFFFF"
  unlit: "#B9C0CA"
  unlit-dark: "#3A3F47"
  track-off: "#848C97"
  track-off-dark: "#636A75"
  trace-1: "#2F6FE0"
  trace-2: "#1E8A50"
  trace-3: "#8A4FC4"
  trace-4: "#1C8C95"
  trace-5: "#3B424B"
  trace-6: "#5560D8"
  trace-1-dark: "#5B97F7"
  trace-2-dark: "#3CBF7A"
  trace-3-dark: "#C792EA"
  trace-4-dark: "#4FC1C9"
  trace-5-dark: "#A3A9B2"
  trace-6-dark: "#9AA4FF"
  chart-a: "#2F6FE0"
  chart-a-dark: "#4C88EE"
  chart-b: "#1E8A50"
  chart-b-dark: "#2EA866"
typography:
  value:
    fontFamily: "Inter, sans-serif"
    fontSize: "30px"
    fontWeight: 600
    letterSpacing: "-0.6px"
    fontFeature: "\"tnum\" 1"
  title:
    fontFamily: "Inter, sans-serif"
    fontSize: "26px"
    fontWeight: 700
    letterSpacing: "-0.4px"
  section:
    fontFamily: "Inter, sans-serif"
    fontSize: "18px"
    fontWeight: 600
    letterSpacing: "-0.2px"
  emphasis:
    fontFamily: "Inter, sans-serif"
    fontSize: "16px"
    fontWeight: 600
  body:
    fontFamily: "Inter, sans-serif"
    fontSize: "14px"
    fontWeight: 400
  small:
    fontFamily: "Inter, sans-serif"
    fontSize: "12px"
    fontWeight: 500
  caption:
    fontFamily: "Inter, sans-serif"
    fontSize: "11px"
    fontWeight: 700
    fontFeature: "\"tnum\" 1"
  mono:
    fontFamily: "JetBrains Mono, monospace"
    fontSize: "14px"
    fontWeight: 400
  mono-code:
    fontFamily: "JetBrains Mono, monospace"
    fontSize: "16px"
    fontWeight: 600
rounded:
  base: "6px"
  large: "10px"
  pill: "11px"
spacing:
  s1: "4px"
  s2: "8px"
  s3: "12px"
  s4: "16px"
  s5: "24px"
  s6: "32px"
  s7: "48px"
motion:
  fast: "120ms"
  normal: "220ms"
  bulbCheck: "1200ms"
  sweep: "1100ms"
  easing: "exponential ease-out"
components:
  button-primary:
    backgroundColor: "{colors.blue-fill}"
    textColor: "{colors.on-fill}"
    rounded: "{rounded.base}"
    padding: "0 14px"
    height: "34px"
  button-primary-hover:
    backgroundColor: "#2A68D8"
  button-primary-active:
    backgroundColor: "#2961C3"
  button-secondary:
    backgroundColor: "transparent"
    textColor: "{colors.text}"
    rounded: "{rounded.base}"
    padding: "0 14px"
    height: "34px"
  button-secondary-hover:
    backgroundColor: "{colors.hover}"
  button-danger:
    backgroundColor: "transparent"
    textColor: "{colors.red-ink}"
    rounded: "{rounded.base}"
    padding: "0 14px"
    height: "34px"
  button-danger-emphasized:
    backgroundColor: "{colors.red-fill}"
    textColor: "{colors.on-fill}"
    rounded: "{rounded.base}"
    height: "34px"
  button-quiet:
    backgroundColor: "transparent"
    textColor: "{colors.text}"
    rounded: "{rounded.base}"
    height: "26px"
  nav-item:
    backgroundColor: "transparent"
    textColor: "{colors.text-muted}"
    typography: "{typography.body}"
    rounded: "{rounded.base}"
    padding: "0 10px"
    height: "32px"
  nav-item-current:
    backgroundColor: "{colors.selected}"
    textColor: "{colors.text}"
  chip:
    backgroundColor: "transparent"
    textColor: "{colors.text-muted}"
    typography: "{typography.small}"
    rounded: "{rounded.pill}"
    padding: "0 8px"
    height: "22px"
  input:
    backgroundColor: "{colors.field}"
    textColor: "{colors.text}"
    typography: "{typography.body}"
    rounded: "{rounded.base}"
    padding: "0 10px"
    height: "36px"
  segmented:
    backgroundColor: "{colors.hover}"
    rounded: "{rounded.base}"
    height: "32px"
  segmented-option-on:
    backgroundColor: "{colors.surface}"
    textColor: "{colors.text}"
    height: "28px"
  toggle-on:
    backgroundColor: "{colors.blue-fill}"
    rounded: "{rounded.pill}"
    width: "38px"
    height: "22px"
  toggle-off:
    backgroundColor: "{colors.unlit}"
---

# Design System: Obdaryn

## Overview

**Creative North Star: "The Instrument Cluster at Key-On"**

Obdaryn is a precise, quiet native desktop tool (Qt Quick, not web) that borrows exactly one thing from the car: its warning-lamp language. Everything else is graphite and paper neutrals with a faint cool tint, set in Inter, at the density of a well-made macOS app. The car's identity lives in the details: the real engine telltale symbol lit amber when the check-engine light is on, state dots that follow ISO 2575, and a self-test flash of every lamp when the car first answers.

Structure comes from hairlines and grouping, not from boxes. Lists are rows separated by 1px rules; detail pages are label/value rows with a hairline under each; the sidebar is grouped navigation on a slightly darker ground. Color is spent only on state, so a lit lamp is the loudest thing on any screen. Both themes are first-class: the app follows the system appearance, and contrast is held in a dim car at night and a bright garage alike.

The world refuses the racing dashboard of neon gauges and themed-artifact pastiche (inspection forms, road signs, maps). Values are big tabular numbers with a thin trace beneath, not dials.

**Key Characteristics:**
- Graphite (dark) and cool paper (light) neutrals; both themes tokenized side by side.
- ISO 2575 telltale colors are the only accents, and they always mean state.
- Inter for all UI; JetBrains Mono only for codes, VINs, hex and the adapter log.
- Hairline-divided grouped lists instead of cards.
- Flat at rest; soft shadow only on true overlays (toast, confirm dialog).
- One signature motion: the ignition bulb check. Everything else is short exponential ease-out, and all of it switches off with Reduce motion.

## Colors

Cool, low-chroma neutrals carry the whole interface; four telltale hues are reserved for state, each with a separate "ink" variant when it has to be read as text.

### Primary
- **Telltale Blue** (`telltale-blue`, `telltale-blue-dark`): information, selection and keyboard focus. Focus rings (2px), selected-field borders, permanent-code dots, the outgoing arrow in the adapter log, and the first live trace.
- **Signal Blue Fill** (`blue-fill`): the ground of the one primary button per screen, the on-state of toggles and text selection. Same value in both themes so white text holds about 4.7:1; on hover it darkens to `#2A68D8` (5.2:1) rather than lightening.

### Secondary
- **Telltale Amber** (`telltale-amber`, `telltale-amber-dark`): caution or pending. The lit check-engine telltale, pending-code dots, incomplete readiness monitors, connecting status, warning toasts.
- **Telltale Red** (`telltale-red`, `telltale-red-dark`) and **Red Fill** (`red-fill`): stop or fault. Stored-code dots, the trouble-code badge in the navigation, failed-connection status, the danger button outline and its filled confirm state.
- **Telltale Green** (`telltale-green`, `telltale-green-dark`): complete or connected. The connected dot, complete monitors, battery charging, success toasts.

### Tertiary
- **Ink variants** (`amber-ink`, `red-ink`, `green-ink`, with `-dark` pairs): the text forms of the state colors. In light theme they are darker than the lamp colors so state text clears 4.5:1 on the window; in dark theme they equal the lamp colors, which already pass. `blue-ink` (`#2860C4` light) is the same for links and blue text.
- **Trace series** (`trace-1` to `trace-6`, with `-dark` pairs): calm, distinct hues for live-value sparklines, cycled by tile index. Blue, green, violet, teal, neutral, periwinkle.

### Neutral
- **Window** (`window`): the content ground behind every screen.
- **Sidebar** (`sidebar`): one step darker (light) or lighter (dark) than the window, separated from content by a 1px hairline. In windows under 760px tall, rows drop from 32 to 28px and group gaps tighten, so Settings and the read-only note still fit at 680px while recording.
- **Surface** (`surface`): raised ground for the few framed panels, the toast, the dialog and the selected segment.
- **Hover / Selected** (`hover`, `selected`): row and button hover; the current nav item and the selected code row.
- **Hairline** (`hairline`): every divider, list rule, chip outline and panel border.
- **Field / Field Border** (`field`, `field-border`): text inputs and secondary-button outlines.
- **Text / Muted / Faint** (`text`, `text-muted`, `text-faint`): primary copy; subtitles, labels and descriptions; group labels, timestamps and notes.
- **Unlit** (`unlit`): a telltale that is off and the disconnected dot.
- **Track off** (`track-off`): the track of a switch that is off, dark enough (3:1 against the window) to read as a control rather than a lamp that is off.

### Named Rules
**The Telltale Rule.** Red, amber, green and blue mean stop/fault, caution/pending, complete/connected and information/selection/focus, as on an ISO 2575 instrument cluster. They are never decoration, and live traces never use red or amber.

**The Ink Rule.** A lamp color is for lamps, dots, icons and outlines. When state has to be read as text, use the matching ink variant (`amber-ink`, `red-ink`, `green-ink`).

**The Not-By-Color-Alone Rule.** Every state color travels with a word or a symbol: a chip pairs its dot with a label, a telltale is a recognizable symbol, a badge carries a count.

## Typography

**Body Font:** Inter (bundled variable font)
**Mono Font:** JetBrains Mono (bundled variable font)

**Character:** Inter does all the talking, tightened slightly at heading sizes and set with tabular figures wherever numbers update. JetBrains Mono is the voice of the car's raw data, kept so rare that it reads as a different register.

### Hierarchy
- **Value** (600, 30px, -0.6px tracking, tabular figures): live readings in the overview tiles, followed by the unit in muted body text on the same baseline.
- **Title** (700, 26px, -0.4px): one per screen, in the page header.
- **Section** (600, 18px, -0.2px): section headings inside a screen, with optional faint 12px detail on the baseline.
- **Emphasis** (600, 16px): status headings, empty-state titles, codes in lists.
- **Body** (400 to 600, 14px): all running UI text, list rows, buttons (600), nav items (500, 600 when current).
- **Small** (400 to 600, 12px): field labels, chip text, notes, nav group labels (600, sentence case, faint).
- **Caption** (700, 11px, tabular): badge counts and trace axis labels.

### Named Rules
**The Mono-Is-The-Car Rule.** JetBrains Mono is reserved for trouble codes, VINs, hex, module addresses and the adapter log. Names, labels, units and explanations stay in Inter.

**The Tabular Rule.** Any number that updates or aligns in a column (values, counts, badges, detail text) uses tabular figures.

## Layout

A fixed 232px sidebar on the left and a scrolling content area on the right. The window opens at 1320 x 860 and has a hard minimum of 1040 x 680; layouts are verified at both. Dashboard content caps at 1180px; single-column reading pages (readiness, vehicle, settings) cap at 920px; the code explanation column caps at 720px. The trouble-codes screen is a master/detail split with a vertical hairline between list and explanation.

Spacing runs on a 4px base (4, 8, 12, 16, 24, 32, 48). 24px is the screen gutter and the gap between major blocks; 12px pads rows and panels; 16px separates label and value. Romanian strings run 20 to 30% longer than English, so text wraps or elides inside its row and chips cap at their container width; nothing truncates silently mid-layout.

## Elevation & Depth

Flat by default. Depth comes from tonal steps (window, sidebar, surface) and 1px hairlines, not shadows. Only two things float, and only they cast shadow: the toast at the bottom of the window and the clear-codes confirmation dialog, which also dims the window behind it. Exact shadow and scrim values live in the sidecar.

### Named Rules
**The Only-Overlays-Float Rule.** Nothing in the page flow casts a shadow. A shadow means "this is above the app and wants your attention now."

## Shapes

Gently rounded rectangles: 6px for controls, rows and nav items; 10px for the few framed panels, the toast and the dialog; full pills (11px on a 22px height) for chips, badges and toggles; circles for state dots (7 to 8px). Focus rings sit outside the shape at radius + 2 or + 3px. Telltales are line icons (Lucide, plus Obdaryn's own engine symbol) drawn as single-color glyphs, never filled shapes.

## Components

### Buttons
Calm and native; one button component with four variants.
- **Shape:** gently rounded (6px), 34px tall, 600-weight 14px label, optional 16px leading or trailing icon.
- **Primary:** Signal Blue Fill with white text. One per screen, for its main action.
- **Secondary:** transparent with a 1px field-border outline. Everything else.
- **Danger:** red outline with red-ink text; filled red only when emphasized, which is the confirm button inside the clear-codes dialog.
- **Quiet:** no border until hovered; toolbar and inline actions such as Copy VIN.
- **Hover / Focus:** hover shifts the ground to `hover` (or lightens the fill); press darkens. Keyboard focus draws a 2px Telltale Blue ring 3px outside the button. Disabled drops to 45% opacity.

### Chips
- **Style:** outlined pill (22px tall, 1px hairline, transparent ground), muted 12px label, optional 7px state dot or 13px icon in front.
- **State:** the KindChip variant colors its dot by code memory: stored red, pending amber, permanent blue.

### Containers and lists
- **Grouped lists:** rows on the window ground separated by 1px hairlines, at least 44 to 52px tall, 6px hover highlight. Code rows lead with the code in mono.
- **Key/value rows:** a 220px muted label column, a medium-weight value (mono when it is a code or hex), optional faint note, and a hairline beneath.
- **Framed panels:** a surface ground with a 1px hairline and 10px radius, used sparingly: the overview status summary, the readiness verdict, the connect form and the log well. No shadow.
- **Module rows (All modules):** a grouped-list row at least 56px tall that grows with a wrapped error reason. From left to right:
  - an 18px state lamp: red triangle for codes, green check for clean, unlit dashed circle for not found, amber help icon for unreadable;
  - the module name at 14px/600 with an optional faint "unconfirmed address" in 12px;
  - beneath the name, a mono 12px address line (`7D1 → 7D9 · KWP2000`);
  - a 190px right-aligned status word, in red ink and 600 weight only when there are codes;
  - a chevron, shown only when the row can expand.

  Expanding unfolds in place: code lines (mono code at 16px/600, title, then kind chips) that themselves unfold the explanation under the title, capped at 720px. A muted 12px line carries part number, software and the raw status byte, with the hex in mono. Raw protocol notes sit behind a quiet "About this list" toggle, never in the first viewport. "Clear this module's codes…" on Engine or Transmission opens the full OBD-II clear dialog (readiness reset, freeze-frame loss, permanent codes); other modules use the module dialog, which names the module's system. Rows spin a Telltale Blue refresh icon while being asked; the spin stops under Reduce motion.

### Inputs / Fields
- **Style:** 36px, field ground, 1px field-border, 6px radius, 10px side padding, 12px muted label above. Mono when the field takes an address or hex.
- **Focus:** border becomes 2px Telltale Blue.
- **Error:** a 1px telltale-red border and, under the field, a 12px `red-ink` sentence that says how to fix the value ("Enter a number between 0.5 and 10").

### Description source line
Under every code explanation there is a quiet 12px faint note. It names where the text comes from (obdex, or Obdaryn's brand pack) and how sure the meaning is (official document or two independent sources), and always ends with "Guidance only; confirm with a workshop". Below it sits a row of source links in `blueInk`, underlined. Each link is keyboard-focusable, with the focus ring 3px outside it. NHTSA documents are labelled by their id, official ones are marked "(official)", and wrapped links align with the first link. The note is hidden when no source wrote the text above it.

### Navigation
- **Manufacturer values (Live data):** a segmented switch at the top ("Standard (OBD-II)" | "Hyundai values") appears only for brands with a data pack. The brand view keeps the setup in one row (model drop-down, "2014, from the VIN" or a year field, "Find again"), states its source and that it isn't checked on this car in one muted line, and lists values in hairline rows grouped by OBDb's groups (translated where Obdaryn has a name, sorted by the name as shown): name, the module with its request in mono, the value in the live-data style. Before anything is found, an empty state says what happens next ("Find values", progress "Asking the modules… 12 of 36", or why nothing answered).
- **Charts (Monitor group):** small multiples, one strip per value (never a dual axis): name in semibold and the current value in text colours, the visible range faint on the right, two y labels, three hairline gridlines, a 2px line. Comparing two recordings draws A solid in `chartA` (blue) and B dashed in `chartB` (green), a pair validated for colour-blind and normal-vision separation on both surfaces, with a line-key legend. Controls sit in one row above the strips; the values to chart are removable chips plus an "Add a value…" list. A vertical crosshair spans all strips with one readout: values first, names after. The readout sits on the far side of the plot from the crosshair, so it never hides the lines being read (in live charts, the newest values). Past the end of the shorter of two recordings, one line says which ended and when, instead of a dash per row. Recordings are named the same everywhere: date and time first, then the car.
- **Adapter panel (top of the adapter log):** the adapter's own identification in semibold, a calm one-line verdict in muted text, the commands tried as chips: a check or an x (never colour alone), a plain name ("Battery voltage"), then the command itself in mono (`AT RV`), with what it does and whether it works in the tooltip and accessible name. A compact two-column list of live numbers sits on the right, in plain words ("Values per request", "Quick answers") rather than protocol terms. Kept short so the log keeps most of the height at 680px.
- **Connection states:** Reconnecting uses the amber dot and "Reconnecting…" in the sidebar; data pages show what is happening ("Reconnecting to the adapter…", "The car isn't answering") with Disconnect, instead of the generic connect prompt. Connection failures explain the fix in a sentence, with an "Open Local Network settings" button on macOS when the adapter is unreachable.
- **Trip (Monitor group, ⌘7):** four big values in a row under hairlines (distance, fuel used, average L/100 km, right now; standing still shows L/h), display size so a passenger reads them at a glance; a faint dash when a value isn't known yet, with a short muted reason under it ("Shown after 0.2 km", "Needs the engine size, below"), so an empty value never goes unexplained. Then a two-column key-value grid (driving, idling, average and top speed), "How fuel is measured" in plain words with the engine-size field only when speed-density needs it, the calibration status, an inline fill-up form (one number: no dialog), an optional fuel price with its currency (the trip's cost joins the key-value grid), and recent trips as hairline rows.
- **Help (sidebar, above Settings; ⌘?):** a reading page with one 620px measure shared by prose and label rows, so the right edge is even. In windows wide enough for it, an "On this page" rail (200px, faint sentence-case group label, items styled like nav rows) stays fixed on the left and marks the section being read with the `selected` ground; clicking scrolls there in 220ms (instant under Reduce motion). In narrower windows the same items sit in a wrapped row under the title. "Supported cars" is built from the shipped data (module lists, code packs, manufacturer values), never hand-written: brand rows with the brand in semibold on the left and one line per capability on the right, its name in the text colour, then the facts in muted text. Sections open with 48px above an 18px semibold heading and 12px below it; setup steps are numbered because their order matters, other lists are bullets; label/explanation rows (170px label) with hairlines between rows for states, screens, troubleshooting and terms. The version and project link close the page under a hairline. Scrolls from the keyboard (arrows, Page Up/Down, Home/End); the rail items are in the Tab order.
- **Settings, Units:** a segmented Metric / UK / US choice under Appearance, saying that exports stay metric.
- **Shareable report (PDF / web page):** a printed document in the light palette only: black ink on white, headings in the sans at 20/13/11 pt, body 10 pt, hairline rules between rows and no boxes, the same as the app. Colour stays on state words only (On, Stored, Not ready, Recently...), each word saying what it means, so it survives black-and-white printing. It opens with a letterhead: the 32px logo mark (full drawing) beside "Obdaryn" in 12pt bold; the web page also carries the small drawing as its favicon, both embedded so the page stays one file. Order: car and VIN, where the data came from (demo and replay said in colour), "At a glance" verdict table, codes with explanations, all modules, readiness, freeze frame, vehicle, about. The PDF uses static system faces (Helvetica Neue / Segoe UI / Arial, Menlo / Consolas) because the variable Inter doesn't survive PDF output; the web page asks for Inter first.
- **Overview header:** "Save report…" (secondary, file icon) beside "Scan again"; ⌘S does the same.
- **Recordings (connect screen):** under the demo car, "Replay a recording" with Open a file…, then hairline rows (car or file name, date · length, a quiet CSV export and a Replay button) and a faint note naming the folder and warning that recordings include the VIN.
- **Menu bar (macOS):** native, in the top bar like any Mac app's, so every command and its shortcut is findable there: Obdaryn (About, Settings… ⌘,, Quit, placed by role so they land correctly in any language), File (connect, demo car, Replay a Recording… ⌘O, Start/Stop Recording ⇧⌘R, Save Report… ⌘S, Disconnect, Close Window), Edit (for text fields), Car (Scan Again ⌘R, Scan All Modules, Read Self-Test Results, Clear Trouble Codes… through the usual confirmation), View (the screens in sidebar order, ⌘1–⌘9, the current one checked), Window, Help (Obdaryn Help ⌘?, Supported Cars, the project page, Report a Problem). Items are disabled, not hidden, when they can't run. Title case in English, as macOS menus are; sentence case in Romanian.
- **About Obdaryn (macOS: Obdaryn menu):** a small fixed window, 400px wide, close button only, centred over the main window; Esc and ⌘W close it. Centred column: the logo mark at 80px (full drawing), "Obdaryn" at 22px bold, the version and a faint build line (Qt and macOS versions), both selectable; one plain sentence on what the app is and the read-only note with its green shield; "Project page" (secondary) and "Copy version info" (quiet, for bug reports, confirms with "Copied"); under a hairline, the licence link and the credits in 12px faint text, then the copyright line ("© 2026 Liviu Hariton and contributors", which becomes a range such as "2026–2028" from the build year) in the same faint text.
- **Sidebar:** the logo mark and product name, a framed connection-status box (state dot plus words, transport line; when connected to a car, a Record button beside an icon-only disconnect; while recording, a red "Recording 1:23" line; while replaying, a blue dot, the recorded car and date, the position "0:05 of 0:19" and Stop replay), then nav groups Diagnose, Monitor, Vehicle, Adapter under faint sentence-case labels, and Help, Settings and a read-only note pinned to the bottom. Below 760px of height the group gaps tighten; if the content still doesn't fit (recording or replaying at the 680px minimum), the sidebar scrolls instead of clipping the read-only note.
- **Nav item:** 32px row, 16px icon plus 14px label in muted text; current item gets the `selected` ground, full text color and 600 weight. A count badge marks codes: red fill when any are stored, `amber-fill` (a darker amber, so the white count holds 5.6:1) when they are only pending (Trouble codes counts generic codes; All modules counts codes outside engine and transmission). ⌘1–⌘8 follow the sidebar order.

### Website (obdaryn.eu)
The presentation site (`site/`, Astro, static) is the app's world on the web, not a new one: the same tokens in `site/src/styles/global.css`, following the system theme, Inter and JetBrains Mono served from the site itself, hairline rows instead of cards, telltale colours only for state, one blue primary button per view (Download for macOS).
- **First viewport:** the thesis is a trouble code explained the way the app explains it, from the app's own databases at build time. It shows the lamp cluster (engine and ABS telltales, lit amber for the lamp the selected code turns on, `unlit` otherwise), a switcher of the demo car's codes (mono tabs with their state dot), the code in mono at display size (168px max), its title at 30px, then hairline rows (what you may notice, likely causes, inspection verdict, typical cost, where it's stored). The offer sits in a 400px column on the right: a 46px headline, one sentence, Download, the version line, three hairline facts led by the green shield. At load the cluster runs the bulb check (every lamp lit for 1.2s, skipped under Reduce motion), the site's only flourish. On phones the offer comes first and the code follows.
- **Below:** four questions owners ask, each beside a real app screenshot cropped to the part that proves it (light and dark captures served by the visitor's theme, in the page's language, framed by a hairline at 10px radius). Then the read-only guard as a refused/allowed ledger with the service IDs in mono, on the sidebar tone; what you need (hairline label rows) and the numbered connection steps; brand rows built from the shipped data; an honest early-access line with an amber dot; a large closing statement with Download and the source link.
- **Type at web scale:** section titles 30–44px/700 at -0.025em, question headings 24–32px/700, body 16–18px. The report-style reading pages (download, guide, supported cars, privacy) use a 70ch measure, and the guide has an "On this page" rail like the app's Help.
- **No third parties:** no cookies, analytics or external fonts. Downloads link to the GitHub release of the version in `CMakeLists.txt`.

### Logo mark
The OBD-II port on a graphite tile (22.5% corner radius, `#262B34` to `#121519` top to bottom): the port outline in `#A3A9B2`, its 16 pins unlit in `#4A515C` except pin 16, battery power, lit `#F2A93B`. At 32px and up the full drawing shows every pin; below 32px a simplified drawing keeps only the outline and the lit pin (`src/core/data/brand/logo.svg` and `logo-small.svg`; the app icon is the same drawing with the macOS margins, and its 16 and 32px slots use the simplified one). In the app, two places only: 24px beside the name at the top of the sidebar (20px in windows under 760px tall), and 56px above "Connect to your car"; in the reports, a 32px letterhead (and the web page's favicon). In dark theme the tile carries a 1px `#262A31` edge (the `-dark` files) so it stays a tile on the dark sidebar.

**The Tile Rule.** Brand amber appears only inside the graphite tile, as the port's lit pin. The amber engine glyph always means the check-engine light is on and is never used as branding.

### Telltale (signature)
An instrument-cluster lamp: a line icon shown in its telltale color when lit and in `unlit` grey when off, cross-fading over 220ms with exponential ease-out. The check-engine lamp is the real engine telltale symbol, lit amber. The lead status cell draws it at 40px; secondary cells at 28px.

### Sparkline
A 1.5px trace of real samples, scaled to its own range, newest at the right edge over a hairline baseline. Drawn in the trace series, never in state colors.

### Ignition bulb check (signature motion)
When the car first answers, every telltale lights for 1.2s and every sparkline sweeps in from the left over 1.1s with exponential ease-out, like a cluster's self-test at key-on; then the real state shows. It runs once per connection. With Settings > Reduce motion on, the bulb check and the sweep are skipped and all transition durations become zero.

## Do's and Don'ts

### Do:
- **Do** use red, amber, green and blue only for the ISO 2575 states they stand for, and pair each with a word, count or symbol.
- **Do** switch to the ink variant (`amber-ink`, `red-ink`, `green-ink`) whenever a state color is set as text.
- **Do** set codes, VINs, hex, module addresses and log lines in JetBrains Mono, and everything else in Inter.
- **Do** group content as hairline-divided rows and key/value lists; reserve the 10px framed panel for a status summary or a contained well.
- **Do** route every duration through the motion tokens (`fast` 120ms, `normal` 220ms, `bulbCheck` 1200ms, `sweep` 1100ms, all exponential ease-out) so Reduce motion zeroes it.
- **Do** check every new screen in both themes and at the 1040 x 680 minimum, in English and Romanian.

### Don't:
- **Don't** draw live traces or decoration in red or amber; those hues are reserved for state.
- **Don't** build card grids or nest framed panels; the page structure is hairlines and grouping.
- **Don't** cast shadows on anything in the page flow; only the toast and the confirm dialog float.
- **Don't** turn values into gauges, dials or neon racing-dashboard displays.
- **Don't** use mono for labels, units, names or explanations.
- **Don't** add uppercase tracked kickers or eyebrows above headings; group labels are sentence case.
- **Don't** add a second signature motion; the bulb check is the only flourish.
