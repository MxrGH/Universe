# Universe

Universe is a Geode mod for Geometry Dash 2.2091. It adds a small in-game panel with local practice, visual, and replay tools.

## Available in the current source

- **Practice controls:** Noclip, a local gameplay speed multiplier, and Safe Mode.
- **StartPos tools:** move between StartPos objects or select the next one ahead of the player.
- **Visual helpers:** object hitboxes and a trail showing recent player positions.
- **Replay tools:** record and replay input events in Practice Mode. Recordings are stored locally.
- **Display options:** panel color presets, a configurable panel color, and an optional floating button for touch screens.
- **Frame timing:** an optional render-interval setting. This changes the render interval; it does not change the game's physics rate.
- **Keybinds:** the main actions can be assigned in Geode's settings.

## Controls

| Action | Default |
| --- | --- |
| Open Universe | Ctrl + U |
| Toggle Noclip | N |
| Toggle Speedhack | M |
| Toggle Safe Mode | Ctrl + Shift + S |
| Toggle hitboxes | H |
| Toggle player trail | J |
| Toggle movement projection | K |
| Toggle FPS Bypass | F8 |
| Start / stop recording | F6 |
| Play saved replay | F7 |

Keybinds can be changed in Geode's settings.

## Notes and limitations

- Safe Mode is a local safeguard. If it is enabled after Noclip or Speedhack has been used during the current attempt, Universe switches that attempt to Practice Mode. It cannot guarantee how a server or leaderboard treats a run.
- Replay playback switches the active level to Practice Mode. Replay files are local CSV files and are not uploaded.
- The player trail shows recent positions. The optional movement projection extends the latest direction in a straight line; it does not simulate gravity, portals, or collisions.
- The icon preview setting is currently a preference only; a full icon preview is not available yet.
- AI Level Maker, predictive trajectory physics, video export, and a movable touch button are not part of the current build.
- The code compiling for a platform does not replace in-game testing. Windows and Android behavior still needs to be checked on real devices.

## Build targets

The GitHub Actions workflow builds Windows x64, Android ARM64, and Android ARMv7, then combines the platform builds into one .geode package.

## Project status

Universe is under active development. The changelog records source changes; check the Actions page for the result of the latest build.
