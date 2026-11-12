# SkellFan — SkellCore integration (v1.7.0)

**Formerly BreezeMod.** One-word product label: **SkellFan**.
Windows plugin outputs are `SkellFan.vst3` and `SkellFan.exe`.
Four modulation modes: Vibrato, Rotary, Tremolo, Chorus.

## SkellCore integration

- CMake pins `skellyruno/skellcore-vst-template` to immutable commit
  `05191f3cfade5a57c1eb97576cce6a348705222c` via FetchContent
  (`SOURCE_SUBDIR SkellCore`). No source-code duplication or floating
  main-branch dependencies.
- `SkellCore::DSP`: the actual `skellcore::TempoClock` is prepared,
  updated from the JUCE host BPM/PPQ, and advanced once per audio block.
  The existing LFO phase-jump detection and four DSP algorithms are retained.
- `SkellCore::UI`: a clickable shared skull-key mark in the top bar cycles
  Ossuary/Neon/Ivory/Obsidian accent palettes. The fan stays retro blue.
  The skin is non-automatable and does not change or process audio.
- The separate `SkellFanCoreTests` suite verifies clock advancement and
  transport state. JUCE UI tests verify that the shared skull toggles
  without affecting a sound parameter.

## Compatibility and migration

To maximize compatibility with old BreezeMod projects, keep **unchanged**:
manufacturer code `SkAu`, plugin code `Bz01`, bundle ID
`audio.skell.breezemod`, the 11 APVTS parameter IDs, and the legacy
ValueTree root tag `BREEZEMOD_PARAMETERS`. Those identifiers deliberately
still contain the old internal name. Preset XML from
`Skell Audio/BreezeMod/Presets` is available in the new preset menu
**without moving or deleting the old files**. New saves are placed in
`Skell Audio/SkellFan/Presets` (the newer folder takes precedence if names
collide). Keep a backup of your old projects/presets while verifying the
rename in FL Studio: hosts may cache plugin names, and installing both
identical-ID binaries side by side is unsupported.

## Build & test

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure
```

Requires a Windows MSVC toolchain and network access for first-time fetch
of JUCE 8.0.12 and pinned SkellCore (or local pre-provisioned sources).
GitHub Actions builds VST3 and Standalone, executes all test suites,
checks the x64 VST3 bundle and publishes `SkellFan-Windows` and
`SkellFan-UI-Snapshots` artifacts. This does not replace actual FL Studio
load/unload, automation, listening and hardware CPU tests.

---

## Previous BreezeMod development notes (historical)

# BreezeMod v1.6 — safer modulation DSP and cleaner fan UI (November 10, 2026)

Preserves all 11 APVTS parameter IDs, factory/user preset formats, and Skell
plugin identity; designed for Windows VST3 and Standalone.

## New in v1.6

- Unity-preserving linear dry/wet blend prevents the +3 dB gain boost of
  equal-power mixing when dry and wet paths are correlated (e.g. 0% Tremolo).
- Fractional delay now bounds cubic interpolation by adjacent samples:
  sharp transitions no longer synthesize out-of-range transient peaks.
- Top-bar WIDTH and MIX show their percentages, the stereo meter uses 0 dBFS
  at the top of the scale, and mode buttons have clearer spacing/labels.
- Cached control face and selection now avoid needless component reordering.
  Hidden LFO monitor no longer runs a 30 Hz repaint timer.
- Numeric-entry fields reject invalid/non-finite values; buttons/knobs support
  keyboard focus and presets have informative hover tooltips.
- Tests include randomized interpolation, amplitude invariants, UI bounds at
  four window sizes, and Windows oversampler latency/CPU stress measurements.
- GitHub Actions now outputs a directly usable VST3/Standalone archive with a
  Windows x64 bundle check and modern Node 24 action runtimes.

This version preserves the original fan artwork, plugin identity, 11 automatable
parameters and factory/user preset formats.

## New in v1.5

- **DSP safety:** reject NaN and infinite samples from upstream effects before
  they contaminate the delay, saturation, rotary, or meter history. All finite
  input samples, including signals above 0 dBFS, remain untouched.
- **Tempo-sync UI:** manual RATE ticks now change into musical divisions in
  HOST mode; the static plate cache refreshes only on mode changes.
- **Visual refinement:** restrained BreezeMod/Skell wordmark in the top bar and
  a subtle blue ambient glow behind the fan, both vector-rendered.
- **Performance:** retain idle animation throttling, lock-free metering and
  allocation-free DSP with no new audio-thread memory allocation.

**Built on v1.3** and preserves the same manufacturer/plugin identifiers and 11
APVTS parameter IDs, so existing host automation and saved parameters remain
compatible. This package includes source and original PNG assets, not a compiled
binary. The static preview is a *layout approximation*, not a host screenshot.

## Previously in v1.4

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

## BreezeMod v1.3 — MSVC build fix (October 21, 2026)

A Windows CI log reported MSVC **C2131** at `Source/ui/Theme.h` lines 11–14.
Those were `constexpr juce::Rectangle<float>` objects. JUCE 8.0.12's rectangle
constructor is not `constexpr` in that MSVC build, so compilation failed in
every translation unit that included this header. Changed them to C++17
`inline const juce::Rectangle<float>` (immutable shared constants, no
compile-time constructor requirement). Also changed fan-layer image drawing
to float destination rectangles and made the knob tooltip use a float
rectangle; made the LFO's double-to-float conversion explicit. Added an
offline regression test to block reintroducing this issue.

**Validation:** offline C++ and Python tests can run without JUCE; the full
Windows VST3/Standalone build must still run on the supplied GitHub Actions
workflow. No verified plugin binary is included.

# BreezeMod v1.2 — four-mode fan-inspired modulation

JUCE 8/C++17 stereo audio-effect plugin created from the user-supplied *compiled StopDial* CMake and GitHub Actions template. Outputs **VST3** and **Standalone**. UI design resolution 956 × 984 with constrained aspect ratio.

## Build on Windows (same workflow as the proven template)

1. Extract the ZIP and upload the **contents of the `BreezeMod/` folder** to the root of your GitHub repository, including its `.github/` directory.
2. Select **Actions → Build BreezeMod → Run workflow** (or use the commands below).
3. Download the artifact **BreezeMod-Windows**. It includes `BreezeMod.vst3` and the standalone executable.

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure
```

