# Changelog

## Build 12 (2026-11-10)

- A gesture loop for each channel, each on its own (as Tonitor's looper is a layer's): REC and PLAY act on the
  chosen channel; a channel's loop records only that channel (its parameters and its effect slot) and replaces the
  loop it had (no overdubs). The loops play together, each its own length. One records at a time: REC on another
  channel ends the recording and arms that one. Sessions keep each channel's loop; a session saved before, with one
  loop for all, loads as a loop for each channel it touched (the output's and the RHYTHM's changes left out).
- HOME draws from **any** recipe until PRESETS chooses one (as on Tonitor). PRESETS has a position for that, **ANY**,
  before the first recipe and after the last. SHIFT + HOME: another recipe than the one playing. A session loaded:
  none chosen.
- Each start is a new patch, from any recipe, and its tempo is drawn between 91 and 299 BPM (before: always the same
  DRONE at 120). The randomness comes from the ADCs' noise and the clock at power-on.
- A channel not in the patch (muted, or its LEVEL and SENDs off with no RHYTHM) is drawn in grey: its number, its
  name, its meter, its pages; in the OVERVIEW too.

## Build 11 (2026-11-10)

- PRESETS no longer flashes "RECIPE": turning it changes the name in the main view, in place, to the recipe's (from
  another page it goes to the main view). Before, the flash went and the main view still named the patch playing.
  HOME draws from the recipe named; turned back to the patch's own recipe, the patch's name is shown again.

## Build 10 (2026-11-10)

- PRESETS are gone: there were sixteen fixed "factory patches" and a page of them. The knob now shows the **RECIPE**
  (an archetype, never a fixed patch): its name, flashed, and nothing else changes. **HOME** draws a new patch from
  the recipe shown, otherwise each time; **SHIFT + HOME** from another recipe, at random. No page of its own.
- Eight new recipes, twenty-four in all: CHOIR, ENGINE, INSECTS, ORGAN, MORSE, FURNACE, AURORA, SONAR. They reach
  for the effects the first sixteen use little (RESONATOR, XMOD, SAG, OCTAVER, REED, DIFFUSER, GATE, CUTS). A session
  keeps its recipe as before (the first sixteen keep their numbers).
- The RHYTHM's pages: OCT+ / OCT- between TEMPO SWING CHAOS LENGTH and a second page, **MIX**: how much of the gates
  is heard against the loops as they run on (100 %: all gate, as before; 0 %: the loops alone). KICK's bursts and
  ACCENT's pushes follow it. Stored in the rhythm's spare byte: older sessions load at 100 %.
- GATES is now LENGTH (the gates' length; "GATE LENGTH" does not fit beside its bar).
- The start: a DRONE (the same patch as before, named after its recipe).

## Build 9 (2026-11-10)

- CPU0 made room: over the same 30 drawn patches (tools/fm1_bench.py), a block's load is 64 % on average (build 8:
  76 %, build 7: 91 %), 82 % at worst (build 8: 102 %), and no patch runs late (build 8: three did, all six channels
  live). The sound is unchanged: the host's renders are build 8's, bit for bit.
  - Each channel's strip runs inside the sample loop, not called (the call, a channel a sample, cost more than the
    tube); its input is read through a pointer set once a block; sends and pan worked out only when not 0.
  - The parameters copied each block a halfword at a time, in RAM (the compiler's copy went bytewise, to the flash).
  - USB's six channel lanes are made on CPU1 with the output stage, and now line up with the output (before, they
    came a block ahead of it).
  - The tanh, exp2, log2 and filter tables in RAM, off the flash's cache.
- tools/fm1_bench.py costs-of I: a drawn patch's costs, a block in parts (copies, modulation, coefficients, gains,
  samples, the tail), and the CPU's clock (324 MHz).
- The build stays -Os: built -O2, the update loader never answers (the FM-1 sits in update mode until power-cycled),
  and the app comes up on USB but never answers there (UBOOT was the way back).

## Build 8 (2026-11-10)

- The screen reader removed, whole: its voice, its words, GLOBAL SETUP's SETTINGS (SCREEN READER, SPEECH RATE) and
  SHIFT + black key 8. A stored GLOBAL SETUP still loads (its two bytes kept, unused). The image 15 KiB smaller.
- CPU0, measured on the FM-1 (tools/fm1_bench.py): every drawn patch ran at 85 .. 100 % (half of them late), CPU1 at
  20 %. The effects are not the cause (most ~3 % of a sample each; RESONATOR 8.6, DIFFUSER 4.8): the six channel
  strips are (~9 % each, run whether heard or not), and the coefficients worked out anew each block (16 %).
  - A channel's coefficients now each anew only when what they are made of moves (the modulation moved one value,
    the whole channel was redone).
  - A channel that reaches nothing (muted, or LEVEL and both SENDs off, no RHYTHM on it) is not run; back, it starts
    from silence. Its insert, if any, runs on.
  - A shelf or the peak at 0 dB is not run, unless a modulation slot moves it.
  The sound is unchanged: the host's renders are the same as build 7's, bit for bit.
- tools/fm1_bench.py: each effect's, each strip's and a block's cost measured on the FM-1, and the load of drawn
  patches as they play (firmware/src/bench.c, over USB MIDI).

## Build 7 (2026-11-10)

- Fixes builds 5 and 6 coming up without USB (nothing reached the FM-1, not even the installer; UBOOT was the way
  back). The every-block part of the mixer (the coefficients, the modulation, the rhythm, the screen reader's voice (since removed),
  ~12 KiB) ran from the flash, through its small cache; as it grew, the render filled CPU0 (~100 %) and USB, served
  between renders, never started. It runs from RAM now, as Tonitor's renders do. The sound is unchanged, bit for bit.
