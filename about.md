# Universe 1 — Snapshot 26.3

Universe 1 is a Geometry Dash 2.2091 Geode toolkit by MxrGH.

It combines Practice Mode utilities, StartPos navigation, visual overlays, movement indicators, configurable FPS rendering, local recording/replay, icon selection, an animated customizable interface, touch controls, and an optional AI level-design assistant.

The AI assistant remains included. Online generation uses an Anthropic-compatible Messages API over HTTPS and reads its key from the Geometry Dash process environment. Offline Draft creates a procedural JSON blueprint without a network request. The standalone Level Maker can validate JSON blueprints and export them to CSV or Markdown.

These files are design blueprints, not native playable `.gmd` files. The icon selector does not unlock locked icons or grant achievements. Noclip and Speedhack remain restricted to Practice Mode; Safe Mode is a local safeguard, not a server-side guarantee.

The release pipeline builds Windows x64, Android ARM64, and Android ARMv7 independently, combines them into one `.geode` package, validates its contents, and publishes Snapshot 26.3 only after all platform builds pass.
