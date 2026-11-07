# Changelog

## Universe 1 — Snapshot 26.5

- Add a local-only icon override that does not alter account unlocks or saved icon selection.
- Add configurable local primary, secondary, and glow colors, plus a local glow toggle.
- Keep the untrained Uni-1 AI creator out of the shipped interface until model training and deployment are complete.
- Align project validation with the online-only AI workflow and remove stale checks for deleted offline/icon enum code.
- Preserve existing gameplay modifiers, Safe Mode, StartPos tools, visual overlays, replay recorder, icon previews, themes, keybinds, presets, and mobile controls.
- Bump Geode package version to v1.1.1; keep publication gated on successful CI and physical-device QA.

## Universe 1 — Snapshot 26.4

- Expand Noclip and Speedhack beyond Practice Mode; Safe Mode ON switches modified attempts to Practice, while Safe Mode OFF leaves the modifiers active in Normal Mode.
- Add an in-game AI draft apply action that converts supported AI-generated events into objects in the currently open Geometry Dash editor.
- Remove the requirement to export a .gmd file or use GDShare for the in-game AI placement flow.
- Add rounded panel geometry and animated closing transitions.
- Replace the rectangular UNIV control with a circular Universe-logo button and fading motion trail while dragging.
- Retain StartPos navigation, trajectory, hitboxes, FPS controls, recorder/replay, themes, keybinds, touch support, and icon previews.
- Bump Geode package version to `v1.1.0`.
- Build and inspect one universal package for Windows x64, Android ARM64, and Android ARMv7 before publishing.

## Universe 1 — Snapshot 26.3

- Harden Practice Mode replay: recording requires an active player, playback requires Practice Mode, and malformed, oversized, unsorted, or overlong replay files are rejected.
- Standardize the in-game online AI response around the same JSON blueprint schema used by the standalone generator.
- Expand icon preview browsing to nine categories and equip only icons the game reports as owned.
- Require HTTPS for AI API requests and keep credentials out of mod settings/source.

## Previous releases

- Snapshot 26.2 — replay/UI hardening and AI blueprint validation.
- Snapshot 26.1 — initial Universe 1 release.

- Replace the local Uni-1 request counter with server-enforced per-installation quotas: 100/day, or 1,000/day after seven UNIV taps; resets at 00:00 UTC.

- Add a reproducible synthetic Uni-1 dataset bootstrap generator, container recipe, deployment guide, and server quota tests.
