# Universe Level Maker

`scripts/level_maker.py` creates and validates Geometry Dash level-design blueprints. It supports deterministic offline drafts, optional online AI generation, importing existing JSON drafts, and exporting JSON, CSV, or Markdown.

## Create a draft offline

```bash
python scripts/level_maker.py --offline --prompt "An underwater level with a calm opening" --difficulty normal --length 48 --style mixed --seed 42 --output Ocean.json
```

Offline mode uses a deterministic template generator. The same prompt, settings, and seed produce the same draft.

## Optional online AI generation

Online mode supports Anthropic Messages and OpenAI-compatible Chat Completions APIs. The default provider is Anthropic. The key is read from an environment variable and is never saved in the repository or output file.

- Anthropic: `ANTHROPIC_API_KEY`
- OpenAI-compatible: `UNIVERSE_AI_API_KEY` by default, or choose another name with `--api-key-env`

Use only an API account you are authorized to use. API calls may incur charges; involve a parent or guardian before using a paid account. Never paste API keys into chat, source files, screenshots, or commits. If a key is exposed, revoke it.

Custom API base URLs must use HTTPS so credentials are not sent over an unencrypted connection.

## Import, validate, and export

Load an existing JSON blueprint and export it to CSV:

```bash
python scripts/level_maker.py --input Ocean.json --format csv --output Ocean.csv
```

Export a readable Markdown design sheet:

```bash
python scripts/level_maker.py --input Ocean.json --format markdown --output Ocean.md
```

Validate and normalize an existing blueprint while keeping JSON:

```bash
python scripts/level_maker.py --input Ocean.json --format json --output Ocean-validated.json
```

The validator rejects unsupported object/gameplay types, invalid or non-finite coordinates, out-of-order events, empty sections, oversized notes, and drafts beyond the configured size limits. CSV output contains one row per object event.

## Blueprint format and limits

The format is `universe-level-draft-v1`. Each section has a name, gameplay mode, and event list. Events have x/y design coordinates, an object type, and an optional note. A draft is limited to 100 sections and 5,000 events.

## In-game AI and offline draft tab

The Universe panel includes an optional AI tab. Save, reload, or clear a prompt. **GENERATE ONLINE** sends it to the configured Anthropic-compatible Messages API and saves a JSON blueprint as `universe-ai-draft.json`. **OFFLINE DRAFT** creates a local procedural JSON blueprint as `universe-offline-draft.json`.

The in-game feature reads `ANTHROPIC_API_KEY` from the Geometry Dash process environment. Its configured API base URL must use HTTPS. The key is not stored in mod settings or written to the repository.

## Important limitation

These files are design blueprints, not native Geometry Dash `.gmd` save files. The tool does not claim to create a playable level or automatically import objects into the Geometry Dash editor. CSV/Markdown exports are planning and editing aids.
