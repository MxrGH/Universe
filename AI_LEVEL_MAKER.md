# Uni-1 development notes

The Uni-1 AI level creator is **not included as an enabled feature** in the Universe 1 Snapshot 26.5 user interface.

The files in `uni1/` are an experimental development foundation only. They include a synthetic dataset generator, a LoRA training script, a FastAPI inference service, and deployment notes. They do not include trained adapter weights or a deployed inference endpoint. The synthetic examples are not human-reviewed or playtested.

Do not enable or advertise the in-game AI workflow until all of the following have been completed and verified:

1. Build a licensed, reviewed, human-authored and playtested training corpus.
2. Train the adapter and evaluate it on held-out prompts and generated layouts.
3. Deploy the inference service with persistent storage, HTTPS, monitoring, and abuse controls.
4. Test the API and its quota behavior against the actual trained model.
5. Build Windows and Android Geode packages and test generation and editor placement in the actual game.

No API key, hosted endpoint, model weights, or working AI feature is supplied by this repository.
