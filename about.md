# Universe 1 — Snapshot 26.6

Universe 1 is a Geometry Dash 2.2091 Geode client by MxrGH. This update ships as Geode package v1.1.2.

The client includes local gameplay modifiers, Safe Mode practice marking, StartPos tools, object hitboxes, player-bounds overlay, movement trail and projection, FPS controls, Practice Mode replay tools, icon previews, local-only icon overrides, keybinds, themes, presets, and touch controls.

The Uni-1 AI page is present as an experimental integration, but generation is not operational: there are no trained adapter weights, no deployed inference endpoint, and no human-reviewed/playtested training corpus. The client and server quota counters both use a UTC daily rollover.

The build workflow targets Windows x64, Android ARM64, and Android ARMv7 and inspects a combined universal package. CI compilation and emulator boot checks do not replace installing Geode and Geometry Dash and testing the mod in the actual game on physical devices.

The mod declares no additional Geode mod dependencies. Its numeric Geometry Dash object-ID catalogue is bundled in the package; Geode itself remains required.
