<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="assets/logo/lilliput-logotype-white.svg">
    <img src="assets/logo/lilliput-logotype-black.svg" alt="Lilliput" width="420">
  </picture>
</p>

# Lilliput

**Alternative firmware for the M-VAVE FM-1: a no-input mixer. 6 channels feeding back into themselves and
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

- **6 channels**, each a mono channel strip: input selector, noise generator, preamp with a choice of
  tube (12AX7, 12AT7, 6C16, EF86, 6L6, 300B; linear or saturated), op-amp (TL072, NE5532, 4558, 741, OP07,
  CA3140) with overload protection that breaks the signal with a crackle, high and low shelves or cuts,
  peak, notch or band-pass, brickwall limiter whose look-ahead sets the loop's pitch, 2 sends, level,
  pan.
- **Everything runs a sample at a time**, so a loop's pitch is free from lowest to highest.
- **Rack of 7 effect slots**: DELAY, FLANGER, CHORUS, TREMOLO, FORMANT, BITCRUSH, GLITCH, each an insert
  on its channel, or fed from a send bus, or from slot before it, into a return bus.
- **Output**: returns, equaliser, insert, and SPACE, a reverb with 6 spaces.
- **25 recipes**: archetypes, not presets. **HOME draws a new patch** from any recipe, **SHIFT + HOME**
  from the one named on screen — new connections, new settings and new motion for the whole mixer, different every time but always
  in the recipe's direction. Every patch drawn sounds at once, and moves. Each start is a new patch too.
