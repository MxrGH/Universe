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
python train.py --dataset dataset/train.jsonl --output artifacts/uni1-adapter
```

Each JSONL line must contain `prompt` and `response` strings. The response must be a valid Universe level-draft JSON object. Start with a small hand-reviewed dataset, then expand it with diverse, licensed examples. Do not train on copied copyrighted level data unless you have permission.

The starter dataset is only a format example; it is nowhere near enough to train a useful model. Training produces an adapter, not a ready-to-publish model release.

## Run inference

Set `UNI1_ADAPTER_DIR` to the adapter directory and start:

```bash
uvicorn server:app --host 127.0.0.1 --port 8000
```

For a remote server, use a proper HTTPS reverse proxy, firewall, monitoring, and a persistent database volume. Do not expose the development server directly to the public internet.

The API is OpenAI-chat-completions-shaped for easy client integration, but it does not call OpenAI. It runs the locally loaded Uni-1 weights and requires no provider API key.

## Quotas and costs

The service enforces a server-side daily allowance of 100 requests per installation ID. Seven calls to `POST /v1/bonus/tap` unlock 1,000 requests per day for that installation. The client integration must persist a random installation ID and call the tap endpoint when the UNIV button is tapped.

This is not account identity: a user who deletes their local installation ID can obtain a new quota. Add authenticated accounts only if Universe later needs cross-device identity. A public hosted service still costs money for GPU time, bandwidth, storage, and operations; removing API keys removes per-user provider credentials, not compute costs.

## Current status

This folder is the training/inference foundation only. No Uni-1 weights have been trained or checked into this repository, no inference server has been deployed, and the Geode client has not yet been switched from its existing provider request code. Those are separate steps and must be verified before claiming Uni-1 is live.
