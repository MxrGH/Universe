# Universe AI Level Editor

Snapshot 26.5 keeps AI editing inside the currently open Geometry Dash editor. It does not require GDShare or a generated .gmd file.

## In-game workflow

1. Open the target level in the Geometry Dash editor.
2. Select objects if you want to use selection-based editing.
3. Open Universe → AI and describe the task.
4. Choose GENERATE ONLINE or OFFLINE DRAFT.
5. Choose APPLY DRAFT TO OPEN EDITOR.
6. Use the editor Undo control to revert inserted or changed objects.

Before online generation, Universe sends a bounded live summary: total object count, up to 120 existing objects with unique IDs, object IDs, positions and rotations, plus up to 30 selected-object details. A new request reads the current editor state, supporting follow-up instructions.

## Supported editor operations

- Add: insert supported blocks, spikes, saws, orbs, and pads at generated coordinates.
- Move selected / rotate selected / delete selected: operate on the current selection.
- Move object / rotate object / delete object: operate on one existing object using its exact unique ID from the live editor summary.

Object-specific actions are accepted only when the target unique ID still exists in the open level at apply time. The AI is instructed not to invent IDs. Transforms are serialized and pasted through the editor integration; deletions use the editor's object-removal path. Use the editor Undo control to revert changes.

The apply routine limits a batch to 1,000 supported operations, caps selected-object processing, and bounds coordinates and offsets before converting them to editor data.

## Online AI configuration

The in-game client reads ANTHROPIC_API_KEY from the Geometry Dash process environment. Configure the model and HTTPS API base URL in Geode settings. Do not paste API keys into chat, source files, commits, or screenshots. API calls may incur charges; involve a parent or guardian before using a paid account.

## Offline draft

OFFLINE DRAFT creates a procedural template without a network request or neural model. It can be applied to the open editor, but it is not a neural AI model.

## Standalone design helper

scripts/level_maker.py remains available for JSON blueprint validation and CSV/Markdown exports. Those exports are optional planning tools; they are not required for the in-game editor workflow.
