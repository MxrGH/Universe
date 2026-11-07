# Universe AI Level Creator

## Uni-1 identity and daily allowance

The in-game client connects to a configurable self-hosted Uni-1 inference server. It does not call Anthropic or OpenAI and does not ask players for provider API keys. Configure the server's HTTPS base URL in Geode > Universe settings. The server enforces 100 requests/day per installation; seven UNIV floating-button taps unlock 1,000/day. Quotas reset at 00:00 UTC. The installation ID is stored locally, so deleting it can create a fresh quota; this is basic per-installation abuse control, not account identity.

The training and inference foundation is in `uni1/`. Generate a synthetic bootstrap corpus with `python generate_dataset.py --count 256` from that folder, review and supplement it with human-authored, playtested examples, and then train. The client is wired to the server, but trained weights and a deployed server are still required before generation can work.

The in-game AI creator generates a JSON level layout and places supported objects directly into the currently open Geometry Dash editor. It does not export a .gmd file and does not require GDShare.

## In-game workflow

1. Open an empty level in the Geometry Dash editor.
2. Open Universe and select the AI tab.
3. Enter a complete level concept in the prompt field.
4. Choose **GENERATE ONLINE** and wait for the result message.
5. Return to the editor and choose **CREATE FULL LEVEL IN EDITOR**.
6. Inspect the layout and use the editor Undo control if you want to revert it.

The in-game prompt requests a complete level made from connected sections. The model cannot guarantee a fun, balanced, or completable level; review and test the generated result.

## Server setup

Deploy the Python service in `uni1/` with a trained adapter, configure the Geode setting with its HTTPS base URL, and keep its SQLite quota database on persistent storage. The service does not use third-party provider keys, but hosting still costs compute and bandwidth. Use HTTPS, a reverse proxy, firewalling, monitoring, and abuse controls. Do not expose the development server directly to the public internet.

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
