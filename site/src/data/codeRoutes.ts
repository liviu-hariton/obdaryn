// SPDX-License-Identifier: GPL-3.0-or-later
//
// The static paths of the code pages, shared by the English (/codes/) and
// Romanian (/ro/coduri/) routes: one page per code, one per group of generic
// codes (/codes/p04/) and one list per brand (/codes/hyundai-kia/).
import { codes, groupsOf, packs, systemOf, type CodeEntry } from "./allCodes";
import { codeStrings } from "../i18n/codes";
import { routes, type Lang } from "../i18n/ui";

export type SegProps =
  | { kind: "code"; entry: CodeEntry }
  | { kind: "list"; title: string; lede: string; entries: CodeEntry[]; pack?: string;
      here: Partial<Record<Lang, string>>; prev?: { href: string; label: string }; next?: { href: string; label: string } };

/** /codes/<seg>/: a generic code, a group of generic codes, or a brand's list. */
export function segPaths(lang: Lang) {
  const c = codeStrings[lang];
  const base = routes[lang].codes;
  const groups = groupsOf(codes[lang].generic);
  const groupIds = (l: Lang) => new Set(groupsOf(codes[l].generic).map((g) => g.id));
  const enGroups = groupIds("en"), roGroups = groupIds("ro");

  const codePages = codes[lang].generic.map((entry) => ({ params: { seg: entry.slug }, props: { kind: "code", entry } as SegProps }));
  const groupPages = groups.map((g, i) => ({
    params: { seg: g.id },
    props: {
      kind: "list", title: c.groupTitle(g.label), lede: c.groupLede(c.systems[systemOf(g.entries[0].code)], g.entries.length),
      entries: g.entries,
      here: { en: enGroups.has(g.id) ? `${routes.en.codes}${g.id}/` : undefined, ro: roGroups.has(g.id) ? `${routes.ro.codes}${g.id}/` : undefined },
      prev: i > 0 ? { href: `${base}${groups[i - 1].id}/`, label: groups[i - 1].label } : undefined,
      next: i < groups.length - 1 ? { href: `${base}${groups[i + 1].id}/`, label: groups[i + 1].label } : undefined,
    } as SegProps,
  }));
  const packPages = packs.map((p) => {
    const entries = codes[lang].brand.filter((e) => e.pack === p.id);
    return {
      params: { seg: p.id },
      props: {
        kind: "list", title: c.packTitle(p.label[lang]), lede: c.packLede(entries.length), entries, pack: p.id,
        here: { en: `${routes.en.codes}${p.id}/`, ro: `${routes.ro.codes}${p.id}/` },
      } as SegProps,
    };
  });
  return [...codePages, ...groupPages, ...packPages];
}

/** /codes/<pack>/<slug>/: one manufacturer code. */
export function brandPaths(lang: Lang) {
  return codes[lang].brand.map((entry) => ({ params: { pack: entry.pack, slug: entry.slug }, props: { entry } }));
}
