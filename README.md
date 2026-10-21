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

This archive contains **source and assets, not a compiled binary**. Six offline source/asset checks passed with `python tests/test_source.py`. The separate JUCE-free `BreezeModPortableTests` was compiled with GCC C++17 and passed **18 checks**, including rapid mode re-targeting, host note divisions, and zero-mix dry transparency. On Windows, CMake builds **both** test targets: `BreezeModPortableTests` and `BreezeModDspTests`. v1.1 accidentally omitted `tests/DSPTests.cpp` from the CMake target, preventing a successful link; v1.2 fixes this. The JUCE DSP executable tests LFO phase, fractional delay including exact zero delay, saturation and buffer-size invariance, and rotary finite output. JUCE-based tests **have not been run here**. The container used to assemble this project had no installed JUCE SDK and could not access GitHub to fetch one, so the actual Windows/FL Studio build and plugin smoke test must be run using GitHub Actions (included). No successful compile is claimed for BreezeMod. Check `build.yml` artifact output, then run the standalone to validate visually, and finally test VST3 scanning in FL Studio.
