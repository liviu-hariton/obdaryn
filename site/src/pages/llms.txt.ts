// SPDX-License-Identifier: GPL-3.0-or-later
//
// /llms.txt (llmstxt.org): a short map of the site for AI agents and answer
// engines, in Markdown. Above all it says how code pages are addressed, so an
// agent asked about "P0420" can go straight to its page. Built from the same data
// as the pages. The server caches .txt files for years, so it names no version
// and links only to addresses that don't move.
import { codes, distinctCodes, packs } from "../data/allCodes";
import { repoUrl } from "../data/site";

const site = "https://obdaryn.eu";
const brandPacks = packs.filter((p) => p.id !== "generic" && codes.en.brand.some((e) => e.pack === p.id));
const thousands = (n: number) => n.toLocaleString("en-GB");

export function GET() {
  const text = `# Obdaryn

> Obdaryn is a free, open-source (GPL-3.0-or-later), read-only OBD-II diagnostics app for macOS 14.4 or later (Apple Silicon and Intel). With an ELM327-compatible adapter it reads trouble codes, readiness monitors, freeze frames, live data and the codes of other modules (ABS, airbag, body and more), and explains each code in plain words. It can't change anything in the car except clearing codes when the user asks. No account, no subscription; it sends nothing about the user or the car online.

This site is in English (${site}/) and Romanian (${site}/ro/). It has a page for every trouble code the app explains: ${thousands(codes.en.generic.length)} generic OBD-II codes and ${thousands(distinctCodes(codes.en.brand))} manufacturer codes, with what the code means, how the car detects it, symptoms, likely causes, whether it matters for the periodic inspection, and a rough repair cost.

## Trouble code pages

- Generic code: ${site}/codes/<CODE>/ with the code in capitals, e.g. [P0420](${site}/codes/P0420/). Romanian, where translated: ${site}/ro/coduri/<CODE>/
- Manufacturer code: ${site}/codes/<brand>/<CODE>/, brands: ${brandPacks.map((p) => `\`${p.id}\` (${p.label.en})`).join(", ")}. A code that several modules use has one page per module: ${site}/codes/<brand>/<CODE>-<module>/, listed on the brand's page.
- [All codes](${site}/codes/): the index, by group (P00xx, P01xx...) and by brand.
${brandPacks.map((p) => `- [${p.label.en} codes](${site}/codes/${p.id}/)`).join("\n")}

The explanations of generic codes come from obdex (CC0); manufacturer codes cite their public sources on each page. They are guidance, not a diagnosis: the car's own workshop data comes first.

## The app

- [Download](${site}/download/): the macOS installer (free), what's needed, and how to open it the first time.
- [User guide](${site}/guide/) ([Markdown](${site}/guide.md)): connecting, every screen, what clearing codes does, troubleshooting.
- [Supported cars](${site}/cars/): what works on every OBD-II car, and what Obdaryn knows per brand.
- [Privacy](${site}/privacy/)
- [Source code and releases](${repoUrl})

## Optional

- [Romanian home page](${site}/ro/) and [user guide in Romanian](${site}/ro/ghid/) ([Markdown](${site}/ro/ghid.md))
- [How Obdaryn talks to the car](${repoUrl}/blob/main/docs/CAR_INTERACTION.md): every request it sends, and what it never sends.
`;
  return new Response(text, { headers: { "Content-Type": "text/plain; charset=utf-8" } });
}
