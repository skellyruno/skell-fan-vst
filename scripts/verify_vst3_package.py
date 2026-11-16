#!/usr/bin/env python3
"""Normalize and verify a staged SkellFan Windows x64 VST3 bundle.

Only the staged bundle's JUCE-generated moduleinfo.json is normalized. The
compiled module is read-only. Provenance is explicitly preview-only.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import re
import struct
import sys
from pathlib import Path
from typing import Any

PRODUCT = "SkellFan"
VENDOR = "Skell Audio"
VERSION_RE = re.compile(r"^\d+\.\d+\.\d+(?:[-+][0-9A-Za-z.-]+)?$")
CID_RE = re.compile(r"^[0-9A-Fa-f]{32}$")


class PackageError(ValueError):
    """A release package failed a fail-closed validation gate."""


def strip_trailing_commas(text: str) -> str:
    """Remove trailing commas only outside JSON strings."""
    output: list[str] = []
    in_string = False
    escaped = False
    index = 0
    while index < len(text):
        char = text[index]
        if in_string:
            output.append(char)
            if escaped:
                escaped = False
            elif char == "\\":
                escaped = True
            elif char == '"':
                in_string = False
            index += 1
            continue
        if char == '"':
            in_string = True
            output.append(char)
            index += 1
            continue
        if char == ",":
            lookahead = index + 1
            while lookahead < len(text) and text[lookahead].isspace():
                lookahead += 1
            if lookahead < len(text) and text[lookahead] in "}]":
                index += 1
                continue
        output.append(char)
        index += 1
    return "".join(output)


def reject_duplicate_keys(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    result: dict[str, Any] = {}
    for key, value in pairs:
        if key in result:
            raise PackageError(f"Duplicate JSON object key: {key!r}")
        result[key] = value
    return result


def load_moduleinfo(path: Path) -> tuple[dict[str, Any], str, bool]:
    try:
        original = path.read_text(encoding="utf-8-sig")
    except OSError as exc:
        raise PackageError(f"Cannot read moduleinfo.json: {exc}") from exc
    normalized = strip_trailing_commas(original)
    try:
        metadata = json.loads(normalized, object_pairs_hook=reject_duplicate_keys)
    except json.JSONDecodeError as exc:
        raise PackageError(f"moduleinfo.json remains invalid after safe normalization: {exc}") from exc
    if not isinstance(metadata, dict):
        raise PackageError("moduleinfo.json root must be an object")
    changed = normalized != original
    if changed:
        try:
            path.write_text(normalized, encoding="utf-8", newline="\n")
        except OSError as exc:
            raise PackageError(f"Cannot write staged moduleinfo.json: {exc}") from exc
    return metadata, normalized, changed


def validate_metadata(metadata: dict[str, Any]) -> list[str]:
    if metadata.get("Name") != PRODUCT:
        raise PackageError(f"Unexpected product name: {metadata.get('Name')!r}")
    version = metadata.get("Version")
    if not isinstance(version, str) or not VERSION_RE.fullmatch(version):
        raise PackageError(f"Invalid semantic version: {version!r}")
    factory = metadata.get("Factory Info")
    if not isinstance(factory, dict) or factory.get("Vendor") != VENDOR:
        raise PackageError("Factory Info vendor must be Skell Audio")
    classes = metadata.get("Classes")
    if not isinstance(classes, list) or len(classes) < 2:
        raise PackageError("Both VST3 component and controller classes are required")
    categories: set[str] = set()
    ids: list[str] = []
    for index, item in enumerate(classes):
        if not isinstance(item, dict):
            raise PackageError(f"VST3 class {index} must be an object")
        if item.get("Name") != PRODUCT or item.get("Vendor") != VENDOR:
            raise PackageError(f"VST3 class {index} product/vendor mismatch")
        if item.get("Version") != version:
            raise PackageError(f"VST3 class {index} version mismatch")
        cid = item.get("CID")
        if not isinstance(cid, str) or not CID_RE.fullmatch(cid):
            raise PackageError(f"VST3 class {index} has an invalid 128-bit CID")
        ids.append(cid.lower())
        if isinstance(item.get("Category"), str):
            categories.add(item["Category"])
    if len(set(ids)) != len(ids):
        raise PackageError("VST3 class CIDs must be unique")
    if not {"Audio Module Class", "Component Controller Class"}.issubset(categories):
        raise PackageError("VST3 component and controller categories are both required")
    return ids


def validate_pe32_plus_x64(binary: bytes) -> None:
    if len(binary) < 0x40 or binary[:2] != b"MZ":
        raise PackageError("Windows PE MZ signature missing")
    pe_offset = struct.unpack_from("<I", binary, 0x3C)[0]
    if pe_offset < 0x40 or pe_offset + 26 > len(binary):
        raise PackageError("Windows PE header offset is outside the module")
    if binary[pe_offset:pe_offset + 4] != b"PE\0\0":
        raise PackageError("Windows PE signature is invalid")
    machine = struct.unpack_from("<H", binary, pe_offset + 4)[0]
    magic = struct.unpack_from("<H", binary, pe_offset + 24)[0]
    if machine != 0x8664:
        raise PackageError(f"Expected AMD64 PE machine 0x8664, found 0x{machine:04x}")
    if magic != 0x20B:
        raise PackageError(f"Expected PE32+ optional header 0x20b, found 0x{magic:03x}")


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def verify_bundle(bundle: Path) -> dict[str, Any]:
    if not bundle.is_dir() or bundle.name != f"{PRODUCT}.vst3":
        raise PackageError(f"Expected a directory named {PRODUCT}.vst3")
    for path in bundle.rglob("*"):
        if path.is_symlink():
            raise PackageError(f"Symlink not allowed: {path.relative_to(bundle)}")
        if path.is_file() and path.suffix.lower() in {".exe", ".zip"}:
            raise PackageError(f"Standalone executable or nested archive not allowed: {path.name}")
    contents = bundle / "Contents"
    module_dir = contents / "x86_64-win"
    binary_path = module_dir / f"{PRODUCT}.vst3"
    info_path = contents / "Resources" / "moduleinfo.json"
    if not binary_path.is_file():
        raise PackageError("Windows x64 VST3 module is missing")
    if not info_path.is_file():
        raise PackageError("Contents/Resources/moduleinfo.json is missing")
    module_files = sorted(p.name for p in module_dir.glob("*.vst3") if p.is_file())
    if module_files != [f"{PRODUCT}.vst3"]:
        raise PackageError(f"Expected exactly one x64 module file, found {module_files!r}")
    metadata, normalized_text, normalized = load_moduleinfo(info_path)
    ids = validate_metadata(metadata)
    binary = binary_path.read_bytes()
    validate_pe32_plus_x64(binary)
    return {
        "version": metadata["Version"],
        "class_ids": ids,
        "binary_path": f"{PRODUCT}.vst3/Contents/x86_64-win/{PRODUCT}.vst3",
        "binary_size_bytes": len(binary),
        "binary_sha256": digest(binary),
        "moduleinfo_path": f"{PRODUCT}.vst3/Contents/Resources/moduleinfo.json",
        "moduleinfo_sha256": digest(normalized_text.encode("utf-8")),
        "moduleinfo_normalized": normalized,
    }


def write_provenance(bundle: Path, manifest_path: Path, source_commit: str,
                     ci_commit: str, run_id: str) -> dict[str, Any]:
    result = verify_bundle(bundle)
    if not source_commit or not ci_commit or not run_id:
        raise PackageError("Source commit, CI checkout commit and run ID are required")
    manifest = {
        "schema_version": 1,
        "product": PRODUCT,
        "version": result["version"],
        "vendor": VENDOR,
        "format": "VST3",
        "platform": "windows-x64",
        "source_commit": source_commit,
        "ci_checkout_commit": ci_commit,
        "workflow_run_id": run_id,
        "binary": {
            "path": result["binary_path"],
            "size_bytes": result["binary_size_bytes"],
            "sha256": result["binary_sha256"],
            "architecture": "AMD64",
            "pe_format": "PE32+",
        },
        "moduleinfo": {
            "path": result["moduleinfo_path"],
            "sha256": result["moduleinfo_sha256"],
            "strict_json": True,
            "unique_class_ids": result["class_ids"],
            "normalization_applied": result["moduleinfo_normalized"],
        },
        "validation": {
            "bundle_structure": "passed",
            "metadata_identity": "passed",
            "pe32_plus_amd64": "passed",
            "pluginval": {"strictness_level": 5, "result": "passed"},
        },
        "release_status": "preview-only",
        "release_blockers": [
            "FL Studio scan, automation, state save/reopen and old-project compatibility are not accepted",
            "Real music A/B and user listening approval are pending",
            "Vendor URL and support email are empty in the current moduleinfo.json",
            "Target Windows hardware CPU and latency acceptance are pending",
            "Third-party license and attribution review is pending",
        ],
    }
    manifest_path.parent.mkdir(parents=True, exist_ok=True)
    manifest_text = json.dumps(manifest, indent=2, sort_keys=True) + "\n"
    manifest_path.write_text(manifest_text, encoding="utf-8", newline="\n")
    sums = [
        f"{result['binary_sha256']}  {result['binary_path']}",
        f"{result['moduleinfo_sha256']}  {result['moduleinfo_path']}",
        f"{digest(manifest_text.encode('utf-8'))}  {manifest_path.name}",
    ]
    (manifest_path.parent / "SHA256SUMS.txt").write_text(
        "\n".join(sums) + "\n", encoding="ascii", newline="\n"
    )
    return manifest


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--bundle", type=Path, required=True)
    parser.add_argument("--manifest", type=Path)
    parser.add_argument("--source-commit", default="")
    parser.add_argument("--ci-commit", default="")
    parser.add_argument("--run-id", default="")
    parser.add_argument("--pluginval-passed", action="store_true")
    args = parser.parse_args(argv)
    try:
        if args.manifest:
            if not args.pluginval_passed:
                raise PackageError("Refusing provenance until exact staged bundle passes pluginval")
            manifest = write_provenance(args.bundle, args.manifest, args.source_commit,
                                        args.ci_commit, args.run_id)
            print(f"PASS: {PRODUCT} {manifest['version']} VST3; pluginval 5; provenance written")
        else:
            result = verify_bundle(args.bundle)
            state = "normalized and verified" if result["moduleinfo_normalized"] else "verified"
            print(f"PASS: {PRODUCT} {result['version']} {state}; "
                  f"{result['binary_size_bytes']} bytes; SHA256 {result['binary_sha256']}")
        return 0
    except (PackageError, OSError) as exc:
        print(f"FAIL: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
