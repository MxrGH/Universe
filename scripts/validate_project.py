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

version = mod.get("version", "")
if not re.fullmatch(r"v?\d+\.\d+\.\d+", version):
    fail("Geode version must use standard three-part semantic versioning")

readme_path = ROOT / "README.md"
readme_text = readme_path.read_text(encoding="utf-8") if readme_path.is_file() else ""
if "**Release:** Universe 1 — Snapshot 26.5" not in readme_text:
    fail("README must document Universe 1 — Snapshot 26.5")
if mod.get("name") != "Universe 1" or mod.get("version") != "v1.1.1":
    fail("release metadata must identify Universe 1 v1.1.1")

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

for feature_marker in (
    'addAction("PRACTICE", 130',
    'addAction("VISUALS", 131',
    'addAction("CLEAN RUN", 132',
    'addAction("RESTART ATTEMPT", 133',
    "if (tag >= 130 && tag <= 132)",
    "if (tag == 133)",
    "CCEaseBackOut::create(CCScaleTo::create(0.22f, 1.f))",
    "CCEaseBackOut::create(CCScaleTo::create(0.18f, 1.f))",
    "m_content->setScale(0.98f)",
    'addAction("RESET TO 1.00x", 134',
    'addAction("SAVE PROMPT", 141',
    'addAction("CLEAR", 142',
    "prompt.size() > 4000",
    'addAction("LOAD SAVED", 143',
    "loaded.size() >= 100000",
    "layer->m_isPracticeMode",
    "drawRoundedPanel",
    "animateClosePanel",
    "applySavedDraftToEditor",
    "buildEditorContext()",
    "move_selected",
    "rotate_selected",
    "delete_selected",
    "move_object",
    "rotate_object",
    "delete_object",
    "targetPattern",
    "getSaveString(editor)",
    "pasteObjects(replacements, false, false)",
    "pasteObjects(serialized, false, false)",
    "drawPolygon",
    "drawDot({34.f, 34.f}",
    "bool speedhackActive = speedhackRequested",
    "panel-offset-x",
    "panel-offset-y",
    "addAction(\"RESET PANEL POSITION\", 146",
    "panel-title",
    "floating-button-label",
    "void generateAIDraft()",
    "Page::AI",
    "ANTHROPIC_API_KEY",
    "m_aiApiKeyInput",
    '.header("x-api-key", apiKey)',
    "local-icon-override",
    "preview-icon-type",
    "isIconUnlocked",
    "applySavedDraftToEditor",
    "pasteObjects(serialized, false, false)",
    "drawRoundedPanel",
    "animateClosePanel",
    "drawDot({34.f, 34.f}",
    "bool speedhackActive = speedhackRequested",
):
    if feature_marker not in source:
        fail("missing 1.0 feature implementation: " + feature_marker)

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

for required in ("README.md", "about.md", "changelog.md", "AI_LEVEL_MAKER.md", "VIDEO_EXPORT.md", "LICENSE", "logo.png"):
    if not (ROOT / required).is_file():
        fail(f"missing project document: {required}")

for required in ("scripts/level_maker.py", "scripts/test_level_maker.py", "scripts/export_video.py"):
    if not (ROOT / required).is_file():
        fail(f"missing tool script: {required}")

logo_path = ROOT / "logo.png"
if logo_path.read_bytes()[:8] != b"\x89PNG\r\n\x1a\n":
    fail("logo.png is not a valid PNG file")
if "MIT License" not in (ROOT / "LICENSE").read_text(encoding="utf-8"):
    fail("LICENSE must contain the MIT License")
if "Snapshot 26.5" not in (ROOT / ".github" / "workflows" / "build.yml").read_text(encoding="utf-8"):
    fail("build workflow must define the Snapshot 26.5 release")
if "pull_request:" not in (ROOT / ".github" / "workflows" / "build.yml").read_text(encoding="utf-8"):
    fail("Snapshot 26.5 workflow must run CI for pull requests")

print(f"Universe {mod.get('version', '(unknown version)')}: metadata checks passed.")
