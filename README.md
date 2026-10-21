# Disco Sauce 2 Pro  (flowmotionlb)

All-in-one vocal plugin for FL Studio (VST3, Windows). Tuner, doubler, EQ, de-esser, compressor,
saturation, delay, reverb, limiter in one window, with **OPEN MAP** to edit every single parameter.

## Get the plugin (no installs needed)
1. Create a new GitHub repository and upload everything in this folder (keep the folder structure).
2. Open the **Actions** tab. The "Build Disco Sauce 2 Pro" workflow starts by itself (or press *Run workflow*).
3. When it finishes, download the artifact **Disco-Sauce-2-Pro-Windows**. Copy `Disco Sauce 2 Pro.vst3`
   to `C:\Program Files\Common Files\VST3\`, then rescan plugins in FL Studio.
4. The workflow also RUNS the tuning tests (every scale, every key) - if a test fails, the build is red.

If the build fails, copy the red error text from the Actions log and send it to Claude - it is a quick fix.

## Your artwork
The 4K (3840 px) volcano (red and black) Disco Sauce artwork is already in `Assets/disco_sauce_volcano_4k.jpg`; replace it to change the look.
Without it a gold-drip "DISCO SAUCE 2 PRO" header is drawn.

## What is inside
* **Auto-Tune** - 26 scales in any of 12 keys, including quarter-tone Maqam Rast / Bayati / Saba, Hijaz, and a
  **Custom note map** (tick the notes you allow, plus +-50 cent micro-tuning per note). Retune 0 ms = hard T-Pain snap.
  Humanize, note stickiness, transpose, fine tune, A4 reference, detection range, sensitivity and pitch gate.
  Formants are preserved. Latency is fixed (about 41 ms at 48 kHz) and reported to the host.
* **Doubler** knob - 1-4 detuned, delayed, panned voices (delay / detune / width / tone all adjustable).
* EQ (low cut, low shelf, 3 bells, high shelf, high cut), de-esser, compressor (parallel mix), saturation,
  delay (tempo sync, ping-pong), reverb (pre-delay, filters), limiter.
* **Presets**: Init, Crisp Studio, T-Pain, Dark Trap, Disco Pro, **DISCO PRO MAX**. *Preset Amount* blends from clean to the
  full preset. Presets never change Key / Scale / Note Map. Save / Load your own `.dspreset` files.
* **OPEN MAP**: a page listing all 97 parameters with sliders you can type exact values into
  (double-click a slider to reset it). Every parameter is also automatable from FL Studio.

## Tests (run on every build)
`Tests/tune_tests.cpp` - quantizer vs brute force, detector accuracy, all 26 scales, quiet/loud/vibrato/falsetto/bass,
`Tests/fx_tests.cpp` - every effect block incl. random-parameter stress, `Tests/param_tests.cpp` - parameter table and presets,
`Tests/engine_tests.cpp` - whole chain: latency alignment, presets, block-size independence, end-to-end tuning.
