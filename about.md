# Universe 1 — Snapshot 26.4

Universe 1 is a Geometry Dash 2.2091 Geode hack client by MxrGH.

Snapshot 26.4 expands gameplay modifiers beyond Practice Mode: Noclip and Speedhack remain active in Normal Mode when Safe Mode is turned off. With Safe Mode enabled, modified attempts are switched to Practice Mode. StartPos tools, hitboxes, movement trail/projection, FPS controls, local replay, keybinds, themes, and icon preview remain part of the client.

The UNIV floating control is now circular, uses the Universe logo, and leaves a fading motion trail while dragged. The panel uses rounded geometry and animated opening/closing.

The AI tab can generate a full level layout and apply supported objects directly into the currently open Geometry Dash editor. Android users can enter an API key for one request in the AI tab; the key is not saved by Universe. It does not export a .gmd file or depend on GDShare. The first editor integration inserts objects; arbitrary edits/deletions of existing objects are not yet supported. Online AI requires a user-provided API key over HTTPS. Offline Draft is procedural, not neural inference.

The release pipeline builds Windows x64, Android ARM64, and Android ARMv7, combines them into one .geode package, validates its contents, and publishes only after all build jobs pass. CI packaging success is not a substitute for testing in the actual game.
