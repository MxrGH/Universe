"""Uni-1 level-design guidance adapted from EditorAI's design_knowledge.hpp.

Original: https://github.com/entity12208/EditorAI
Copyright (c) 2026 Entity12208
Licensed under the MIT License; see vendor/editorai/LICENSE.
Adapted for Uni-1's currently supported draft schema by MxrGH.
"""

DESIGN_GUIDANCE = r"""
DESIGN WORKFLOW:
Plan sections by musical phrase or platformer room: gameplay purpose, mode, entry/exit
state, dominant shape, palette roles, focal point, and transition. Establish a short
representative section before repeating it. Reuse its visual language with variations
in silhouette and scale, not identical object stamps across the entire level.

GAMEPLAY MODE CONTRACTS:
- Cube: fixed jump arc; provide takeoff and landing clearance, including headroom.
- Ship: preserve momentum; build smooth corridors with recovery space after portals.
- Ball: gravity swaps need reachable opposing surfaces and time to cross between them.
- UFO: discrete airborne jumps; plan click heights and recovery from early/late clicks.
- Wave: diagonal hold/release path; check both segments and mini size, not cube jumps.
- Robot: vary hold lengths purposefully; allow readable landings and release windows.
- Spider: instantaneous vertical transfer; opposing landing surfaces must overlap X.
- Swing: gravity switching retains momentum; allow room before direction reverses.
- Dual requires ONE compatible input sequence for both players, not two independently passable lanes.
Introduce a mechanic, vary it, combine it, then provide recovery. Difficulty is timing
tolerance and control, not a universal obstacle spacing or a mandatory speed increase.

SYNC AND LEVEL TYPE:
Choose which musical voice inputs, landings and visual accents follow; not every note
needs a hazard. Use measured timing when available and distinguish it from an estimate.
Convert time to distance separately across speed changes; account for song offset,
portals and travel direction. Never promise exact sync from one constant X-per-second calculation.
Platformer uses rooms, navigation, readable goals, return routes and safe retries, not
an auto-scroll obstacle strip. Do not use classic X-span as proof of platformer duration or completion.

DECORATION WORKFLOW:
Begin with large shapes and clear gameplay edges, then add material detail and lighting.
Separate palette roles (background, structure, hazard/readable edge, accent), reserving
strong contrast for the player path. Use background silhouettes and selective foreground
framing for depth; preserve intentional empty space. A small repeated motif with meaningful
variation is better than unrelated objects or filling every gap. Decoration density must
fit the requested style; an undecorated layout request must stay a layout.

STYLE RECIPES:
- Modern/minimal: broad geometric masses, intentional asymmetry, restrained accents,
  crisp edges and negative space; spend objects on composition rather than tiny fill.
- Glow/neon: dark supporting shapes, narrow bright edges, selective soft light around
  focal elements; keep hazards distinct and avoid stacking glow over the play path.
- Design/tech/industrial: repeat a structural module with trim, joints and inset panels;
  vary module sizes and supports. Machinery belongs to an architectural system.
- Art/nature: establish terrain and a recognizable silhouette first; use coherent
  materials, irregular contours and depth-scaled scenery, not random decorative stamps.
- Pixel/retro: consistent visual pixel size, stepped contours, limited palette and
  reusable tiles; keep decorative pixels separate from the collision skeleton.
- Effect/abstract: develop one shape or motion idea over a phrase; use contrast and
  transitions to reveal structure. Effects must not conceal required inputs.
- Dark/horror: readable foreground silhouettes against subdued scenery; use light to
  direct attention. Darkness must not turn hazards into blind memorization.
- Classic/simple: cohesive block family and strong rhythm; simplicity is intentional,
  not permission for featureless repetition or unrelated saws.

IMPLEMENTATION AND REVIEW:
Use only documented schema fields and known object IDs. Do not invent object IDs or
assume raw IDs configure shaders, particles, triggers, or complex logic. Decorative
solids and hazards must not obstruct the player path. Visibility changes do not disable
collision. Keep gameplay cues readable and decoration out of the collision skeleton.
Use distinct groups for motion and visibility, and initialize colors before first use.
Review whole-section composition AND close-up hazard readability. Check transitions,
visual occlusion, collision flags, empty accidental stretches and unnecessary overlap.
Simplify clutter instead of endlessly adding objects. A geometric passability scan or
cube simulation is NOT proof that every mode or dual is playable; report unsupported
checks and reserve verified completion for actual playtesting.

CURRENT UNI-1 COMPATIBILITY OVERRIDE:
The current server schema supports only cube gameplay and block, spike, saw, orb, and
pad events. Do not emit decoration placeholders, portals, triggers, other game modes,
or unsupported object properties. Describe decoration ideas in notes only; do not
pretend those notes have been placed. Never claim that a level was playtested.
"""
