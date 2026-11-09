# Uni-1 development status

The Uni-1 page is present in the Universe 1 Snapshot 26.6 client. It remains an experimental integration, **not a working public level-generation service**.

## What is implemented

- A strict draft validator for the Universe level-draft format.
- A reproducible synthetic curriculum generator covering multiple gameplay modes, mode portals, primitive obstacles, and decoration.
- A LoRA fine-tuning pipeline with a held-out evaluation split and provenance manifest.
- A FastAPI inference service with readiness checks, bounded requests, output validation, repair retry, and per-installation quotas.
- A Docker image and a loopback-bound Compose deployment template.
- Client-side request cancellation, editor context collection, draft validation, and chunked/cancellable object placement.
- A bundled numeric Geometry Dash editor object-ID catalogue.

## What is not complete

The repository does **not** contain trained Uni-1 adapter weights, a deployed inference endpoint, or a human-reviewed and playtested production corpus. The checked-in dataset is a one-row format fixture. Synthetic examples are useful for schema and smoke testing only; they are not evidence of playable level quality and cannot pass the production model gate.

The production training gate requires a minimum reviewed corpus and held-out evaluation. Even passing those structural checks would not prove that a generated level is fair, fun, or completable. Physical in-game playtesting is still required.

## Object IDs versus Node IDs

The client needs **numeric Geometry Dash object IDs** when it creates actual editor objects. For example, an ID identifies which block, portal, decoration, or trigger the editor should place. Universe bundles its verified ID catalogue in `resources/object_ids.txt`.

That is unrelated to the optional Geode mod named `geode.node-ids`. Universe's source does not use that mod's API, so Snapshot 26.6 removes the dependency. No extra Geode mod is required; Geode itself is still required to load Universe.

## Remaining work before Uni-1 can be called complete

1. Assemble a licensed, human-authored, human-reviewed, and playtested corpus with varied prompts and layouts.
2. Train the adapter on a suitable compute environment and record held-out generation evaluation.
3. Review generated drafts for playability, fairness, and structural correctness in Geometry Dash 2.2091.
4. Deploy the adapter-backed service with HTTPS, persistent storage, monitoring, and abuse controls.
5. Test quota rollover, rate limits, cancellation, errors, and generation against the live service.
6. Test the complete generate-to-editor workflow on actual Windows and Android Geometry Dash installations.

The current release ships the client-side experimental panel and the development pipeline, not a trained AI model or hosted AI service. No claim of a working AI generator is made.
