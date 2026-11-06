# Changelog

## Universe 1 — Snapshot 26.3

- Harden Practice Mode replay: recording requires an active player, playback requires Practice Mode, and malformed, oversized, unsorted, or overlong replay files are rejected.
- Prevent replay state from lingering if the active player disappears or a level scene changes.
- Standardize the in-game online AI response around the same JSON blueprint schema used by the standalone generator.
- Save online AI results as `universe-ai-draft.json` and direct users to the validation/export CLI.
- Upgrade the standalone Level Maker with blueprint import, strict validation, and JSON, CSV, and Markdown export.
- Expand the regression suite to cover invalid sections/gameplay, JSON output, CSV rows, and Markdown sections.
- Add preview browsing for Cube, Ship, Ball, UFO, Wave, Robot, Spider, Swing, and Jetpack; equip only icons the game reports as owned.
- Require HTTPS for AI API requests and keep credentials out of mod settings/source.
- Retain all Snapshot 26.2 practice, visual, UI, replay, and AI tools.
- Bump Geode package version to `v1.0.2`; release name is Snapshot 26.3.
- Build and inspect one universal package for Windows x64, Android ARM64, and Android ARMv7 before publishing.

## Previous releases

- Snapshot 26.2 — replay/UI hardening and AI blueprint validation.
- Snapshot 26.1 — initial Universe 1 release.
