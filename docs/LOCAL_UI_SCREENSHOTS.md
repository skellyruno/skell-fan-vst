# Skell Fan — real local JUCE Quick4 🩻

Local-only source target: `SkellFanQuick4` enabled by `-DSKELLFAN_BUILD_UI_QUICK4=ON` in `CMakeLists.txt`. It constructs **the actual SkellFan processor and editor**, not a mockup, and runs under Xvfb with pinned JUCE and SkellCore. The Windows VST3 release is unchanged. Source: `tests/Quick4.cpp`.

Default four scenes are `main_default`, `rotary_controls`, `active_processing`, and `theme_alternate` (the real skull's theme cycle, not an invented color). `--full` adds the compact/large/max windows and one additional real mode. All screenshots are from `createComponentSnapshot`, and the script checks control bounds, image validity, colour richness and manifest. The active scene processes a real audio buffer rather than painting a fake meter.

Local build steps (with pinned sources already available):

```bash
cmake -S . -B build-ui -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DJUCE_PATH=/path/to/JUCE-8.0.12 \
  -DFETCHCONTENT_SOURCE_DIR_SKELLCORE=/path/to/pinned/skellcore-repo \
  -DSKELLFAN_BUILD_TESTS=OFF -DSKELLFAN_BUILD_UI_QUICK4=ON
cmake --build build-ui --target SkellFanQuick4 --parallel 1
xvfb-run -a SkellFanQuick4 --output /tmp/skellfan-quick4
```

Use `skellyruno/skellcore-vst-template/tools/quick4_projects.py` as the shared dispatcher. Until a compiler run proves this target with its exact pinned SkellCore revision and four valid screenshots, its status is **wired, NOT verified**. Do not present old CI snapshots as current. Windows/FL Studio/VST3/DSP tests remain independent.

## 2026-11-13 local test results
A real Linux JUCE `SkellFanQuick4` executable built and ran successfully, returning **4/4 screenshots, zero errors, 2.89 seconds** for capture (not compilation). Verified `main_default`, `rotary_controls`, real audio-driven `active_processing`, and `theme_alternate`. The initial alternate scene showed no theme change; fixed the source to invoke the existing skull `onClick` callback and verified its header image differs at the pixel level. **IMPORTANT:** This local compiler test used a *compatible* shared SkellCore source checkout rather than the plugin's exact pinned commit `05191f3cfade5a57c1eb97576cce6a348705222c`. Don't call the pinned-release appearance certified until the exact pinned shared dependency is tested. Windows VST3 tests remain separate.
