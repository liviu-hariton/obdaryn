// SPDX-License-Identifier: GPL-3.0-or-later
//
// /codes/lookup.json: the "Look up a code" list (built in src/data/lookup.ts).
import { lookupJson } from "../../data/lookup";

export function GET() {
  return new Response(lookupJson, { headers: { "Content-Type": "application/json; charset=utf-8" } });
}
