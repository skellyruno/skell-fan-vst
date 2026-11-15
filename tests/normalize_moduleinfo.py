"""Normalize JUCE-generated VST3 moduleinfo.json into strict JSON.

JUCE 8 sometimes emits a trailing comma after the last member/array element.
Keep all values and classes identical; normalize only the serialization.
This runs on the build/packaging thread, never inside the audio plugin.
"""
from __future__ import annotations

import json
import sys
from pathlib import Path


def strip_trailing_commas(source: str) -> str:
    output: list[str] = []
    quoted = False
    escaped = False
    for index, character in enumerate(source):
        if quoted:
            output.append(character)
            if escaped:
                escaped = False
            elif character == "\\":
                escaped = True
            elif character == '"':
                quoted = False
            continue
        if character == '"':
            quoted = True
            output.append(character)
            continue
        if character == ',':
            following = index + 1
            while following < len(source) and source[following].isspace():
                following += 1
            if following < len(source) and source[following] in (']', '}'):
                continue
        output.append(character)
    if quoted:
        raise ValueError("Unterminated moduleinfo.json string")
    return ''.join(output)


def normalize_file(path: Path) -> None:
    raw = path.read_text(encoding="utf-8")
    document = json.loads(strip_trailing_commas(raw))
    if not isinstance(document, dict):
        raise ValueError("VST3 moduleinfo must contain an object")
    classes = document.get("Classes")
    if not isinstance(classes, list) or not classes:
        raise ValueError("VST3 moduleinfo must include plugin classes")
    if not isinstance(document.get("Factory Info"), dict):
        raise ValueError("VST3 moduleinfo must include Factory Info")
    sanitized = json.dumps(document, ensure_ascii=False, indent=2) + "\n"
    json.loads(sanitized)
    path.write_text(sanitized, encoding="utf-8", newline="\n")
    print(f"Verified strict VST3 moduleinfo.json: {len(classes)} classes")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit("Usage: normalize_moduleinfo.py <path-to-moduleinfo.json>")
    normalize_file(Path(sys.argv[1]))