- USB can no longer be shut out: if the renders leave it no gap for 2 ms, it is served inside one.
- The build fails if anything the render calls each block is left in the flash, or the RAM for code runs low.
- Includes build 6 (the RHYTHM), never installed.

## Build 6 (2026-11-10)

- The RHYTHM, stage one: a lane a channel (up to 32 steps, eight step lengths, accents, gate, depth, chance, rolls,
  mutation), played CHOP, CHOKE, KICK or ACCENT; sixteen styles (INDUSTRIAL, POWER NOISE, BROKEN, AKSAK, POLYMETER,
  POLYRHYTHM, MARCH, HALFTIME, GABBER, ROLLS, CHOP, TRIBAL, TURING, ERRATIC, SPARSE, CLOCKWORK) drawn for the patch
  as it sounds. The 2nd black key from the right: on and off; SHIFT +: a new rhythm; held: the TEMPO page (10 .. 500
  BPM, SWING, CHAOS, GATES; PRESETS through the styles).
- Kept with snapshots and sessions; a new patch keeps it playing. Sessions saved by builds 4 and 5 load, the rhythm
  off.

## Build 5 (2026-11-10)

- Six effects in the rack, none moving pitch: CYCLES (WAVELOSS, SPUTTER, WAVESET), REED (a clarinet's bore, its
  note a fixed step), OCTAVER (one or two octaves down), DIFFUSER (four allpasses), SAG (a power supply starving),
  CUTS (BBCut-style phrases of cuts and rolls; GLITCH unchanged). Their notes, modes, sizes and windows are steps,
  out of the modulation's reach. The families use them.
- Sessions saved by build 4 load as they were.

## Build 4 (2026-11-10)

- FOLD in every channel, after the tube: a wavefolder (x 1.2 .. x 16), more harmonics as it is driven, never more
  level. On the AMP page (TUBE, GAIN, FOLD, OP-AMP; SHIFT: MODE), a modulation destination, in the families.
- Four effects in the rack: RESONATOR (six tuned resonators, a root and a chord, fixed: no modulation reaches its
  tuning), XMOD (ring, XOR, fold, compare with another channel or bus), DUCK and GATE (keyed by another channel or
  bus). The families use them.
- DELAY's and FLANGER's feedback keep their rounding error (after ChoralRoot's FM-1 delay): repeats fade as they
  should.
- No pitch shifting or frequency shifting added, on purpose.
- Build numbers only: the release option gone; each build tagged build-N in git and kept as
  build/lilliput-build-N.fwsc.
- Sessions saved by build 3 are not read.

## Build 3 (2026-11-10)

- The modulation on pages of its own, the channel's last: LFO 1, LFO 2, and the matrix -- now eight slots -- on
  MOD 1 TO 4 and MOD 5 TO 8 (KNOB: amount; SHIFT + KNOB: destination; PRESETS: source of the slot in hand).
- SHIFT + black key 1: the chosen channel, every parameter at random (its mute kept).
- The GESTURE loop: REC arms, the first change records, REC ends and plays; PLAY plays or stops it; REC held empties
  it. Every change on any channel. The output's mute moves to SHIFT + PLAY.
- SNAPSHOTS: SAVE short opens them; sixteen whole-mixer states on the white keys (held: store; short: recall;
  SHIFT: empty), with a morph time (KNOB 1, up to 30 s). Kept with the session, as is the gesture loop.
- The screen: a second canvas (one band drawn while the last is sent), the panel's link at 20 MHz (was 12), the main
  view drawn without a wipe -- pages change about twice as fast.
- Sessions saved by build 2 are not read.

## Build 2 (2026-11-10)

- LEVEL: 1 dB a detent up to -5 dB, then 0.1 dB a detent; PAN: one step a detent, L 50 .. R 50.
- Sixteen families of patches, as Tonitor's: each its connections, its voicing, its effects, its space and its
  motion. HOME: another family; SHIFT + HOME: the same family; SHIFT + PRESETS: the family before or after; HOME held:
  the nudge.
- Tonitor's LFOs and modulation matrix on every channel: the MOD page (after FX), SHIFT + SEQ for a random one;
  sources LFO 1 and 2, the channel's own loudness, the output's, a random value at each onset.
- The pages redrawn: names small, values large on their own line, nothing over anything; SHIFT held shows a page's
  switches; text sized to fit, never squeezed. A test checks every word fits.
- The trim keeps the peaks under -4 dB, and a guard brings the main level down if the output holds at its ceiling.
- Sessions saved by build 1 are not read.

## Build 1 (2026-11-10)

The first build of Lilliput: a no-input mixer for the M-VAVE FM-1, after NoInputMixer, on Tonitor's screen and
platform (Tonitor build 74), with none of Tonitor's engines.

- Six channels a sample at a time: input selector (PRE, POST, MIX L/R, SEND 1/2, RET 1/2, NONE, CH 1-6), noise
  generator (six types), preamp, six tubes, six op-amps, overload protection, shelves or cuts, peak, notch or
  band-pass, brickwall limiter, two sends, level, pan, main mix.
- A rack of seven slots: DELAY, FLANGER, CHORUS, TREMOLO, FORMANT, BITCRUSH, GLITCH.
- The output: returns, equaliser, insert, SPACE (six reverb spaces), main level; on CPU1.
- HOME draws a new patch from six recipes; SHIFT + HOME nudges it; each patch woken up and trimmed to one loudness.
- Sixteen factory patches, 64 sessions (their own record: other firmwares' data left alone), GLOBAL SETUP, MIDI
  CCs, USB audio of the output or the six channels, the screen reader.
