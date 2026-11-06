# Universe AI Level Editor

Snapshot 26.5 keeps the AI workflow inside the currently open Geometry Dash editor. It does not require GDShare or a generated .gmd file.

## In-game workflow

1. Open the target level in the Geometry Dash editor.
2. Select the existing objects you want the AI to change, if any.
3. Open Universe → AI and describe the task.
4. Choose GENERATE ONLINE (configured model) or OFFLINE DRAFT (procedural template).
5. Choose APPLY DRAFT TO OPEN EDITOR.
6. Use the editor Undo control to revert the inserted or changed objects.

Before online generation, Universe includes a compact live context summary: total object count and the selected objects' IDs, positions, and rotations. Each new request reads the current editor state, so follow-up instructions can be based on the latest selection.

## Supported editor operations

- Add: insert supported blocks, spikes, saws, orbs, and pads at generated coordinates.
- Move selected: move the current selection by an AI-proposed offset.
- Rotate selected: rotate the current selection by an AI-proposed amount.
- Delete selected: remove the current selection.

Move, rotate, and delete actions are restricted to the current selection. Select the intended objects first. The AI does not identify or modify arbitrary unselected objects by guessing. The editor Undo control is the recovery path.

The apply routine limits one operation batch to 1,000 supported objects and bounds coordinates and offsets before conversion to editor data.

## Online AI configuration

The in-game client reads ANTHROPIC_API_KEY from the Geometry Dash process environment. Configure the model and HTTPS API base URL in Geode settings. Do not paste API keys into chat, source files, commits, or screenshots. API calls may incur charges; involve a parent or guardian before using a paid account.

## Offline draft

OFFLINE DRAFT creates a procedural template without a network request or neural model. It can be applied to the open editor, but it is not a neural AI model.

## Standalone design helper

scripts/level_maker.py remains available for JSON blueprint validation and CSV/Markdown exports. Those exports are optional planning tools; they are not required for the in-game editor workflow.
