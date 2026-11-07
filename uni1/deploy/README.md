# Deploying Uni-1

This repository does not provide a hosted server or GPU. You must deploy the inference service on a machine you control before the Geode client can generate levels.

## 1. Train and prepare weights

Follow [../README.md](../README.md). Place the trained adapter at `uni1/artifacts/uni1-adapter`. The directory must contain the PEFT adapter config/weights and tokenizer files.

The checked-in dataset is only a schema fixture, not a training corpus. Do not deploy the base model as if it were a trained Uni-1 release.

## 2. Build the container

From the repository root:

```bash
docker build -f uni1/Dockerfile -t uni1-inference uni1
```

The included image uses CPU PyTorch for portability. It can run the small base model, but generation may be slow. For production use, build a CUDA image with a PyTorch wheel matching the host's supported CUDA runtime and use a GPU machine.

## 3. Run with persistent storage

```bash
docker run --name uni1 \
  -p 127.0.0.1:8000:8000 \
  -v "$PWD/uni1/artifacts:/models:ro" \
  -v uni1-data:/data \
  -e UNI1_ADAPTER_DIR=/models/uni1-adapter \
  -e UNI1_QUOTA_DB=/data/uni1-quota.sqlite3 \
  uni1-inference
```

Keep the port bound to localhost behind a reverse proxy. Configure HTTPS, request-size limits, monitoring, and a firewall before making it reachable from the internet. Never expose the raw development server directly.

## 4. Configure Universe

In Geode > Universe settings, set the AI base URL to the public HTTPS origin of this service, without the `/v1` suffix. Leave the model name as `uni-1`. No player API key is required.

## Quota limitations

The service uses a random per-installation identifier saved by the client. Deleting the client data can create a new identifier. This is a basic per-installation quota, not account authentication. A public service needs additional abuse prevention and operational monitoring. Seven UNIV taps unlock the 1,000/day allowance for that installation.

The quota database must be on persistent storage. Backing it with an ephemeral container filesystem resets quotas on redeploy.
