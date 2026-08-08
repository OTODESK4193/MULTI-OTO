# MULTI-OTO User Manual

**Version 1.1.0** / Windows (64-bit) / VST3 & Standalone
By OTODESK ([@kijyoumusic](https://x.com/kijyoumusic))

---

## 1. Introduction

MULTI-OTO cascades up to **128 multiband compression nodes in series**.

On the surface it is a multiband upward/downward compressor with ADAA (Anti-Derivative Antialiasing) saturation. In practice it is a **sound design tool**, not a mixing or mastering utility.

- **Up to about 8 nodes** — conventional OTT territory. Adds weight, brightness and density.
- **16 nodes and above** — the plugin becomes an artifact and noise generator. **It is designed to let the sound break.**

---

## 2. Signal Flow

```
Input
 -> Pre-Drive (ADAA saturation + DC blocker)
 -> [ Crossover -> Upward/Downward Compression -> Resynthesis ] x (2 to 128 cascades)
 -> Post Filters (HPF / LPF)
 -> Dry/Wet blend
 -> Master Limiter (LIMIT / CLIP)
 -> Output
```

**STAGE 1** handles the first half of the cascade and **STAGE 2** the second half. With COUNT at 64, Stage 1 owns nodes 1–32 and Stage 2 owns nodes 33–64. Every node inside a stage shares that stage's settings.

---

## 3. Interface

### Header

| Control | Description |
|---|---|
| **MOD** | LFO modulation matrix |
| **CONFIG** | Colour theme and limiter settings |
| **PRESET** | Preset browser |

The currently loaded preset name is shown to the left of the buttons.

### Top: Dynamics Meters

Real-time display for Stage 1 and Stage 2. Band widths follow the crossover frequencies on a logarithmic scale.

**Mouse actions**

| Action | Effect |
|---|---|
| Drag the **upper half** of a band | UPWARD amount |
| Drag the **lower half** of a band | DOWNWARD amount |
| Shift or Alt + drag | BAND GAIN |
| Double-click | Reset GAIN to 0 dB |
| Drag a **band boundary** (↔ cursor) | Crossover frequency |
| Alt + drag | Fine adjust (1/6 sensitivity) |

### Middle: PRE-DRIVE / MASTER

| Control | Range | Description |
|---|---|---|
| **PRE-DRIVE** (button) | on/off | Enables the saturation block. Also engages a 17 kHz low-pass |
| **IN** | -24 to +24 dB | Input gain |
| **DRIVE** | 0–100 | Amount fed into the ADAA saturator |
| **ODD** | 0–100 | Odd harmonics — aggressive, square-wave bite |
| **EVEN** | 0–100 | Even harmonics — warm, asymmetrical thickness |
| **HPF** | 20–1000 Hz | Post-cascade high-pass |
| **LPF** | 1k–20k Hz | Post-cascade low-pass |
| **DRY/WET** | 0–100 % | Blend with the original signal |
| **OUT** | -24 to +24 dB | Output gain |
| **CEILING** | -2.0 to -0.1 dB | Limiter ceiling |
| **OTT xN** | 2–128 | Total cascade count. The heart of the plugin |
| **PHASE MODE** | COLOR / ALIGN | See below |

### Bottom: STAGE 1 / STAGE 2

Both stages are shown side by side. Each is a **3 × 5 matrix** — rows LOW / MID / HIGH, columns GAIN / UP / DN / ATK / REL.

| Control | Range | Description |
|---|---|---|
| **ON** | on/off | Bypass the whole stage |
| **TIME** | 10–1000 % | Multiplier over every attack and release in the stage. Lower is faster |
| **MIX** | 0–100 % | Per-node parallel amount (equivalent to Xfer OTT's "Depth") |
| **LOW X** | 20–1000 Hz | LOW / MID split point |
| **HIGH X** | 1k–20k Hz | MID / HIGH split point |
| **LINK** (Stage 2) | on/off | When on, moving Stage 1's LOW X / HIGH X also moves Stage 2's. An editing convenience only — it has no direct effect on the sound |
| **GAIN** | -24 to +24 dB | Per-band makeup gain |
| **UP** | 0–100 % | Upward compression amount |
| **DN** | 0–100 % | Downward compression amount |
| **ATK** | 0.1–100 ms | Attack time |
| **REL** | 1–1000 ms | Release time |

> **Important change in v1.1.0**
> Stage 1 and Stage 2 crossovers are now **fully independent**. Setting different splits per stage drops one stage's boundary inside the other stage's band, producing complex phase interference and resonance from the overlap. This is what separates MULTI-OTO from a plain serial OTT chain.

---

## 4. How It Behaves

### GAIN compounds with the node count

Band gain is applied **at every node**, so the cascade total is `GAIN × count`.

| Count | Per node | Total |
|---|---|---|
| x2 | +8 dB | +16 dB |
| x8 | +3 dB | +24 dB |
| x32 | +1.4 dB | +45 dB |
| x128 | +0.5 dB | +64 dB |

Setting +8 dB at 128 nodes would ask for +1024 dB, well past what a 32-bit float can hold (about +770 dB). Always lower GAIN as you raise the count. The factory presets follow this ratio.

### Upward compression builds the tail

Signals below the -40 dB threshold are lifted by up to +36 dB. Through 128 nodes, the reverb tails and noise floor left after a sound stops get amplified stage by stage into the long "schwaaa" sweep MULTI-OTO is known for.

Offsetting REL per band (for example REL H at 40 ms and REL L at 250 ms) makes that tail sweep across the spectrum.

### PHASE MODE

This switch only matters when DRY/WET is below 100 %.

- **COLOR PHASE** — the dry signal passes through untouched while the wet path accumulates crossover phase rotation. Mixing them causes cancellation, so DRY/WET behaves as a **comb-filter colour control** rather than a blend.
- **ALIGN PHASE** — the dry signal is passed through the same crossover split and summed straight back, giving it identical phase rotation without any compression. Nothing cancels, so DRY/WET works as a **plain blend**. Use this when you want to keep the core of the original and just dial in the effect.

At 100 % wet the two modes are identical.

---

## 5. CONFIG

### COLOR THEME

Ten GUI colour schemes. No effect on audio. Themes change not just the background but **the knob accents and meter band colours as well**.

`Midnight` / `Sakura` / `Ocean` / `Forest` / `Sunset` / `Mono` / `Neon` / `Amber` / `Ice` / `Vapor`

Mono also converts the band colours to greyscale. Your theme choice is never overwritten by presets.

### LIMITER

| Item | Description |
|---|---|
| **LIMIT** | Follows the peak envelope and reduces smoothly. RELEASE sets the recovery speed |
| **CLIP** | Folds back instantly at the ceiling. Adds harmonics, increases density |
| **CEILING** | -2.0 to -0.1 dB |
| **RELEASE** | 1–500 ms (LIMIT mode only) |

In both modes the output **never exceeds CEILING**. Cascaded upward compression generates enormous gain — watch your monitoring level.

---

## 6. MOD MATRIX

Open it from the **MOD** button in the header. This drives MULTI-OTO's own parameters from LFOs and the input level.

### Sources

| Source | Description |
|---|---|
| **LFO 1-4** | Independent low-frequency oscillators. Bipolar (-1 to +1) |
| **Env Follow** | The **input level itself**. Unipolar (0 to 1). Lets the bands move only while something is playing |
| **Drift** | **Continuous** randomness that never steps. It picks a new target roughly once a second and eases toward it over 0.4 s. Bipolar |

MULTI-OTO is an effect and receives no MIDI notes, so **Env Follow** takes the place of Velocity and Note.

### LFO

| Item | Description |
|---|---|
| **Wave** | Sine / Triangle / Saw / Square / S&H / Chaos / **Rnd Trig** |
| **SYNC** | On locks to host tempo, off uses Hz |
| **RATE** | 0.01-30 Hz (0.01 Hz is roughly a 100 second cycle), skewed so slow rates are easy to dial |
| **Sync rate** | 1/1 to 1/32, 13 divisions including dotted and triplet |

**Chaos** layers the fundamental with a frequency √2 times higher — an irrational ratio, so the shape never repeats.

**Rnd Trig** only updates its value on **50 % of cycles**. The period stays constant but whether it moves does not, which stalls and stutters like a glitching step sequencer. This is the most powerful wave for artifact work.

#### Telling the three random sources apart

They all produce arbitrary values, but they **move** in completely different ways.

| | Motion | Period | Use for |
|---|---|---|---|
| LFO **S&H** | Steps abruptly | Fixed (set by RATE, can sync) | Switching values in time with the beat |
| LFO **Rnd Trig** | Steps, but **skips half the time** | Fixed period, irregular events | Stuttering, gap-ridden glitches |
| Source **Drift** | **Never steps — it wanders** | No period (direction changes about once a second) | Organic wobble, as if nudged by hand |

S&H and Rnd Trig follow the LFO's RATE, so they **can lock to tempo**. Drift has no concept of sync; it just keeps drifting.

The bar on the right shows each LFO's live value — centre is zero, left and right are ±.

### Matrix (8 slots)

Each slot links `SOURCE → DESTINATION` by `AMOUNT` (-100 to +100 %).

- **POL (UNI)** — off swings both ways (− to +), on swings one way only (0 to +). Use off to wobble around the base value, on to push it in a single direction.
- Assigning several slots to the same destination **sums** them.

### Destinations

| Destination | Modulation depth |
|---|---|
| S1 / S2 **Time** | ±2 octaves (1/4x to 4x) |
| S1 / S2 **Low X / High X** | ±2 octaves |
| S1 / S2 **Mix** | ±50 % |
| Per-band **Atk / Rel** (12) | ±3 octaves (1/8x to 8x) |
| **LFO 1-4 Rate** | ±15 Hz |

Frequencies and times modulate **exponentially** rather than by addition. Adding ±500 Hz to 88 Hz would collapse the downward side; ±2 octaves swings symmetrically from 22 Hz to 352 Hz.

**Modulating one LFO's rate from another** is the heart of this matrix — the period itself stretches and contracts, breaking out of simple repetition. An LFO can even modulate its own rate (it resolves safely with one block of delay).

### Modulation range display

Any knob that is a destination grows a **faint band around its outer edge** — that is the span the current assignment can sweep — with a **bright dot** riding it at the live position. On the TIME / MIX / LOW X / HIGH X bars the span appears as a tinted region with a vertical line for the current value.

The band is computed through `ModMatrix::applyModToValue()`, the same function the DSP uses, so **what you see is exactly what you hear**.

Modulation is computed at block rate and the knobs themselves do not move — the base value is preserved and the offset is applied on the way into the DSP. That is the usual synth convention, and it means you can still edit the base value while modulation is running.

### Starting points

- **LFO1 (Sine, 1/4 sync) → S1 Low X, 40 %** — the low split point rides the beat and the bass centre of gravity sways.
- **LFO2 (Rnd Trig, 1/16 sync) → S1 Rel Hi, 80 %** — the high band's release stretches irregularly and the tail stumbles.
- **Env Follow → S2 Mix, UNI on, 60 %** — stage 2 only bites while the signal is loud.
- **Drift → S2 High X, 35 %** — the high split point wanders aimlessly, so the same phrase never resonates quite the same way twice.
- **LFO3 (Saw, 0.05 Hz) → LFO1 Rate, 50 %** — LFO1 accelerates over 20 seconds.

---

## 7. PRESETS

### Browser

Categories on the left, presets on the right. The search field filters by name and category.

- **Double-click / Enter** — load
- **Right-click** — Load / Delete
- **SAVE** — store every current parameter as a user preset
- **INIT** — reset all parameters to defaults (theme is preserved)

Entries marked **FACTORY** are built into the plugin and cannot be deleted. User presets live at:

```
%APPDATA%\MULTI-OTO\Presets\<Category>\<Name>.motopreset
```

Loading a preset also switches **OTT COUNT** to its recommended value — the `(xNN)` suffix in the preset name. Presets marked x128 are CPU-heavy.

### Factory presets

| Category | Contents |
|---|---|
| **Basic** (6) | Classic OTT / Gentle Glue / Vocal Presence / Drum Punch / Bass Weight / Sub Guardian |
| **Bass** (6) | Riddim Growl / Color Bass Shimmer / Neuro Grit / Tearout Screech / Wobble Enhancer / Hard Clip Lead |
| **Texture** (8) | Droopy Laser / Phase Smear / Metallic Resonator / Granular Dust / Infinite Tail / Spectral Freeze / Split Band Fracture / Ghost Reverb |
| **Destroy** (6) | Pulveriser / White Hot / Digital Meltdown / Screaming Comb / Sub Annihilator / Stutter Chaos |
| **Drive** (3) | Odd Harmonic Stack / Even Warmth / Fuzz Crusher |
| **Utility** (1) | INIT (Bypass) |

---

## 8. Recipes

### A. Conventional use (mix support)

- **COUNT**: 2 or 4
- **PRE-DRIVE**: off, or Drive 10 % / Even 20 %
- **PHASE MODE**: ALIGN PHASE (prevents the hollow comb sound when blending)
- **STAGE 1**: UP / DN around 60–80 %, TIME slowed to about 150 %
- **STAGE 2**: off
- **MIX**: 20–40 % (classic parallel compression)

Start from `Classic OTT (x2)` or `Gentle Glue (x2)`.

### B. Destruction (sound design)

- **COUNT**: 64 or 128
- **PHASE MODE**: COLOR PHASE (128 uncompensated crossovers twist the phase by tens of thousands of degrees, stretching transients into laser-like sweeps)
- **PRE-DRIVE**: Drive 80–100 % / Odd 100 %
- **STAGE 1** (flattener): UP / DN at 100 %, TIME 15–30 %
- **STAGE 2** (texture extractor): LOW UP at 0 % to protect the sub, MID / HIGH at 100 %. Fast REL H, slow REL L
- **Crossovers**: offset the two stages heavily (e.g. S1 = 88 / 2.5k, S2 = 350 / 6k)

Start from `Pulveriser (x128)` or `Split Band Fracture (x32)`.

### C. Metallic resonance

Push LOW X and HIGH X close together (for example 400 Hz / 1.1 kHz) so the narrow mid band rings. Offset the second stage slightly (S2 = 630 / 1.5k) to stack a second resonance on top. That is exactly what `Metallic Resonator (x64)` does.

---

## 9. Troubleshooting

**No sound, or silence after a loud passage**
v1.0.0 had a bug where internal state could be corrupted after clipping and never recover. **Fixed in v1.1.0.** If it still happens, switching OTT COUNT resets all node states.

**Clicks or steps when turning knobs**
v1.1.0 smooths every parameter over 20 ms. If you still hear steps, raise TIME to slow the envelope response.

**High CPU usage**
Lower COUNT. Cost scales almost linearly with node count — x128 is 16 times x8.

**Other DAWs**
Untested. Windows and AVX2 are required.

---

## 10. Requirements

- **OS**: Windows 10 / 11 (64-bit)
- **CPU**: AVX2 required (Intel Haswell 2013+ / AMD Excavator 2015+)
- **Formats**: VST3, Standalone
- **Tested host**: Ableton Live 11 / 12

---

## 11. Disclaimer

This software is provided "as is", without warranty of any kind. Cascading 128 multiband compressors can produce extreme output levels depending on your settings. CEILING is always active, but **place a limiter after MULTI-OTO** and keep an eye on your monitoring volume.

Licence: GNU AGPLv3 (per JUCE 8's open-source licensing terms)
