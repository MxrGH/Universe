# Universe 1

**Snapshot 26.1** · Geometry Dash **2.2091** · Geode · by **MxrGH**

Universe 1 is a feature-rich Geometry Dash toolkit with a custom animated interface, practice utilities, visual overlays, local replay tools, and an optional AI level-design assistant.

- **Release:** Universe 1 — Snapshot 26.1
- **Geode package version:** `v1.0.0` (standard SemVer required by Geode)
- **Mod ID:** `mxrmc.universe`
- **Supported game version:** Geometry Dash `2.2091`
- **Targets:** Windows x64, Android ARM64, Android ARMv7
- **Package:** one combined, universal `.geode` containing all three platform binaries

## Features

### Practice and gameplay tools
- Practice Mode-only Noclip and configurable Speedhack.
- Safe Mode moves an attempt to Practice before applying a requested gameplay modifier.
- StartPos navigation, nearest-ahead StartPos selection, and quick attempt restart.
- One-tap Practice, Visuals, and Clean Run presets.
- Local input recording and replay for Practice Mode.

### Visual tools
- Live object hitbox overlay.
- Recent movement trail and short movement projection.
- Render FPS interval override with 120/240/360 quick presets.
- Current player mode, position, and speed information.

### Interface and customization
- Animated panel entrance, page transitions, and button feedback.
- Draggable panel and floating touch button with remembered positions.
- Custom panel title and floating-button label.
- Live theme changes and position reset.
- Touch-friendly Android controls and Windows mouse/keyboard support.

### Icon tools
- Preview cube IDs and select an icon to equip through the game's own player settings.
- Does not grant locked icons, achievements, or account unlock flags.

### AI Level Maker
**The AI assistant is included in Universe 1. It has not been removed.**

- **Online AI:** send a prompt to the configured Anthropic-compatible Messages API and save the generated level-design outline.
- **Offline Draft:** generate a deterministic local JSON blueprint without internet, an API key, or a model.
- **Prompt tools:** paste, edit, save, reload, and clear prompts.
- **Configurable model and API base URL** through Geode settings.

Online AI reads `ANTHROPIC_API_KEY` from the Geometry Dash process environment. The key is not stored in mod settings or source. Requests can incur provider charges; use a paid API account only with a parent or guardian's permission. Never put keys in chat, source files, commits, or screenshots.

Offline Draft is procedural generation, not neural inference. Both online and offline outputs are design blueprints, **not native/importable Geometry Dash levels**.

### Extra tools
- Standalone Python level-design blueprint generator.
- FFmpeg helper to convert an existing gameplay recording to MP4; it does not capture gameplay.

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

Keybinds can be changed in Geode settings. On Android, use the floating **UNIV** button.

## Install

1. Install the matching Geode version for Geometry Dash 2.2091.
2. Download `Universe-1-Snapshot-26.1.geode` from the GitHub release.
3. Open the `.geode` file with Geode, or place it in the Geode mods folder.
4. Enable Universe in Geode and restart the game if requested.

The release package contains Windows x64, Android ARM64, and Android ARMv7 binaries.

## Build and release verification

GitHub Actions builds all three targets independently, combines them, inspects the resulting `.geode` archive, verifies all three binaries and required resources, and only then creates the Snapshot 26.1 prerelease.

A successful CI build confirms packaging. It does not replace gameplay testing on actual Windows and Android devices. Replay timing and performance can vary by device and game build.

## Project structure

| Path | Purpose |
|---|---|
| `src/main.cpp` | In-game interface, practice tools, visual helpers, AI interface, and replay code |
| `mod.json` | Geode metadata, settings, and keybinds |
| `logo.png` | Universe 1 release logo |
| `scripts/level_maker.py` | Offline and AI-assisted blueprint generator |
| `scripts/export_video.py` | FFmpeg conversion helper |
| `.github/workflows/build.yml` | Multi-platform build, package verification, and release |
| `LICENSE` | MIT License |

## Important limitations

- Safe Mode is a local safeguard, not a server-side leaderboard guarantee.
- Noclip and Speedhack are restricted to Practice Mode.
- Movement projection is an estimate, not full Geometry Dash physics.
- Replay playback may vary with timing, level state, game version, and device.
- AI outputs are design blueprints, not native `.gmd` files.
- The icon selector cannot unlock content or award achievements.
- CI cannot prove bug-free gameplay; report reproducible issues with device, OS, Geode, and game versions.

## Credits

- **MxrGH** — Universe 1
- **Geode SDK** — mod framework
- **RobTop Games** — Geometry Dash
