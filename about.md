# Universe

Universe is a Geode mod for Geometry Dash 2.2091, developed by MxrGH.

The current source includes Practice Mode-only gameplay modifiers, Safe Mode, StartPos tools, object hitboxes, a movement trail and projection, cube preview/equip selection without unlock spoofing, a draggable touch button, a movable panel, animated navigation, customizable labels, live theme switching, persistent panel placement, quick FPS presets, and a local input recorder/replay system.

The AI page supports saved prompts, optional online generation, and an in-game offline procedural draft generator. Offline generation works without a network request or API key, including on Android, but it is not neural AI. A true offline neural model still needs a bundled model and an on-device inference runtime.

The repository also includes:
- A standalone level-design blueprint generator with offline templates and optional online AI assistance.
- An FFmpeg helper for converting existing gameplay recordings to MP4.

The level generator creates JSON design blueprints, not native Geometry Dash level files. Video conversion requires FFmpeg and a recording captured separately. Selecting a cube ID does not unlock it or grant achievements.

Noclip and Speedhack effects are restricted to Practice Mode. With Safe Mode enabled, requesting either modifier in a Normal Mode attempt switches that attempt to Practice before the modifier is applied. Safe Mode is a local safeguard, not a server-side guarantee.

Windows and Android builds still need testing inside Geometry Dash on actual devices.
