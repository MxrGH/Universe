# Level draft generator

The repository includes scripts/level_maker.py. It can create a deterministic template draft without a network connection, or ask an OpenAI-compatible Chat Completions API to generate a design blueprint.

## Offline mode

~~~bash
python scripts/level_maker.py --offline --prompt "An underwater level with a calm opening" --difficulty normal --length 48 --style mixed --seed 42
~~~

Offline mode uses a repeatable template generator. Changing the seed changes the layout.

## AI-assisted mode

Set an API key in the environment variable UNIVERSE_AI_API_KEY, then run:

~~~bash
python scripts/level_maker.py --prompt "A neon city level with a difficult wave section" --difficulty demon --length 80 --style wave
~~~

The default endpoint is https://api.openai.com/v1 and the default model name is gpt-4o-mini. Both can be changed:

~~~bash
python scripts/level_maker.py --base-url https://your-compatible-endpoint/v1 --model your-model --prompt "..."
~~~

The key is read from the local environment. Do not paste it into source files, commit it, or put it in a public mod release.

## Output format

The output is a JSON blueprint with sections and events. Events contain an x/y design coordinate, a basic object type, and an optional note. The script checks the structure and limits the draft to 5,000 events.

This is not a native Geometry Dash level file and cannot be imported directly into the game. A native level serializer and an in-game editing workflow are still needed.


## In-game AI and offline draft tab

The Universe panel includes an optional AI tab (enabled by default; configurable in Geode > Universe settings). Save, reload, or clear a prompt. **GENERATE ONLINE** sends it to the configured Anthropic-compatible Messages API and saves the returned text to `universe-ai-draft.txt`. **OFFLINE DRAFT** creates a deterministic procedural JSON blueprint locally, without network access or an API key, and saves it as `universe-offline-draft.json`.

The in-game feature reads `ANTHROPIC_API_KEY` from the environment of the Geometry Dash process. Set the variable before launching the game, then restart Geometry Dash so the process can read it. The key is not stored in mod settings or written to the repository. Both outputs are design blueprints, not playable or importable Geometry Dash levels. The offline option is procedural generation, not a neural model; true offline neural inference still needs a model runtime and model file.

The in-game prompt is limited to 4,000 characters. Requests use the `claude-sonnet-4-6` model and can incur API charges. Use an account and billing method only with a parent or guardian's permission.


## Anthropic (default provider)

The generator now uses Anthropic's Messages API by default. It reads the credential from the local `ANTHROPIC_API_KEY` environment variable; it does not save the key in the project, mod settings, generated JSON, or Git. The default model is `claude-sonnet-4-6`.

On Windows PowerShell, set the environment variable for the current terminal session (use a replacement key only after revoking any key that was exposed):

```powershell
$env:ANTHROPIC_API_KEY = Read-Host "Paste your API key"
python scripts/level_maker.py --prompt "Make my level Ribbons much cooler" --output Ribbons-Draft.json
```

For another provider, pass `--provider openai-compatible` and optionally `--base-url`, `--model`, and `--api-key-env`. Use `--offline` to generate a local template without contacting an AI service.

API calls may incur charges according to the provider account. Only use an account and billing method you are authorized to use, with a parent or guardian's help where required. Never paste keys into chat, source code, screenshots, or commits. If a key is exposed, revoke it and replace it.
