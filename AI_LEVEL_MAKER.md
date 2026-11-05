# AI Level Maker — Coming Soon

AI Level Maker is a planned Universe feature, not yet available in the current build.

## Intended workflow

1. Describe a level idea, including theme, gameplay style, difficulty, and approximate length.
2. Generate an editable level layout.
3. Preview and adjust the generated sections.
4. Configure song timing and gameplay pacing.
5. Export a normal editable Geometry Dash level draft.

## Design goals

- Keep generated levels editable in the Geometry Dash editor.
- Offer style and difficulty controls rather than generating an opaque finished level.
- Make generation repeatable with a seed.
- Keep generation and export local to the user's game/editor session where possible.
- Never fabricate online verification, creator rewards, coins, achievements, or leaderboard results.

## Implementation status

The actual AI generation engine, prompt interface, song synchronization, in-game preview, and native level export are not implemented yet. The mod now has an implementation plan, but an AI service or local model must be selected and integrated before this can generate levels from natural-language prompts. API credentials must never be embedded in the public mod source.
