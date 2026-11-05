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

The generation engine, prompt interface, song synchronization, preview, and level export integration are not implemented yet. An AI service or local model strategy must be selected before claiming AI generation works. API credentials must never be embedded in the public mod source.
