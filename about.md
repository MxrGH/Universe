# Universe

Universe is a Geode mod for Geometry Dash 2.2091, developed by MxrMC.

The current source includes an in-game panel, local practice controls, StartPos tools, visual helpers, and a basic input recorder/replay system. Some features still need testing in the game, especially on Android.

## Current limitations

- Safe Mode changes local game state only. It cannot guarantee server-side leaderboard enforcement.
- The player trail displays recent positions. The optional movement projection is a straight-line estimate, not a full physics simulation.
- The icon preview setting does not currently render an icon.
- AI Level Maker and video export are not implemented.
- The floating touch button is fixed in place and cannot be dragged.

Universe does not modify server-side unlocks, achievements, verification, coins, profiles, or leaderboard records.

## Build targets

- Geometry Dash 2.2091
- Windows x64
- Android ARM64 and ARMv7
- Geode SDK 6.0.0-alpha.1
