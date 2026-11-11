// SPDX-License-Identifier: GPL-3.0-or-later
//
// Facts the whole site shares. The version is read from the app's CMakeLists.txt
// at build time, so the download button always names the version just released.
import { readFileSync } from "node:fs";
import { resolve } from "node:path";

/** The app's repository root. Builds run from site/ (npm run build), one level below it.
 *  (Not import.meta.url: the bundled build moves this file elsewhere.) */
export const repoRoot = resolve(process.cwd(), "..") + "/";

function appVersion(): string {
  const cmake = readFileSync(repoRoot + "CMakeLists.txt", "utf8");
  const m = cmake.match(/project\(Obdaryn\s+VERSION\s+([0-9.]+)/);
  if (!m) throw new Error("Can't find the version in CMakeLists.txt");
  return m[1];
}

export const version = appVersion();
export const repoUrl = "https://github.com/liviu-hariton/obdaryn";
/** The installer of this version, attached to its GitHub release (tools/publish-release.sh). */
export const dmgUrl = `${repoUrl}/releases/download/v${version}/Obdaryn-${version}.dmg`;
export const releasesUrl = `${repoUrl}/releases`;
export const issuesUrl = `${repoUrl}/issues`;
export const contactEmail = "hello@obdaryn.eu";
/** Where "Buy me a coffee" leads (a plain link: nothing is loaded from there). */
export const coffeeUrl = "https://www.buymeacoffee.com/liviuhariton";
export const firstPublished = 2026;
export const copyrightYears = (() => {
  const now = new Date().getFullYear();
  return now > firstPublished ? `${firstPublished}–${now}` : String(firstPublished);
})();

/** Obdaryn as schema.org data (JSON-LD), for the home and download pages: what it
 *  is, what it runs on, that it's free, its licence and where to get it. */
export function softwareJsonLd(lang: "en" | "ro", pageUrl: string, description: string) {
  return {
    "@context": "https://schema.org",
    "@type": "SoftwareApplication",
    name: "Obdaryn",
    url: pageUrl,
    description,
    applicationCategory: "UtilitiesApplication",
    operatingSystem: "macOS 14.4 or later",
    softwareVersion: version,
    downloadUrl: dmgUrl,
    isAccessibleForFree: true,
    offers: { "@type": "Offer", price: "0", priceCurrency: "EUR" },
    license: "https://www.gnu.org/licenses/gpl-3.0.html",
    inLanguage: ["en", "ro"],
    image: `https://obdaryn.eu/og-${lang}.png`,
    author: { "@type": "Person", name: "Liviu Hariton" },
    sameAs: [repoUrl],
  };
}
