# Changelog

## Universe 1 — Snapshot 26.5

- Align the Uni-1 client quota date with the server's 00:00 UTC reset.
- Add regression coverage for UTC quota rollover.
- Keep the Geode package version at v1.1.1; this is a maintenance update within Snapshot 26.5.

- Add a player-bounds overlay alongside object hitboxes, movement trail, and movement projection.
- Refresh the Universe panel with theme accents, selected-tab state, local-client status, and touch-friendly controls.
- Add local-only icon preview and color overrides; account unlocks and saved icon selections are not modified.
- Add a self-hosted Uni-1 client/server development path with prompt validation, request cancellation, bounded editor placement, and server-enforced per-installation quotas.
- Keep Uni-1 generation explicitly non-operational until a reviewed dataset, trained adapter, deployed server, and live game test exist.
- Limit generated drafts to 6–8 sections and 96–144 geometry placements, with a hard 1,000-object validation ceiling.
- Build and inspect a universal package for Windows x64, Android ARM64, and Android ARMv7 before any deliberate publication.

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
