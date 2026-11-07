# Universe 1

**Snapshot 26.5** · Geometry Dash **2.2091** · Geode · by **MxrGH**

Universe is a Geometry Dash hack client with gameplay modifiers, editor tools, visual overlays, replay features, and an animated in-game interface.

- **Release:** Universe 1 — Snapshot 26.5
- **Geode package version:** `v1.1.1`
- **Mod ID:** `mxrmc.universe`
- **Supported game version:** Geometry Dash `2.2091`
- **Targets:** Windows x64, Android ARM64, Android ARMv7
- **Package:** one combined universal `.geode`

## Gameplay hacks

- **Noclip:** toggle collision bypass while playing.
- **Speedhack:** adjust gameplay speed from 0.25x to 3.00x.
- **Safe Mode:** ON automatically switches an attempt to Practice Mode when Noclip or Speedhack is active. OFF leaves the selected modifiers active in Normal Mode.
- **StartPos Switcher:** previous, next, and nearest-ahead StartPos selection.
- **Replay Bot:** local input recording and playback. Recording/replay remains Practice Mode-only.
- **Quick presets:** Practice, Visuals, and Clean Run.

Safe Mode is a local safeguard. Universe does not claim to guarantee server-side leaderboard treatment or legitimate completion status for modified attempts.

## Visual and interface features

- Object hitbox overlay, movement trail, and movement projection.
- FPS render interval override with 120/240/360 presets.
- Current player mode, position, and speed information.
- Rounded animated panel with page transitions and button feedback.
- Circular draggable UNIV control with the Universe logo and a fading motion trail.
- Saved positions, touch-friendly Android controls, Windows mouse/keyboard support, themes, and global click sound.

## Icon tools

- Preview Cube, Ship, Ball, UFO, Wave, Robot, Spider, Swing, and Jetpack IDs.
- Equip an icon only when Geometry Dash reports it as unlocked on the current account.
- Local previews do not grant account unlocks or achievements.

## AI level editor

The AI tab is designed to modify the level currently open in Geometry Dash's editor—not to produce a downloadable `.gmd` file.

1. Enter a level idea in the Universe AI tab.
2. Choose **GENERATE ONLINE** to ask the configured AI model for a level section.
3. Open the target level in the Geometry Dash editor.
4. Choose **APPLY DRAFT TO OPEN EDITOR**. Universe converts supported block, spike, saw, orb, and pad events into editor objects and pastes them into the current level.
5. Use the editor's Undo control to revert the placement if needed.

The 26.5 editor integration reads the live editor object count, up to 120 existing objects with their unique IDs/object IDs/positions/rotations, and up to 30 selected objects with their unique IDs before generation. It can add supported objects, move or rotate selected objects, delete the selection, and move/rotate/delete a specific existing object by its exact unique ID from the live context. The live context is intentionally bounded; objects outside the first 120 are not targetable by ID in that request. Follow-up prompts receive a fresh editor summary. Transforms are rebuilt through the editor's object serialization/paste path so the editor Undo control can revert them. The AI must use IDs from the supplied context rather than guessing. No GDShare or `.gmd` export is required.

Online AI uses the Anthropic-compatible Messages API over HTTPS and reads `ANTHROPIC_API_KEY` from the Geometry Dash process environment. The key is not stored in mod settings or source. API calls may incur charges; use a paid API account only with a parent or guardian's permission. Never put keys in chat, source files, commits, or screenshots.

**OFFLINE DRAFT** creates a procedural template, not neural inference. It can also be applied to the open editor.

## Default controls

| Action | Key |
|---|---|
| Open/close Universe | `Ctrl + U` |
| Noclip | `N` |
| Speedhack | `M` |
| Safe Mode | `Ctrl + Shift + S` |
| Hitboxes | `H` |
| Movement trail | `J` |
| Movement projection | `K` |
| FPS Bypass | `F8` |
| Start/stop recorder | `F6` |
| Play saved replay | `F7` |

Keybinds can be changed in Geode settings. On Android, use the circular **UNIV** button.

## Install

1. Install a Geode build compatible with Geometry Dash 2.2091.
2. Download `Universe-1-Snapshot-26.5.geode` from the GitHub release.
3. Open the `.geode` file with Geode, or place it in the Geode mods folder.
4. Enable Universe in Geode and restart the game if requested.

The release package contains Windows x64, Android ARM64, and Android ARMv7 binaries.

## Build and verification

GitHub Actions builds all three targets independently, runs the Python regression suite, combines them, inspects the resulting archive, verifies platform binaries and resources, and only then publishes Snapshot 26.5.

A successful CI build confirms compilation and packaging. It does not replace gameplay testing on actual Windows and Android devices. No claim of zero bugs is made.

## Project structure

| Path | Purpose |
|---|---|
| `src/main.cpp` | In-game UI, gameplay modifiers, visual helpers, editor AI apply, and replay code |
| `mod.json` | Geode metadata, settings, and keybinds |
| `logo.png` | Universe logo |
| `scripts/level_maker.py` | Offline/online level-design helper |
| `scripts/export_video.py` | FFmpeg conversion helper |
| `.github/workflows/build.yml` | Multi-platform build, package verification, and release |
| `LICENSE` | MIT License |

## Important limitations

- AI editing can target an exact existing object unique ID shown in its live editor context, or operate on the current selection. It does not guess IDs that are absent from that context.
- Noclip and Speedhack can run in Normal Mode when Safe Mode is OFF. Modified attempts are not represented as legitimate server-verified completions.
- Replay recording and playback are Practice Mode-only.
- Movement projection estimates recent motion; it is not full Geometry Dash physics.
- Icon previews do not unlock content or award achievements.
- CI cannot prove bug-free gameplay. Report reproducible issues with device, OS, Geode, and game versions.

## Credits

- **MxrGH** — Universe
- **Geode SDK** — mod framework
- **RobTop Games** — Geometry Dash
