## Local gameplay and customization

Universe includes local Noclip and Speedhack modifiers, Safe Mode practice marking, object hitboxes, player-bounds overlay, movement visualizers, FPS controls, Practice Mode input recording/replay, StartPos tools, and a draggable in-game panel. The Player tab includes an icon lab and a local-only icon override; it does not alter account unlock flags or saved icon selections. The interface uses animated page transitions, theme presets, selected-tab highlighting, and a compact local-client status header.

# Universe 1

**Release:** Universe 1 — Final

**Final Release** · Geometry Dash **2.2091** · Geode · by **MxrGH**

Universe is a Geometry Dash hack client with gameplay modifiers, editor tools, visual overlays, replay features, and an animated in-game interface.

- **Release:** Universe 1 — Final
- **Geode package version:** `v1.1.2`
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

## In-game diagnostics

Open Universe, go to **Settings**, and choose **RUN IN-GAME SELF TEST**. It checks that the mod instance, game scene, GameManager, and core settings can be accessed, and reports whether a gameplay or editor context is currently open. This is a runtime smoke test only; it does not simulate every feature or replace manual gameplay testing on Windows and Android.

## Icon tools

- Preview Cube, Ship, Ball, UFO, Wave, Robot, Spider, Swing, and Jetpack IDs.
- Apply a local-only icon override with configurable primary, secondary, and glow colors plus a local glow toggle in Geode settings.
- Equip an icon only when Geometry Dash reports it as unlocked on the current account.
- Local previews do not grant account unlocks or achievements.

## Features not included in this snapshot

The Uni-1 AI panel is present in the client, but generation is **not operational yet**. Its daily quota rollover uses UTC to match the server. The client needs an HTTPS server URL in Geode settings, a trained LoRA adapter, and a deployed inference server. No trained adapter or live server is included. The checked-in synthetic dataset is only a bootstrap fixture and does not establish playable level quality. Do not treat the panel as a working AI feature until the model is trained, deployed, and live-tested.

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

## Dependencies

Universe declares no additional Geode mod dependencies. The numeric Geometry Dash object IDs used by editor tools are bundled in `resources/object_ids.txt`; they are ordinary in-game object identifiers, not a required Node.js package or Geode Node IDs mod. Geode itself is still required to load any Geode mod.

## Install

1. Install a Geode build compatible with Geometry Dash 2.2091.
2. Obtain a `.geode` package from a successful GitHub Actions build artifact or a published GitHub release.
3. Open the `.geode` file with Geode, or place it in the Geode mods folder.
4. Enable Universe in Geode and restart the game if requested.

The release package contains Windows x64, Android ARM64, and Android ARMv7 binaries.

## Build and verification

GitHub Actions is configured to build all three targets independently, run project checks, combine the platform outputs, and inspect the resulting archive. A release is only published from `main` after the release gate is deliberately triggered. A successful CI build still does not replace gameplay testing on actual Windows and Android devices.

A successful CI build confirms compilation and packaging. It does not replace gameplay testing on actual Windows and Android devices. No claim of zero bugs is made.

## Project structure

| Path | Purpose |
|---|---|
| `src/main.cpp` | In-game UI, gameplay modifiers, visual helpers, and replay code |
| `mod.json` | Geode metadata, settings, and keybinds |
| `logo.png` | Universe logo |
| `scripts/level_maker.py` | Online level-design blueprint helper |
| `scripts/export_video.py` | FFmpeg conversion helper |
| `.github/workflows/build.yml` | Multi-platform build, package verification, and release |
| `LICENSE` | MIT License |

## Important limitations

- Uni-1's panel and client integration are experimental. The `uni1/` directory contains training and server code, but no trained weights or deployed inference service. Generation cannot work until those external prerequisites exist.
- Noclip and Speedhack can run in Normal Mode when Safe Mode is OFF. Modified attempts are not represented as legitimate server-verified completions.
- Replay recording and playback are Practice Mode-only.
- Movement projection estimates recent motion; it is not full Geometry Dash physics.
- Icon previews do not unlock content or award achievements.
- CI cannot prove bug-free gameplay. Report reproducible issues with device, OS, Geode, and game versions.

## Credits

- **MxrGH** — Universe
- **Geode SDK** — mod framework
- **RobTop Games** — Geometry Dash

Uni-1 development materials are not part of the shipped feature set. Generated examples are synthetic and do not certify gameplay quality.
.
.
.
:
.
.
.
.

Happy 400th Commit! I have been putting so much effort in to this project for all geode players! I Just want this project to be not forked but you can fork it thats just fine. Just thank you all for supporting (or not aupporting) this project :)
