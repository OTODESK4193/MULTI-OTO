# MULTI-OTO

![Release](https://img.shields.io/badge/release-v1.1.0-blue)
![License](https://img.shields.io/badge/license-AGPLv3-green)
![JUCE](https://img.shields.io/badge/JUCE-8.0.8-blue)
![Platform](https://img.shields.io/badge/platform-Windows%20-lightgrey)
![Downloads](https://img.shields.io/github/downloads/OTODESK4193/MULTI-OTO/total.svg)

##
<img src="Source/Assets/Screenshot.jpg" width="600">

<img src="Source/Assets/Mod.jpg" width="600">

## Overview

**MULTI-OTO** is an open-source multiband dynamics and saturation VST3 plugin that cascades up to **128 multiband compression nodes in series**.

It is not a mixing or mastering utility — it is a **sound design tool**. Up to about 8 nodes it behaves like a conventional OTT, adding weight and brightness. Past 16 nodes it turns into an artifact generator, and the design deliberately allows the sound to break.

Built for Color Bass, Riddim and Neurofunk: extracting microscopic texture, generating infinite spectral sweeps, and producing phase-dispersion glitch effects that ordinary dynamics processors cannot reach.

👉 **[Demo video on X (動作デモ動画)](https://x.com/kijyoumusic/status/2055442936884826283?s=20)**

## Key Features

### Extreme Cascade Architecture

Selectable cascade count (2, 4, 8, 16, 32, 64, 128). Stage 1 owns the first half of the chain, Stage 2 the second half, each with its own settings.

### Independent Per-Stage Crossovers *(new in 1.1.0)*

Stage 1 and Stage 2 have **completely separate band splits**. Setting different crossovers per stage drops one stage's boundary inside the other's band, producing phase interference and resonance from the overlap. Band boundaries can also be dragged directly on the meters.

### True OTT Dynamics Engine

Simultaneous upward and downward compression per band, with RMS envelope detection and upward expansion capped at +36 dB so the signature tail can breathe without hitting a wall.

### Phase Dispersion & Glitch Textures

**COLOR PHASE** passes the signal through up to 128 uncompensated Linkwitz-Riley crossovers, accumulating massive group delay that stretches transients into laser sweeps. **ALIGN PHASE** routes the dry signal through the same phase rotation so Dry/Wet stays a clean blend.

### Pre-Drive ADAA Saturation

C2-continuous polynomial soft clipper with anti-derivative antialiasing. Independent Drive, Odd and Even harmonic controls.

### Modulation Matrix *(new in 1.1.0)*

Open it from the **MOD** button in the header.

**Sources.** Four LFOs, each with seven waves — Sine / Triangle / Saw / Square / S&H / Chaos / **Rnd Trig** — running free or locked to host tempo from 1/1 down to 1/32. Two more sources replace the velocity and note that an effect never receives:

| Source | Behaviour |
|---|---|
| **Env Follow** | Tracks the input level. Unipolar, so bands only move while something is playing |
| **Drift** | Continuous randomness that *never steps*. Picks a new target about once a second and eases toward it over 0.4 s |

The three random behaviours are deliberately distinct: **S&H** steps on every LFO cycle, **Rnd Trig** steps on only half of them (so the rhythm stalls and stutters), and **Drift** never steps at all.

**Destinations.** Eight slots route any source to 30 targets — both stages' Time, crossovers and Mix, all six band gains, all twelve band attacks and releases, plus **the LFO rates themselves**, so one LFO can stretch another's period and break out of simple repetition. An LFO can even modulate its own rate. With that many targets the picker is a categorised tree rather than one long list.

Frequencies and times modulate **exponentially** (±2 to ±3 octaves) rather than by addition. Adding ±500 Hz to an 88 Hz crossover would collapse the downward side; ±2 octaves swings symmetrically from 22 Hz to 352 Hz. Band gain is the special case: because it compounds with the node count, its modulation depth is divided by that count so the sweep is always ±12 dB across the whole cascade — otherwise ±3 dB at 128 nodes would mean ±384 dB.

Every modulated control shows its live range directly on the ring, with a marker at the current position. The display runs through the same function the DSP uses, so what you see is what you hear.

**MOD survives preset changes.** The matrix is treated as part of your working environment rather than the patch, so switching presets never wipes the modulation you built. The MOD page has its own **RESET** button when you do want to clear it.

### Musical Randomiser *(new in 1.1.0)*

The **RANDOM** button in the header rerolls the main panel — but not uniformly, which would only ever produce noise. Band gains are drawn around the correct per-node value for your current cascade count (so 128 nodes never asks for +1024 dB), frequencies and times are log-uniform, depth and up/down are derived from a single intensity macro so they stay coherent, releases are always longer than attacks and shorter in the highs to keep the signature spectral sweep, and the two stages always land on different crossovers. Output gain scales down automatically with the node count.

Cascade count, phase mode, limiter settings, colour theme and the MOD matrix are left alone — those are structural choices, and RANDOM fills in the character around them.

### Preset Browser & Config

30 factory presets across Basic / Bass / Texture / Destroy / Drive / Utility, plus user preset saving. Ten colour themes that recolour the knobs and meters as well as the background, and a configurable limiter (LIMIT / CLIP mode, ceiling, release).

### AVX2 SIMD Engine

`DynamicsNode` and `ADAASaturator` are written in raw AVX2 intrinsics, processing 8 lanes at once, with vectorised fast `exp2` / `log2` for the decibel conversions.

## What's New in 1.1.0

**Fixed — the silence bug.** In 1.0.0, once the signal overflowed to `Inf`, the envelope state latched to `NaN` and the limiter envelope latched to `Inf`, permanently multiplying the output by zero with no way to recover. All node and limiter states are now sanitised and clamped, and every parameter is smoothed over 20 ms so knob moves cannot spike the cascade.

**Fixed — Drive / ODD / EVEN were effectively frozen.** Their smoothers advanced one sample per block instead of one per sample, so a knob move took roughly 25 seconds to arrive. They now respond in 0.05 s.

**Fixed — ALIGN PHASE did nothing.** The dry path was a no-op, so both phase modes sounded identical. It is now implemented properly.

**Fixed — crossover reconstruction error.** The low band was missing the high crossover's allpass, so the three bands did not sum flat. That error compounded across every stage. Corrected.

**New** — modulation matrix (4 LFOs + Env Follow + Drift, 8 slots, 30 destinations with a categorised tree picker, live range display on every target control), musical RANDOM button, independent per-stage crossovers with LINK, meter boundary dragging, preset browser with 30 factory presets, CONFIG panel with ten colour themes and limiter mode/ceiling/release.

**Hardened** — removed a heap allocation from the audio thread (MOD parameter IDs were being concatenated per block), cached all 108 parameter pointers at construction, added an alignment static-assert on the AVX2 SIMD path, and declared a 4 second tail so offline bounces no longer truncate the upward-compression sweep.

**Redesigned GUI** — knob diameter roughly doubled (31 px → 65 px), both stages visible at once in a 3 × 5 band matrix, window reduced from 880 × 750 to 880 × 620.

## System Requirements

* **OS**: Windows 10 / 11 (64-bit)
* **CPU**: AVX2 required (Intel Haswell 2013+ / AMD Excavator 2015+)
* **Format**: VST3, Standalone
* **Tested host**: Ableton Live 11 / 12

⚠️ Compiled and optimised exclusively for Windows. Verified in Ableton Live only. Other DAWs are unverified and unsupported.

## Installation

1. Download the latest `MULTI-OTO.vst3` from [Releases](https://github.com/OTODESK4193/MULTI-OTO/releases/latest).
2. Copy the **whole `MULTI-OTO.vst3` folder** (it is a bundle, not a single file) to `C:\Program Files\Common Files\VST3`.
3. Rescan plugins in your DAW.

## Building from Source

Requires JUCE 8 at `C:/JUCE` and Visual Studio 2022 or newer.

```
cmake -B out/build/x64-Release -G "Visual Studio 17 2022" -A x64 .
cmake --build out/build/x64-Release --config Release
```

`-A x64` is required — the Visual Studio generator defaults to Win32, which produces a plugin no 64-bit host can load.

## User Guide

* [日本語マニュアル](./Source/Assets/MULTI-OTO_UserManual_JP.md)
* [English manual](./Source/Assets/MULTI-OTO_UserManual_EN.md)

## Disclaimer & Stability

This software is provided "as is", without any warranty. Cascading 128 multiband compressors can produce extreme output levels depending on your settings. The built-in ceiling is always active, but **always place a limiter after MULTI-OTO** and watch your monitoring volume to protect your hearing and your speakers.

## License

GNU Affero General Public License v3.0 (AGPLv3) — see [LICENSE](LICENSE).
Built with the **JUCE 8** framework; distributed under AGPLv3 in accordance with JUCE 8's open-source licensing terms.

## Credits

**Developer**: [@kijyoumusic](https://x.com/kijyoumusic) (OTODESK)
**Target DAW**: Ableton Live 11+
**Framework**: JUCE 8.0.8