- **Each channel has 2 LFOs and 4 modulation slots** (Tonitor's matrix), so its parameters move by
  themselves.
- 64 sessions, MIDI CCs, USB audio (output, or 6 channels).

The screen is Tonitor's, made for **low vision**: very few words, in very large type, with high contrast, and
each channel in its own colour.

> [!NOTE]
> Lilliput is new. Its sound and its screens have been tested on the computer (see
> [Testing without the FM-1](#testing-without-the-fm-1)); **its CPU load on the FM-1 has not been measured
> yet**. 6 channels computed a sample at a time is far more work per sample than Tonitor's blocks: see
> CPU page (SHIFT + black key 7) when you first run it.

## Installing

Build and install from source ([Building from source](#building-from-source)): with the FM-1 connected by USB
and switched on, `./install.sh`. It installs over whatever the FM-1 runs now — M-VAVE's own firmware, Tonitor,
Bigorna or another alternative firmware. Do not unplug the FM-1 while it writes. `./install.sh --info` says what
the connected FM-1 runs. Switch the FM-1 off and on once after installing.

**If something goes wrong:** hold **OCT−** while switching on for USB rescue mode (screen says
RESCUE), then install again. 2 failed starts in a row also enter it. Holding **OCT− and OCT+** for 5
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
| **6 top buttons** | choose channel **1–6** (lit: chosen; glowing: sounding). On a channel's page, another channel shows same page; chosen channel again: its main view. From any other page, back to channel. |
| **SELECT** knob | chosen channel's **LEVEL**: its fader. In most loops it is what makes them sing. **1 dB a detent** up to −5 dB, then **0.1 dB a detent** (−4.9, −4.8 …) up to +12 dB, where a loop's edge is. Below −60 dB: off. |
| **ALGORITHM** knob | its **PAN**, **1 step a detent**, L 50 … R 50. In a no-input mixer the pan shapes the sound, not only its place: in a loop through mix bus it is part of the loop. |
| **KNOB 1–4** | on main view: **GAIN**, **FREQ** (middle filter's), **LIMIT** (loop's pitch), **SEND 1**; with **SHIFT**: **NOISE**, **MID** (middle filter's gain or width), **PROTECT** (overload protection), **SEND 2**. Dial covers screen while you turn. On a page: its 4 rows. |
| **OCT− / OCT+** | channel's pages: MAIN, AMP, NOISE, EQ, SEND, FX (on SEQ's pages: through those). Their lights: a page that way. |
| **SHIFT** held on a page | its rows show page's switches (FX: IN, OUT, LO-FI, BYPASS; EQ: FILTER, MIDDLE; OUT: SPACE): SHIFT + KNOB 1–4 set them. |
| **SEQ** | **COMPRESSOR**, OUT, OUT EQ, OUT FX, then chosen channel's motion: LFO 1, LFO 2, MOD 1 TO 4, MOD 5 TO 8; then back to channel's page. On LFO and MOD pages a channel's button shows same page for that channel. |
| **SHIFT + SEQ** | chosen channel's modulation thrown anew: both LFOs and 2 to 4 slots, at random. |
| **ARP** | **OVERVIEW**: 6 channels, their inputs and levels at once. |
| **HOME** | **new patch** from **any recipe** (see [New patches](#new-patches)). |
| **SHIFT + HOME** | new patch from **recipe named** in main view: the one playing, or the one PRESETS turned to. |
| **HOME** held 0.8 s | patch **nudged**: every channel's tone moved a little, its connections and levels kept. |
| **PRESETS** knob | **recipe**, from the one playing: its name in place of patch's, in main view; nothing else changes until SHIFT + HOME. On MOD pages: where slot comes from; on RHYTHM's: styles. |
| **SAVE** | **SNAPSHOTS** page (again: back): see [Snapshots](#snapshots). |
| **SAVE** held | save session: over the one loaded, at once; otherwise a new one (asked: PRESETS chooses NO or YES, SAVE answers). |
| **SHIFT + SAVE** | **SESSIONS** page: PRESETS chooses, SAVE loads. Held 0.8 s: [GLOBAL SETUP](#global-setup-and-midi). |
| **REC** | arm chosen channel's **gesture loop**; REC again ends it and it plays. Held 1 s: empty it. See [Gestures](#gestures). |
| **PLAY** | chosen channel's gesture loop played, or stopped (lit while it plays). |
| **SHIFT + PLAY** | output muted, or back (OUT MUTED on screen). Loops keep going, unheard. |
| **White keys 1–15** | chosen channel's **INPUT**: PRE, POST, MIX L, MIX R, SEND 1, SEND 2, RET 1, RET 2, NONE, CH 1 … CH 6 (lit: the one it has). |
| **White key 16** | its **MAIN MIX** on or off. On drums' pages, held: **BREAK** (see [Break](#break)). |
| **Black keys 1–6** | mute channels 1–6 (lit: muted). |
| **SHIFT + black key 1** | chosen channel **at random**: every parameter — its input, voice, filters, sends, slot, LFOs and matrix, fader and pan (set so it sings); its mute kept. |
| **Black key 10** (2nd from right) | **RHYTHM** pages: TEMPO, then MIX (again: back). See [Rhythm](#rhythm). |
| **SHIFT + black key 10** | **new rhythm**, of another style, playing. |
| **Black key 10** held 0.8 s | **RHYTHM** on or off (first time: one drawn). Its light: on while it plays, dark a moment on each beat. |
| **Black key 9** (3rd from right) | **DRUMS** pages (again: back). See [Drums](#drums). |
| **SHIFT + black key 9** | **new drum rhythm**, of another style, playing. |
| **Black key 9** held 0.8 s | **drums** on or off. Its light: on while they play. |
| **SHIFT + black key 7**, held | CPU page. |
| **Black key 11** (last) | **SHIFT**. |
| **MASTER** knob | level after everything. |
| **OCT− and OCT+**, held 5 s | UBOOT, chip's own updater. |

## The main view

Chosen channel's number, very large, in its colour — in grey when the channel is not in the patch (muted, or
nothing of it reaches anything: its LEVEL and SENDs off, with no RHYTHM); under it its **input**; under that **patch's name**
(or a message, black on white; or MUTED, white on red); its **meter**, with a white bar where its fader is
(red at left when fader is down); its **pan** along the bottom.

## A channel

Left to right, as the app's block diagram:

**input selector → + noise → GAIN → tube → FOLD → op-amp (and its overload protection) → HIGH, LOW → middle filter →
brickwall limiter → mute → SEND 1, SEND 2 → LEVEL → insert effect → PAN → main mix**

| Page | Rows (KNOB 1–4) | With SHIFT |
| --- | --- | --- |
| **AMP** | TUBE, GAIN (0 … +48 dB), FOLD (off, ×1.2 … ×16), OP-AMP | KNOB 1: MODE (LINEAR, SATURATED, SPIRAL, DENSITY, FUZZ, HARD CLIP) |
| **NOISE** | NOISE (off, −90 … −18 dB; GEIGER: its clicks' rate), TYPE (WHITE, PINK, RED, CRACKLE, GEIGER, HUM), LIMIT, PROTECT | |
| **EQ** | HIGH and LOW (shelves ±15 dB, or cuts), middle (PEAK ±18 dB, or NOTCH or BAND and their Q), FREQ (30 Hz … 16 kHz) | KNOB 1: SHELF or CUT; KNOB 3: PEAK, NOTCH, BAND |
| **SEND** | INPUT, SEND 1, SEND 2, MAIN MIX | |
| **FX** | channel's slot: EFFECT, its 2 parameters, PRESENCE | KNOB 1: IN; KNOB 2: OUT; KNOB 3: LO-FI; KNOB 4: BYPASS |
| **LFO 1**, **LFO 2** | SHAPE, RATE (a step a detent), MODE, FADE | |
| **MOD 1 TO 4**, **MOD 5 TO 8** | matrix's 8 slots, 4 a page: each row a slot — its number and where it comes FROM, small; where it goes TO and by how much, large; amount a bar from middle. KNOB 1–4: AMOUNT (amount on an empty slot takes it from LFO 1) | KNOB 1–4: TO. **PRESETS**: FROM, of slot in hand (last turned, its row lit) |

A page's rows show each parameter's name small, with a bar, and its value large under it. Hold **SHIFT** to see
page's switches in place of its rows.

**The input** is what makes the loop:

| Input | Loop | What makes it sing |
| --- | --- | --- |
| **PRE** | channel alone, from after its level | its LEVEL |
| **POST** | channel and its insert effect | its LEVEL; effect shapes the loop |
| **MIX L / MIX R** | side of mix bus: every channel feeding it, and returns | its LEVEL and PAN — and every other channel's |
| **SEND 1 / SEND 2** | send bus: every channel feeding it | its SEND |
| **RET 1 / RET 2** | return bus: through rack's effects on sends | its SEND, through effects |
| **NONE** | no loop: noise generator alone, through channel | |
| **CH 1 … CH 6** | another channel's output (after its insert): channels in a chain or ring | each channel's LEVEL round the ring |

**GAIN** amplifies noise and loop. The **tube** shapes the sound as it rises (SATURATED drives it
harder). The other 4 MODEs, the **shapers**, drive it as SATURATED does, into another curve: SPIRAL (after
Airwindows' Spiral) rounds the peaks off, then folds them inward as it is driven harder, bright and buzzing;
DENSITY (after Airwindows' Density) fattens; FUZZ has a sharp knee; HARD CLIP flattens peaks at full scale.
None of them moves pitch. The **op-amp** amplifies it again and adds its own colour: 741 and OP07 are slow and dark, 4558
warm, CA3140 lopsided, TL072 and NE5532 clean. **PROTECT** sets threshold of overload
protection: when op-amp's signal goes over it, the protection breaks the signal for a few milliseconds and
the loop starts again from the noise, with the crackle no-input players know. Higher, it lets more through and
the op-amp saturates more.

**FOLD** drives signal after tube into a wavefolder: past full scale it folds back instead of clipping, so
the harder it is driven — by FOLD, by GAIN, by a loop growing — the more harmonics, never more level. It changes
timbre only, never pitch.

**LIMIT** is brickwall limiter's speed. Its look-ahead is the delay that sets base pitch of a loop: fast
(a fraction of a millisecond) gives high tones, slow (tens of milliseconds) low ones. The page shows it in
milliseconds.

Filters set where a loop rings: a loop sings where its gain is highest, and a peak or band-pass is where
you tune it. A loop can only ring at pitches its delay allows, so a narrow band-pass between them can keep a
loop silent.

## The rack

7 slots: slot 1–6 are channels', slot 7 output's (OUT FX). A slot's **IN** is OFF, INSERT (its
channel's insert; slot 7: output's), SEND 1, SEND 2 or PREV (slot before it); its **OUT** is NONE, RET 1
or RET 2. An insert's output goes on through its channel, and to a return too if OUT says so.

| Effect | Parameter 1 | Parameter 2 |
| --- | --- | --- |
| DELAY | TIME (1 ms … 0.37 s; 0.74 s in LO-FI) — turned, it bends pitch as a tape would | FEEDBACK |
| FLANGER | DEPTH | RATE |
| CHORUS | INTENSITY | RATE |
| TREMOLO | RATE | VIBRATO |
| FORMANT | VOWEL (A E I O U) | WIDTH |
| BITCRUSH | ALGORITHM (fewer bits; lower rate; bits folded on themselves) | OFFSET |
| GLITCH | LENGTH of part that sticks | REPEATS |
| RESONATOR | ROOT, C2 … B5, a semitone a step | CHORD: OCTAVES, FIFTHS, MAJOR, MINOR, SUS, CLUSTER, PELOG, HARMONICS |
| XMOD | MODE: RING (diode ring), XOR (2 signals' bits), FOLD (their sum and product, folded), COMPARE (louder of 2 signals) | WITH: CH 1 … CH 6, MIX L, MIX R, SEND 1, SEND 2 |
| DUCK | DEPTH | KEY: CH 1 … CH 6, MIX L, MIX R, SEND 1, SEND 2 — the louder it is, the quieter this |
| GATE | DEPTH | KEY: as DUCK's — the louder it is, the more this opens |
| CYCLES | MODE: WAVELOSS (first n of every 8 cycles silent), SPUTTER (half-cycle now and then turned over or silent), WAVESET (each cycle repeated) | AMOUNT |
| REED | NOTE, C2 … B5, a semitone a step | BREATH |
| OCTAVER | MODE: ONE DOWN, TWO DOWN, BOTH | TONE |
| DIFFUSER | SIZE 1 … 8 | DENSITY |
| SAG | TIME: window it averages over, 1.5 … 371 ms | DEPTH |
| CUTS | UNIT, 10 … 46 ms | DENSITY |
| AUTO-LEVEL | LEVEL it holds, −30 … 0 dB | SPEED |
| LP GATE | DEPTH | KEY: as DUCK's — opens it, closing slowly and darker |
| CROSSOVER | MODE: DEAD ZONE, INSIDE OUT | AMOUNT |
| FRICTION | SPRING: where it rings, 55 … 660 Hz, a step | FRICTION |
| GATELOPE | THRESHOLD, −60 … −6 dB | RELEASE |
| FREEZE | LENGTH, 50 … 371 ms (twice in LO-FI), a step | FADE: 0 frozen |

**RESONATOR** is 6 tuned resonators on the root, in the chord's intervals, ringing about 1.2 s (after 4ms's SMR).
In a loop it pulls feedback onto those notes and holds it there. Its ROOT and CHORD are steps and no LFO can
move them: it steadies pitch, never glides it. **XMOD** (after Mutable Instruments' Warps) and **DUCK / GATE**
(after its Streams) take a 2nd signal, so one loop can shape another: ring-modulate it, mangle its bits, duck it
or open it in time with it.

Build 5's 6, all steady in pitch: **CYCLES** works on whole wave cycles between zero crossings — WAVELOSS
drops some in a regular pattern (rhythmic holes a loop rebuilds itself through), SPUTTER turns half-cycles over or
silences them at random (crackle), WAVESET repeats each cycle (time-stretched stutter that sinks into sub-drones).
**REED** is a clarinet's bore and reed (after STK's Clarinet): signal is its breath, and it blows at its NOTE,
which is a step and never moves. **OCTAVER** adds 1 or 2 octaves down, locked to signal by flip-flops on its
zero crossings (after OWL's OctaveDown). **DIFFUSER** smears transients into a dense, metallic wash (Clouds'
diffuser). **SAG** is a power supply starving (after Airwindows' PowerSag2): loud a while, gain sinks and comes
back, so loops pulse and breathe; TIME is how long it remembers. **CUTS** cuts sound into phrases (3 3 2, 2 2 2 2
and the like); a cut plays on, or repeats what came just before it, whole or in halves or quarters — structured
stutters after BBCut and Livecut. GLITCH is unchanged.

Build 20's 6, all steady in pitch too: **AUTO-LEVEL** follows a signal's loudness and turns it up or down to
LEVEL, ±24 dB at most (after Befaco's Oneiroi): a loop held right at its edge without blowing up; SPEED is how
quickly it follows. **LP GATE** is a low-pass gate (after Mutable Instruments' Streams): its KEY opens it, and it
closes as a vactrol does, quickly at first, then slowly, darker as it closes, so loops decay like struck skins.
**CROSSOVER** (after SuperCollider's): DEAD ZONE silences everything under AMOUNT, so loops burst and sputter
out of silence; INSIDE OUT turns signal over, quiet loud and loud quiet. **FRICTION** is a mass on a spring,
dragged by the signal (after SuperCollider's Friction): it sticks till the spring tears it off, then rings at
SPRING's pitch until it catches again; low FRICTION rings, high drags, between them it creaks and rattles.
**GATELOPE** (after Airwindows') is a gate that closes by sweeping its filters inward, treble down and bass up,
till they meet: loops become decaying plucks. **FREEZE** loops its last LENGTH of sound: FADE 0 holds it frozen
(nothing new comes in); above, new sound is laid over it and each pass fades a little.

**PRESENCE** is how much of the effect is heard; **LO-FI** halves its rate and its bits.

HOME's patches always feature the rack (build 22): 2 or 3 effects heard in every patch — inserts on heard channels
at 55 … 100 % PRESENCE, often inside the channel's loop, or a loop through the rack — each a different effect,
every effect a recipe has about as likely as the next. About half of patches also have an output insert (OUT FX,
build 23), at 20 … 60 % PRESENCE, never a keyed effect (DUCK, GATE, LP GATE, XMOD) or REED.

## The output

| Page | Rows | With SHIFT |
| --- | --- | --- |
| **COMPRESSOR** (SEQ's first) | **AMOUNT** (OFF .. 100 %), **THRESH**, **RELEASE**, **SIDECHAIN** (KICK or OFF) | **ATTACK**, **RATIO** (2:1 .. 20:1, AMOUNT's share of it) |
| **OUT** | LEVEL (main level), RET 1, RET 2, SPACE MIX | KNOB 4: SPACE (ROOM, HALL, PLATE, CAVE, ABYSS, VOID) |
| **OUT EQ** | HIGH, LOW, GAIN, FREQ | KNOB 1: SHELF or CUT; KNOB 3: WIDE or NARROW |
| **OUT FX** | slot 7, as a channel's FX page | as there |

**The COMPRESSOR** works on the mixer's sound, before the drums join it, to hold its own against them. **AMOUNT**
does it all at once: it compresses (that share of RATIO) and makes up the gain, so the mix comes **forward**, and
with **SIDECHAIN** on KICK it ducks the mix under each kick (up to 18 dB) and lets it back over RELEASE: low AMOUNT,
a little lift and a gentle pump; high, mix up front and a deep pump. At start: 85 %, RELEASE an eighth note at the
starting tempo (70 BPM 429 ms, 120 BPM 250 ms, 150 BPM 200 ms), following TEMPO until RELEASE is turned or a
session loaded. Measured on host with a drawn patch: 35 %
brings mix 5.6 dB forward and ducks it 4.7 dB under kick; 90 %, as far forward (output's headroom) and
12 dB under kick. OFF leaves mix untouched. A session keeps it; HOME does not touch it.

At top, SPACE MIX leaves reverb alone.

## New patches

As on Tonitor: **HOME** draws a new patch from any recipe, so each press can be anything; **SHIFT + HOME** draws one
from the recipe named in main view. That is the playing patch's recipe, or another: **PRESETS** turns through
them from it, the name changing in place. Lilliput starts with a patch drawn from any recipe and a tempo drawn
between 70 and 150 BPM, otherwise each time, with COMPRESSOR on at 85 % and its RELEASE an eighth note at that
tempo; a session loaded names its patch's recipe. A recipe is an archetype, not a preset: a kind of no-input sound, each with its own way of connecting
channels, its own register, filters, tubes, op-amps, noises, effects and space — and its own **motion**, LFOs on
parameters that make its character, drawn anew for each channel. Every draw comes out otherwise, in the
recipe's direction:

| Recipe | What it is | How it moves |
| --- | --- | --- |
| DRONE | low loops of their own or in a ring, slow limiters, resonant lows; caves and abysses | filters drift; pans wander |
| WHISTLE | high, clean tones: fast limiters, narrow band-passes | slight vibrato; levels breathe |
| SIREN | tones in their own loops | filters sweep up and down, every channel in step |
| CHIRP | quick loops | limiters and filters jump on fast saws and squares |
| STATIC | low overload thresholds: protection trips and crackles; dust, crushers, glitches | threshold and levels jump |
| GEIGER | GEIGER noise pinging resonant loops at their edge | clicks' rate wanders |
| SWARM | 4 to 6 channels on mix bus, saturated | pans — inside the loop — wander |
| BEATING | channels in rings, their filters close together | slow drifts: beating changes |
| PULSE | loops gated by square and stepped LFOs | in step: a rhythm |
| CAVERN | loops through long delays into deep spaces | delay times bend |
| VOICES | formant filters on inserts and returns | vowels move |
| BROKEN RADIO | band-passes, crackle and hum, tremolo | tuning jumps |
| SUBTERRANEAN | lowest loops, dark cuts, slow op-amps | limiters drift |
| SCREAM | saturated tubes, high gain, notches | notches sweep |
| GLITCH | glitch and crush on inserts and returns | stepped limiters and effects |
| TIDES | loops through sends | sends swell and ebb, in step |
| CHOIR | resonator chords and formants on returns, soft pink noise | chords and vowels drift; levels breathe |
| ENGINE | lowest loops, saturated power tubes, ring modulation, sag | in step: a motor's throb; folds pulse |
| INSECTS | many tiny high loops, crackle and Geiger clicks, gates and cuts | pitch and pans jump on fast steps |
| ORGAN | channels in rings, close peaks, reeds and octavers, chorus | slow tremolo, in step; faint drift |
| MORSE | a few clean beeps, gated, with short delays | in step: on and off, in patterns |
| FURNACE | dense saturated web, hard folds, crushers, protection tripping | gain, folds and threshold churn |
| AURORA | high shimmering loops, chorus, flanger and diffusion, vast spaces | slow sweeps; pans and effects drift |
| SONAR | 1 to 3 pings into long delays, deep spaces | in step: pings that swell and fade; delays jump |
| FREE | no direction: any connection, voice, effect and space, each as likely as the next, over widest ranges the recipes use — as Lilliput drew before it had recipes. Only what makes it sound is kept: 2 channels heard at least, faders past where loops sing | each gesture to a destination drawn for each channel |

**HOME** draws from any recipe; **SHIFT + HOME** from the one named. The patch's name, on main view, is its
recipe's.

Faders are set a little past where each loop starts to sing, so a patch sounds at once and the faders are
where the playing is. Some channels come with their fader down: brought up, they join in. For its first 2
seconds Lilliput plays into a new patch as a player would: a channel whose fader is up but which stays silent has
its GAIN raised until it sings. Then it listens to output and sets main LEVEL so that every patch comes out
about as loud, its peaks under −4 dB. After that a guard stays on: if the output holds at its ceiling, main
LEVEL comes down a dB at a time. Faders are never touched.

**HOME** held 0.8 s moves every channel's GAIN, NOISE, filters, LIMIT and PROTECT a little, connections and
levels kept: same patch, elsewhere.

## Modulation

Each channel has 2 **LFOs** and 8 **slots** (Tonitor's modulation matrix, twice its slots). An LFO has SHAPE (SINE, TRI, SAW
UP, SAW DN, SQUARE, STEPS — a new random value each cycle —, DRIFT — smooth random), RATE (0.01 … 20 Hz), MODE
(FREE; ONSET: from start each time channel's loop bursts into sound; ONCE: 1 cycle from each onset) and
FADE (in over up to 10 s from an onset). A slot takes a source — LFO 1, LFO 2, LEVEL (channel's own loudness: a
loop that moves itself as it grows), OUTPUT (whole mix's), RANDOM (drawn at each onset) — to a destination —
GAIN, NOISE, FREQ, MID, HIGH, LOW, LIMIT, PROTECT, LEVEL, PAN, SEND 1, SEND 2, channel's slot's 2 parameters
and its mix, or either LFO's rate — by an amount, −100 … +100. The panel's values stay where you set them; the
sound moves round them. Every new patch comes with its family's motion; **SHIFT + SEQ** throws chosen channel's
modulation anew.

## Rhythm

The RHYTHM turns the mixer into a rhythmic noise machine: it plays channels in time — gating them, choking them,
striking them, pushing them — in structures drawn from a wide palette of styles.

**Each channel gets a lane**: its steps (up to 32), its step's length (1/2, 1/4, 1/8, 1/8 triplets, 1/16, 1/16
quintuplets, 1/16 triplets, 1/32), its hits and accents, how long a hit holds, how far the gate closes, each hit's
chance, its chance of a roll (2 to 4 hits in a step) and its mutation (its steps flipping now and then, each
round). Lanes of different lengths drift against each other (polymeter); lanes of different step lengths run in
different time (polyrhythm). A lane plays its channel 1 of 4 ways:

- **CHOP** — what you hear is gated: mutes and unmutes in time. The loop rings on behind the gate.
- **CHOKE** — loop itself is gated: it dies between hits and grows back on each, a swell.
- **KICK** — loop is choked, and struck at each hit by a burst of noise, pushed past its edge (+6 dB) while it
  is open: loop's own sound made a drum.

A channel in a ring (it takes another channel, or another takes it) is never choked: CHOKE and KICK gate what is
heard instead, so the ring runs on behind the gate (choked, it died and never came back).
- **ACCENT** — no gate: each hit pushes GAIN and FOLD up for a moment. A drone pulses in timbre, never in pitch.

**SHIFT + the 2nd black key from the right** draws a new rhythm for the patch as it is: the darkest loop gets the
kick's part, the brightest the hats', those between the snare's and the percussion's; a channel with its fader down
is left out, and now and then a drone is left free under it all. A loop slow to grow (slow limiter, narrow
band-pass, loop through the sends) is chopped rather than choked. A sparse lane — open 1/5 of the time or less —
is chopped too, over a bed 14 dB down, so its channel is never all but silent. Every recipe with every style sounds
(host's test plays them all). Styles:

| Style | |
| --- | --- |
| INDUSTRIAL | 4 on the floor, backbeat, off-beat hats |
| POWER NOISE | 4/4 kicks, drone chopped in 16ths, accents |
| BROKEN | syncopated Euclidean kicks and snares (Toussaint's evenly spread hits), swung |
| AKSAK | odd meters as groups: 5 (3+2), 7 (2+2+3), 9 (2+2+2+3), 11 (3+3+3+2), 13 (3+3+3+2+2) |
| POLYMETER | lanes of 16, 12, 10, 9, 7, 5, 3 steps against each other |
| POLYRHYTHM | quarters against triplets against quintuplets against 8ths |
| MARCH | snare rolls, ratchets, military kick |
| HALFTIME | slow and heavy, long gates |
| GABBER | kick on every beat, noise bursts between |
| ROLLS | sparse hits rolled into bursts |
| CHOP | drones chopped in 16ths and 32nds, holes by chance |
| TRIBAL | tresillo, cinquillo, clave, 12/8 bell |
| TURING | random steps that repeat and slowly mutate |
| ERRATIC | uneven lengths, steps and chances, swung, ghosts |
| SPARSE | few hits, long choked swells |
| CLOCKWORK | dividers — every 2, 3, 4, 6, 8 steps — gated hard |

Its key works as drums' key does: short, its pages (again: back); held 0.8 s, on or off; SHIFT + it, new rhythm.

**TEMPO page** (key, short): its title is the style playing (or RHYTHM OFF).

| KNOB | |
| --- | --- |
| 1 | **TEMPO**, 10 … 500 BPM: a BPM a detent, 5 when turned fast |
| 2 | **SWING**: off steps late, up to 1/3 of a step |
| 3 | **CHAOS**: hits skipped, ghost hits added |
| 4 | **LENGTH**: every gate shorter or longer (100 %: as drawn) |
| PRESETS | styles: new rhythm of each, playing |
| OCT+ | 2nd page: **MIX** |

**MIX page** (OCT+ from TEMPO, OCT− back): **MIX** sets how much of the gates is heard, against the loops as
they run on: 100 % all gate (as before), 0 % loops alone, rhythm unheard. KICK's bursts and ACCENT's pushes
follow it too.

A new patch keeps the rhythm playing (its lanes stay with channels); snapshots and sessions keep their own. The
rhythm's changes — on, off, new one, tempo — are gestures like any other: the gesture loop records them.

## Drums

Drum section beside the mixer, in the spirit of Bram Bos's Ruismaker Noir: synthesized, punchy and dirty, for
rhythmic noise. 4 voices — **KICK**, **SNARE**, **HATS**, **PERC** (struck metal) — of a single engine, each with its
own lane of steps, on RHYTHM's tempo and beat. Started, or given a new rhythm, they come in on the next beat; the
RHYTHM's changes never disturb them, nor theirs the RHYTHM.

**Black key 9** (3rd from right), short, opens their pages (again: back); held 0.8 s, they start or stop;
**SHIFT +** it draws a new drum rhythm and plays it. On their pages:

- **white keys 1–4** choose KICK, SNARE, HATS, PERC (lit: the one chosen); **SHIFT +** one draws **new sound** for
  it, from its recipes; **held 0.7 s**, it is **muted** (its key blinks, dim; its page says MUTED), held again, back;
- **white key 16** (last, under SHIFT), **held**: [BREAK](#break), variation of rhythm playing, heard while held;
- **PRESETS** goes through drum rhythms, a new one of each, playing;
- **OCT− / OCT+** through 3 pages.

| Page | Rows (KNOB 1–4) | With SHIFT |
| --- | --- | --- |
| voice (its name and its sound's) | **TUNE** (Hz), **DECAY**, **DRIVE**, **LEVEL** | **SWEEP** (how far above pitch it starts), **ATTACK** (HARD: sharpest transient; up to 100 ms: a soft one), **NOISE** (tone or noise), **REVERB** (its send into SPACE) |
| its 2nd page | **BEND** (how fast sweep falls), **COLOR** (noise filter), **CLICK** (burst on hit), **PAN** | **METAL** (tone made metallic: off or 6 ratios), **FILTER** (low-, band- or high-pass) |
| drums' (their rhythm's name) | **SWING**, **VARIATION** | |

**A voice**: tone, swept down in pitch, and noise through a filter, mixed by NOISE; envelope rising over ATTACK
and falling over DECAY; CLICK on top; then **DRIVE**, a tanh up to 32 times over: every voice has its own. The dry
drums go to the output after the reverb, so SPACE never washes them away; each voice's **REVERB** sends it into
SPACE's reverb, heard even when mixer's SPACE MIX is down. Their level is their own: the trim that sets
mixer's main level never moves it.

**Sounds** (SHIFT + a voice's white key draws one, its values anew each time): kicks DEEP, PUNCH, GABBER, SOFT (slow
attack), TOM, SUB, CRUNCH; snares SNAP, NOISE, BODY, METAL, RIM, CRUSH, LONG; hats CLOSED, OPEN, METAL, SHAKER,
TICK, CYMBAL, DIRT; percussion ANVIL, PIPE, SHEET, CLANK, CHAIN, SPRING, GRIND.

**Rhythms** (build 21: away from four on the floor). Each style is a way of building a rhythm, drawn afresh each
time, so no 2 draws are alike. Lanes of different lengths drift against each other (polymeter); lanes of
different step lengths run in different time (polyrhythm).

| Style | How it is built |
| --- | --- |
| **AKSAK** | groups of 2 and 3 added up to an odd meter, 5 … 13 (once, or twice with a changed 2nd half); kick and snare on group starts, perc limping on the 3s; now and then hats in 16 against it |
| **PHASING** | 1 cell on hats and on perc, perc's a step longer: they drift apart and meet again (after Reich) |
| **EUCLID MESH** | each voice a Euclidean rhythm of its own length (5 … 16), turned its own way |
| **POLYRHYTHM** | each voice its own subdivision: 8ths, triplets, quintuplets, 16ths at once |
| **TIMELINE** | a bell's timeline (12/8 bell, clave, gahu, bossa, shiko, soukous; sometimes turned) against a pulse of the other kind |
| **COLOTOMIC** | cycles in cycles, after gamelan: a gong, ticks between, hats and snare interlocking |
| **CASCADE** | a bouncing ball: hits ever closer; snare's ever farther |
| **LIMP** | a long odd bar (18 … 28 16ths), the backbeat late, an 8th missing |
| **DUST** | a cloud: lanes of uneven lengths, every hit a chance |
| **CANON** | 1 figure, each voice entering it at another place |
| **HOCKET** | 1 dense line shared out, no 2 voices on a step |
| **RITUAL** | half time, slow: a kick, a snare far off, triplet hats |
| **BROKEN** | kicks off the beat, backbeat pushed or pulled, ghosts |
| **TRIBAL** | kick drawn Euclidean, dense, as toms are played; anvil 3 against 4 |
| **DIST BREAK** | a distorted break, straight |
| **TEMPO CANON** | 1 figure in several tempi at once: 8ths, triplets, 16ths, quintuplets, meeting now and then (after Nancarrow) |

**VARIATION** (drums' page, KNOB 2) is how often a lane's round is played otherwise: a hit dropped, a pick-up
before a hit, a hit pushed a step, an accent moved, a hit doubled, the lane turned, its 2nd half backwards, a
round's rest (never the kick's). Each variant comes from the lane's own pattern, and the next round comes home, so
the rhythm never wanders off. Each style draws its own amount, and each lane its own share of it (hats and perc
more than kick). 0: every round as written; sessions from before build 21 load at 0.

A step can be more than a hit: accented, or **ALT**, its decay cut to 1/5 (closed hats among open ones, ghost
snares). Nothing changes a voice's pitch: each voice keeps its TUNE on every step, in rhythms and in breaks, so the
drums never turn into a melody (build 22; a session saved before with pitched steps loads with them level).

### Break

On drums' pages, **hold white key 16** (last, under SHIFT): drums play a **variation of their own rhythm** — fill,
break, flams, beat repeat — for as long as it is held. Let go, rhythm carries on from where it would have been: nothing
is written, each lane's steps run on underneath.

Each press draws one way, never the same as the one before, and keeps it while held; drum page's title names it. A
break is **quantized**: it starts on the rhythm's next beat (next half bar for HALF TIME and MIRROR), playing as
written until then, and fills are measured against the bar, so they land on its next downbeat however late they start.
Every way is made from rhythm playing — its hits, accents and pitches — and from what each voice is:

| Way | What plays |
| --- | --- |
| STUTTER | bar's busiest beat repeated: whole, then half, then a single step, ratcheting faster |
| AVALANCHE | fill over the bar: snare from ghosts in 8ths to accented 16ths and doubles, kick on beats; lands on downbeat, all accented, and builds again |
| FLAMS | rhythm as written, each kick, snare and perc hit flammed (soft grace note before it); accented snares dragged (2 graces) |
| DROP | kick and snare out; hats and perc on, hats' gaps filled with ghosts |
| HALF TIME | kick, snare and perc at half speed; hats at full speed over them |
| CELL 3, 5, 6 or 7 | each lane loops a cell of that many of its own steps against the bar; kick keeps each downbeat |
| SWAP | voices trade lanes: kick plays snare's steps, snare kick's, hats perc's, perc hats' |
| MIRROR | each half bar backwards; kick keeps its first step |
| TUMBLE | perc on every step over 2 beats, from an accent down to ghosts, again and again; kick as written |
| INTERLOCK | kick and snare keep 1st half of each beat, hats and perc the 2nd; after a bar they trade whole beats |
| SCATTER | bar's 8 pairs of steps in another order, first in place |
| BUZZ | rhythm as written, hats in 3s then 4s, snare and perc doubled then tripled |
| GHOSTS | every gap filled with ghost notes, hats accented 3 3 2, kick pushing ahead of beat 4 |

The drums are not part of a patch: HOME, snapshots and gesture loops leave them alone. A session keeps them.

## Gestures

Each channel has its own gesture loop. It records what your hands do to that channel — its knobs, its input key,
its mute, its effect slot, channel thrown at random, a MIDI CC to it — and plays it back, round and round, while
you go on playing over it. Loops are independent: each its own length, each started and stopped on its own.
REC and PLAY act on chosen channel, and their lights show its loop.

- **REC** arms channel's loop (screen says ARMED, REC blinks; its loop, if it plays, stops). Nothing is
  recorded until the channel changes: its first change starts recording (REC on screen, white on red, with
  seconds; REC lit). A loop it had is replaced: there are no overdubs, 1 loop a channel.
- **REC** again ends it: loop is as long as from that first change to now, and plays at once (PLAY lit).
- **PLAY** stops it, or plays it again from its start. Stopped, the channel stays where the loop left it.
- **REC** held 1 s empties it.

Only 1 channel records at a time: REC on another channel ends recording (it plays) and arms that one. Each round
begins from the channel as it was when its recording began — only what the loop touches — so it plays the same way
every time; what you change that the loop does not touch stays yours. Loops are yours, not patch's: a new patch
leaves them playing, same moves over new patch (build 24); it is never recorded into a loop. A session stops
them (PLAY plays each again). Up to 2048 changes among all loops, 10 minutes each. The output's,
output slot's and RHYTHM's settings belong to no channel and are never recorded. The loops are kept with
the session; a session saved before build 12, with 1 loop for all, loads as a loop for each channel it touched.

## Snapshots

16 complete states of the mixer — every channel, every slot, output, modulation — kept with the
session. **SAVE** (short) opens SNAPSHOTS page: the 16 as white keys lie, stored ones filled,
last recalled framed.

- **white key** let go soon: its snapshot recalled;
- held 0.8 s: mixer stored in it;
- **SHIFT +** it: emptied;
- **KNOB 1**: **MORPH** time, off … 30 s. A recall then glides every value that can glide — levels, pans, gains,
  noise, filters, sends, limiters, protection, effects' parameters, returns, main level — from where it
  is to the snapshot's; the rest (inputs, types, routings, LFOs) changes at once.

**SAVE** again, or a channel's button, goes back.

## Sessions

**SAVE** held saves the whole
mixer as a session (64 of them); **SHIFT + SAVE** opens the sessions. A session keeps every parameter,
channel chosen, patch's name and family, its 16 snapshots and its gesture loop.

## Global setup and MIDI

**SHIFT + SAVE** held 0.8 s opens GLOBAL SETUP: **MIDI** (each channel's IN and OUT channel, AUTO for chosen
channel, CC in and out on or off, USB and TRS ports), **AUDIO** (USB audio: STEREO, output, or CHANNELS,
6 channels each mono). A channel answers to and sends
these CCs: **7** LEVEL, **10** PAN, **16** GAIN, **17** FREQ, **18** LIMIT, **19** SEND 1, **74** MID, **91**
SEND 2, **92** NOISE.

# Building from source

macOS or Linux:

```
./setup.sh          # once: the Python environment, the JieLi toolchain, 3 files of the AC79 SDK
./build.sh          # build/lilliput.fwsc
./install.sh        # builds, then installs over USB
tests/run_tests.sh  # the tests, on the computer
```

On macOS the toolchain runs in Docker: Docker Desktop must be running to build.

**Builds.** Lilliput has build numbers only — build 1, 2, 3 … — never version numbers. The number is `VERSION` in
`tools/build.py`; splash screen shows it; each build's package is also kept as `build/lilliput-build-N.fwsc`.
Every build is a git commit tagged `build-N`. To go back to a build: `git checkout build-N`, then `./install.sh`
(or install its kept package: `./install.sh build/lilliput-build-N.fwsc`).

## Testing without the FM-1

`tests/run_tests.sh` compiles the whole firmware for the computer and runs it:

- target check: nothing that needs a runtime the chip lacks (no float, no 64-bit division);
- **check**: 200 patches HOME draws, each must sing, none clip or sit on DC; each family's pitch, movement and
  loudness reported;
- **fits**: every page's words in their place — each name and every value any parameter can take, every recipe's
  name, whole, without being squeezed;
- **flows**: panel — channels, pages, input keys, mutes, HOME, PLAY, sessions saved and loaded back, recipes,
  GLOBAL SETUP, another firmware's data left alone;
- **screens**: every page, as images in `build/screens`;
- **cores**: output stage on 2nd core sounds the same as on one;
- render of a patch from every recipe to `build/sounds` (WAV).

`build/host render build/sounds v` prints each patch's settings beside its figures; `build/host calib` shows
where a single channel's loop starts to sing, its level swept.

## How it is built

`firmware/src/lilliput.c` is a single compilation unit that includes the rest: `mixer.c` (channels, rack,
buses, output; per-sample loop runs from RAM on CPU0, output stage a block late on CPU1),
`patch.c` (recipes, HOME's patches, trim), `lui.c` (panel and screen), `lses.c` (sessions),
`setup.c` (GLOBAL SETUP, MIDI), `drums.c` / `drumseq.c` / `drumkit.c` (DRUMS), `logo.c` (splash), and the
platform from Tonitor, Felucca and SLOOP. The logo's SVGs (`assets/logo`) come from `tools/gen_logo.py`, from the same
numbers the splash draws.

# Licence

GPL-3.0-only; see [LICENSING.md](LICENSING.md).

# Credits

- **NoInputMixer** by Igor Vasiliev: model — its signal flow, its controls, its manual's descriptions.
- **Tonitor** (Nuno Zimas): screen, panel's ways, sessions, reverb.
- **Felucca** (Leo Kuroshita, Hügelton Instruments) and **SLOOP** (isod89): hardware layer, boot, USB,
  installer and rescue mode.
- **Melodee** (Kerem Kilic): 2nd core.

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
