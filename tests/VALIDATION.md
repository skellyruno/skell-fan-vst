# v1.6 verification matrix (November 10, 2026)

- Stage 1 on Windows GitHub Actions: VST3 and Standalone compilation PASS,
  JUCE DSP tests PASS, portable tests PASS, x64 package upload PASS,
  MSVC warnings NONE. CI run 42818582341.
- Portable GNU C++17 (-Wall -Wextra -Werror): PASS. Clang ASan + UBSan: PASS.
- Mix invariants: correlated ±0.85 inputs show no dry/wet gain boost at any mix;
  endpoints exact. Bounded interpolation prevents 1.25x transient overshoot;
  30k randomized cases stay finite and within adjacent sample bounds.
- UI geometry: static source checks for 574x590, 765x787, 956x984 and
  1434x1476. These do NOT prove the actual JUCE renderer at those sizes.
- Stage 2 Windows CI (42820510227): VST3 + Standalone, Windows x64 packaging,
  JUCE DSP and portable regressions PASS. Reported saturation latency 4 samples;
  measured impulse peak at sample 5 for 44.1/48/96 kHz.
- 2x oversampled saturation CPU on GitHub's Windows runner: 0.566% (44.1 kHz),
  0.614% (48 kHz) and 1.217% (96 kHz) of real-time audio duration.
- Stage 3 additionally runs real BreezeMod processor audio renders, four
  offscreen JUCE editor snapshots, keyboard/mode click interaction, state
  roundtrips, layout containment and whole-plugin CPU checks.
- Outstanding external checks: FL Studio VST3 scan/load/unload, true-peak
  measurements against a reference meter, loudness matching, host automation
  at high stress, GUI mouse/keyboard testing on Windows at minimum size.

---

# v1.4 validation (Nov 6, 2026)

The 23 JUCE-independent C++ checks cover tempo divisions, click-free mode
crossfade continuity, dry-at-zero-mix behavior, and **five new motion-smoothing
regressions** (bounded step rate, settling, reset, nonfinite input, low sample
rate). These tests were run with GCC C++17 and ASan/UBSan.

Eight source consistency tests cover v1.3's MSVC constexpr regression and the
new DSP/UI files, APVTS parameter schema, and CMake registration.

The JUCE SDK is unavailable locally: Windows JUCE compile, plugin binary,
FL Studio runtime and actual CPU usage are **not verified**. Use the supplied
GitHub Action to check those next. The v1.4 mockup is a static approximation.

---

# BreezeMod v1.2 validation matrix for Windows build

Run `python tests/test_source.py` for source consistency. Run `ctest --test-dir build -C Release --output-on-failure` for the JUCE DSP smoke tests. The source suite has six checks; the C++17 portable suite passed 18 checks locally. The JUCE suite is attached to its CMake target; the DSP executable runs in the Windows workflow.

Perform these checks on the GitHub Actions binary:

1. **Project builds** VST3 + Standalone using the unchanged JUCE pin from the prior StopDial template.
2. **FL Studio scan / open / close**: open/close BreezeMod several times; reopen FL Studio projects, check state persists (important after prior wrapper crash reports).
3. **Mono/stereo**: render an impulse and a sine sweep, exercise all four modes without NaN/Inf/DC drift.
4. **Power switch**: repeatedly bypass and enable on sustained 1 kHz sinusoid, listen for clicks and observe fan inertia.
5. **Host sync**: set 120 BPM, select 1/4 straight, then dotted and triplet; confirm rate and host playback PPQ lock even after transport jumps.
6. **Rate=0**: fan spins down and vibrato/tremolo output stops sweeping.
7. **30 Hz modulation**, **sample rates 44.1/48/96 kHz**, and **blocks 64/256/2048**: no buffer overruns or heap allocation on render thread.
8. **Saturation at 0 and 100**: compare loudness, DC offset and reported host latency. Oversampling latency may not be zero.
9. **Mode automation**: rapidly switch modes and the power parameter; fades must not pop.
10. **Preset roundtrip**: save XML, change settings, reload, close/reopen plugin, confirm parameters and UI state.
11. **Resize**: change size from 574×590 through 1434×1476 and confirm all controls remain reachable.
12. **Asset completeness**: Verify embedded artwork PNGs display; rotor rotates behind stationary grille.

Run further host tests before distribution. The build was not runnable inside the source-assembly environment because JUCE could not be fetched.
