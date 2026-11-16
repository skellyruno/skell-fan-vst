"""Regression tests for the SkellFan VST3 package verifier (stdlib only)."""
from __future__ import annotations
import contextlib
import io
import json
import struct
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))
import verify_vst3_package as verifier  # noqa: E402


def metadata():
    return {
        "Name": "SkellFan",
        "Version": "1.9.0",
        "Factory Info": {"Vendor": "Skell Audio", "URL": "", "E-Mail": ""},
        "Classes": [
            {"CID": "ABCDEF019182FAEB536B4175427A3031", "Category": "Audio Module Class",
             "Name": "SkellFan", "Vendor": "Skell Audio", "Version": "1.9.0"},
            {"CID": "ABCDEF011234ABCD536B4175427A3031", "Category": "Component Controller Class",
             "Name": "SkellFan", "Vendor": "Skell Audio", "Version": "1.9.0"},
        ],
    }


def pe(machine=0x8664, magic=0x20B):
    data = bytearray(512)
    data[:2] = b"MZ"
    struct.pack_into("<I", data, 0x3C, 0x80)
    data[0x80:0x84] = b"PE\0\0"
    struct.pack_into("<H", data, 0x84, machine)
    struct.pack_into("<H", data, 0x98, magic)
    return bytes(data)


class NormalizationTests(unittest.TestCase):
    def test_nested_trailing_commas(self):
        self.assertEqual(verifier.strip_trailing_commas('{"a":[1,2,],"b":{"c":true,},}'),
                         '{"a":[1,2],"b":{"c":true}}')

    def test_string_contents_and_escapes_are_preserved(self):
        source = r'{"message":"comma, } and escaped quote \" still text","items":["x, ]",],}'
        expected = r'{"message":"comma, } and escaped quote \" still text","items":["x, ]"]}'
        self.assertEqual(verifier.strip_trailing_commas(source), expected)

    def test_normalization_is_idempotent(self):
        value = verifier.strip_trailing_commas('{"x":1,}')
        self.assertEqual(verifier.strip_trailing_commas(value), value)

    def test_duplicate_json_keys_fail_closed(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / "moduleinfo.json"
            path.write_text('{"Name":"SkellFan","Name":"Other"}', encoding="utf-8")
            with self.assertRaisesRegex(verifier.PackageError, "Duplicate JSON object key"):
                verifier.load_moduleinfo(path)


class MetadataTests(unittest.TestCase):
    def test_expected_identity(self):
        self.assertEqual(len(verifier.validate_metadata(metadata())), 2)

    def test_duplicate_cids_rejected(self):
        data = metadata()
        data["Classes"][1]["CID"] = data["Classes"][0]["CID"]
        with self.assertRaisesRegex(verifier.PackageError, "unique"):
            verifier.validate_metadata(data)

    def test_wrong_vendor_rejected(self):
        data = metadata()
        data["Factory Info"]["Vendor"] = "Other Audio"
        with self.assertRaisesRegex(verifier.PackageError, "vendor"):
            verifier.validate_metadata(data)

    def test_class_version_must_match(self):
        data = metadata()
        data["Classes"][0]["Version"] = "1.8.0"
        with self.assertRaisesRegex(verifier.PackageError, "version"):
            verifier.validate_metadata(data)


class PETests(unittest.TestCase):
    def test_amd64_pe32_plus(self):
        verifier.validate_pe32_plus_x64(pe())

    def test_x86_rejected(self):
        with self.assertRaisesRegex(verifier.PackageError, "AMD64"):
            verifier.validate_pe32_plus_x64(pe(machine=0x14C))

    def test_pe32_rejected(self):
        with self.assertRaisesRegex(verifier.PackageError, r"PE32\+"):
            verifier.validate_pe32_plus_x64(pe(magic=0x10B))

    def test_invalid_offset_rejected(self):
        data = bytearray(pe())
        struct.pack_into("<I", data, 0x3C, 0xFFFF)
        with self.assertRaisesRegex(verifier.PackageError, "outside"):
            verifier.validate_pe32_plus_x64(bytes(data))


class BundleTests(unittest.TestCase):
    def make_bundle(self, root):
        bundle = root / "SkellFan.vst3"
        module = bundle / "Contents" / "x86_64-win" / "SkellFan.vst3"
        info = bundle / "Contents" / "Resources" / "moduleinfo.json"
        module.parent.mkdir(parents=True)
        info.parent.mkdir(parents=True)
        info.write_text(json.dumps(metadata(), indent=2)[:-2] + ",\n}\n", encoding="utf-8")
        module.write_bytes(pe())
        return bundle, info

    def test_normalizes_bundle_and_preserves_binary(self):
        with tempfile.TemporaryDirectory() as temp:
            bundle, info = self.make_bundle(Path(temp))
            before = (bundle / "Contents/x86_64-win/SkellFan.vst3").read_bytes()
            result = verifier.verify_bundle(bundle)
            self.assertTrue(result["moduleinfo_normalized"])
            self.assertEqual(result["version"], "1.9.0")
            json.loads(info.read_text(encoding="utf-8"))
            self.assertEqual(before, (bundle / "Contents/x86_64-win/SkellFan.vst3").read_bytes())

    def test_standalone_exe_rejected(self):
        with tempfile.TemporaryDirectory() as temp:
            bundle, _ = self.make_bundle(Path(temp))
            (bundle / "Contents/x86_64-win/SkellFan.exe").write_bytes(b"MZ")
            with self.assertRaisesRegex(verifier.PackageError, "Standalone"):
                verifier.verify_bundle(bundle)

    def test_provenance_requires_pluginval_gate(self):
        with tempfile.TemporaryDirectory() as temp:
            bundle, _ = self.make_bundle(Path(temp))
            with contextlib.redirect_stderr(io.StringIO()):
                result = verifier.main(["--bundle", str(bundle), "--manifest",
                                        str(Path(temp) / "build-info.json")])
            self.assertEqual(result, 1)

    def test_provenance_writes_hashes_and_preview_status(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            bundle, _ = self.make_bundle(root)
            manifest = verifier.write_provenance(bundle, root / "build-info.json",
                                                  "a" * 40, "b" * 40, "12345")
            self.assertEqual(manifest["release_status"], "preview-only")
            sums = (root / "SHA256SUMS.txt").read_text(encoding="ascii")
            self.assertIn(manifest["binary"]["sha256"], sums)
            self.assertIn(manifest["moduleinfo"]["sha256"], sums)


if __name__ == "__main__":
    unittest.main(verbosity=2)
