# Uni-1 development status

The Uni-1 page is present in the Universe 1 Snapshot 26.6 client. It is an experimental integration, **not a working level-generation feature yet**.

The repository contains:
- A strict draft validator for the supported geometry-only schema.
- A reproducible synthetic-data generator for schema and layout-pattern smoke tests.
- A LoRA fine-tuning pipeline with a held-out evaluation split.
- A FastAPI inference service with a readiness check, bounded requests, and server-enforced per-installation quotas.
- A Docker image and a loopback-bound Compose deployment template.
- Client-side request cancellation, draft validation, editor context collection, and chunked/cancellable object placement.

The repository does **not** contain trained Uni-1 weights, a hosted inference endpoint, or a human-reviewed/playtested training corpus. The checked-in dataset is a single schema fixture. Synthetic examples are not evidence of playable level quality and must not be presented as production training data.

The current generated-level contract is deliberately narrow: 6–8 cube-mode sections and 96–144 ordered placements using blocks, spikes, saws, orbs, and pads. Portals, mode changes, triggers, song objects, and generic decoration are not generated. The native editor placement ceiling is 1,000 objects.

## Required before Uni-1 can be called complete

1. Assemble a licensed, human-authored, reviewed, and playtested corpus with varied prompts and layouts.
2. Train the LoRA adapter and record held-out evaluation results.
3. Review generated levels for playability, fairness, and structural correctness.
4. Deploy the adapter-backed server with HTTPS, persistent storage, monitoring, and abuse controls.
5. Test quota rollover, rate limits, cancellation, errors, and generation against the live service.
6. Install the Windows and Android builds in Geometry Dash 2.2091 with Geode and test the complete generate-to-editor workflow.

No API key, trained model, hosted endpoint, or claim of a functioning AI generator is supplied by this repository.
