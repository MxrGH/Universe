## Local gameplay and customization

Universe includes local Noclip and Speedhack modifiers, Safe Mode practice marking, object hitboxes, player-bounds overlay, movement visualizers, FPS controls, Practice Mode input recording/replay, StartPos tools, and a draggable in-game panel. The Player tab includes an icon lab that previews icon IDs locally and only equips icons already unlocked on the account; it does not alter unlock flags or claim server-side unlocks. The interface uses animated page transitions, theme presets, selected-tab highlighting, and a compact local-client status header.

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
- Equip an icon only when Geometry Dash reports it as unlocked on the current account.
- Local previews do not grant account unlocks or achievements.

## AI full-level creator

The AI tab is a **full-level generator**, not just an existing-object editor.

1. Open a **new, empty level** in the Geometry Dash editor.
2. Enter a complete concept in the Universe AI tab. Include the theme, difficulty, gameplay feel, and any section ideas.
3. Choose **GENERATE ONLINE**. The configured AI model is instructed to design one complete level from its opening to its ending, using 6–12 connected sections and normally approximately 150–500 object placements. The absolute per-draft ceiling is **1,000,000 generated object placements**; oversized drafts are rejected before any editor changes.
4. Choose **CREATE FULL LEVEL IN EDITOR**. Universe pastes the generated layout directly into the currently open editor.
5. Review the result and use the editor's Undo control if you want to revert the placement.

The in-editor generator currently places blocks, spikes, saws, orbs, pads, and decoration objects. Important: the one-million figure is a hard validation ceiling, not a claim that the current paste engine can practically place one million objects. The current apply path still has a 1,000-operation processing guard and a 2 MB draft-file limit; large-scale chunked placement, progress reporting, and cancellation are not implemented yet. It does not yet create gameplay portals, triggers, or song timing. It adds the layout to the open editor; it does not automatically erase existing objects, so start with an empty level for a clean full-level build.

The 26.5 live editor context can still inspect up to 500 existing objects and 100 selected objects for future editing workflows. The full-level creation prompt deliberately does not depend on that context or on selected objects.

Online AI uses the Anthropic-compatible Messages API over HTTPS and reads `ANTHROPIC_API_KEY` from the Geometry Dash process environment. The key is not stored in mod settings or source. API calls may incur charges; use a paid API account only with a parent or guardian's permission. Never put keys in chat, source files, commits, or screenshots.

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
