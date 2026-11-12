# SkellFan v1.7.0 — SkellCore integration and rename

- Changed JUCE product, CMake targets, VST3/Standalone artifact names,
  on-screen SKELLFAN wordmark and preset dialogs from BreezeMod to SkellFan.
- Retained original VST3 manufacturer/plugin codes, old bundle ID,
  `BREEZEMOD_PARAMETERS` root, and all 11 APVTS parameter IDs.
- Pinned SkellCore shared DSP/UI modules to an immutable commit.
- Routed host tempo bookkeeping through shared `skellcore::TempoClock`,
  retaining the plugin's LFO/rotary/vibrato/chorus/tremolo/saturation engines.
- Added SkellCore's shared clickable skull mark and non-audio accent cycle.
- Kept old BreezeMod user presets discoverable, read-only in their original
  directory; saves go to the new SkellFan directory.
- Expanded C++/JUCE/UI tests and Windows VST3 CI for renamed targets.
- The shared skull-key geometry is SkellCore's current mark; future master
  Skell brand asset updates should happen centrally in SkellCore.

---

# BreezeMod v1.6 — DSP correctness, performance and UI polish (November 10, 2026)

- Replaced correlated equal-power dry/wet law with unity-preserving linear mix;
  eliminated unused trigonometric gain smoothers from the audio loop.
- Added bounded cubic fractional delay interpolation to eliminate 1.25x
  overshoot from otherwise valid ±1.0 input sequences.
- Expanded portable regression tests with 30,000 randomized interpolation cases,
  extreme finite values, mix automation endpoints and peak stability.
- Added JUCE tests for fractional-delay peak limits, oversampling impulse
  latency, NaN/denormal safety, bounded sine peaks and per-rate CPU timing.
- UI: always-visible top-bar percentages, 0 dBFS meter top, mode-key spacing,
  keyboard focus, preset tooltips, numeric-entry validation and idle scope
  timer savings. Restored HOST/MANUAL state before first editor tick.
- Strengthened Windows workflow: Node 24 actions, VST3 x64 binary check,
  verbose test logs and one-layer artifact packaging with longer retention.
- Preserves the existing 11 parameter IDs and user XML preset formats.
- Added a Windows JUCE editor/processor integration test that renders the actual
  574×590, 765×787, 956×984 and 1434×1476 window sizes; checks child bounds,
  click-select of Rotary, state roundtrip, finite output/peak stability and
  per-mode real-time CPU ratios. Snapshots are published as a CI artifact.
- Added visible keyboard focus outlines for mode keys, knob dials and depth.

---

# BreezeMod v1.5 — UI clarity and audio safety (November 7, 2026)

- Fixed non-finite input propagation into persistent stereo DSP state while
  preserving all ordinary finite samples at their original level.
- Added 3 new portable audio-input guard cases and a ninth source consistency
  test. Updated MSVC CI runs both JUCE DSP and JUCE-free regression tests.
- Added HOST-aware rate face markings, a cached ambient fan halo, and a
  compact wordmark; retained original retro blue fan visual assets.
- Maintains binary compatibility: all 11 existing APVTS parameter IDs and
  manufacturer/plugin codes are unchanged.
- Previous v1.4 source successfully built VST3 and Standalone on Windows;
  v1.5 requires a fresh workflow run before shipping.

---

# BreezeMod v1.4 — Modulation motion + UI polish (November 6, 2026)

**Built on v1.3** and preserves the same manufacturer/plugin identifiers and 11
APVTS parameter IDs, so existing host automation and saved parameters remain
compatible. This package includes source and original PNG assets, not a compiled
binary. The static preview is a *layout approximation*, not a host screenshot.

## New in v1.4

- **Smooth LFO edges**: new allocation-free `ModulationSmoother` with its
  coefficient calculated once per sample rate. Vibrato and chorus glide their
  fractional delay read heads over 1.5–2 ms; square/saw tremolo gains glide
  over 1.4 ms, and rotary horn/drum motion is smoothed over 2–3 ms. The change
  reduces abrupt read-head jumps and modulation zippering; it does not
  guarantee removal of all artifacts at extreme settings.
- **Rotary efficiency**: simple wrap logic replaces per-sample `fmod` for rotor
  phase; rotary inertia remains separate for the horn and drum.
- **Live stereo output meter**: two compact signal columns with RMS levels,
  peak indicators, and clip hold. Values come from lock-free atomics published
  by the audio thread. Readings decay to silence if the host stops processing.
- **Mode/status panel**: current mode, effect description, waveform, host/free
  clock status, and live rate, drawn as vector UI rather than external assets.
- **Clearer interaction**: Depth slider shows its actual value and accent
  track; the preset name gains an asterisk after editing sound parameters; the
  fan-animation toggle displays pause/play appropriately.
- **Smaller window**: new default 765 × 787 (~20% smaller in width/height),
  while respecting the original 956:984 aspect ratio and allowing resize.
  Previously saved window sizes continue to restore.

## Validation and known limitations

- 8 Python source/asset consistency tests pass.
- 23 portable DSP control tests pass with GCC C++17; also run with
  AddressSanitizer and UndefinedBehaviorSanitizer.
- JUCE-based DSP smoke tests are included in CMake/CI but cannot run here
  without the JUCE SDK. **No full VST3/Standalone build or DAW test has been
  verified locally.** Run the Windows GitHub Actions workflow and test the
  resulting plugin in FL Studio, especially automation and preset recall.
- The new smoothing may soften intentionally sharp square/saw movement; it
  trades hard-edged modulation for lower transient artifacts.

---

## v1.3.0 — Windows MSVC build unblock
- Fixed MSVC C2131 when `juce::Rectangle<float>` was declared `constexpr` in `Source/ui/Theme.h`.
- Applied C++17 `inline const` for shared visual rectangles (and inline colors).
- Replaced warning-prone image draw coordinates with floating-point rectangles.
- Removed ChromeKnob tooltip float-to-int implicit conversions and made LFO sine narrowing explicit.
- Added a regression guard to offline source tests for this compiler issue.
- Full Windows build remains unverified until GitHub Actions completes.

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
