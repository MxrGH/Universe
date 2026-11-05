# Universe

Universe is a Geode mod for Geometry Dash 2.2091. It adds an in-game panel for local practice, visual tools, replay recording, and level-design helpers.

## In-game features

- **Practice:** Noclip, a local gameplay speed multiplier, Safe Mode, and StartPos switching.
- **Visuals:** object hitboxes, a recent movement trail, and a short trajectory estimate based on recent velocity and acceleration.
- **Player:** a cube icon preview with a local preview ID selector. It does not unlock or equip icons.
- **Recorder:** local input recording and replay in Practice Mode.
- **Touch controls:** a floating Universe button that can be dragged to a new position. Its position is saved in Geode settings.
- **Display:** panel color presets and a configurable panel color.
- **Frame timing:** an optional render-interval setting. It does not change the game's physics rate.

## Keybinds

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

Bindings can be changed in Geode's settings.

## Level draft generator

The scripts/level_maker.py tool creates a structured level-design draft.

For a deterministic local draft:

~~~bash
python scripts/level_maker.py --offline --prompt "Neon city with fast ship sections" --difficulty insane --length 80 --style ship --output draft.json
~~~

For AI-assisted generation, set UNIVERSE_AI_API_KEY in your local environment and run:

~~~bash
python scripts/level_maker.py --prompt "A cave level that changes into a bright city" --difficulty hard --length 60 --output draft.json
~~~

The script uses an OpenAI-compatible Chat Completions endpoint. You can change --base-url and --model for another compatible provider. API keys are read from the environment and are not stored in the repository or mod.

The output is a JSON design blueprint. It is not currently converted into a native, import-ready Geometry Dash level file.

## Video export

scripts/export_video.py converts a gameplay screen recording to MP4 using an installed FFmpeg executable:

~~~bash
python scripts/export_video.py recording.mkv --output Universe-Gameplay.mp4 --fps 60 --crf 20
~~~

FFmpeg must be installed and available on PATH. This helper converts an existing screen recording; it does not capture gameplay from inside Geometry Dash.

## Notes and limitations

- Safe Mode is a local safeguard. It cannot guarantee how a server or leaderboard treats a run.
- Replay playback switches the active level to Practice Mode. Replay files stay on the device.
- The movement projection extrapolates recent motion for a short time. It does not simulate portals, collisions, or future gameplay events.
- The icon preview is local only. It does not unlock icons or change the equipped player icon.
- The level generator creates a design blueprint, not a native Geometry Dash level.
- Video export converts a screen recording; capture must be done separately.
- The code compiling for a platform does not replace in-game testing. Windows and Android behavior still needs to be checked on real devices.

## Build targets

The GitHub Actions workflow builds Windows x64, Android ARM64, and Android ARMv7, then combines the platform builds into one .geode package.
