# Universe

A Geode mod project targeting Geometry Dash **2.2091** on Windows and Android.

Repository: https://github.com/MxrGH/Universe

## Build targets

The GitHub Actions workflow builds:
- Windows x64
- Android ARM64 (Android64)
- Android ARMv7 (Android32)

The platform builds are combined into a single universal `.geode` package. When the workflow completes, download the artifact named **Universe-Windows-Android-GD-2.2091** from the repository's Actions run.

## Current implementation status

- Geode project metadata and multi-platform build workflow are present.
- A Universe entry button is present in the main menu.
- Settings are declared for Safe Mode, click sound, noclip, local icon preview, and practice tools.
- The full hack panel and gameplay hooks are still under development.
- The supplied click sound has not yet been included in the packaged resources.

Safe Mode is intended to keep modified gameplay from being submitted as legitimate records. Icon previews and testing features are local-only; the project will not spoof server-side data or fabricate online achievements.
