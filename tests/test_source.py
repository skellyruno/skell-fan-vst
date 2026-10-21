"""Offline project consistency checks (requires only Python's standard library).

These checks do not substitute for a JUCE build or an audio-host test.
"""
import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class SourceSanity(unittest.TestCase):
    def test_required_source_files(self):
        cmake = (ROOT / 'CMakeLists.txt').read_text()
        paths = re.findall(r'\b(?:Source/[\w./-]+\.(?:cpp|h)|Assets/[\w_-]+\.png)\b', cmake)
        self.assertGreater(len(paths), 20)
        for f in paths:
            with self.subTest(file=f):
                self.assertTrue((ROOT / f).is_file(), f)

    def test_parameter_schema_unchanged(self):
        source = (ROOT / 'Source/PluginProcessor.cpp').read_text()
        ids = re.findall(r'ParameterID\{"(\w+)",1\}', source)
        self.assertEqual(ids, ['power', 'mode', 'depth', 'rate', 'syncMode',
                               'syncDiv', 'noteType', 'wave', 'saturation',
                               'width', 'mix'])

    def test_all_assets_nonzero(self):
        cmake = (ROOT / 'CMakeLists.txt').read_text()
        for asset in set(re.findall(r'Assets/[\w_-]+\.png', cmake)):
            with self.subTest(asset=asset):
                self.assertGreater((ROOT / asset).stat().st_size, 100)

    def test_host_and_latency_safeguards(self):
        processor = (ROOT / 'Source/PluginProcessor.cpp').read_text()
        lfo = (ROOT / 'Source/dsp/Lfo.h').read_text()
        self.assertIn('setLatencySamples(saturation.getLatency())', processor)
        self.assertIn('getPpqPosition()', processor)
        self.assertIn('jumped', processor)
        self.assertIn('wasTransportPlaying', processor)
        self.assertIn('setCurrentAndTargetValue(f)', lfo)
        self.assertIn('modeFader.setTarget(wanted)', processor)
        self.assertIn('modeFader.advance()', processor)
        self.assertIn('vibratoDryAtMix', processor)
        self.assertIn('juce::ScopedNoDenormals', processor)
        self.assertIn('const int count=juce::jmin(maxBlock,n-blockStart)', processor)

    def test_regression_test_targets_are_linked(self):
        cmake = (ROOT / 'CMakeLists.txt').read_text()
        self.assertIn('target_sources(BreezeModDspTests PRIVATE tests/DSPTests.cpp)', cmake)
        self.assertIn('add_executable(BreezeModPortableTests tests/PortableTests.cpp)', cmake)
        self.assertIn('add_test(NAME BreezeModPortableTests COMMAND BreezeModPortableTests)', cmake)
        rotary = (ROOT / 'Source/dsp/RotarySim.h').read_text()
        self.assertIn('bool render = true', rotary)
        self.assertIn('hornA = 1.0f - std::exp', rotary)

    def test_no_deprecated_float_helpers(self):
        sat = (ROOT / 'Source/dsp/Saturator.h').read_text()
        self.assertIn('initProcessing', sat)
        self.assertIn('oversampling->reset()', sat)
        self.assertIn('x += a * (clipped - x)', sat)
        self.assertIn('amount.setTargetValue', sat)


if __name__ == '__main__':
    unittest.main(verbosity=2)
