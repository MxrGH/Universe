# Universe

A Geometry Dash 2.2091 Geode utility mod by MxrMC.

## Current build (v0.2.0)
- In-game tabbed panel: Player, Practice, Level, Visuals, Recorder, Keybinds, Settings.
- Working settings toggles for local icon preview, practice tools, noclip, global click sound, and Safe Mode.
- Global touch/click sound using the supplied `resources/click.mp3`.
- Windows x64, Android ARM64, and Android ARMv7 build targets.

## Important status
The tabbed interface is implemented. Some tabs (Level, Visuals, Recorder, Keybinds) are clearly marked as planned and are informational placeholders; their underlying systems are not implemented yet. Noclip is currently a stored setting toggle, not a gameplay hook. Icon preview is currently a stored local-only setting, not yet an icon renderer.

Safe Mode is a preference flag in this build, not a complete submission-integrity enforcement system. Do not submit modified runs as legitimate records. Universe does not spoof achievements, coins, verification, online profiles, or leaderboard results.

## Build
GitHub Actions builds Windows x64 and Android ARM64/ARMv7, then combines the platform outputs into a universal `.geode` artifact.
