# Changelog

## Universe 1 — Snapshot 26.2

- Fix tab switching so old page buttons cannot remain active or stack beneath the current page.
- Clear page-owned AI input references when switching tabs.
- Make StartPos previous/next navigation wrap around at either end of the list.
- Add defensive checks around optional UI and audio objects.
- Correct the Anthropic API version header used by the standalone AI level maker.
- Improve AI blueprint validation: reject invalid coordinates, unsupported object types, out-of-order events, and oversized drafts.
- Return readable HTTP error details from the standalone AI generator and handle output-file errors cleanly.
- Add automated regression tests for offline generation and blueprint validation.
- Keep the in-game AI assistant, offline draft generator, and prompt save/load tools.
- Update the Geode package version to `v1.0.1`; release name is Snapshot 26.2.
- Build Windows x64, Android ARM64, and Android ARMv7 into one inspected universal `.geode`.

## Previous development

See the repository history for the feature work leading to Universe 1.
