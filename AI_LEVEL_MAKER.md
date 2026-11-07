# Universe AI Level Creator

## Uni-1 identity and daily allowance

The assistant is presented in Universe as **Uni-1**. This is the mod's assistant/product name; the actual model ID sent to the configured Anthropic-compatible endpoint remains the provider model configured in Geode settings. Uni-1 currently allows 100 requests per local calendar day. Tapping the in-game UNIV floating button seven times unlocks a 1,000-request daily allowance. Usage resets at local midnight. The counter is stored locally and is not tamper-proof or shared between devices; only a hosted service can enforce a reliable account-wide quota. Provider usage may incur charges.

The in-game AI creator generates a JSON level layout and places supported objects directly into the currently open Geometry Dash editor. It does not export a .gmd file and does not require GDShare.

## In-game workflow

1. Open an empty level in the Geometry Dash editor.
2. Open Universe and select the AI tab.
3. Enter a complete level concept in the prompt field.
4. Enter an API key in the AI tab for this request. On desktop, the `ANTHROPIC_API_KEY` process environment variable is also accepted.
5. Choose **GENERATE ONLINE** and wait for the result message.
6. Return to the editor and choose **CREATE FULL LEVEL IN EDITOR**.
7. Inspect the layout and use the editor Undo control if you want to revert it.

The in-game prompt requests a complete level made from connected sections. The model cannot guarantee a fun, balanced, or completable level; review and test the generated result.

## Android and key handling

Android apps normally do not inherit a key from a shell environment, so the AI tab has a per-request API-key field. The key entered there is held only for the current request and is not written to Geode settings or a file by Universe. It is cleared when the AI page is rebuilt. The key is visible while typing; avoid screen recording or screenshots during entry.

The model name and HTTPS API base URL are configured in **Geode → Universe settings**. The current in-game request uses the Anthropic Messages API format. API use may cost money; use a provider account only with the account owner's permission. Never send API keys in chat, commit them, or include them in screenshots.

## Supported object placements and limits

The current in-game creator supports blocks, spikes, saws, orbs, pads, and decoration objects. It does not create portals, triggers, or song timing.

Current safety limits:
- The saved JSON draft must be no larger than 2 MB.
- The preflight rejects more than 1,000,000 generated placements before changing the editor.
- The current apply loop processes at most 1,000 add/edit/delete operations in one pass. The one-million value is only a validation ceiling, **not** the practical placement capacity.
- The parser uses regular-expression event extraction rather than a complete JSON schema parser. Unexpected model output may be rejected or skipped.
- The apply path is not chunked and has no progress/cancel UI. Do not request extremely large levels.

Start with an empty level. The creator adds objects; it does not clear or replace existing contents. Use the editor Undo control to revert.

## Standalone blueprint helper

`scripts/level_maker.py` validates and exports planning blueprints as JSON, CSV, or Markdown. These are design documents, not native Geometry Dash level saves and are not required for the in-game workflow. The standalone helper currently limits a blueprint to 5,000 events.
