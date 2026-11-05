# Changelog

## v0.1.0
- Initial Geode project scaffold.
- Added Universe entry button and starter popup.
- Added initial settings for click audio, Safe Mode, noclip, icon preview, and practice tools.

## v0.8.0
- Add a cube icon preview with a local ID selector.
- Replace the fixed floating menu button with a touch layer that can be dragged and remembers its position.
- Replace the straight-line path estimate with a short projection based on recent velocity and acceleration.
- Add a level-draft tool with offline templates and optional AI-assisted generation through an OpenAI-compatible API.
- Add an FFmpeg helper for converting existing gameplay recordings to MP4.
- Expand build checks to compile the Python tools and run a level-generator smoke test.

## v0.7.0
- Release held inputs when a practice replay ends or is stopped.
- Show live player mode, position, and local speed in the Player tab.
- Let the floating touch button be enabled or disabled from the in-game Settings tab.
- Handle missing or stale active StartPos references without using an invalid array index.
- Add a short straight-line movement projection with a separate toggle and keybind.
- Add a dependency-free metadata check to the build workflow.
- Reworked project documentation to describe current behavior and limitations in plain language.
- Clarified Safe Mode and icon preview settings.

## v0.6.0
- Added all-mode Noclip source hooks and independently switchable Safe Mode.
- Added Speedhack controls, configurable keybinds, FPS override, hitbox overlay, StartPos Switcher, and practice input recording/replay.
- Added Smart StartPos selection.
- Announced the upcoming AI Level Maker.
- Added an Eclipse-inspired panel expansion roadmap.
