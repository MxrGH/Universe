# Universe 1 — Snapshot 26.2

Universe 1 is a Geometry Dash 2.2091 Geode toolkit by MxrGH.

It combines Practice Mode utilities, StartPos navigation, visual overlays, movement indicators, configurable FPS rendering, local recording/replay, icon selection, an animated customizable interface, and an optional AI level-design assistant.

The AI assistant remains included. Online generation uses an Anthropic-compatible Messages API and reads its key from the Geometry Dash process environment. Offline Draft creates a procedural JSON blueprint without a network request. Neither output is a native playable level file.

The release pipeline builds Windows x64, Android ARM64, and Android ARMv7 independently, combines them into one .geode package, validates its contents, and publishes Snapshot 26.2 only after all platform builds pass.

Noclip and Speedhack are restricted to Practice Mode. Safe Mode is a local safeguard, not a server-side guarantee. The icon selector does not unlock locked icons or grant achievements.
