// SPDX-License-Identifier: GPL-3.0-or-later
//
// obdaryn.eu: a static site, built on the developer's Mac and copied to the
// server with tools/deploy-site.sh. English at /, Romanian at /ro/.
import { defineConfig } from "astro/config";
import sitemap from "@astrojs/sitemap";
import { unified } from "@astrojs/markdown-remark";

// The guide is the repo's docs/USER_GUIDE*.md. Its links to other docs are relative;
// on the site they point at the same files on GitHub.
const docsOnGitHub = "https://github.com/liviu-hariton/obdaryn/blob/main/docs/";
function rehypeDocLinks() {
  const walk = (node) => {
    if (node.tagName === "a" && node.properties?.href) {
      const href = String(node.properties.href);
      if (!/^(https?:|mailto:|#|\/)/.test(href)) node.properties.href = docsOnGitHub + href;
    }
    node.children?.forEach(walk);
  };
  return walk;
}

export default defineConfig({
  markdown: { processor: unified({ rehypePlugins: [rehypeDocLinks] }) },
  site: "https://obdaryn.eu",
  trailingSlash: "always",
  // Stylesheets stay separate files, cached once for all 10,000-odd code pages,
  // instead of being copied into every page.
  build: { format: "directory", inlineStylesheets: "never" },
  integrations: [
    sitemap({
      i18n: { defaultLocale: "en", locales: { en: "en", ro: "ro" } },
      // The download-started pages aren't for search results, and neither are the 404s.
      filter: (page) => !/\/(started|a-inceput|404)\/$/.test(page),
    }),
  ],
  vite: {
    // The guide and the data files live in the app's repo, one level up.
    server: { fs: { allow: [".."] } },
  },
});
