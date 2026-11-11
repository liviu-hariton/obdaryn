# Making an installable Obdaryn (macOS)

Everything happens on your Mac. Nothing is uploaded and no paid service is involved.

```bash
tools/make-release.sh
```

It builds with **the official Qt binaries**, not Homebrew's: Homebrew builds Qt and its libraries for the newest macOS only, so an installer made with them opens only on that version. Install the official Qt once, into `~/Qt` (the script looks for `~/Qt/6.12.0/macos`; set `QT=...` to use another folder). [aqtinstall](https://github.com/miurahr/aqtinstall) downloads it without a Qt account:

```bash
python3 -m venv /tmp/aqt && /tmp/aqt/bin/pip install aqtinstall
/tmp/aqt/bin/aqt install-qt mac desktop 6.12.0 clang_64 --outputdir ~/Qt
```

Day-to-day development keeps using Homebrew's Qt; only the installer needs the official one.

The script:

1. builds a Release configuration in `build-release/` (configured fresh each time) for macOS 14.4 and later, as one universal binary for Apple Silicon and Intel, and runs the unit tests;
2. copies `Obdaryn.app` to `dist/` and makes it self-contained with `macdeployqt` (the Qt frameworks and plugins go inside the app), leaving out the database drivers Qt adds that need client libraries the app doesn't ship;
3. removes the build's Qt library path, so a missing library can't hide behind the Qt installed on this Mac;
4. checks that every binary in the app runs on macOS 14.4, and stops if one needs a newer macOS;
5. signs the app ad hoc (`codesign --sign -`), which Apple Silicon requires before an app may run;
6. makes `dist/Obdaryn-<version>.dmg` with the app and an Applications shortcut.

`macdeployqt` prints "Cannot resolve rpath" for some QML plugins (3D, virtual keyboard, PDF, timeline) and "no file at" for the Mimer, ODBC and PostgreSQL client libraries. Obdaryn never loads any of them, so those messages are expected.

## Installing and the first launch

1. Open the `.dmg` and drag **Obdaryn** to **Applications**.
2. The app is signed ad hoc but not notarized, so macOS refuses it the first time. Open it, click **Done**, then go to **System Settings → Privacy & Security** and click **Open Anyway** next to "Obdaryn was blocked". After that it opens normally. (Right-click → Open no longer bypasses this on macOS 15 and later.)
3. The first time you connect to the adapter, macOS asks whether Obdaryn may find devices on the local network. Allow it.

## Which macOS versions it runs on

macOS 14.4 (Sonoma) and later, on Apple Silicon and Intel. That is the minimum of the official Qt 6.12 binaries; `MIN_MACOS` in `tools/make-release.sh` sets the build's deployment target and `LSMinimumSystemVersion` in `Info.plist` to match. Going lower needs an older Qt (or Qt built from source) with a lower minimum.

The Intel half has been run under Rosetta on an Apple Silicon Mac, not yet on an Intel Mac.

## Publishing a release and the website

1. `tools/make-release.sh` builds `dist/Obdaryn-<version>.dmg`.
2. `tools/publish-release.sh` tags the commit `v<version>` and creates the GitHub release with the `.dmg` and its SHA-256. It needs the GitHub CLI, signed in once (`brew install gh`, `gh auth login`).
3. `tools/deploy-site.sh` rebuilds the website so its download button names the new version, and copies it to obdaryn.eu. See [the website section](#the-website).

## The website

The site at https://obdaryn.eu lives in `site/` (Astro, static, English at `/` and Romanian at `/ro/`). It reads the version from `CMakeLists.txt`, the explained codes and supported cars from `src/core/data/`, and the guide from `docs/USER_GUIDE.md` and `docs/USER_GUIDE.ro.md`, so it changes with the app. It loads nothing from other servers and sets no cookies.

- Preview: `cd site && npm install && npm run dev`, then open http://localhost:4321.
- Size: about 10,700 pages (one per trouble code, in English, and in Romanian where the explanation is translated), about 180 MB, built in a few seconds. The first upload takes a while; later ones send only what changed.
- Screenshots: `tools/capture-site-images.sh` (after building the app) captures every screen the site shows, in both languages and themes, from the demo car.
- Deploy: create `site/.deploy.env` (git-ignored) with `OBDARYN_SITE_SSH="user@server"`, then run `tools/deploy-site.sh` (or `--dry-run` first). It builds, asks, and `rsync`s `site/dist/` into the HestiaCP web root (`~/web/obdaryn.eu/public_html/`).
- Social images: `site/public/og-en.png` and `og-ro.png` are captures of the home page's first screen at 1200 × 630, dark theme. Recapture them when the hero changes.

## Version and icon

- The version comes from `project(Obdaryn VERSION …)` in the top-level `CMakeLists.txt`.
- The in-app logo is `src/core/data/brand/logo.svg` (32px and up) and `logo-small.svg` (below 32px), each with a `-dark` variant that adds the hairline edge. The app icon is the same drawing with the macOS margins: `src/app/macos/icon-master.svg` and `icon-master-small.svg`. Change them together.
- The icon is `src/app/macos/Obdaryn.icns`, built from the PNGs in `src/app/macos/Obdaryn.iconset` (the 16 and 32px slots use the small drawing, so don't resample them from the 1024px master):

  ```bash
  iconutil -c icns src/app/macos/Obdaryn.iconset -o src/app/macos/Obdaryn.icns
  ```

## Signing for other people

Anyone can build and run their own copy as above. To hand out a `.dmg` that opens without warnings, the app must be signed with an Apple Developer ID and notarized by Apple (`macdeployqt -sign-for-notarization`, then `xcrun notarytool`). That needs a paid Apple Developer account, which this project doesn't use.
