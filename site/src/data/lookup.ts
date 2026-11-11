// SPDX-License-Identifier: GPL-3.0-or-later
//
// The list the "Look up a code" box checks against: every generic code, the ones
// with a Romanian page, and each manufacturer code with the brands that use it.
// Served at /codes/lookup.json and fetched with ?v=<fingerprint of its contents>:
// the web server tells browsers to keep .json files for years, so a new list
// needs a new address, or returning visitors would keep searching the old one.
import { createHash } from "node:crypto";
import { codes, packs } from "./allCodes";
import { codeStrings } from "../i18n/codes";
import { routes } from "../i18n/ui";

function build(): string {
  const brand: Record<string, [string, string, string, string][]> = {};
  for (const e of codes.en.brand) {
    const p = packs.find((x) => x.id === e.pack)!;
    const label = (l: "en" | "ro") => p.label[l] + (e.slug !== e.code ? ` · ${codeStrings[l].modules[e.module!]}` : "");
    (brand[e.code] ??= []).push([e.pack, e.slug, label("en"), label("ro")]);
  }
  return JSON.stringify({
    en: routes.en.codes,
    ro: routes.ro.codes,
    generic: codes.en.generic.map((e) => e.code),
    genericRo: codes.ro.generic.map((e) => e.code),
    brand,
  });
}

export const lookupJson = build();
/** Where the page fetches the list: changes whenever the list does. */
export const lookupUrl = `/codes/lookup.json?v=${createHash("sha256").update(lookupJson).digest("hex").slice(0, 12)}`;
