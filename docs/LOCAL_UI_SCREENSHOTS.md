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
