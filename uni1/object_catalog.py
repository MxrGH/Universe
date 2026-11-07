"""Relevant Geometry Dash editor-object lookup for Uni-1.

The underlying name-to-ID catalogue is adapted from EditorAI; attribution and
license are in vendor/editorai/UPSTREAM.md and vendor/editorai/LICENSE.
"""

from __future__ import annotations

import json
import re
from pathlib import Path

_CATALOG_PATH = Path(__file__).parent / "vendor" / "editorai" / "object_ids.json"
with _CATALOG_PATH.open("r", encoding="utf-8") as _handle:
    OBJECT_IDS: dict[str, int] = json.load(_handle)

_SYNONYMS = {
    "deco": "decor",
    "decoration": "decor",
    "decorations": "decor",
    "glows": "glow",
    "pulses": "pulse",
    "pulsing": "pulse",
    "colours": "color",
    "colors": "color",
    "coloured": "colored",
    "colour": "color",
    "portals": "portal",
    "triggers": "trigger",
    "effects": "effect",
    "backgrounds": "background",
    "grounds": "ground",
    "modes": "portal",
    "camera": "camera",
}


def relevant_object_catalog(prompt: str, limit: int = 90) -> str:
    """Return a compact, prompt-relevant object-ID list; never invent IDs."""
    tokens = set(re.findall(r"[a-z0-9]+", prompt.lower()))
    tokens |= {_SYNONYMS[token] for token in tuple(tokens) if token in _SYNONYMS}
    scored: list[tuple[int, str, int]] = []
    for name, object_id in OBJECT_IDS.items():
        parts = set(name.split("_"))
        score = len(tokens & parts)
        if score:
            scored.append((score, name, object_id))

    scored.sort(key=lambda item: (-item[0], item[1]))
    selected = scored[:limit]

    # Keep a few representative items from important editor families available
    # even when the natural-language prompt does not explicitly name them.
    family_prefixes = ("effect_", "portal_", "decor_", "block_", "spike_")
    for prefix in family_prefixes:
        family = [item for item in scored if item[1].startswith(prefix)]
        for item in family[:3]:
            if item not in selected:
                selected.append(item)
    selected = selected[: limit + 15]
    return "\n".join(f"{name} = {object_id}" for _, name, object_id in selected)
