# Making an installable Obdaryn (macOS)

Everything happens on your Mac. Nothing is uploaded and no paid service is involved.

```bash
tools/make-release.sh
```

The script:

1. builds a Release configuration in `build-release/` and runs the unit tests;
2. copies `Obdaryn.app` to `dist/` and makes it self-contained with `macdeployqt` (the Qt frameworks and plugins go inside the app);
3. removes the build's Homebrew library path, so a missing library can't hide behind Homebrew on this Mac;
4. signs the app ad hoc (`codesign --sign -`), which Apple Silicon requires before an app may run;
5. makes `dist/Obdaryn-<version>.dmg` with the app and an Applications shortcut.

`macdeployqt` prints "Cannot resolve rpath" for some QML plugins (3D, virtual keyboard, PDF, timeline). Obdaryn never loads them, so those messages are expected.

## Installing and the first launch

1. Open the `.dmg` and drag **Obdaryn** to **Applications**.
2. The app is signed ad hoc but not notarized, so macOS refuses a normal double-click the first time. Right-click Obdaryn → **Open** → **Open**. After that it opens normally.
   - On recent macOS versions you may have to allow it under **System Settings → Privacy & Security → Open Anyway**.
3. The first time you connect to the adapter, macOS asks whether Obdaryn may find devices on the local network. Allow it.

## Version and icon

- The version comes from `project(Obdaryn VERSION …)` in the top-level `CMakeLists.txt`.
- The in-app logo is `src/core/data/brand/logo.svg` (32px and up) and `logo-small.svg` (below 32px), each with a `-dark` variant that adds the hairline edge. The app icon is the same drawing with the macOS margins: `src/app/macos/icon-master.svg` and `icon-master-small.svg`. Change them together.
- The icon is `src/app/macos/Obdaryn.icns`, built from the PNGs in `src/app/macos/Obdaryn.iconset` (the 16 and 32px slots use the small drawing, so don't resample them from the 1024px master):

  ```bash
  iconutil -c icns src/app/macos/Obdaryn.iconset -o src/app/macos/Obdaryn.icns
  ```

## Signing for other people

Anyone can build and run their own copy as above. To hand out a `.dmg` that opens without warnings, the app must be signed with an Apple Developer ID and notarized by Apple (`macdeployqt -sign-for-notarization`, then `xcrun notarytool`). That needs a paid Apple Developer account, which this project doesn't use.
