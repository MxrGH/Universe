# Changelog

## Universe 1 — Snapshot 26.5

- Refresh the panel with a purple/cyan accent frame, local-client status header, selected-tab highlighting, and clearer icon-lab state labels.
- Clarify that icon browsing is a local preview and equipping remains restricted to icons already unlocked on the account.

- Remove the procedural offline draft generator from the in-game AI tab and standalone Level Maker; online AI remains API-key-based.
- Add a Settings-page in-game runtime self-test for mod, scene, GameManager, and settings access.
- Remove the standalone offline generator CLI option and its regression test; CI now validates blueprint conversion without generating fake AI content.

- Replace the AI editor's section-editing brief with a full-level creation brief: 6–12 connected sections and normally approximately 150–500 object placements generated from scratch.
- Enforce a hard ceiling of 1,000,000 generated object placements per draft; preflight rejects an oversized draft before it modifies the editor.
- Document that the one-million ceiling is not yet the practical paste capacity: the current apply path retains a 1,000-operation processing guard and 2 MB draft-file limit; chunked placement, progress, and cancellation remain unfinished.
- Add a dedicated **CREATE FULL LEVEL IN EDITOR** action that pastes the generated layout into the currently open editor; it does not erase existing content, so an empty level is recommended.
- Expand the online AI response budget to 8192 tokens for larger complete-level layouts.
- Document current placement limits: blocks, spikes, saws, orbs, pads, and deco; portals, triggers, and song timing are not yet generated.
- Add live editor context to AI generation: object count and up to 500 existing objects and 100 selected objects, including unique IDs, object IDs, positions, and rotations.
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
