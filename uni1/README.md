# Uni-1 — Geometry Dash specialist model

Uni-1 is intended to become Universe's own fine-tuned model, not a renamed third-party API.

## Architecture

- **Base checkpoint:** `Qwen/Qwen2.5-0.5B-Instruct` (Apache-2.0; see the model card and license before redistribution).
- **Specialization:** LoRA fine-tuning on Geometry Dash level-design examples in the Universe JSON blueprint format.
- **Inference:** self-hosted FastAPI service. The Universe client will send prompts to this service; it will not need an Anthropic/OpenAI key.
- **Output:** structured JSON blueprints. Universe must still validate every response before editing a level.

The base checkpoint is an upstream starting point. After training, the LoRA adapter is Uni-1's learned specialization. Do not describe an untrained base checkpoint as a completed Uni-1 model.

## Train

Python 3.10+ is recommended. A CUDA GPU is strongly recommended; CPU training will be very slow.

```bash
python -m venv .venv
# Windows:
.venv\Scripts\activate
# Linux/macOS:
source .venv/bin/activate
pip install -r requirements.txt
# Optional schema/layout bootstrap experiment (synthetic data is NOT production quality):
python generate_dataset.py --count 256 --output dataset/bootstrap.jsonl
python train.py --dataset dataset/bootstrap.jsonl --output artifacts/uni1-bootstrap --allow-synthetic-bootstrap

# Production training: dataset/train.jsonl must contain at least 32 unique,
# human-reviewed, playtested examples, each marked with "reviewed": true.
python train.py --dataset dataset/train.jsonl --output artifacts/uni1-adapter --validation-ratio 0.1
```

Each JSONL line must contain `prompt` and `response` strings. The response must be a valid Universe level-draft JSON object. Generate a synthetic bootstrap corpus with `python generate_dataset.py --count 256`, then review it and expand it with diverse, licensed, human-authored examples. Synthetic examples teach schema and simple spacing patterns; they do not replace playtested design data. Do not train on copied copyrighted level data unless you have permission.

The checked-in one-line dataset is only a format fixture and does not meet the complete-level contract. Before training, the quality gate requires at least 32 examples, at least 90% unique prompts and responses, and at least four styles and three difficulty classes. Normal production training additionally requires every example to be explicitly marked `"reviewed": true`; this marker is a human workflow attestation, not an automatic proof of playability. Synthetic examples are rejected unless `--allow-synthetic-bootstrap` is explicitly passed. The generator can create a reproducible synthetic bootstrap corpus, but synthetic examples are only useful for schema/layout experiments and do not establish playable-level quality. A useful release still needs a larger, reviewed and playtested corpus. Training produces an adapter, not a ready-to-publish model release. The training script reserves 10% of the corpus for held-out evaluation by default, keeps prompt tokens out of the loss, and saves the best evaluation-loss checkpoint. It rejects examples that do not match the complete-level contract (6–8 cube-mode sections and 96–144 supported placements).

## Run inference

Set `UNI1_ADAPTER_DIR` to the adapter directory and start:

```bash
uvicorn server:app --host 127.0.0.1 --port 8000
```

For a local CPU deployment with Docker Compose, first train and save the adapter to `uni1/artifacts/uni1-adapter`, then run:

```bash
cd uni1
docker compose -f compose.yaml up --build -d
docker compose -f compose.yaml ps
docker compose -f compose.yaml logs -f uni1
```

The compose file binds port 8000 to loopback only and persists quota state in a named volume. It mounts the adapter read-only. The first startup downloads the base checkpoint and may take several minutes; CPU inference can be slow. The health endpoint returns HTTP 503 until model and tokenizer are loaded. Stop it with `docker compose -f compose.yaml down`; the named quota volume is retained unless explicitly removed. On the same desktop running Geometry Dash, set Geode's Uni-1 server URL to `http://127.0.0.1:8000`. This loopback HTTP exception is accepted only for localhost addresses. Android devices and other computers cannot reach a loopback-bound desktop service; they need a reachable HTTPS deployment.

For a remote server, use a proper HTTPS reverse proxy, firewall, monitoring, backups, and a persistent database volume. Do not expose the development server directly to the public internet. The included Compose setup is a local deployment template, not a hosted public service.

The API is OpenAI-chat-completions-shaped for easy client integration, but it does not call OpenAI. It runs the locally loaded Uni-1 weights and requires no provider API key.

## Quotas and costs

The service resets at 00:00 UTC and enforces a server-side daily allowance of 100 requests per installation ID. Seven calls to `POST /v1/bonus/tap` unlock 1,000 requests per day for that installation. The Geode client persists a random installation ID and calls the tap endpoint when the UNIV button is tapped.

This is not account identity: a user who deletes their local installation ID can obtain a new quota. Add authenticated accounts only if Universe later needs cross-device identity. A public hosted service still costs money for GPU time, bandwidth, storage, and operations; removing API keys removes per-user provider credentials, not compute costs.

## Current status

The Geode client is wired to this keyless inference endpoint and the server validates model output before returning it. However, no Uni-1 weights have been trained and no inference server has been deployed. The checked-in dataset is only a schema fixture; run the generator, review and expand the corpus, train the adapter, deploy the server, and test live inference before claiming Uni-1 is live.

## Tests

Run quota tests from this folder with `pip install -r requirements-dev.txt` and `pytest -q`.

## Open-source level-editor integrations

Uni-1 now adapts selected MIT-licensed components and design guidance from:
- [EditorAI](https://github.com/entity12208/EditorAI): level-design workflow, gameplay readability, decoration planning, and style recipes are injected into the inference system prompt.
- [GD-AI-bridge](https://github.com/RibomX/GD-AI-bridge): its compact ASCII map representation and gameplay-object priority rules are adapted into the in-editor context sent with Uni-1 generation requests.

These are source-level adaptations, not bundled upstream binaries. Their MIT notices are preserved in `vendor/`.
[LayoutGenerator](https://github.com/ProfessorDragon/LayoutGenerator) was reviewed, but no source was copied because the inspected repository did not provide a license file.

Compatibility remains limited to the current Universe draft schema: cube gameplay and block, spike, saw, orb, and pad placements. The added design guidance does not make unsupported decoration, portals, triggers, or other modes available yet.

