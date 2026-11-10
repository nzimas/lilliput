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
- **Twenty-four recipes**: archetypes, not presets. PRESETS shows one, and **HOME draws a new patch from it** — new
  connections, new settings and new motion for the whole mixer, different every time but always in the recipe's
  direction. Every patch drawn sounds at once, and moves.
- **Each channel has two LFOs and four modulation slots** (Tonitor's matrix), so its parameters move by
  themselves.
- Sixty-four sessions, MIDI CCs, USB audio (the output, or the six channels).

The screen is Tonitor's, made for **low vision**: very few words, in very large type, with high contrast, and
each channel in its own colour.

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
| **SELECT** knob | the chosen channel's **LEVEL**: its fader. In most loops it is what makes them sing. **1 dB a detent** up to −5 dB, then **0.1 dB a detent** (−4.9, −4.8 …) up to +12 dB, where a loop's edge is. Below −60 dB: off. |
| **ALGORITHM** knob | its **PAN**, **one step a detent**, L 50 … R 50. In a no-input mixer the pan shapes the sound, not only its place: in a loop through the mix bus it is part of the loop. |
| **KNOB 1–4** | on the main view: **GAIN**, **FREQ** (the middle filter's), **LIMIT** (the loop's pitch), **SEND 1**; with **SHIFT**: **NOISE**, **MID** (the middle filter's gain or width), **PROTECT** (the overload protection), **SEND 2**. A dial covers the screen while you turn. On a page: its four rows. |
| **OCT− / OCT+** | the channel's pages: MAIN, AMP, NOISE, EQ, SEND, FX, LFO 1, LFO 2, MOD 1 TO 4, MOD 5 TO 8 (on the output's pages: OUT, OUT EQ, OUT FX). Their lights: a page that way. |
| **SHIFT** held on a page | its rows show the page's switches (FX: IN, OUT, LO-FI, BYPASS; EQ: FILTER, MIDDLE; OUT: SPACE): SHIFT + KNOB 1–4 set them. |
| **SEQ** | the output's pages: OUT, OUT EQ, OUT FX, then back. |
| **SHIFT + SEQ** | the chosen channel's modulation thrown anew: both LFOs and two to four slots, at random. |
| **ARP** | the **OVERVIEW**: the six channels, their inputs and levels at once. |
| **HOME** | a **new patch** from the recipe shown (see [New patches](#new-patches)). |
| **SHIFT + HOME** | a new patch from **another recipe**, drawn at random. |
| **HOME** held 0.8 s | the patch **nudged**: every channel's tone moved a little, its connections and levels kept. |
| **PRESETS** knob | the **recipe**: its name in place of the patch's, in the main view; nothing else changes until HOME. On the MOD pages: where the slot comes from; on the RHYTHM's: the styles. |
| **SAVE** | the **SNAPSHOTS** page (again: back): see [Snapshots](#snapshots). |
| **SAVE** held | save the session: over the one loaded, at once; otherwise a new one (asked: PRESETS chooses NO or YES, SAVE answers). |
| **SHIFT + SAVE** | the **SESSIONS** page: PRESETS chooses, SAVE loads. Held 0.8 s: [GLOBAL SETUP](#global-setup-and-midi). |
| **REC** | arm the **gesture loop**; REC again ends it and it plays. Held 1 s: empty it. See [Gestures](#gestures). |
| **PLAY** | the gesture loop played, or stopped (lit while it plays). |
| **SHIFT + PLAY** | the output muted, or back (OUT MUTED on the screen). The loops keep going, unheard. |
| **White keys 1–15** | the chosen channel's **INPUT**: PRE, POST, MIX L, MIX R, SEND 1, SEND 2, RET 1, RET 2, NONE, CH 1 … CH 6 (lit: the one it has). |
| **White key 16** | its **MAIN MIX** on or off. |
| **Black keys 1–6** | mute channels 1–6 (lit: muted). |
| **SHIFT + black key 1** | the chosen channel **at random**: every parameter — its input, voice, filters, sends, slot, LFOs and matrix, fader and pan (set so it sings); its mute kept. |
| **Black key 10** (2nd from the right) | the **RHYTHM** on or off (the first time: one drawn). Its light: on while it plays, dark a moment on each beat. |
| **SHIFT + black key 10** | a **new rhythm**, of another style, playing. |
| **Black key 10** held 0.8 s | the **TEMPO** page (again: back). See [Rhythm](#rhythm). |
| **SHIFT + black key 7**, held | the CPU page. |
| **Black key 11** (last) | **SHIFT**. |
| **MASTER** knob | the level after everything. |
| **OCT− and OCT+**, held 5 s | UBOOT, the chip's own updater. |

## The main view

The chosen channel's number, very large, in its colour; under it its **input**; under that the **patch's name**
(or a message, black on white; or MUTED, white on red); its **meter**, with a white bar where its fader is
(red at the left when the fader is down); its **pan** along the bottom.

## A channel

Left to right, as the app's block diagram:

**input selector → + noise → GAIN → tube → FOLD → op-amp (and its overload protection) → HIGH, LOW → middle filter →
brickwall limiter → mute → SEND 1, SEND 2 → LEVEL → insert effect → PAN → main mix**

| Page | Rows (KNOB 1–4) | With SHIFT |
| --- | --- | --- |
| **AMP** | TUBE, GAIN (0 … +48 dB), FOLD (off, ×1.2 … ×16), OP-AMP | KNOB 1: MODE (LINEAR, SATURATED) |
| **NOISE** | NOISE (off, −90 … −18 dB; GEIGER: its clicks' rate), TYPE (WHITE, PINK, RED, CRACKLE, GEIGER, HUM), LIMIT, PROTECT | |
| **EQ** | HIGH and LOW (shelves ±15 dB, or cuts), the middle (PEAK ±18 dB, or NOTCH or BAND and their Q), FREQ (30 Hz … 16 kHz) | KNOB 1: SHELF or CUT; KNOB 3: PEAK, NOTCH, BAND |
| **SEND** | INPUT, SEND 1, SEND 2, MAIN MIX | |
| **FX** | the channel's slot: EFFECT, its two parameters, PRESENCE | KNOB 1: IN; KNOB 2: OUT; KNOB 3: LO-FI; KNOB 4: BYPASS |
| **LFO 1**, **LFO 2** | SHAPE, RATE (a step a detent), MODE, FADE | |
| **MOD 1 TO 4**, **MOD 5 TO 8** | the matrix's eight slots, four a page: each row a slot — its number and where it comes FROM, small; where it goes TO and by how much, large; the amount a bar from the middle. KNOB 1–4: the AMOUNT (an amount on an empty slot takes it from LFO 1) | KNOB 1–4: TO. **PRESETS**: FROM, of the slot in hand (the last turned, its row lit) |

A page's rows show each parameter's name small, with a bar, and its value large under it. Hold **SHIFT** to see the
page's switches in place of its rows.

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

**FOLD** drives the signal after the tube into a wavefolder: past full scale it folds back instead of clipping, so
the harder it is driven — by FOLD, by GAIN, by a loop growing — the more harmonics, never more level. It changes the
timbre only, never the pitch.

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
| RESONATOR | ROOT, C2 … B5, a semitone a step | CHORD: OCTAVES, FIFTHS, MAJOR, MINOR, SUS, CLUSTER, PELOG, HARMONICS |
| XMOD | MODE: RING (a diode ring), XOR (the two signals' bits), FOLD (their sum and product, folded), COMPARE (the louder of the two) | WITH: CH 1 … CH 6, MIX L, MIX R, SEND 1, SEND 2 |
| DUCK | DEPTH | KEY: CH 1 … CH 6, MIX L, MIX R, SEND 1, SEND 2 — the louder it is, the quieter this |
| GATE | DEPTH | KEY: as DUCK's — the louder it is, the more this opens |

| CYCLES | MODE: WAVELOSS (the first n of every 8 cycles silent), SPUTTER (a half-cycle now and then turned over or silent), WAVESET (each cycle repeated) | AMOUNT |
| REED | NOTE, C2 … B5, a semitone a step | BREATH |
| OCTAVER | MODE: ONE DOWN, TWO DOWN, BOTH | TONE |
| DIFFUSER | SIZE 1 … 8 | DENSITY |
| SAG | TIME: the window it averages over, 1.5 … 371 ms | DEPTH |
| CUTS | UNIT, 10 … 46 ms | DENSITY |

**RESONATOR** is six tuned resonators on the root, in the chord's intervals, ringing about 1.2 s (after 4ms's SMR).
In a loop it pulls the feedback onto those notes and holds it there. Its ROOT and CHORD are steps and no LFO can
move them: it steadies pitch, never glides it. **XMOD** (after Mutable Instruments' Warps) and **DUCK / GATE**
(after its Streams) take a second signal, so one loop can shape another: ring-modulate it, mangle its bits, duck it
or open it in time with it.

Build 5's six, all steady in pitch: **CYCLES** works on whole wave cycles between the zero crossings — WAVELOSS
drops some in a regular pattern (rhythmic holes a loop rebuilds itself through), SPUTTER turns half-cycles over or
silences them at random (crackle), WAVESET repeats each cycle (a time-stretched stutter that sinks into sub-drones).
**REED** is a clarinet's bore and reed (after STK's Clarinet): the signal is its breath, and it blows at its NOTE,
which is a step and never moves. **OCTAVER** adds one or two octaves down, locked to the signal by flip-flops on its
zero crossings (after OWL's OctaveDown). **DIFFUSER** smears transients into a dense, metallic wash (Clouds'
diffuser). **SAG** is a power supply starving (after Airwindows' PowerSag2): loud a while, the gain sinks and comes
back, so loops pulse and breathe; TIME is how long it remembers. **CUTS** cuts the sound into phrases (3 3 2, 2 2 2 2
and the like); a cut plays on, or repeats what came just before it, whole or in halves or quarters — structured
stutters after BBCut and Livecut. GLITCH is unchanged.

**PRESENCE** is how much of the effect is heard; **LO-FI** halves its rate and its bits.

## The output

| Page | Rows | With SHIFT |
| --- | --- | --- |
| **OUT** | LEVEL (the main level), RET 1, RET 2, SPACE MIX | KNOB 4: the SPACE (ROOM, HALL, PLATE, CAVE, ABYSS, VOID) |
| **OUT EQ** | HIGH, LOW, GAIN, FREQ | KNOB 1: SHELF or CUT; KNOB 3: WIDE or NARROW |
| **OUT FX** | slot 7, as a channel's FX page | as there |

At the top, SPACE MIX leaves the reverb alone.

## New patches

**PRESETS** turns through the **recipes**: the main view's name becomes each one's, in place; **HOME** then draws a new patch from the one
shown. A recipe is an archetype, not a preset: a kind of no-input sound, each with its own way of connecting the
channels, its own register, filters, tubes, op-amps, noises, effects and space — and its own **motion**, LFOs on
the parameters that make its character, drawn anew for each channel. Every draw comes out otherwise, in the
recipe's direction:

| Recipe | What it is | How it moves |
| --- | --- | --- |
| DRONE | low loops of their own or in a ring, slow limiters, resonant lows; caves and abysses | the filters drift; the pans wander |
| WHISTLE | high, clean tones: fast limiters, narrow band-passes | a slight vibrato; the levels breathe |
| SIREN | tones in their own loops | the filters sweep up and down, every channel in step |
| CHIRP | quick loops | the limiters and filters jump on fast saws and squares |
| STATIC | low overload thresholds: the protection trips and crackles; dust, crushers, glitches | the threshold and the levels jump |
| GEIGER | GEIGER noise pinging resonant loops at their edge | the clicks' rate wanders |
| SWARM | four to six channels on the mix bus, saturated | the pans — inside the loop — wander |
| BEATING | channels in rings, their filters close together | slow drifts: the beating changes |
| PULSE | loops gated by square and stepped LFOs | in step: a rhythm |
| CAVERN | loops through long delays into deep spaces | the delay times bend |
| VOICES | formant filters on inserts and returns | the vowels move |
| BROKEN RADIO | band-passes, crackle and hum, tremolo | the tuning jumps |
| SUBTERRANEAN | the lowest loops, dark cuts, slow op-amps | the limiters drift |
| SCREAM | saturated tubes, high gain, notches | the notches sweep |
| GLITCH | glitch and crush on inserts and returns | stepped limiters and effects |
| TIDES | loops through the sends | the sends swell and ebb, in step |
| CHOIR | resonator chords and formants on the returns, soft pink noise | the chords and vowels drift; the levels breathe |
| ENGINE | the lowest loops, saturated power tubes, ring modulation, sag | in step: a motor's throb; the folds pulse |
| INSECTS | many tiny high loops, crackle and Geiger clicks, gates and cuts | the pitch and the pans jump on fast steps |
| ORGAN | channels in rings, close peaks, reeds and octavers, chorus | a slow tremolo, in step; a faint drift |
| MORSE | a few clean beeps, gated, with short delays | in step: on and off, in patterns |
| FURNACE | a dense saturated web, hard folds, crushers, the protection tripping | the gain, the folds and the threshold churn |
| AURORA | high shimmering loops, chorus, flanger and diffusion, vast spaces | slow sweeps; the pans and the effects drift |
| SONAR | one to three pings into long delays, deep spaces | in step: pings that swell and fade; the delays jump |

**HOME** draws from the recipe shown; **SHIFT + HOME** from another, drawn at random. The patch's name, on the main
view, is its recipe's.

The faders are set a little past where each loop starts to sing, so a patch sounds at once and the faders are
where the playing is. Some channels come with their fader down: brought up, they join in. For its first two
seconds Lilliput plays into a new patch as a player would: a channel whose fader is up but which stays silent has
its GAIN raised until it sings. Then it listens to the output and sets the main LEVEL so that every patch comes out
about as loud, its peaks under −4 dB. After that a guard stays on: if the output holds at its ceiling, the main
LEVEL comes down a dB at a time. The faders are never touched.

**HOME** held 0.8 s moves every channel's GAIN, NOISE, filters, LIMIT and PROTECT a little, the connections and
levels kept: the same patch, elsewhere.

## Modulation

Each channel has two **LFOs** and eight **slots** (Tonitor's modulation matrix, twice its slots). An LFO has a SHAPE (SINE, TRI, SAW
UP, SAW DN, SQUARE, STEPS — a new random value each cycle —, DRIFT — smooth random), a RATE (0.01 … 20 Hz), a MODE
(FREE; ONSET: from the start each time the channel's loop bursts into sound; ONCE: one cycle from each onset) and a
FADE (in over up to 10 s from an onset). A slot takes a source — LFO 1, LFO 2, LEVEL (the channel's own loudness: a
loop that moves itself as it grows), OUTPUT (the whole mix's), RANDOM (drawn at each onset) — to a destination —
GAIN, NOISE, FREQ, MID, HIGH, LOW, LIMIT, PROTECT, LEVEL, PAN, SEND 1, SEND 2, the channel's slot's two parameters
and its mix, or either LFO's rate — by an amount, −100 … +100. The panel's values stay where you set them; the
sound moves round them. Every new patch comes with its family's motion; **SHIFT + SEQ** throws the chosen channel's
modulation anew.

## Rhythm

The RHYTHM turns the mixer into a rhythmic noise machine: it plays the channels in time — gating them, choking them,
striking them, pushing them — in structures drawn from a wide palette of styles.

**Each channel gets a lane**: its steps (up to 32), its step's length (1/2, 1/4, 1/8, 1/8 triplets, 1/16, 1/16
quintuplets, 1/16 triplets, 1/32), its hits and accents, how long a hit holds, how far the gate closes, each hit's
chance, its chance of a roll (2 to 4 hits in a step) and its mutation (its steps flipping now and then, each
round). Lanes of different lengths drift against each other (polymeter); lanes of different step lengths run in
different time (polyrhythm). A lane plays its channel one of four ways:

- **CHOP** — what you hear is gated: mutes and unmutes in time. The loop rings on behind the gate.
- **CHOKE** — the loop itself is gated: it dies between the hits and grows back on each, a swell.
- **KICK** — the loop is choked, and struck at each hit by a burst of noise, pushed past its edge (+6 dB) while it
  is open: the loop's own sound made a drum.
- **ACCENT** — no gate: each hit pushes GAIN and FOLD up for a moment. A drone pulses in timbre, never in pitch.

**SHIFT + the 2nd black key from the right** draws a new rhythm for the patch as it is: the darkest loop gets the
kick's part, the brightest the hats', those between the snare's and the percussion's; a channel with its fader down
is left out, and now and then a drone is left free under it all. A loop slow to grow (a slow limiter, a narrow
band-pass, a loop through the sends) is chopped rather than choked. The styles:

| Style | |
| --- | --- |
| INDUSTRIAL | four on the floor, the backbeat, off-beat hats |
| POWER NOISE | 4/4 kicks, a drone chopped in sixteenths, accents |
| BROKEN | syncopated Euclidean kicks and snares (Toussaint's evenly spread hits), swung |
| AKSAK | odd meters as groups: 5 (3+2), 7 (2+2+3), 9 (2+2+2+3), 11 (3+3+3+2), 13 (3+3+3+2+2) |
| POLYMETER | lanes of 16, 12, 10, 9, 7, 5, 3 steps against each other |
| POLYRHYTHM | quarters against triplets against quintuplets against eighths |
| MARCH | snare rolls, ratchets, a military kick |
| HALFTIME | slow and heavy, long gates |
| GABBER | a kick on every beat, noise bursts between |
| ROLLS | sparse hits rolled into bursts |
| CHOP | drones chopped in sixteenths and thirty-seconds, holes by chance |
| TRIBAL | tresillo, cinquillo, clave, the 12/8 bell |
| TURING | random steps that repeat and slowly mutate |
| ERRATIC | uneven lengths, steps and chances, swung, ghosts |
| SPARSE | few hits, long choked swells |
| CLOCKWORK | dividers — every 2, 3, 4, 6, 8 steps — gated hard |

**The TEMPO page** (the key held 0.8 s): its title is the style playing (or RHYTHM OFF).

| KNOB | |
| --- | --- |
| 1 | **TEMPO**, 10 … 500 BPM: a BPM a detent, five when turned fast |
| 2 | **SWING**: the off steps late, up to a third of a step |
| 3 | **CHAOS**: hits skipped, ghost hits added |
| 4 | **LENGTH**: every gate shorter or longer (100 %: as drawn) |
| PRESETS | the styles: a new rhythm of each, playing |
| OCT+ | the second page: **MIX** |

**The MIX page** (OCT+ from TEMPO, OCT− back): **MIX** sets how much of the gates is heard, against the loops as
they run on: 100 % all gate (as before), 0 % the loops alone, the rhythm unheard. KICK's bursts and ACCENT's pushes
follow it too.

A new patch keeps the rhythm playing (its lanes stay with the channels); snapshots and sessions keep their own. The
rhythm's changes — on, off, a new one, the tempo — are gestures like any other: the gesture loop records them.

## Gestures

The gesture loop records what your hands do — on any channel, at once: a knob, an input key, a mute, a channel
thrown at random, a new patch, a MIDI CC — and plays it back, round and round, while you go on playing over it.

- **REC** arms it (the screen says ARMED, REC blinks; a loop playing stops). Nothing is recorded until something
  changes: the first change starts the recording (REC on the screen, white on red, with the seconds; REC lit).
- **REC** again ends it: the loop is as long as from that first change to now, and plays at once (PLAY lit).
- **PLAY** stops it, or plays it again from its start. Stopped, everything stays where the loop left it.
- **REC** held 1 s empties it.

Each round begins from the mixer as it was when the recording began — only what the loop touches — so it plays the
same way every time; what you change that the loop does not touch stays yours. A new patch or a session
stops the loop (PLAY plays it again, over the new mixer). Up to 2048 changes and ten minutes. The output's mute
(SHIFT + PLAY) is never recorded. The loop is kept with the session.

## Snapshots

Sixteen complete states of the mixer — every channel, every slot, the output, the modulation — kept with the
session. **SAVE** (short) opens the SNAPSHOTS page: the sixteen as the white keys lie, the stored ones filled, the
last recalled framed.

- a **white key** let go soon: its snapshot recalled;
- held 0.8 s: the mixer stored in it;
- **SHIFT +** it: emptied;
- **KNOB 1**: the **MORPH** time, off … 30 s. A recall then glides every value that can glide — levels, pans, gains,
  noise, filters, sends, limiters, protection, the effects' parameters, the returns, the main level — from where it
  is to the snapshot's; the rest (inputs, types, routings, LFOs) changes at once.

**SAVE** again, or a channel's button, goes back.

## Sessions

**SAVE** held saves the whole
mixer as a session (64 of them); **SHIFT + SAVE** opens the sessions. A session keeps every parameter, the
channel chosen, the patch's name and family, its sixteen snapshots and its gesture loop.

## Global setup and MIDI

**SHIFT + SAVE** held 0.8 s opens GLOBAL SETUP: **MIDI** (each channel's IN and OUT channel, AUTO for the chosen
channel, CC in and out on or off, the USB and TRS ports), **AUDIO** (USB audio: STEREO, the output, or CHANNELS,
the six channels each mono). A channel answers to and sends
these CCs: **7** LEVEL, **10** PAN, **16** GAIN, **17** FREQ, **18** LIMIT, **19** SEND 1, **74** MID, **91**
SEND 2, **92** NOISE.

# Building from source

macOS or Linux:

```
./setup.sh          # once: the Python environment, the JieLi toolchain, three files of the AC79 SDK
./build.sh          # build/lilliput.fwsc
./install.sh        # builds, then installs over USB
tests/run_tests.sh  # the tests, on the computer
```

On macOS the toolchain runs in Docker: Docker Desktop must be running to build.

**Builds.** Lilliput has build numbers only — build 1, 2, 3 … — never version numbers. The number is `VERSION` in
`tools/build.py`; the splash screen shows it; each build's package is also kept as `build/lilliput-build-N.fwsc`.
Every build is a git commit tagged `build-N`. To go back to a build: `git checkout build-N`, then `./install.sh`
(or install its kept package: `./install.sh build/lilliput-build-N.fwsc`).

## Testing without the FM-1

`tests/run_tests.sh` compiles the whole firmware for the computer and runs it:

- the target check: nothing that needs a runtime the chip lacks (no float, no 64-bit division);
- **check**: 200 patches HOME draws, each must sing, none clip or sit on DC; each family's pitch, movement and
  loudness reported;
- **fits**: every page's words in their place — each name and every value any parameter can take, every recipe's
  name, whole, without being squeezed;
- **flows**: the panel — channels, pages, input keys, mutes, HOME, PLAY, sessions saved and loaded back, recipes,
  GLOBAL SETUP, another firmware's data left alone;
- **screens**: every page, as images in `build/screens`;
- **cores**: the output stage on the second core sounds the same as on one;
- a render of a patch from every recipe to `build/sounds` (WAV).

`build/host render build/sounds v` prints each patch's settings beside its figures; `build/host calib` shows
where a single channel's loop starts to sing, its level swept.

## How it is built

`firmware/src/lilliput.c` is one compilation unit that includes the rest: `mixer.c` (the channels, the rack, the
buses, the output; the per-sample loop runs from RAM on CPU0, the output stage a block late on CPU1),
`patch.c` (the recipes, HOME's patches, the trim), `lui.c` (the panel and the screen), `lses.c` (sessions),
`setup.c` (GLOBAL SETUP, MIDI), and the platform from Tonitor, Felucca and SLOOP.

# Licence

GPL-3.0-only; see [LICENSING.md](LICENSING.md).

# Credits

- **NoInputMixer** by Igor Vasiliev: the model — its signal flow, its controls, its manual's descriptions.
- **Tonitor** (Nuno Zimas): the screen, the panel's ways, the sessions, the reverb.
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
