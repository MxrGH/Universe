# Uni-1 training data

One JSON object per line:

```json
{"prompt":"Create a short beginner cube level with a bright neon theme.","response":"{\"title\":\"Neon Steps\",\"prompt\":\"beginner cube level\",\"difficulty\":\"normal\",\"style\":\"neon\",\"sections\":[{\"name\":\"Opening\",\"gameplay\":\"cube\",\"events\":[{\"x\":0,\"y\":0,\"type\":\"block\",\"note\":\"starting platform\"},{\"x\":90,\"y\":0,\"type\":\"spike\",\"note\":\"single jump\"}]}],\"notes\":[\"Example only; playtest before use.\"]}"}
```

Every response must be reviewed and validated against the current Universe draft schema. Include varied difficulties, modes, section transitions, and object combinations. Avoid duplicate templates and impossible geometry. Do not include real player information, API keys, or copyrighted level exports without permission.

The single starter example in `train.jsonl` is a schema smoke example, not a useful training corpus. Build a substantial reviewed dataset before training or publishing a model.
\nTo create a larger reproducible synthetic bootstrap set, run `python generate_dataset.py --count 256 --output dataset/train.jsonl` from the `uni1/` directory. These generated examples are for schema and layout-pattern bootstrapping only; add human-reviewed, playtested examples before treating the model as production quality.\n