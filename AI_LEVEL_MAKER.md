# Universe AI Level Editor

Universe's in-game AI tab is intended to work with the level currently open in Geometry Dash's editor. It does not require a generated `.gmd` file or GDShare.

## In-game flow

1. Open Universe and go to the AI tab.
2. Enter a level concept and select **GENERATE ONLINE**.
3. Open the target level in the Geometry Dash editor.
4. Select **APPLY DRAFT TO OPEN EDITOR**.
5. Universe converts supported events into Geometry Dash editor objects and pastes them into the current level. Use the editor Undo control to revert.

The current implementation supports block, spike, saw, orb, and pad events, with a hard limit of 1,000 inserted objects per apply operation. It adds objects to the current level; it does not yet interpret arbitrary natural-language instructions to move, recolor, or delete existing objects.

## Online AI configuration

The in-game client reads `ANTHROPIC_API_KEY` from the Geometry Dash process environment. Configure the model and HTTPS API base URL in Geode settings. Do not paste API keys into chat, source files, commits, or screenshots. API calls may incur charges; involve a parent or guardian before using a paid account.

## Offline draft

**OFFLINE DRAFT** creates a procedural template without a network request or neural model. The same Apply action can place its supported objects into the open editor.

## Standalone design helper

`scripts/level_maker.py` remains available for JSON blueprint validation and CSV/Markdown exports. Those exports are optional planning tools; they are not required for the in-game editor workflow.
