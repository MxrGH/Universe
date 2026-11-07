## Local gameplay and customization

Universe includes local Noclip and Speedhack modifiers, Safe Mode practice marking, object hitboxes, player-bounds overlay, movement visualizers, FPS controls, Practice Mode input recording/replay, StartPos tools, and a draggable in-game panel. The Player tab includes an icon lab and a local-only icon override; it does not alter account unlock flags or saved icon selections. The interface uses animated page transitions, theme presets, selected-tab highlighting, and a compact local-client status header.

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

## In-game diagnostics

Open Universe, go to **Settings**, and choose **RUN IN-GAME SELF TEST**. It checks that the mod instance, game scene, GameManager, and core settings can be accessed, and reports whether a gameplay or editor context is currently open. This is a runtime smoke test only; it does not simulate every feature or replace manual gameplay testing on Windows and Android.

## Icon tools

- Preview Cube, Ship, Ball, UFO, Wave, Robot, Spider, Swing, and Jetpack IDs.
- Apply a local-only icon override with configurable primary, secondary, and glow colors plus a local glow toggle in Geode settings.
- Equip an icon only when Geometry Dash reports it as unlocked on the current account.
- Local previews do not grant account unlocks or achievements.

## AI full-level creator

The AI tab generates a new level layout and places supported objects directly into the open editor.

1. Open a new, empty level in the Geometry Dash editor.
2. Enter the complete concept in the Universe AI tab.
3. Enter an API key in the AI tab for this request. On desktop, `ANTHROPIC_API_KEY` from the process environment is also accepted.
4. Choose **GENERATE ONLINE**, wait for the result message, then return to the editor.
5. Choose **CREATE FULL LEVEL IN EDITOR** and review the result. Use the editor Undo control to revert it.

The in-game API key field is designed for Android as well as desktop. The key is used for the current request only and is not saved by Universe. It is visible while typing, so avoid screen recording. Configure the model and HTTPS API base URL in Geode → Universe settings. The current in-game request uses the Anthropic Messages API format. API calls may incur charges; use a paid account only with a parent or guardian's permission.

The current in-editor generator supports blocks, spikes, saws, orbs, pads, and decoration objects. It does not yet create gameplay portals, triggers, or song timing. The saved JSON draft is limited to 2 MB. A preflight ceiling of 1,000,000 placements is a validation guard, **not** the practical apply capacity: the current apply loop processes at most 1,000 operations per pass. Chunked placement, progress reporting, and cancellation are not implemented. The parser currently extracts events with regular expressions rather than a complete JSON schema parser. Start with an empty level; the creator adds objects and does not clear existing contents.

The standalone `scripts/level_maker.py` helper validates and exports planning blueprints as JSON, CSV, or Markdown. Those files are not native Geometry Dash level saves and are not required for the in-game editor workflow. Its current blueprint limit is 5,000 events.

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
| `scripts/level_maker.py` | Online level-design blueprint helper |
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
