# Universe

A Geometry Dash 2.2091 Geode utility and local-practice mod by MxrMC.

## Current source (v0.5.0)
- In-game tabbed panel: Player, Practice, Level, Visuals, Recorder, Keybinds, Settings.
- Functional object and slope collision bypass (Noclip) in Normal and Practice Mode (in-game verification still pending).
- Gameplay Speedhack with an adjustable 0.25x–3.00x multiplier.
- Safe Mode can be toggled independently at any time.
- With Safe Mode and Noclip enabled, Universe marks the active run as Practice Mode locally. This is intended to prevent a normal completion from being recorded, but it still needs in-game verification.
- Speedhack scales the PlayLayer update delta; physics, triggers, and timing can behave differently at altered speeds.
- With Safe Mode disabled, Noclip still works, but the run is not protected from being treated as a normal attempt.
- Settings toggles for local icon preview, practice tools, noclip, global click sound, and Safe Mode.
- Global touch/click sound using resources/click.mp3.
- Windows x64, Android ARM64, and Android ARMv7 build targets.

## Integrity and status
Noclip now works in both Normal and Practice Mode. Safe Mode is a user-controlled local guard: when it is enabled together with Noclip, the active PlayLayer is marked as Practice Mode. Turning Safe Mode off does not undo that flag for the already-running attempt. This is not a guarantee about server-side leaderboard enforcement, and the behavior still needs in-game testing. Do not submit modified runs as legitimate records. Universe does not spoof verification, coins, achievements, profiles, or leaderboard results.

The Player, Level, Visuals, Recorder, and Keybinds tabs still contain informational placeholders for features not yet implemented. The Speedhack is implemented in source but has not yet been built or tested in-game. Icon preview is currently a stored local-only setting, not yet an icon renderer.

## Build
GitHub Actions builds Windows x64 and Android ARM64/ARMv7, then combines the platform outputs into a universal .geode artifact.