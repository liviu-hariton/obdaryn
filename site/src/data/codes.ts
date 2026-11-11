// SPDX-License-Identifier: GPL-3.0-or-later
//
// The trouble codes the home page explains: the ones the app's built-in demo car
// reports, with the same explanations the app shows, read at build time from the
// app's own databases (obdex for generic codes, Obdaryn's brand packs for the rest).
// Nothing here is written by hand, so the page can't drift from the app.
import { readFileSync } from "node:fs";
import { repoRoot } from "./site";

type Lang = "en" | "ro";
type Cause = [likelihood: "high" | "medium" | "low", text: string];

interface RawText { t: string; d?: string; c?: Cause[]; s?: string[] }
interface RawGeneric extends RawText { f?: string[]; r?: { difficulty?: string; diy?: boolean; eur?: [number, number] } }

/** One code as the page shows it, in one language. */
export interface ShownCode {
  code: string;
  title: string;
  /** Where the demo car keeps it, as the app's memory kinds. */
  kinds: ("stored" | "pending" | "permanent")[];
  /** Which lamp it lights in the demo car: the engine light, the ABS light, or none yet. */
  lamp: "engine" | "abs" | "off";
  module: "engine" | "abs";
  symptoms: string[];
  causes: Cause[];
  emissions: boolean;
  driveCycle: boolean;
  repair?: { difficulty: string; diy: boolean; eur: [number, number] };
  /** Where the explanation comes from, for the source line. */
  source: "obdex" | "brand";
}

const dataDir = repoRoot + "src/core/data/";
const read = (path: string) => JSON.parse(readFileSync(dataDir + path, "utf8"));

const generic: Record<string, RawGeneric> = read("dtc_generic.json").codes;
const genericRo: Record<string, RawText> = read("dtc_generic_ro.json").codes;
const hyundai: Record<string, { f?: string[]; en: RawText; ro: RawText }> = read("dtc/brands/hyundai-kia.json").codes;

// The demo car (src/core/transport/SimulatedTransport.cpp): P0420 stored and
// permanent, P0133 stored, P0171 pending, and C1203 in the ABS module.
const demo = [
  { code: "P0420", kinds: ["stored", "permanent"], lamp: "engine" },
  { code: "P0133", kinds: ["stored"], lamp: "engine" },
  { code: "P0171", kinds: ["pending"], lamp: "off" },
  { code: "C1203", kinds: ["stored"], lamp: "abs" },
] as const;

function shown(lang: Lang): ShownCode[] {
  return demo.map(({ code, kinds, lamp }) => {
    const g = generic[code];
    if (g) {
      // The Romanian file carries only the texts; flags and repair come from the English one.
      const text: RawText = lang === "ro" && genericRo[code] ? genericRo[code] : g;
      return {
        code, kinds: [...kinds], lamp, module: "engine", title: text.t,
        symptoms: text.s ?? [], causes: text.c ?? [],
        emissions: !!g.f?.includes("emissions"), driveCycle: !!g.f?.includes("drive_cycle"),
        repair: g.r?.eur ? { difficulty: g.r.difficulty ?? "", diy: !!g.r.diy, eur: g.r.eur } : undefined,
        source: "obdex",
      };
    }
    const b = hyundai[code];
    if (!b) throw new Error(`${code} isn't in the app's databases`);
    const text = b[lang] ?? b.en;
    return {
      code, kinds: [...kinds], lamp, module: "abs", title: text.t,
      symptoms: text.s ?? [], causes: text.c ?? [],
      emissions: false, driveCycle: false, source: "brand",
    };
  });
}

export const demoCodes: Record<Lang, ShownCode[]> = { en: shown("en"), ro: shown("ro") };
/** How many generic codes the app explains, for the copy. */
export const genericCount = Object.keys(generic).length;
