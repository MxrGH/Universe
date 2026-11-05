# Universe

A Geode mod project targeting Geometry Dash **2.2091** on Windows and Android.

Repository: https://github.com/MxrGH/Universe

## Build targets

The GitHub Actions workflow builds Windows x64, Android ARM64, and Android ARMv7, then combines them into one universal `.geode` package.

Download the artifact named **Universe-Windows-Android-GD-2.2091** from the repository's Actions run.

## Implemented

- Main-menu Universe entry button.
- Global, non-blocking touch listener.
- Bundled click sound: plays on Android screen taps throughout Geometry Dash when **Global click sound** is enabled.
- Click sound is included as a mod resource.

## In progress / not implemented yet

- Full Eclipse/Mega Hack-style control panel.
- Noclip gameplay hook and record-safety enforcement.
- Local icon preview controls.
- Local practice helper controls.

Safe Mode is intended to keep modified gameplay from being submitted as legitimate records. This project does not spoof server-side icons, fabricate online achievements, or bypass leaderboard validation.
