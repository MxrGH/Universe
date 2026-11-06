# Changelog

## Universe 1 — Snapshot 26.3

- Harden Practice Mode replay: require an active player, require Practice Mode for playback, and reject malformed, oversized, unsorted, or overlong replay files.
- Clear replay state safely when the player or active level scene disappears.
- Standardize in-game online AI output around the Level Maker JSON blueprint schema.
- Save online AI results as `universe-ai-draft.json`.
- Add JSON blueprint import/validation and CSV/Markdown export to the standalone Level Maker.
- Expand regression tests for invalid gameplay/sections and all supported export formats.
- Require HTTPS for AI API endpoints to protect API keys in transit.
- Retain the existing Practice Mode tools, overlays, customization, mobile controls, and AI assistant.

## Previous releases

- Snapshot 26.2 — replay/UI hardening and AI blueprint validation.
- Snapshot 26.1 — initial Universe 1 release.
