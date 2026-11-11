// SPDX-License-Identifier: GPL-3.0-or-later
//
// The list the "Look up a code" box checks against: every generic code, the ones
// with a Romanian page, and each manufacturer code with the brands that use it.
import { codes, packs } from "../../data/allCodes";
import { codeStrings } from "../../i18n/codes";
import { routes } from "../../i18n/ui";

export function GET() {
  const brand: Record<string, [string, string, string, string][]> = {};
  for (const e of codes.en.brand) {
    const p = packs.find((x) => x.id === e.pack)!;
    const label = (l: "en" | "ro") => p.label[l] + (e.slug !== e.code ? ` · ${codeStrings[l].modules[e.module!]}` : "");
    (brand[e.code] ??= []).push([e.pack, e.slug, label("en"), label("ro")]);
  }
  return new Response(JSON.stringify({
    en: routes.en.codes,
    ro: routes.ro.codes,
    generic: codes.en.generic.map((e) => e.code),
    genericRo: codes.ro.generic.map((e) => e.code),
    brand,
  }));
}
