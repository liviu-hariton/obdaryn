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
  text-faint: "#6B737E"
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
  red-fill: "#C8372D"
  on-fill: "#FFFFFF"
  unlit: "#B9C0CA"
  unlit-dark: "#3A3F47"
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
  trace-5-dark: "#E8EAED"
  trace-6-dark: "#9AA4FF"
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
components:
  button-primary:
    backgroundColor: "{colors.blue-fill}"
    textColor: "{colors.on-fill}"
    rounded: "{rounded.base}"
    padding: "0 14px"
    height: "34px"
  button-primary-hover:
    backgroundColor: "#3378F2"
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
- **Signal Blue Fill** (`blue-fill`): the ground of the one primary button per screen, the on-state of toggles and text selection. Same value in both themes so white text holds about 4.6:1.

### Secondary
- **Telltale Amber** (`telltale-amber`, `telltale-amber-dark`): caution or pending. The lit check-engine telltale, pending-code dots, incomplete readiness monitors, connecting status, warning toasts.
- **Telltale Red** (`telltale-red`, `telltale-red-dark`) and **Red Fill** (`red-fill`): stop or fault. Stored-code dots, the trouble-code badge in the navigation, failed-connection status, the danger button outline and its filled confirm state.
- **Telltale Green** (`telltale-green`, `telltale-green-dark`): complete or connected. The connected dot, complete monitors, battery charging, success toasts.

### Tertiary
- **Ink variants** (`amber-ink`, `red-ink`, `green-ink`, with `-dark` pairs): the text forms of the state colors. In light theme they are darker than the lamp colors so state text clears 4.5:1 on the window; in dark theme they equal the lamp colors, which already pass.
- **Trace series** (`trace-1` to `trace-6`, with `-dark` pairs): calm, distinct hues for live-value sparklines, cycled by tile index. Blue, green, violet, teal, neutral, periwinkle.

### Neutral
- **Window** (`window`): the content ground behind every screen.
- **Sidebar** (`sidebar`): one step darker (light) or lighter (dark) than the window, separated from content by a 1px hairline.
- **Surface** (`surface`): raised ground for the few framed panels, the toast, the dialog and the selected segment.
- **Hover / Selected** (`hover`, `selected`): row and button hover; the current nav item and the selected code row.
- **Hairline** (`hairline`): every divider, list rule, chip outline and panel border.
- **Field / Field Border** (`field`, `field-border`): text inputs and secondary-button outlines.
- **Text / Muted / Faint** (`text`, `text-muted`, `text-faint`): primary copy; subtitles, labels and descriptions; group labels, timestamps and notes.
- **Unlit** (`unlit`): a telltale that is off, the disconnected dot, and the off toggle track.

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

### Inputs / Fields
- **Style:** 36px, field ground, 1px field-border, 6px radius, 10px side padding, 12px muted label above. Mono when the field takes an address or hex.
- **Focus:** border becomes 2px Telltale Blue.

### Navigation
- **Sidebar:** product name, a framed connection-status box (state dot plus words, transport line, connect or disconnect), then nav groups Diagnose, Monitor, Vehicle, Adapter under faint sentence-case labels, and Settings plus a read-only note pinned to the bottom.
- **Nav item:** 32px row, 16px icon plus 14px label in muted text; current item gets the `selected` ground, full text color and 600 weight. A red count badge marks stored codes.

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
- **Do** route every duration through the motion tokens (120ms or 220ms, exponential ease-out) so Reduce motion zeroes it.
- **Do** check every new screen in both themes and at the 1040 x 680 minimum, in English and Romanian.

### Don't:
- **Don't** draw live traces or decoration in red or amber; those hues are reserved for state.
- **Don't** build card grids or nest framed panels; the page structure is hairlines and grouping.
- **Don't** cast shadows on anything in the page flow; only the toast and the confirm dialog float.
- **Don't** turn values into gauges, dials or neon racing-dashboard displays.
- **Don't** use mono for labels, units, names or explanations.
- **Don't** add uppercase tracked kickers or eyebrows above headings; group labels are sentence case.
- **Don't** add a second signature motion; the bulb check is the only flourish.