By default, CMake fetches the **same pinned JUCE 8.0.12** as the original working template. To avoid a fetch, pass `-DJUCE_PATH=C:/path/to/JUCE` to the Configure command. Needs MSVC, CMake 3.22+, and internet for the first JUCE download if there is no checkout.

## DSP

- **Vibrato**: 4 ms centre fractional delay, ±3 ms excursion at maximum depth, 4-point Hermite interpolation; dry delay alignment while in Vibrato mode.
- **Rotary**: 800 Hz band split; separate acceleration for horn/drum, 0.7× drum rate, Doppler and amplitude changes.
- **Tremolo**: LFO amplitude modulation with width-driven L/R phase shift.
- **Chorus**: three independently phased modulated delays at 12/16/20 ms and stereo width panning.
- **LFO**: 64-bit phase; manual rate 0–32 Hz (smoothed) or PPQ-synchronised musical divisions (exact clock). Host phase locks only on transport start, division changes, or detected seek/loop; it is not needlessly reset every block. Dotted/triplet selectors, polyBLEP wave smoothing, live phase and BPM reporting.
- **Saturation**: 2× JUCE IIR oversampling before modulation; 0% saturation bypasses the nonlinear function (filters stay active to preserve constant host latency). At nonzero drive: smoothed `tanh` clipping, DC blocking, and reference-level gain compensation. Oversampling's integer latency is reported to the host and dry path compensated.
- **Mix/power**: equal-power dry/wet crossfades, a unit-normalized, interruption-safe 20 ms four-way mode fader, smoothed bypass and controls. At 0% MIX, all four modes use the *same* dry delay, including Vibrato; Vibrato's extra dry compensation progressively engages only with the wet signal.
- **Performance improvements (v1.2)**: rotary speed/inertia coefficients precomputed at sample-rate setup, inactive rotary histories kept warm while skipping its expensive fractional taps; chorus pan coefficients and dry/wet equal-power gains calculate trigonometry at block boundaries and are then smoothed sample-by-sample. Saturation DC-block amount is smoothed per sample, not only per block; no dynamic allocations in the process callback.
- **High-rate sync**: the LFO supports the highest dotted/triplet sync rates at up to 400 BPM without the previous 128 Hz rate clamp (new bound 256 Hz).

**Important:** the fan is only an animation. The pause-animation button does *not* stop the LFO or audio. The graphic smoothly spins down on bypass.

## Project controls

`power`, `mode`, `depth`, `rate`, `syncMode`, `syncDiv`, `noteType`, `wave`, `saturation`, `width`, `mix` are APVTS automatable parameters. Mode, sync and note buttons are backed by ComboBox attachments, knobs and depth by Slider attachments, and power by Button attachment. All window and visual preferences live in the APVTS state as nonautomatable properties.

Factory presets are accessible from the top preset field and the arrows. Custom presets are written as `*.xml` in the user's application data folder under `Skell Audio/BreezeMod/Presets` with a confirmation before deletion. All non-factory presets can be renamed by saving them again. The shuffle button randomises values within musical ranges.

You can right-click a knob for numeric entry or change its drag style; double click for parameter default; Shift-drag for precision. The oscilloscope toggle is at the upper left. A live status strip below the control plate displays the manual LFO rate or host division, dotted/triplet selection, and BPM. Pausing the fan drops animation refresh to 12 FPS without affecting audio.

## Graphics

The supplied fan screenshot was used strictly as a photographic layout reference and as the initial *fan body texture*. The third-party wordmark was removed from the generated body PNG; the logo drawn by the editor says `BREEZEMOD`. Rotating blades, grilles, caps, buttons and knobs are original layered assets generated procedurally. All included PNG files are part of the project's `BinaryData` resource target and do not rely on disk files at runtime. The source asset script `scripts/generate_assets.py` requires Pillow, NumPy and OpenCV if you want to regenerate assets, but **these Python dependencies are not needed for building the plugin**.

Fonts use the host's system sans typeface as in the original StopDial project; no proprietary font files are included.

## Build validation

Windows CI confirms both v1.6 DSP and UI-polish stages build VST3 and
Standalone, pass source checks and JUCE DSP/portable C++ suites, and produce
x64 Windows bundle artifacts without MSVC warnings. The included JUCE
UI/processor integration suite also exercises the actual editor, validates
window geometry, exports PNG snapshots at four sizes, tests a clickable mode
button and preset state roundtrip, and times each real processing mode.
These automated checks do not substitute for FL Studio host validation or
guarantee true-peak limiting for arbitrary input.

Build via Actions > Build BreezeMod. The workflow runs three test suites,
checks the Windows x64 VST3 binary, and publishes two artifacts:
BreezeMod-Windows (direct VST3 and exe, no inner ZIP) and
BreezeMod-UI-Snapshots (real offscreen JUCE editor renderings).
