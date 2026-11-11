// SPDX-License-Identifier: GPL-3.0-or-later
//
// The user guide as Markdown (/guide.md, /ro/ghid.md), for AI agents and anyone
// who'd rather read the source: the repo's docs/USER_GUIDE*.md as they are, with
// links to other docs pointed at GitHub (as on the HTML pages, astro.config.mjs).
import { readFileSync } from "node:fs";
import { repoRoot, repoUrl } from "./site";

export function guideMarkdown(lang: "en" | "ro"): Response {
  const file = lang === "ro" ? "USER_GUIDE.ro.md" : "USER_GUIDE.md";
  const text = readFileSync(`${repoRoot}docs/${file}`, "utf8")
    .replace(/\]\((?!https?:|mailto:|#|\/)([^)]+)\)/g, `](${repoUrl}/blob/main/docs/$1)`);
  return new Response(text, { headers: { "Content-Type": "text/markdown; charset=utf-8" } });
}
