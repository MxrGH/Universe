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


## In-game AI tab

Universe includes an experimental AI information tab. It is hidden by default; enable **Show AI tab (experimental)** in Geode's Universe mod settings to display it. The tab links to the external draft workflow and explains its current output limitations.

The in-game tab is currently informational only: it does not yet contain a prompt editor or send API requests from Geometry Dash. Use the command-line workflow above for actual AI-assisted prompts. API credentials remain in your local environment and must not be committed to the repository.


## Anthropic (default provider)

The generator now uses Anthropic's Messages API by default. It reads the credential from the local `ANTHROPIC_API_KEY` environment variable; it does not save the key in the project, mod settings, generated JSON, or Git. The default model is `claude-sonnet-4-6`.

On Windows PowerShell, set the environment variable for the current terminal session (use a replacement key only after revoking any key that was exposed):

```powershell
$env:ANTHROPIC_API_KEY = Read-Host "Paste your API key"
python scripts/level_maker.py --prompt "Make my level Ribbons much cooler" --output Ribbons-Draft.json
```

For another provider, pass `--provider openai-compatible` and optionally `--base-url`, `--model`, and `--api-key-env`. Use `--offline` to generate a local template without contacting an AI service.

API calls may incur charges according to the provider account. Only use an account and billing method you are authorized to use, with a parent or guardian's help where required. Never paste keys into chat, source code, screenshots, or commits. If a key is exposed, revoke it and replace it.
