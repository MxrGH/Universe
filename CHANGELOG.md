# Changelog

## Universe 1 — Snapshot 26.5

- Add live editor context to AI generation: object count and up to 120 existing objects and 30 selected objects, including unique IDs, object IDs, positions, and rotations.
- Clarify the AI apply summary so exact-ID edits/deletions are not mislabeled as selection-only operations.
- Extend AI drafts with selected-object and exact-unique-ID move, rotate, and delete operations, alongside object placement.
- Include a bounded live summary of existing editor objects so follow-up requests can refer to real object IDs and positions.
- Rebuild transformed selections through the editor object serialization/paste path and keep deletion on the editor undo history.
- Keep all prior StartPos, hitbox, trail/projection, FPS, replay, icon preview, theme, keybind, preset, touch, and floating-button features.
- Tighten apply limits and reject invalid/non-finite operation values before editing.
- Bump Geode package version to v1.1.1.
- Build and inspect one universal package for Windows x64, Android ARM64, and Android ARMv7 before publishing.

## Universe 1 — Snapshot 26.3

- Harden Practice Mode replay: require an active player, require Practice Mode for playback, and reject malformed, oversized, unsorted, or overlong replay files.
- Clear replay state safely when the player or active level scene disappears.
- Standardize in-game online AI output around the Level Maker JSON blueprint schema.
- Save online AI results as `universe-ai-draft.json`.
- Add JSON blueprint import/validation and CSV/Markdown export to the standalone Level Maker.
- Expand regression tests for invalid gameplay/sections and all supported export formats.
- Add all nine icon categories to the local preview; equip only icons the account owns.
- Require HTTPS for AI API endpoints to protect API keys in transit.
- Retain the existing Practice Mode tools, overlays, customization, mobile controls, and AI assistant.

## Previous releases

- Snapshot 26.2 — replay/UI hardening and AI blueprint validation.
- Snapshot 26.1 — initial Universe 1 release.
