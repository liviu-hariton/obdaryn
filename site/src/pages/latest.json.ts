// SPDX-License-Identifier: GPL-3.0-or-later
//
// /latest.json: the newest version, for the app's opt-in update check
// (src/app/UpdateChecker.cpp). Only the version: the app links to its own,
// fixed download page, so nothing here can send anyone elsewhere.
import { version } from "../data/site";

export function GET() {
  return new Response(JSON.stringify({ version }) + "\n", {
    headers: { "Content-Type": "application/json; charset=utf-8" },
  });
}
