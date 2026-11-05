# Universe

A Geometry Dash 2.2091 Geode utility and local-practice mod by MxrMC.

## Current source (v0.6.0)

### Panel and controls
- Tabbed Universe panel: Player, Practice, Level, Visuals, Recorder, Keybinds, and Settings.
- Global mouse/touch click sound.
- Configurable Geode keybinds and RGBA panel background.
- StartPos Switcher with Previous / Next and Smart selection (nearest StartPos ahead of the player).

### Gameplay and practice tools
- Object and slope collision bypass (Noclip) for Normal and Practice Mode.
- Adjustable gameplay Speedhack (0.25x–3.00x).
- Safe Mode can be toggled independently. It remembers whether Noclip or Speedhack was used during the current PlayLayer and locally marks the run as Practice Mode when enabled.
- Practice-only input recorder and local replay playback.
- Object hitbox overlay.
- FPS render-interval override (60–360 target setting).

### Important limitations
- Source features are not automatically confirmed working in-game. Noclip, Safe Mode behavior, Smart StartPos, hitboxes, FPS override, and recorder/replay still require gameplay testing across supported platforms.
- Safe Mode is a local guard, not a guarantee of server-side leaderboard enforcement. Do not submit modified runs as legitimate records.
- Icon preview is currently a stored local-only setting, not a completed icon renderer. Universe does not spoof online unlocks, verification, coins, achievements, profiles, or leaderboard results.
- Trajectory prediction, expanded player/icon tools, theme presets, FFmpeg export, and the full mobile floating panel are not implemented yet.

## Coming soon

### AI Level Maker
**AI Level Maker is coming soon.** The planned tool will turn a written idea into a Geometry Dash level draft, with configurable gameplay style, difficulty, length, and song timing. The generated layout will be editable before export. This is an announcement, not a released feature; the generation engine and editor integration are still in development.

### Eclipse-inspired panel expansion
More panel tools are planned, including trajectory display, additional visual controls, expanded recorder/replay management, UI themes, and improved PC/mobile access.

## Build

GitHub Actions builds Windows x64 and Android ARM64/ARMv7, then combines the platform outputs into a universal .geode artifact.
