# Changelog

## Universe 1 — Snapshot 26.6

- Remove the unused `geode.node-ids` dependency; no extra Geode mods are required.
- Keep the verified Geometry Dash numeric object-ID catalogue bundled inside the `.geode` package.
- Bump the Geode package version to v1.1.2.
- Gate publishing on successful Windows x64, Android ARM64, Android ARMv7, package inspection, and Android emulator boot checks.
- Keep Uni-1 clearly experimental: training data, adapter weights, a hosted inference service, and real in-game AI validation are still outstanding.

## Universe 1 — Snapshot 26.5

- Align Uni-1's client-side daily quota rollover with the server's 00:00 UTC reset.
- Add regression coverage for UTC quota rollover.
- Keep the Geode package version at v1.1.1.
- Add the player-bounds overlay, separate from object hitboxes.
- Refresh the in-game panel with theme accents, selected-tab highlighting, local-client status, and clearer icon-lab state.
- Keep icon overrides local-only; no account unlock flags or saved icon selections are modified.
- Add Uni-1 client/server development foundations, including request cancellation, strict generated-draft validation, chunked editor placement, and server-enforced quotas.
- Keep Uni-1 generation marked experimental and non-operational until training data, model weights, deployment, and live game testing are complete.
- Retain local gameplay modifiers, Safe Mode, StartPos tools, visual overlays, replay tools, keybinds, presets, themes, and Android touch controls.

## Previous releases

- Snapshot 26.4 — editor draft placement, expanded local gameplay modifiers, and panel polish.
- Snapshot 26.3 — replay hardening, JSON blueprint validation, and icon preview browsing.
- Snapshot 26.2 — replay/UI hardening and AI blueprint validation.
- Snapshot 26.1 — initial Universe 1 release.
