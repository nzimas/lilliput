# Lilliput

**An alternative firmware for the M-VAVE FM-1: a no-input mixer. Six channels feeding back into themselves and
each other, through tubes, op-amps, filters, limiters and a rack of effects.**

> [!WARNING]
> Installing Lilliput replaces the firmware M-VAVE ships with the FM-1. It is unofficial, is not made,
> endorsed or supported by M-VAVE, and installing it **may void any warranty that applies to your device**.
> You install it at your own risk; read the [disclaimer](#disclaimer) first.

## What it is

*No-input mixing* plays a mixing desk with nothing plugged into it: its outputs are fed back into its own
inputs, and the desk's noise, amplified round and round, becomes the sound. Lilliput models such a desk on the
FM-1, after the iOS and macOS app **NoInputMixer** (Igor Vasiliev): its channel strip, its buses, its input
selector that makes each kind of loop, its rack of effects and its output. It is written anew for the FM-1, in
fixed point.

- **Six channels**, each a mono channel strip: an input selector, a noise generator, a preamp with a choice of
  tube (12AX7, 12AT7, 6C16, EF86, 6L6, 300B; linear or saturated), an op-amp (TL072, NE5532, 4558, 741, OP07,
  CA3140) with an overload protection that breaks the signal with a crackle, high and low shelves or cuts, a
  peak, notch or band-pass, a brickwall limiter whose look-ahead sets the loop's pitch, two sends, the level,
  the pan.
- **Everything runs a sample at a time**, so a loop's pitch is free from the lowest to the highest.
- **A rack of seven effect slots**: DELAY, FLANGER, CHORUS, TREMOLO, FORMANT, BITCRUSH, GLITCH, each an insert
  on its channel, or fed from a send bus, or from the slot before it, into a return bus.
- **The output**: the returns, an equaliser, an insert, and SPACE, a reverb with six spaces.
- **HOME draws a new patch**: new connections and new settings for the whole mixer, from recipes no-input
  players use. Every patch drawn sounds at once.
- Sixteen factory patches, sixty-four sessions, MIDI CCs, USB audio (the output, or the six channels).

The screen is Tonitor's, made for **low vision**: very few words, in very large type, with high contrast, and
each channel in its own colour. It has a screen reader.

> [!NOTE]
> Lilliput is new. Its sound and its screens have been tested on the computer (see
> [Testing without the FM-1](#testing-without-the-fm-1)); **its CPU load on the FM-1 has not been measured
> yet**. Six channels computed a sample at a time is far more work per sample than Tonitor's blocks: see the
> CPU page (SHIFT + black key 7) when you first run it.

## Installing

Build and install from source ([Building from source](#building-from-source)): with the FM-1 connected by USB
and switched on, `./install.sh`. It installs over whatever the FM-1 runs now — M-VAVE's own firmware, Tonitor,
Bigorna or another alternative firmware. Do not unplug the FM-1 while it writes. `./install.sh --info` says what
the connected FM-1 runs. Switch the FM-1 off and on once after installing.

**If something goes wrong:** hold **OCT−** while switching on for the USB rescue mode (the screen says
RESCUE), then install again. Two failed starts in a row also enter it. Holding **OCT− and OCT+** for five
seconds enters the chip's own updater (UBOOT). If the FM-1 no longer starts at all, recovery needs
[FM-1-transporter](https://github.com/kurogedelic/FM-1-transporter).

**Sessions and other firmwares.** Lilliput keeps its sessions in the flash area Tonitor, Bigorna and SLOOP use.
It does not read theirs, and **it leaves their data alone**: it writes over another firmware's data only when a
save finds no other room. (Tonitor does not return the courtesy: Tonitor blanks what it does not recognise, so
going back to Tonitor erases Lilliput's sessions.)

# Manual

## Playing it

| Control | What it does |
| --- | --- |
| **6 top buttons** | choose channel **1–6** (lit: chosen; glowing: sounding). On a channel's page, another channel shows the same page; the chosen channel again: its main view. From any other page, back to the channel. |
| **SELECT** knob | the chosen channel's **LEVEL**: its fader. In most loops it is what makes them sing. |
| **ALGORITHM** knob | its **PAN**. In a loop through the mix bus it is part of the loop. |
| **KNOB 1–4** | on the main view: **GAIN**, **FREQ** (the middle filter's), **LIMIT** (the loop's pitch), **SEND 1**; with **SHIFT**: **NOISE**, **MID** (the middle filter's gain or width), **PROTECT** (the overload protection), **SEND 2**. A dial covers the screen while you turn. On a page: its four rows. |
| **OCT− / OCT+** | the channel's pages: MAIN, AMP, NOISE, EQ, SEND, FX (on the output's pages: OUT, OUT EQ, OUT FX). Their lights: a page that way. |
| **SEQ** | the output's pages: OUT, OUT EQ, OUT FX, then back. |
| **ARP** | the **OVERVIEW**: the six channels, their inputs and levels at once. |
| **HOME** | a **new patch**: the whole mixer drawn again (see [New patches](#new-patches)). |
| **SHIFT + HOME** | the patch **nudged**: every channel's tone moved a little, its connections and levels kept. |
| **PRESETS** knob | the factory's patches; **SAVE** loads the one shown. |
| **SAVE** held | save the session: over the one loaded, at once; otherwise a new one (asked: PRESETS chooses NO or YES, SAVE answers). |
| **SHIFT + SAVE** | the **SESSIONS** page: PRESETS chooses, SAVE loads. Held 0.8 s: [GLOBAL SETUP](#global-setup-and-midi). |
| **PLAY** | the output muted, or back (it blinks while muted). The loops keep going, unheard. |
| **White keys 1–15** | the chosen channel's **INPUT**: PRE, POST, MIX L, MIX R, SEND 1, SEND 2, RET 1, RET 2, NONE, CH 1 … CH 6 (lit: the one it has). |
| **White key 16** | its **MAIN MIX** on or off. |
| **Black keys 1–6** | mute channels 1–6 (lit: muted). |
| **SHIFT + black key 7**, held | the CPU page. |
| **SHIFT + black key 8** | the screen reader on or off. |
| **Black key 11** (last) | **SHIFT**. |
| **MASTER** knob | the level after everything. |
| **OCT− and OCT+**, held 5 s | UBOOT, the chip's own updater. |

## The main view

The chosen channel's number, very large, in its colour; under it its **input**; under that the **patch's name**
(or a message, black on white; or MUTED, white on red); its **meter**, with a white bar where its fader is
(red at the left when the fader is down); its **pan** along the bottom.

## A channel

Left to right, as the app's block diagram:

**input selector → + noise → GAIN → tube → op-amp (and its overload protection) → HIGH, LOW → middle filter →
brickwall limiter → mute → SEND 1, SEND 2 → LEVEL → insert effect → PAN → main mix**

| Page | Rows (KNOB 1–4) | With SHIFT |
| --- | --- | --- |
| **AMP** | TUBE, MODE (LINEAR, SATURATED), GAIN (0 … +48 dB), OP-AMP | |
| **NOISE** | NOISE (off, −90 … −18 dB; GEIGER: its clicks' rate), TYPE (WHITE, PINK, RED, CRACKLE, GEIGER, HUM), LIMIT, PROTECT | |
| **EQ** | HIGH and LOW (shelves ±15 dB, or cuts), the middle (PEAK ±18 dB, or NOTCH or BAND and their Q), FREQ (30 Hz … 16 kHz) | KNOB 1: SHELF or CUT; KNOB 3: PEAK, NOTCH, BAND |
| **SEND** | INPUT, SEND 1, SEND 2, MAIN MIX | |
| **FX** | the channel's slot: EFFECT, its two parameters, PRESENCE | KNOB 1: IN; KNOB 2: OUT; KNOB 3: LO-FI; KNOB 4: BYPASS |

**The input** is what makes the loop:

| Input | The loop | What makes it sing |
| --- | --- | --- |
| **PRE** | the channel alone, from after its level | its LEVEL |
| **POST** | the channel and its insert effect | its LEVEL; the effect shapes the loop |
| **MIX L / MIX R** | a side of the mix bus: every channel feeding it, and the returns | its LEVEL and PAN — and every other channel's |
| **SEND 1 / SEND 2** | a send bus: every channel feeding it | its SEND |
| **RET 1 / RET 2** | a return bus: through the rack's effects on the sends | its SEND, through the effects |
| **NONE** | no loop: the noise generator alone, through the channel | |
| **CH 1 … CH 6** | another channel's output (after its insert): channels in a chain or a ring | each channel's LEVEL round the ring |

**GAIN** amplifies the noise and the loop. The **tube** shapes the sound as it rises (SATURATED drives it
harder). The **op-amp** amplifies it again and adds its own colour: the 741 and OP07 are slow and dark, the 4558
warm, the CA3140 lopsided, the TL072 and NE5532 clean. **PROTECT** sets the threshold of the overload
protection: when the op-amp's signal goes over it, the protection breaks the signal for a few milliseconds and
the loop starts again from the noise, with the crackle no-input players know. Higher, it lets more through and
the op-amp saturates more.

**LIMIT** is the brickwall limiter's speed. Its look-ahead is the delay that sets the base pitch of a loop: fast
(a fraction of a millisecond) gives high tones, slow (tens of milliseconds) low ones. The page shows it in
milliseconds.

The filters set where a loop rings: a loop sings where its gain is highest, and a peak or a band-pass is where
you tune it. A loop can only ring at pitches its delay allows, so a narrow band-pass between them can keep a
loop silent.

## The rack

Seven slots: slot 1–6 are the channels', slot 7 the output's (OUT FX). A slot's **IN** is OFF, INSERT (its
channel's insert; slot 7: the output's), SEND 1, SEND 2 or PREV (the slot before it); its **OUT** is NONE, RET 1
or RET 2. An insert's output goes on through its channel, and to a return too if OUT says so.

| Effect | Parameter 1 | Parameter 2 |
| --- | --- | --- |
| DELAY | TIME (1 ms … 0.37 s; 0.74 s in LO-FI) — turned, it bends the pitch as a tape would | FEEDBACK |
| FLANGER | DEPTH | RATE |
| CHORUS | INTENSITY | RATE |
| TREMOLO | RATE | VIBRATO |
| FORMANT | VOWEL (A E I O U) | WIDTH |
| BITCRUSH | ALGORITHM (fewer bits; a lower rate; the bits folded on themselves) | OFFSET |
| GLITCH | LENGTH of the part that sticks | REPEATS |

**PRESENCE** is how much of the effect is heard; **LO-FI** halves its rate and its bits.

## The output

| Page | Rows | With SHIFT |
| --- | --- | --- |
| **OUT** | LEVEL (the main level), RET 1, RET 2, SPACE MIX | KNOB 4: the SPACE (ROOM, HALL, PLATE, CAVE, ABYSS, VOID) |
| **OUT EQ** | HIGH, LOW, GAIN, FREQ | KNOB 1: SHELF or CUT; KNOB 3: WIDE or NARROW |
| **OUT FX** | slot 7, as a channel's FX page | as there |

At the top, SPACE MIX leaves the reverb alone.

## New patches

**HOME** draws a new patch: how the channels are connected — each one's input, the rack's slots and their
effects — and how each one is set. A patch is drawn from one of the shapes no-input players reach for:

- **SOLO**: each channel in a loop of its own (PRE, or POST through its insert), voices side by side;
- **RING**: channels feeding each other round a ring;
- **WEB**: channels fed from the mix bus, everything heard in one tangle;
- **SEND**: loops through the send buses;
- **RETURN**: loops through the rack — the sends into an effect, its return into the channels;
- **HYBRID**: a bit of each, channel by channel.

The faders are set a little past where each loop starts to sing, so a patch sounds at once and the faders are
where the playing is. Some channels are drawn with their fader down: brought up, they join in. For its first
two seconds Lilliput plays into a new patch as a player would: a channel whose fader is up but which stays
silent has its GAIN raised until it sings. Then it listens to the output for a second and sets the main LEVEL so
that every patch comes out about as loud. The faders are never touched.

**SHIFT + HOME** moves every channel's GAIN, NOISE, filters, LIMIT and PROTECT a little, the connections and
levels kept: the same patch, elsewhere.

## Presets and sessions

**PRESETS** turns through sixteen factory patches; **SAVE** loads the one shown. **SAVE** held saves the whole
mixer as a session (64 of them); **SHIFT + SAVE** opens the sessions. A session keeps every parameter, the
channel chosen and the patch's name.

## Global setup and MIDI

**SHIFT + SAVE** held 0.8 s opens GLOBAL SETUP: **MIDI** (each channel's IN and OUT channel, AUTO for the chosen
channel, CC in and out on or off, the USB and TRS ports), **AUDIO** (USB audio: STEREO, the output, or CHANNELS,
the six channels each mono), **SETTINGS** (the screen reader, its speech rate). A channel answers to and sends
these CCs: **7** LEVEL, **10** PAN, **16** GAIN, **17** FREQ, **18** LIMIT, **19** SEND 1, **74** MID, **91**
SEND 2, **92** NOISE.

## Screen reader

The FM-1 can say what its screen shows (GLOBAL SETUP → SETTINGS, or SHIFT + black key 8 from anywhere). It is
Tonitor's screen reader, with Lilliput's words.

# Building from source

macOS or Linux:

```
./setup.sh          # once: the Python environment, the JieLi toolchain, three files of the AC79 SDK
./build.sh          # build/lilliput.fwsc
./install.sh        # builds, then installs over USB
tests/run_tests.sh  # the tests, on the computer
```

On macOS the toolchain runs in Docker: Docker Desktop must be running to build. `./build.sh --release X.Y`
builds a release (identity FM-1_8XY).

## Testing without the FM-1

`tests/run_tests.sh` compiles the whole firmware for the computer and runs it:

- the target check: nothing that needs a runtime the chip lacks (no float, no 64-bit division);
- **check**: 200 patches HOME draws, each must sing, none clip or sit on DC;
- **flows**: the panel — channels, pages, input keys, mutes, HOME, PLAY, sessions saved and loaded back, presets,
  GLOBAL SETUP, another firmware's data left alone;
- **screens**: every page, as images in `build/screens`;
- **cores**: the output stage on the second core sounds the same as on one;
- a render of every factory patch to `build/sounds` (WAV).

`build/host render build/sounds v` prints each patch's settings beside its figures; `build/host calib` shows
where a single channel's loop starts to sing, its level swept.

## How it is built

`firmware/src/lilliput.c` is one compilation unit that includes the rest: `mixer.c` (the channels, the rack, the
buses, the output; the per-sample loop runs from RAM on CPU0, the output stage a block late on CPU1),
`patch.c` (HOME's patches, the trim, the factory's), `lui.c` (the panel and the screen), `lses.c` (sessions),
`setup.c` (GLOBAL SETUP, MIDI), and the platform from Tonitor, Felucca and SLOOP.

# Licence

GPL-3.0-only; see [LICENSING.md](LICENSING.md).

# Credits

- **NoInputMixer** by Igor Vasiliev: the model — its signal flow, its controls, its manual's descriptions.
- **Tonitor** (Nuno Zimas): the screen, the panel's ways, the sessions, the screen reader, the reverb.
- **Felucca** (Leo Kuroshita, Hügelton Instruments) and **SLOOP** (isod89): the hardware layer, the boot, USB,
  the installer and the rescue mode.
- **Melodee** (Kerem Kilic): the second core.

# Disclaimer

Lilliput is an **unofficial** firmware. It is not made, endorsed, approved or supported by M-VAVE or by the
maker of the FM-1, nor by the maker of NoInputMixer, and it is not affiliated with them.

**Installing it replaces the device's original firmware and may void any warranty that applies to your
FM-1**, if any applies at all; check the terms of your warranty before installing. Installing any firmware
carries a risk: an interrupted or failed update can leave the device unable to start until it is recovered
(see [Installing](#installing)).

The software is provided **"as is", without warranty of any kind**, express or implied, including but not
limited to the warranties of merchantability, fitness for a particular purpose and non-infringement. In no
event shall the authors or contributors be liable for any claim, damages or other liability — including
damage to your device, loss of data, or loss of the use of the device — arising from, out of or in
connection with the software, its installation or its use. You install and use it entirely at your own
risk. See also sections 15 and 16 of the [GPL](LICENSE).

**Feedback can be loud.** A no-input mixer can jump from silence to full scale. Start with the MASTER knob low,
and keep PLAY (mute) under your hand.
