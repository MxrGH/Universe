# Universe

A Geometry Dash 2.2091 Geode utility and local-practice mod by MxrMC.

## Current build (v0.3.0)
- In-game tabbed panel: Player, Practice, Level, Visuals, Recorder, Keybinds, Settings.
- Functional object and slope collision bypass (Noclip), restricted to Practice Mode.
- Safe Mode is required for Noclip and cannot be disabled while Noclip is enabled.
- Noclip is automatically inactive in normal mode, including normal online runs.
- Settings toggles for local icon preview, practice tools, noclip, global click sound, and Safe Mode.
- Global touch/click sound using resources/click.mp3.
- Windows x64, Android ARM64, and Android ARMv7 build targets.

## Integrity and status
Noclip is intended for local Practice Mode testing only. It does not bypass normal-mode collisions, submit records, or spoof verification, coins, achievements, profiles, or leaderboard results. Safe Mode and the practice-mode check are enforced by the collision hooks.

The Player, Level, Visuals, Recorder, and Keybinds tabs still contain informational placeholders for features not yet implemented. Icon preview is currently a stored local-only setting, not yet an icon renderer.

## Build
GitHub Actions builds Windows x64 and Android ARM64/ARMv7, then combines the platform outputs into a universal .geode artifact.