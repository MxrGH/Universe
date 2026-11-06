# Changelog

## v1.1.0 — Display label 1.1.0.1

### Added
- In-game offline procedural draft generation that works without network access or an API key, including on Android.
- Load Saved Prompt action alongside save and clear.
- Bounded replay file loading and event validation, plus stable event ordering before playback.
- Practice Mode-only Noclip and Speedhack behavior.
- Custom panel title and floating-button labels, live theme updates, and saved panel position with reset.
- Fixed a Cocos layer animation compile error that blocked the previous Windows and Android builds.
- Continued animated panel entrance, page transitions, and button feedback.
- One-tap speed reset and quick FPS target presets at 120, 240, and 360.

### Important AI note
- Offline Draft is procedural generation, not neural AI.
- True offline neural inference on Android still needs a bundled model and mobile inference runtime.

## v1.0.0 — Display label 1.00.0.00

This is the first release candidate for the 1.0 line. The Geode manifest uses `v1.0.0`; the four-part display label is `1.00.0.00`.

### Added
- One-tap Practice, Visuals, and Clean Run setups in the Settings page.
- A Restart Attempt action on the Start positions page.
- Clearer, hand-written panel wording and more direct explanations of what each tool does.

### Updated
- Updated the project guide and release metadata for the 1.0 line.
- Clarified that the cube selector changes the equipped cube ID but does not grant an unlock or achievement.
- Kept the online level idea helper optional; the offline generator remains available without an API key.

### Notes
- CI success confirms that the Windows and Android packages build and combine. It does not prove every feature has been tested inside the game on physical devices.
- Practice modifications are local tools. Safe Mode is not a guarantee about server or leaderboard treatment.
- The level idea helper produces a design outline, not an importable Geometry Dash level.

## v0.8.0

- Added local practice tools, visual overlays, StartPos controls, and input recording/replay.
- Added the floating panel button and configurable panel appearance.
- Added offline and AI-assisted level-design blueprint helpers.
- Added the FFmpeg recording conversion script.
