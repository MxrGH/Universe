#!/usr/bin/env python3
"""Small, dependency-free checks for the Universe repository."""

import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def fail(message: str) -> None:
    print(f"ERROR: {message}", file=sys.stderr)
    raise SystemExit(1)


mod_path = ROOT / "mod.json"
source_path = ROOT / "src" / "main.cpp"
if not mod_path.is_file() or not source_path.is_file():
    fail("mod.json or src/main.cpp is missing")

try:
    mod = json.loads(mod_path.read_text(encoding="utf-8"))
except json.JSONDecodeError as error:
    fail(f"mod.json is not valid JSON: {error}")

if mod.get("id") != "mxrmc.universe":
    fail("unexpected mod ID")
if mod.get("gd", {}).get("win") != "2.2091":
    fail("Windows target must remain Geometry Dash 2.2091")
if mod.get("gd", {}).get("android") != "2.2091":
    fail("Android target must remain Geometry Dash 2.2091")

settings = mod.get("settings")
if not isinstance(settings, dict):
    fail("mod.json must contain a settings object")

source = source_path.read_text(encoding="utf-8")
missing_keybinds = [
    key for key in settings
    if key.startswith("keybind-") and key not in source
]
if missing_keybinds:
    fail("keybind settings are not referenced in source: " + ", ".join(missing_keybinds))

defaults = {}
for key, setting in settings.items():
    if not isinstance(setting, dict):
        fail(f"setting {key!r} must be an object")
    if setting.get("type") == "keybind":
        default = setting.get("default")
        if not isinstance(default, str) or not default:
            fail(f"keybind {key!r} needs a default binding")
        if default in defaults:
            fail(f"duplicate default keybind {default!r}: {defaults[default]} and {key}")
        defaults[default] = key

for required in ("README.md", "about.md", "changelog.md", "AI_LEVEL_MAKER.md", "VIDEO_EXPORT.md"):
    if not (ROOT / required).is_file():
        fail(f"missing project document: {required}")

for required in ("scripts/level_maker.py", "scripts/export_video.py"):
    if not (ROOT / required).is_file():
        fail(f"missing tool script: {required}")

print(f"Universe {mod.get('version', '(unknown version)')}: metadata checks passed.")
