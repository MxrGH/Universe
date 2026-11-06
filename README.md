# Universe

**Universe** is a Geode mod for **Geometry Dash 2.2091** by **MxrMC**. It combines local practice controls, visual debugging, input recording, a movable in-game panel, and an AI-assisted level concept tool.

> Universe is a work in progress. The feature list below distinguishes implemented tools from limitations. A successful CI build is not a substitute for testing inside Geometry Dash on every target device.

## Contents

- [Highlights](#highlights)
- [Feature guide](#feature-guide)
- [Controls](#controls)
- [AI level assistant](#ai-level-assistant)
- [Build and install](#build-and-install)
- [Project layout](#project-layout)
- [Known limitations](#known-limitations)
- [Security](#security)
- [Contributing](#contributing)

## Highlights

- In-game panel with Player, Practice, Level, Visuals, Recorder, Keybinds, Settings, and AI pages.
- Local Noclip and speed multiplier controls, with a Safe Mode safeguard.
- StartPos navigation, including a nearest-ahead Smart StartPos action.
- Hitbox overlay, recent movement trail, and a short movement projection.
- Practice-mode input recording and local replay playback.
- Draggable floating button and draggable panel.
- Configurable panel appearance and click sound.
- AI-generated level design blueprints, saved locally.

## Feature guide

| Area | What it does | Important detail |
| --- | --- | --- |
| Player | Shows current player state and a cube preview selector | The cube selector can equip an icon only when it is already unlocked on the account |
| Practice | Noclip and local speed multiplier | These modify local gameplay; use Practice Mode |
| Safe Mode | Marks a modified attempt as Practice after Noclip or Speedhack is used | Local safeguard only; it cannot promise how any server or leaderboard treats a run |
| Level | Previous/next StartPos and Smart StartPos | Switching StartPos resets the current attempt |
| Visuals | Object hitboxes, recent path trail, movement projection | Projection is an estimate, not a full physics simulation |
| Recorder | Record and replay input events | Local files; recording starts in Practice Mode |
| Display | Movable panel, floating touch button, panel colors, click sound | Button position is saved in Geode settings |
| FPS | Changes the render animation interval | Does not increase the game's physics tick rate |
| AI | Sends a level concept to an Anthropic-compatible Messages API | Produces a text/JSON design blueprint, not a playable level file |

## Controls

Default bindings (editable in Geode's mod settings):

| Action | Default |
| --- | --- |
| Open / close Universe | `Ctrl + U` |
| Toggle Noclip | `N` |
| Toggle Speedhack | `M` |
| Toggle Safe Mode | `Ctrl + Shift + S` |
| Toggle hitboxes | `H` |
| Toggle movement trail | `J` |
| Toggle movement projection | `K` |
| Toggle FPS Bypass | `F8` |
| Start / stop recording | `F6` |
| Play saved replay | `F7` |

On touch devices, use the floating **UNIV** button if enabled. Drag it to reposition it. The main panel can be moved by dragging its title bar.

## AI level assistant

The AI page includes a prompt field, model field, and API base URL field. It sends requests to an Anthropic-compatible **Messages API** and writes the returned draft to the mod save directory as `universe-ai-draft.txt`.

### Configuration

1. Create an API key through an account you are authorized to use. If billing is enabled, ask a parent or guardian to help review the costs first.
2. Set the `ANTHROPIC_API_KEY` environment variable on the computer running Geometry Dash.
3. Restart Geometry Dash so the game process receives the environment variable.
4. Open Universe → AI and enter your level concept.
5. Set the model and API base URL if your provider requires different values, then choose **GENERATE DRAFT**.

Default values:

- Model: `claude-sonnet-4-6`
- Base URL: `https://api.anthropic.com`
- Endpoint path: `/v1/messages`
- Key variable: `ANTHROPIC_API_KEY`

The API key is read from the process environment. Universe does not provide a key textbox and does not save the key in Geode settings. The model and base URL are ordinary configuration values, not secrets.

The response is a design document. It is **not** a native Geometry Dash level and cannot be imported directly as a playable level. Review generated gameplay spacing and build the level in the editor.

### Local offline generator

The Python helper can create a deterministic draft without contacting an AI service:

```bash
python scripts/level_maker.py --offline --prompt "Neon city with fast ship sections" --difficulty insane --length 80 --style ship --output draft.json
```

For AI generation through the script, set `ANTHROPIC_API_KEY` in the local environment and run:

```bash
python scripts/level_maker.py --prompt "A cave that transforms into a bright city" --output draft.json
```

The script also supports an OpenAI-compatible Chat Completions provider with `--provider openai-compatible`, plus `--base-url`, `--model`, and `--api-key-env` options. Use `--offline` to avoid network requests.

## Video export helper

`scripts/export_video.py` converts an existing gameplay recording to MP4 using FFmpeg:

```bash
python scripts/export_video.py recording.mkv --output Universe-Gameplay.mp4 --fps 60 --crf 20
```

FFmpeg must be installed and available on PATH. This script converts a recording; it does not capture gameplay.

## Build and install

Universe uses the Geode SDK and CMake-based Geode build tooling.

1. Install the Geode SDK/toolchain for your platform.
2. Clone this repository.
3. Configure and build with the Geode SDK workflow for your platform.
4. Install the generated `.geode` package through Geode.
5. Test on a copy of your Geometry Dash data before using a development build.

The GitHub Actions workflow targets Windows x64, Android ARM64, and Android ARMv7. Check the Actions page for the status of the latest build; do not assume a commit has a downloadable package until the workflow has completed successfully.

## Project layout

| Path | Purpose |
| --- | --- |
| `src/main.cpp` | Geode mod UI, gameplay hooks, recorder, and AI request |
| `mod.json` | Mod metadata, keybinds, and settings |
| `scripts/level_maker.py` | Offline and AI-assisted level blueprint generator |
| `scripts/export_video.py` | FFmpeg-based recording conversion helper |
| `.github/workflows/build.yml` | CI build and validation workflow |
| `AI_LEVEL_MAKER.md` | Additional level generator instructions |

## Known limitations

- The cube selector equips owned/unlocked cube icons locally. Locked icons remain unavailable; Universe does not unlock them or spoof achievements.
- Universe does not generate native `.gmd` / import-ready level files.
- AI requests require a working network connection and a compatible API account.
- AI provider configuration in the in-game page currently targets the Anthropic Messages API format. It is not a universal adapter for every provider.
- Safe Mode is local and cannot guarantee leaderboard or server-side treatment.
- Movement projection does not simulate portals, gravity changes, collisions, or upcoming triggers.
- FPS Bypass changes render timing only; performance depends on the device and game.
- Replays are local input-event recordings. Playback can differ if the game version, level, physics, or device timing changes.
- Windows and Android builds need real-device testing after CI succeeds.

## Security

- Never paste API keys into chat, issue reports, screenshots, source files, or commits.
- Keep API keys in environment variables and never commit them.
- If a key is exposed, revoke it with the provider and create a replacement. Deleting a repository commit does not invalidate a leaked key.
- Review API billing and usage limits before making requests.
- Do not publish a repository until its current files, commit history, branches, workflow logs, and releases have been checked for secrets.

## Contributing

Bug reports and focused pull requests are welcome. Include:

- Geometry Dash version and platform.
- Geode version.
- Steps to reproduce the issue.
- Relevant logs with tokens, account details, and personal information removed.
- Whether the issue occurs with other mods disabled.

Please keep pull requests focused and describe what was actually tested.

## Credits

- **MxrMC** — Universe author
- **Geode SDK** — mod loading and development framework
- **RobTop Games** — Geometry Dash
