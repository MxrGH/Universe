# Universe

Universe is a Geometry Dash 2.2091 Geode mod by **MxrGH**. It puts practice controls, visual helpers, replay tools, and a few quality-of-life shortcuts in one in-game panel.

- **Geode package version:** `v1.0.0`
- **Display/build label:** `1.00.0.00`
- **Game version:** Geometry Dash `2.2091`

The four-part display label is for the project and release notes. The actual `mod.json` version stays on standard three-part semantic versioning because Geode expects a semantic version.

## What you can do

### Practice
- Toggle Noclip and a local speed multiplier.
- Use Safe Mode to switch a modified attempt to Practice Mode.
- Pick the next StartPos in front of you, or move to the previous/next StartPos.
- Restart the current attempt from the panel.

### Visuals
- Draw object hitboxes.
- Follow your recent movement with a short trail.
- Preview a short movement projection based on recent movement.
- Change the render frame interval with FPS Bypass. This does not change the game's physics tick.

### Recording
- Record input events during a Practice Mode session.
- Play back a saved local recording.
- Stop or clear a recording from the panel.
- Convert an existing screen recording to MP4 with the optional FFmpeg helper in `scripts/export_video.py`.

### Player
- Preview cube IDs and set the selected cube as your equipped cube.
- This does not unlock icons, grant achievements, or change your account's unlock flags. Whether a selected ID appears on a synced profile depends on Geometry Dash's own save and server behavior.

### Quick setups
The Settings page includes three one-tap setups:
- **Practice** turns on Safe Mode and leaves Noclip and Speedhack off.
- **Visuals** turns on the hitbox, trail, and movement projection helpers.
- **Clean Run** turns off practice modifications and visual overlays.

### Optional level idea helper
The AI page is optional. It can send a level idea to an Anthropic Messages API-compatible endpoint and save the returned design outline as `universe-ai-draft.txt`. It does not create a playable level or import a level into Geometry Dash.

If you do not want to use an online AI service, use the offline generator instead:

```bash
python scripts/level_maker.py --offline --prompt "Neon city with fast ship sections" --difficulty insane --length 80 --style ship --output draft.json
```

Online requests require an API key in the `ANTHROPIC_API_KEY` environment variable. The key is read from the game's process environment and is not stored in Geode settings. Check API pricing with a parent or guardian before enabling paid usage. Never put a key in source code, commits, screenshots, or chat.

## Controls

| Action | Default |
| --- | --- |
| Open or close Universe | `Ctrl + U` |
| Toggle Noclip | `N` |
| Toggle Speedhack | `M` |
| Toggle Safe Mode | `Ctrl + Shift + S` |
| Toggle hitboxes | `H` |
| Toggle movement trail | `J` |
| Toggle movement projection | `K` |
| Toggle FPS Bypass | `F8` |
| Start or stop recording | `F6` |
| Play saved replay | `F7` |

You can change keybinds in Geode's mod settings. On touch devices, open the panel with the floating **UNIV** button. Drag it to move it. The panel can also be moved by its title area.

## Build and install

Universe uses the Geode SDK and the Geode CMake build workflow. The GitHub Actions workflow builds these targets:

- Windows x64
- Android ARM64
- Android ARMv7

A successful run uploads a combined `.geode` package as a workflow artifact. Check the run's artifacts section for the download. A CI build confirms that the package was produced; it does not replace testing in Geometry Dash on each device.

## Project files

| Path | Purpose |
| --- | --- |
| `src/main.cpp` | In-game panel, practice tools, visual helpers, and replay code |
| `mod.json` | Geode metadata, settings, and keybinds |
| `scripts/level_maker.py` | Offline and optional online level-design outline generator |
| `scripts/export_video.py` | FFmpeg conversion helper |
| `.github/workflows/build.yml` | Cross-platform build and package checks |
| `CHANGELOG.md` | Release history |

## Important limits

- Practice modifications change local gameplay. Use them in Practice Mode.
- Safe Mode is a local safeguard. It cannot guarantee how an online service or leaderboard treats a run.
- The movement projection is an estimate. It does not simulate portals, gravity changes, collisions, or future triggers.
- Replay playback can vary with game version, level, physics, and device timing.
- The AI helper returns a design outline, not a native `.gmd` file.
- The cube selector does not award unlocks or achievements.
- Windows and Android builds still need real-device testing.

## Security

Keep API keys private. If a key is exposed, revoke it with its provider; removing a file or commit does not invalidate the key. Before making the repository public, review the current files, all branches and tags, commit history, releases, and CI logs for secrets.

## Credits

- **MxrGH** — project author
- **Geode SDK** — mod framework
- **RobTop Games** — Geometry Dash
