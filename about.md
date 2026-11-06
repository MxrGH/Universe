# Universe

Universe is a Geode mod for Geometry Dash 2.2091, developed by MxrGH.

The current source includes local practice controls, StartPos tools, object hitboxes, a short movement projection, an icon preview, a draggable touch button, a movable panel, animated navigation, quick FPS presets, prompt save/clear controls, and a local input recorder.

The repository also includes two standalone tools:
- A level-design draft generator with offline templates and an optional OpenAI-compatible API.
- An FFmpeg helper for converting existing gameplay recordings to MP4.

The level generator currently produces JSON design blueprints rather than native Geometry Dash level files. The standalone offline option is deterministic templating, not neural AI; in-game offline model inference on Android is not implemented yet. Video conversion requires FFmpeg and a recording captured separately. The icon preview does not unlock or equip icons.

Safe Mode only changes local game state. It cannot enforce server-side leaderboard rules. Features still need in-game testing on Windows and Android.
