# Universe

Universe is a Geode mod for Geometry Dash 2.2091, developed by MxrMC.

The current source includes local practice controls, StartPos tools, object hitboxes, a short movement projection, an icon preview, a draggable touch button, and a local input recorder.

The repository also includes two standalone tools:
- A level-design draft generator with offline templates and an optional OpenAI-compatible API.
- An FFmpeg helper for converting existing gameplay recordings to MP4.

The level generator currently produces JSON design blueprints rather than native Geometry Dash level files. Video conversion requires FFmpeg and a recording captured separately. The icon preview does not unlock or equip icons.

Safe Mode only changes local game state. It cannot enforce server-side leaderboard rules. Features still need in-game testing on Windows and Android.
