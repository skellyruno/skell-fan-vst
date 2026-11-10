"""Offline project consistency checks (requires only Python's standard library).

These checks do not substitute for a JUCE build or an audio-host test.
"""
import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class SourceSanity(unittest.TestCase):
    def test_required_source_files(self):
        cmake = (ROOT / 'CMakeLists.txt').read_text(encoding='utf-8')
        paths = re.findall(r'\b(?:Source/[\w./-]+\.(?:cpp|h)|Assets/[\w_-]+\.png)\b', cmake)
        self.assertGreater(len(paths), 20)
        for f in paths:
            with self.subTest(file=f):
                self.assertTrue((ROOT / f).is_file(), f)

    def test_parameter_schema_unchanged(self):
        source = (ROOT / 'Source/PluginProcessor.cpp').read_text(encoding='utf-8')
        ids = re.findall(r'ParameterID\{"(\w+)",1\}', source)
        self.assertEqual(ids, ['power', 'mode', 'depth', 'rate', 'syncMode',
                               'syncDiv', 'noteType', 'wave', 'saturation',
                               'width', 'mix'])

    def test_all_assets_nonzero(self):
        cmake = (ROOT / 'CMakeLists.txt').read_text(encoding='utf-8')
        for asset in set(re.findall(r'Assets/[\w_-]+\.png', cmake)):
            with self.subTest(asset=asset):
                self.assertGreater((ROOT / asset).stat().st_size, 100)

    def test_host_and_latency_safeguards(self):
        processor = (ROOT / 'Source/PluginProcessor.cpp').read_text(encoding='utf-8')
        lfo = (ROOT / 'Source/dsp/Lfo.h').read_text(encoding='utf-8')
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
        cmake = (ROOT / 'CMakeLists.txt').read_text(encoding='utf-8')
        self.assertIn('target_sources(BreezeModDspTests PRIVATE tests/DSPTests.cpp)', cmake)
        self.assertIn('add_executable(BreezeModPortableTests tests/PortableTests.cpp)', cmake)
        self.assertIn('add_test(NAME BreezeModPortableTests COMMAND BreezeModPortableTests)', cmake)
        self.assertIn('add_test(NAME BreezeModUiTests COMMAND BreezeModUiTests)', cmake)
        self.assertIn('tests/UITests.cpp', cmake)
        rotary = (ROOT / 'Source/dsp/RotarySim.h').read_text(encoding='utf-8')
        self.assertIn('bool render = true', rotary)
        self.assertIn('hornA = 1.0f - std::exp', rotary)

    def test_msvc_constexpr_and_image_rectangles(self):
        """Regression for MSVC C2131 in Theme.h and C4244 drawing warnings."""
        theme = (ROOT / 'Source/ui/Theme.h').read_text(encoding='utf-8')
        fan = (ROOT / 'Source/ui/FanView.cpp').read_text(encoding='utf-8')
        knob = (ROOT / 'Source/ui/ChromeKnob.cpp').read_text(encoding='utf-8')
        lfo = (ROOT / 'Source/dsp/Lfo.h').read_text(encoding='utf-8')
        self.assertNotRegex(theme, r'constexpr\s+juce::Rectangle')
        for name in ('rotorRect', 'grilleRect', 'hubRect', 'plateRect'):
            self.assertRegex(theme, rf'inline const juce::Rectangle<float> {name}\b')
        self.assertIn('juce::RectanglePlacement::stretchToFit', fan)
        self.assertNotIn('hubRect.getX(),hubRect.getY()', fan)
        self.assertIn('juce::Rectangle<float>{cx-32.0f*scale', knob)
        self.assertIn('static_cast<float>(std::sin', lfo)

    def test_v14_ui_and_modulation_safety(self):
        processor = (ROOT / 'Source/PluginProcessor.cpp').read_text(encoding='utf-8')
        hdr = (ROOT / 'Source/PluginProcessor.h').read_text(encoding='utf-8')
        editor = (ROOT / 'Source/PluginEditor.cpp').read_text(encoding='utf-8')
        rotary = (ROOT / 'Source/dsp/RotarySim.h').read_text(encoding='utf-8')
        tests = (ROOT / 'tests/PortableTests.cpp').read_text(encoding='utf-8')
        cmake = (ROOT / 'CMakeLists.txt').read_text(encoding='utf-8')
        self.assertIn('ModulationSmoother', hdr)
        self.assertIn('vibratoMotion', processor)
        self.assertIn('chorusMotion', processor)
        self.assertIn('tremoloMotion', processor)
        self.assertIn('hornMotion[ch].tick(', rotary)
        self.assertIn('displayRms', hdr)
        self.assertIn('displayPeak', hdr)
        self.assertIn('outputMeter.refresh()', editor)
        self.assertIn('modeInfo.refresh()', editor)
        self.assertIn('StatusPanels.cpp', cmake)
        self.assertIn('smoothingTests()', tests)
        self.assertRegex(processor, r'setProperty\("uiWidth",765,nullptr\)')

    def test_v15_input_safety_and_sync_plates(self):
        processor = (ROOT / 'Source/PluginProcessor.cpp').read_text(encoding='utf-8')
        helper = (ROOT / 'Source/dsp/ModulationMath.h').read_text(encoding='utf-8')
        editor = (ROOT / 'Source/PluginEditor.cpp').read_text(encoding='utf-8')
        tests = (ROOT / 'tests/PortableTests.cpp').read_text(encoding='utf-8')
        self.assertIn('sanitizeAudioSample', helper)
        self.assertIn('sanitizeAudioSample(wetInput[j])', processor)
        self.assertIn('lastPlateSync != s', editor)
        self.assertIn('if (hostSync)', editor)
        self.assertIn('inputSafetyTests()', tests)

    def test_ui_resize_spacing_and_control_readouts(self):
        editor = (ROOT / 'Source/PluginEditor.cpp').read_text(encoding='utf-8')
        knob = (ROOT / 'Source/ui/ChromeKnob.cpp').read_text(encoding='utf-8')
        meter = (ROOT / 'Source/ui/StatusPanels.cpp').read_text(encoding='utf-8')
        scope = (ROOT / 'Source/ui/LfoScope.h').read_text(encoding='utf-8')
        result = re.search(r'const float modeX\[\]=\{([^}]+)\};const float modeW\[\]=\{([^}]+)\};', editor)
        self.assertIsNotNone(result, 'Could not inspect mode-key bounds')
        x = [int(v.strip()) for v in result.group(1).split(',')]
        widths = [int(v.strip()) for v in result.group(2).split(',')]
        self.assertEqual(len(x), 4)
        self.assertEqual(len(widths), 4)
        for i in range(3):
            self.assertGreaterEqual(x[i+1] - x[i] - widths[i], 3,
                                    'Mode keys should have visible separation')
        for win_w, win_h in ((574,590), (765,787), (956,984), (1434,1476)):
            scale_x, scale_y = win_w / 956.0, win_h / 984.0
            for xpos, w in zip(x, widths):
                self.assertGreaterEqual(w * scale_x, 24.0)
                self.assertLessEqual((xpos + w) * scale_x, win_w)
            for bx, by, bw, bh in ((25,743,116,183),(778,743,155,183),
                                    (476,907,239,24),(648,661,57,70)):
                self.assertGreaterEqual(bx*scale_x, 0)
                self.assertGreaterEqual(by*scale_y, 0)
                self.assertLessEqual((bx+bw)*scale_x, win_w)
                self.assertLessEqual((by+bh)*scale_y, win_h)
        self.assertIn('if (lastPlateSync != s)', editor)
        self.assertIn('small && name.isEmpty()', knob)
        self.assertIn('std::strtod', knob)
        self.assertIn('(db + 54.0f)/54.0f', meter)
        self.assertIn('visibilityChanged()', scope)

    def test_min_size_labels_are_not_font_dependent(self):
        editor=(ROOT / 'Source/PluginEditor.cpp').read_text(encoding='utf-8')
        preset=(ROOT / 'Source/ui/PresetBar.cpp').read_text(encoding='utf-8')
        top=(ROOT / 'Source/ui/TopBar.cpp').read_text(encoding='utf-8')
        depth=(ROOT / 'Source/ui/DepthSlider.cpp').read_text(encoding='utf-8')
        for symbol in ('♩', '∿', '∧', '⊓', '⌁', 'Ⅱ', '▶', ' · '):
            self.assertNotIn(symbol,editor)
        for symbol in ('▲', '▼', '▣', '▤', '⤨'):
            self.assertNotIn(symbol,preset)
        for short in ('"VIB"', '"ROT"', '"TRM"', '"CHO"'):
            self.assertIn(short,editor)
        self.assertIn('const char* notes[]={"S","D","T"}',editor)
        self.assertIn('place(saturation,648,624,57,70)',editor)
        self.assertIn('getWidth()/203.0f,getHeight()/64.0f',depth)
        self.assertIn('constexpr float width=203.0f, cy=45.0f',depth)
        self.assertIn('saturation(p,"saturation","",true,true)',editor)
        self.assertIn('"DRY / WET"',top)

    def test_no_deprecated_float_helpers(self):
        sat = (ROOT / 'Source/dsp/Saturator.h').read_text(encoding='utf-8')
        self.assertIn('initProcessing', sat)
        self.assertIn('oversampling->reset()', sat)
        self.assertIn('x += a * (clipped - x)', sat)
        self.assertIn('amount.setTargetValue', sat)


if __name__ == '__main__':
    unittest.main(verbosity=2)
