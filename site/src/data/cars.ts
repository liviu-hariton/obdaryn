// SPDX-License-Identifier: GPL-3.0-or-later
//
// Supported cars, built at build time from the data the app ships (module lists,
// brand code packs, OBDb manufacturer values), the same way the app's
// Help → Supported cars does. Never written by hand.
import { readdirSync, readFileSync } from "node:fs";
import { repoRoot } from "./site";

const dataDir = repoRoot + "src/core/data/";
const json = (path: string) => JSON.parse(readFileSync(dataDir + path, "utf8"));

export interface Brand {
  id: string;
  label: string;
  makes: string[];
  modules: number;
  modulesConfirmed: number;
  codes: number;
  /** Models with manufacturer values, by make (OBDb). */
  models: Record<string, string[]>;
}

interface Vehicles { id: string; label: string; makes: string[]; modules: { verified?: boolean }[] }

const packs = new Map<string, number>();
for (const file of readdirSync(dataDir + "dtc/brands")) {
  if (!file.endsWith(".json")) continue;
  const pack = json("dtc/brands/" + file);
  packs.set(pack.brand, Object.keys(pack.codes).length);
}

const obdbModels: { make: string; model: string }[] = [];
for (const file of readdirSync(dataDir + "obdb")) {
  if (file.endsWith(".json")) obdbModels.push(...json("obdb/" + file).models);
}

const all: Brand[] = readdirSync(dataDir + "vehicles")
  .filter((f) => f.endsWith(".json"))
  .map((f) => json("vehicles/" + f) as Vehicles)
  .map((v) => {
    const models: Record<string, string[]> = {};
    for (const m of obdbModels) {
      if (!v.makes.includes(m.make)) continue;
      (models[m.make] ??= []).push(m.model);
    }
    for (const list of Object.values(models)) list.sort((a, b) => a.localeCompare(b, "en", { numeric: true }));
    return {
      id: v.id, label: v.label, makes: v.makes,
      modules: v.modules.length, modulesConfirmed: v.modules.filter((m) => m.verified).length,
      codes: packs.get(v.id) ?? 0, models,
    };
  });

/** Brands with their own data, alphabetically. */
export const brands = all.filter((b) => b.id !== "generic").sort((a, b) => a.label.localeCompare(b.label));
/** The standard module list every other car gets. */
export const generic = all.find((b) => b.id === "generic")!;
export const brandCodeTotal = brands.reduce((n, b) => n + b.codes, 0);
export const modelCount = obdbModels.length;
