# Changelog

Notable changes to Universe are recorded here. This project is under active development, so entries describe repository changes rather than guaranteeing that every feature has been verified in-game.

## Unreleased

- Added editable AI model and API base URL fields to the in-game AI page.
- Updated the cube selector to set the equipped cube ID without modifying unlock flags; public profile sync depends on Geometry Dash's normal account-save behavior.
- Kept the Anthropic API key outside mod settings; the game reads `ANTHROPIC_API_KEY` from its process environment.
- Fixed MATJSON array construction and response iteration in the AI request flow.
- Fixed conversion of the Geode save-directory path before opening the generated draft file.
- Reworked the README with setup instructions, feature details, security guidance, and known limitations.
- Added this changelog.

## v0.8.0

- Added local practice tools, visual overlays, StartPos controls, and input recording/replay.
- Added the floating panel button and configurable panel appearance.
- Added offline and AI-assisted level-design blueprint helpers.
- Added the FFmpeg recording conversion script.
