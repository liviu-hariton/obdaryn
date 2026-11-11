// SPDX-License-Identifier: GPL-3.0-or-later
//
// Every trouble code the app explains, for the site's code pages (/codes/P0420/
// and /ro/coduri/P0420/), read at build time from the app's own databases: obdex
// (CC0) for the generic codes and Obdaryn's brand packs for manufacturer codes.
// Nothing is written by hand, so a code page always says what the app says.
//
// English pages exist for every code. Romanian pages exist only where the whole
// explanation is translated (all brand codes and part of the generic ones), so no
// Romanian page is a title over English text; the lookup sends the rest to English.
import { readFileSync, readdirSync } from "node:fs";
import { repoRoot } from "./site";

export type Lang = "en" | "ro";
export type Likelihood = "high" | "medium" | "low";
type Cause = [Likelihood, string];
interface RawText { t: string; d?: string; c?: Cause[]; s?: string[] }
interface RawGeneric extends RawText {
  f?: string[];
  r?: { difficulty?: string; diy?: boolean; eur?: [number, number] };
  rel?: string[];
}
interface RawBrandCode {
  module: string;
  confidence: "official" | "two-sources";
  src: { u: string; o?: boolean }[];
  f?: string[];
  en: RawText;
  ro?: RawText;
}

/** One code's page, in one language. */
export interface CodeEntry {
  code: string;
  /** "generic", or the brand pack's id (hyundai-kia, ford, ...). */
  pack: string;
  /** The module the brand pack files it under (brand codes only). */
  module?: string;
  /** Path segment: the code, plus the module when one brand uses the code in two modules. */
  slug: string;
  title: string;
  /** How the car detects it (the app's "How the car detects it"). */
  detects?: string;
  symptoms: string[];
  causes: Cause[];
  flags: { mil: boolean; emissions: boolean; driveCycle: boolean; limp: boolean; lamp: boolean };
  repair?: { difficulty: string; diy: boolean; eur?: [number, number] };
  related: string[];
  /** Brand codes: where the meaning comes from ("o" marks an official document). */
  sources?: { u: string; o?: boolean }[];
  confidence?: "official" | "two-sources";
}

export interface Pack {
  id: string;
  /** The brand on its own, for titles and lists ("Hyundai/Kia", "Volkswagen group"). */
  label: Record<Lang, string>;
  /** The pack's own name, for sentences ("Obdaryn's Ford pack", "pachetul Obdaryn pentru Ford"). */
  phrase: Record<Lang, string>;
  count: number;
}

const dataDir = repoRoot + "src/core/data/";
const read = (path: string) => JSON.parse(readFileSync(dataDir + path, "utf8"));

const generic: Record<string, RawGeneric> = read("dtc_generic.json").codes;
const genericRo: Record<string, RawText> = read("dtc_generic_ro.json").codes;

const flagsOf = (f: string[] = []) => ({
  mil: f.includes("mil"), emissions: f.includes("emissions"), driveCycle: f.includes("drive_cycle"),
  limp: f.includes("limp_mode"), lamp: f.includes("warning_lamp"),
});

function genericEntry(code: string, lang: Lang): CodeEntry | undefined {
  const g = generic[code];
  const text: RawText | undefined = lang === "en" ? g : genericRo[code]?.d ? genericRo[code] : undefined;
  if (!text) return undefined;
  return {
    code, pack: "generic", slug: code, title: text.t, detects: text.d,
    symptoms: text.s ?? [], causes: text.c ?? [], flags: flagsOf(g.f),
    repair: g.r?.difficulty ? { difficulty: g.r.difficulty, diy: !!g.r.diy, eur: g.r.eur } : undefined,
    related: (g.rel ?? []).filter((r) => generic[r]),
  };
}

const capitalise = (s: string) => s.charAt(0).toUpperCase() + s.slice(1);

const packFiles = readdirSync(dataDir + "dtc/brands").filter((f) => f.endsWith(".json")).sort();
const rawPacks = packFiles.map((f) => read("dtc/brands/" + f));

/** The brand packs, in the order the cars page lists brands. */
export const packs: Pack[] = rawPacks
  .map((p) => ({
    id: p.brand as string,
    // The pack labels read "Obdaryn's Ford pack"; the pages name just the brand.
    label: { en: brandName(p.label), ro: capitalise(brandName(p.label_ro)) },
    phrase: { en: p.label as string, ro: p.label_ro as string },
    count: new Set(Object.keys(p.codes).map((k) => k.split("@")[0])).size,
  }))
  .sort((a, b) => a.label.en.localeCompare(b.label.en));

function brandName(packLabel: string): string {
  return packLabel.replace(/^Obdaryn's /, "").replace(/ pack$/, "").replace(/^pachetul Obdaryn pentru /, "");
}

function brandEntries(lang: Lang): CodeEntry[] {
  const out: CodeEntry[] = [];
  for (const p of rawPacks) {
    for (const [key, raw] of Object.entries(p.codes as Record<string, RawBrandCode>)) {
      const text = lang === "ro" ? raw.ro : raw.en;
      if (!text?.d) continue;
      const [code, module] = key.split("@");
      out.push({
        code, pack: p.brand, module: raw.module, slug: module ? `${code}-${module}` : code,
        title: text.t, detects: text.d, symptoms: text.s ?? [], causes: text.c ?? [],
        flags: flagsOf(raw.f), related: [], sources: raw.src, confidence: raw.confidence,
      });
    }
  }
  return out.sort((a, b) => a.code.localeCompare(b.code));
}

const byLang = (lang: Lang) => ({
  generic: Object.keys(generic).sort().flatMap((c) => genericEntry(c, lang) ?? []),
  brand: brandEntries(lang),
});

export const codes: Record<Lang, { generic: CodeEntry[]; brand: CodeEntry[] }> = { en: byLang("en"), ro: byLang("ro") };

/** How many distinct codes a list holds, per brand: a code a brand explains for two
 *  modules (B1040 in the airbag and the body unit) is one code with two pages. */
export const distinctCodes = (entries: CodeEntry[]) => new Set(entries.map((e) => `${e.pack}:${e.code}`)).size;

/** Whether a generic code has a page in the language (all do in English). */
export const hasGeneric = (lang: Lang, code: string) =>
  lang === "en" ? !!generic[code] : !!genericRo[code]?.d;

/** Code groups for browsing: the first three characters (P04 → "P04xx"). */
export function groupsOf(list: CodeEntry[]): { id: string; label: string; entries: CodeEntry[] }[] {
  const map = new Map<string, CodeEntry[]>();
  for (const e of list) {
    const id = e.code.slice(0, 3);
    if (!map.has(id)) map.set(id, []);
    map.get(id)!.push(e);
  }
  return [...map.entries()].sort(([a], [b]) => a.localeCompare(b))
    .map(([id, entries]) => ({ id: id.toLowerCase(), label: `${id}xx`, entries }));
}

/** The four code systems, by the code's first letter. */
export const systemOf = (code: string) => ({ P: "powertrain", B: "body", C: "chassis", U: "network" })[code[0]] as
  "powertrain" | "body" | "chassis" | "network";
