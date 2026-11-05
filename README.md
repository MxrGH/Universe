# Universe

A Geometry Dash 2.2091 Geode utility and local-practice mod by MxrMC.

## Current source (v0.6.0)
- In-game tabbed panel: Player, Practice, Level, Visuals, Recorder, Keybinds, Settings.
- Functional object and slope collision bypass (Noclip) in Normal and Practice Mode (in-game verification still pending).
- Gameplay Speedhack with an adjustable 0.25x–3.00x multiplier.
- Configurable Geode keybinds for opening the menu, toggling Noclip, Speedhack, and Safe Mode.
- Customizable RGBA panel background color through Geode mod settings.
- Safe Mode can be toggled independently at any time.
- Safe Mode tracks whether Noclip or Speedhack was active during the current PlayLayer. If Safe Mode is enabled at any point after a cheat was used, Universe marks that run as Practice Mode locally. This is intended to prevent a normal completion from being recorded, but it still needs in-game verification.
- Speedhack scales the PlayLayer update delta; physics, triggers, and timing can behave differently at altered speeds.
- With Safe Mode disabled, Noclip still works, but the run is not protected from being treated as a normal attempt.
- Settings toggles for local icon preview, practice tools, noclip, global click sound, and Safe Mode.
- Global touch/click sound using resources/click.mp3.
- Windows x64, Android ARM64, and Android ARMv7 build targets.

## Integrity and status
Noclip now works in both Normal and Practice Mode. Safe Mode is a user-controlled local guard: it remembers whether Noclip or Speedhack was used during the current PlayLayer and marks it as Practice Mode when Safe Mode is enabled. Turning Safe Mode off does not undo that flag for the already-running attempt. This is not a guarantee about server-side leaderboard enforcement, and the behavior still needs in-game testing. Do not submit modified runs as legitimate records. Universe does not spoof verification, coins, achievements, profiles, or leaderboard results.

The Player, Level, Visuals, and Recorder tabs still contain informational placeholders for larger systems not yet implemented. Keybind actions and the Speedhack are implemented in source but have not yet been built or tested in-game. The panel color is configurable in Geode mod settings. Icon preview is currently a stored local-only setting, not yet an icon renderer.

## Build
GitHub Actions builds Windows x64 and Android ARM64/ARMv7, then combines the platform outputs into a universal .geode artifact.