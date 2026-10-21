# BreezeMod changelog

## v1.2.0 — 2026-10-21

### Build and regression testing
- Fixed a real CMake linker blocker: `tests/DSPTests.cpp` was not added to
  `BreezeModDspTests`. Both JUCE and JUCE-free tests now have declared sources.
- Added `BreezeModPortableTests` (pure C++17) covering PPQ timing, note values,
  exact zero-mix dry transparency, rapid re-targeted transitions and random
  mode automation. **18 checks passed under local GCC**.
- Expanded JUCE tests for exact zero-delay read and saturation automation
  consistency with 64- and 256-sample callbacks; these require a Windows build.
- Added source sanity check to GitHub Actions before the JUCE configuration.

### Audio
- Replaced the hand-coded four-way effect crossfade with a JUCE-independent
  `ModeFader` with interruption-safe, unity-normalized sample-by-sample gains.
- Fixed an audible mismatch: switching to Vibrato at MIX=0 had added 4 ms of
  dry delay. Full-dry output is now identical across modes, and compensation
  ramps up with the wet mix.
- Saturation's DC-filter blend now follows the smoothed parameter **per sample**
  instead of jumping to the end-of-block value. This is designed to eliminate
  audible buffer-size dependence during fast automation.
- Equal-power dry/wet trigonometry and chorus width panning coefficients are
  now computed once per block and smoothly interpolated per sample.
- Rotary inertia coefficients are computed in `prepareToPlay`, not exp() twice
  every single sample; inactive rotary keeps its states warm but skips reads.
- Allows exact zero latency dry delay when the oversampling stage reports 0.
- Increased syncable LFO's bound to 256 Hz to handle high host BPM with 1/64
  triplets instead of silently capping host sync to 128 Hz.

### Validation status
- Verified: source schema and assets, GCC 17 portable control-math tests.
- Pending: full JUCE VST3 + Standalone compilation, JUCE DSP tests, FL Studio
  lifecycle and listening tests. No verified binary is included in this ZIP.

## v1.1.0 — 2026-10-21

### Audio engine
- Corrected the 0% saturation case: the nonlinear transfer is now neutral and
  avoided entirely at zero. The oversampling filter remains active to keep
  a stable plugin latency during saturation automation.
- Per-oversampled-frame saturation automation smoothing and DC suppression.
- Reworked host-synced LFO to follow BPM exactly and only hard-sync phase when
  playback starts, the user changes note divisions, or a seek/loop is detected.
  Eliminates repeated phase resets every audio block.
- Reworked mode switching to fade from a distribution of currently audible modes,
  rather than resetting to the previous target when switching rapidly.
- Chorus voice weights are normalized, reducing loudness spikes at narrow widths.
- Unused vibrato/chorus/tremolo interpolations are skipped when not fading.
- Rate for rotary and other DSP comes from the actual LFO phase increment rather
  than a second, independently smoothed rate parameter.

### Interface
- Added a live rate information strip: manual Hz, or host note division, note
  type, and host BPM.
- Knob popup values now show musical divisions and waveform names, rather than
  internal indexes.
- Fan animation dynamically idles at 12 FPS when still/paused and returns
  to 60 FPS when moving; audio keeps running independently.
- Preset navigation now follows an externally restored factory preset name.

### Engineering / QA
- Version bumped to 1.1.0; 11 existing automatable parameter identifiers remain
  unchanged for compatibility with previous sessions.
- Added JUCE DSP smoke tests (LFO, delay, saturation, rotary) and CTest to Windows
  Actions. Standalone packaging now fails explicitly if the executable is absent.
- Added five standard-library source consistency tests, executed locally.

**Verification:** source checks passed. No JUCE compile or in-DAW host test could
be performed in this offline environment. Treat this as an unverified source
update until the Windows workflow and actual FL Studio validation pass.
