# Changelog

## Build 27 (2026-11-12)

- Snapshots are whole machine states: storing one keeps DRUMS (voices, rhythm as drawn and its DENSITY, SWING,
  VARIATION, mutes, on or off) and COMPRESSOR too, besides mixer, RHYTHM and tempo; recall restores all of it at
  once. Kept with sessions; a session from before keeps its snapshots' mixer only.
- MORPH gone: a recall is at once; SNAPSHOTS page's KNOB 1 does nothing.
- Gesture loops: 1664 changes among all loops (was 2048), room for snapshots' drums. A session holding more no
  longer loads.

## Build 26 (2026-11-12)

- DENSITY shows its dial: PRESETS on drums' pages brings up a rotary dial over page, as main view's knobs do,
  DENSITY and its value 0 … 50; page back a second after last detent.

## Build 25 (2026-11-12)

- **DENSITY** on drums' pages, PRESETS (before: through drum rhythms; SHIFT + DRUMS key still draws a new one): 0 …
  50, a step a detent, rhythm drawn at 25. Left starves it, weakest hits first, to silence at 0; right fills it,
  pick-ups, doubles and syncopations first, half of them short (ALT), up to 55 % of lane's rests at 50 (on host,
  200 rhythms: 7 hits at 10, 18 at 25, 47 at 50 of 70 steps). Same way back; VARIATION plays on it; new rhythm or
  session: 25.
- REED's NOTE worked out in main loop (audio RAM: 1128 B spare).

## Build 24 (2026-11-12)

- New patch at its loudness from 1 s on: trim takes a first look at 0.25 … 0.9 s and sets main level at once
  (before: ~10 dB down until 3.6 s, as if ducked). Its look at 3.6 s now only mends what the loops woken since
  changed; while patch is new, guard is quicker (−3 dB each 120 ms at ceiling).
- Gesture loops are yours, not patch's: a new patch leaves them playing, same moves over new patch. A REC armed
  stays armed; patch's own change is never recorded into a loop. A session still stops them.
- At start: tempo drawn 70 … 150 BPM (was 91 … 299); COMPRESSOR on at 85 %, its RELEASE an eighth note at the
  tempo (70 BPM 429 ms, 120 BPM 250 ms, 150 BPM 200 ms), following TEMPO until RELEASE is turned or a session
  loaded. With it, mix about 6 dB more forward.

## Build 23 (2026-11-12)

- HOME's patches: output insert (OUT FX) in about half of patches (was 1 in 7), its type and settings drawn from
  the recipe, at 20 … 60 % PRESENCE. Never DUCK, GATE, LP GATE or XMOD there (with quiet key, whole mix silent),
  nor REED (its 1 note over whole mix rang into clip with drums).

## Build 22 (2026-11-11)

- Drums: no pitch moves. Every voice keeps its TUNE on every step -- no melodic lines on perc, no pitched toms, in
  rhythms and in breaks (AVALANCHE no longer rises; TUMBLE falls in loudness, from an accent to ghosts). Sessions
  saved with pitched steps load with them level.
- HOME's patches feature the rack: 2 or 3 effects heard in every patch (before: 1 on average, a third of patches
  none), inserts at 55 … 100 % PRESENCE (was 30 … 85 %), often inside the channel's loop; no effect twice in a
  patch; each recipe's effects about equally likely, so builds 5 and 20's effects come up as often as the first
  ones (each in about 1 patch in 10). CROSSOVER's dead zone drawn narrow, so a loop can begin.

## Build 21 (2026-11-11)

- Drum rhythms anew, away from four on the floor: 16 styles, each a way of building a rhythm, drawn afresh each
  time -- AKSAK, PHASING, EUCLID MESH, POLYRHYTHM, TIMELINE, COLOTOMIC, CASCADE, LIMP, DUST, CANON, HOCKET,
  RITUAL, BROKEN, TRIBAL, DIST BREAK, TEMPO CANON. Odd meters, lanes of unequal lengths (polymeter), triplets
  against quintuplets against 16ths (polyrhythm), timelines, pitched toms and perc lines. POWER 4/4, PISTON,
  MARCH, OFFBEAT, RUMBLE, 3 AGAINST 4, SHUFFLE, HALF RITUAL and BURST FILL are gone.
- **VARIATION** (drums' page, KNOB 2): now and then a lane's round is played otherwise -- a hit dropped, a
  pick-up, a step pushed, an accent moved, a hit doubled, the lane turned, a half backwards, a round's rest -- then
  home again. Sessions from before load at 0, as they were.

## Build 20 (2026-11-11)

- 4 **shapers** as tube MODEs on AMP page (SHIFT + KNOB 1): SPIRAL, DENSITY, FUZZ, HARD CLIP. Driven as
  SATURATED, into another curve; SPIRAL folds inward when driven hard.
- 6 rack effects: **AUTO-LEVEL** (holds loudness at LEVEL), **LP GATE** (vactrol low-pass gate, opened by its
  KEY), **CROSSOVER** (DEAD ZONE, INSIDE OUT), **FRICTION** (stick-slip mass on a spring, ringing at SPRING's
  pitch), **GATELOPE** (gate closing by sweeping its filters inward), **FREEZE** (a loop of LENGTH, frozen or
  fading). None moves pitch; SPRING and LENGTH are steps out of modulation's reach.
- HOME's recipes draw them: a third of saturated tubes a shaper; each recipe its own weights for the effects.
- RESONATOR's tuning, tubes and op-amps worked out outside audio's time: same sound, more room for audio code.

## Build 19 (2026-11-11)

- RHYTHM's key (2nd black key from right) works as drums' key: short, its pages (TEMPO, MIX; again: back); held
  0.8 s, rhythm on or off; SHIFT + it, new rhythm, as before.
- **BREAK**: on drums' pages, white key 16 (last, under SHIFT), held, plays variation of drums' rhythm while held:
  STUTTER, AVALANCHE, FLAMS, DROP, HALF TIME, CELL, SWAP, MIRROR, TUMBLE, INTERLOCK, SCATTER, BUZZ, GHOSTS. Each
  press draws one, named on the page. Quantized: it starts on rhythm's next beat (half bar for HALF TIME and
  MIRROR); fills land on bar's downbeat. Let go, rhythm carries on in place.
- Drum steps can now be ghosts (soft), besides plain and accented: breaks' grace notes, rolls and fills use them.

## Build 18 (2026-11-11)

- Logo: name drawn on a single grid -- "lilliput" from bare strokes, squares and 2 arcs, every stroke 1 gap from
  the next, so the word reads as a pulse train; its icon, "li". In assets/logo (SVG: logotype in white, in black
  and with channels' colours; icon), made by tools/gen_logo.py. Splash draws it 1 unit to a pixel, build
  under it; README opens with it.

## Build 17 (2026-11-11)

- New RHYTHM for the mixer no longer restarts drums (it did, audibly). Single beat clock runs both: whichever
  starts, or gets new pattern, comes in on next beat; the other plays on untouched.
- Drums' rhythms anew, after the genre (Bigorna's generator; Attack Magazine's industrial and dark Berlin techno
  breakdowns; genre's descriptions): POWER 4/4, PISTON, MARCH, OFFBEAT, RUMBLE, BROKEN, 3 AGAINST 4, TRIBAL,
  SHUFFLE, DIST BREAK, HALF RITUAL, BURST FILL. Every lane in 16ths, 16 or 32 steps: pace is always the
  tempo's (before, kicks on every 16th read as double time, and 7-, 9- and 10-step lanes or hat at 1/8 lost
  the pace). Swing only where the genre swings.
- 4th voice, **PERC**: struck metal (ANVIL, PIPE, SHEET, CLANK, CHAIN, SPRING, GRIND), white key 4 on
  drums' pages.
- Voice's white key held 0.7 s mutes it (its key blinks, dim; its page says MUTED); held again, back.
- Drums' own LEVEL is gone from their 3rd page: voice's LEVEL is the only one.
- **COMPRESSOR**, SEQ's first page: on mixer's sound, before drums; AMOUNT compresses, brings mix forward
  and, with SIDECHAIN on KICK, ducks it under each kick (up to 18 dB) -- more AMOUNT, more lift and more pump.
  THRESH, RELEASE; with SHIFT, ATTACK and RATIO. Sessions keep it.
- Sessions saved with build 16's drums load with PERC resting.
- Sine table moved from RAM for code to RAM's pool (2 KiB of code room back); sound is the same, bit
  for bit.

## Build 16 (2026-11-10)

- **DRUMS**: drum section beside the mixer, after Ruismaker Noir (Bram Bos): KICK, SNARE and HATS, a single engine
  (swept tone and filtered noise, ATTACK from hard transient to 100 ms, DECAY, CLICK, METAL, DRIVE), each with
  its lane of steps (accents, pitches, ALT) on RHYTHM's tempo, coming in on its beat. Black key 9 (3rd from the
  right): short, their pages; SHIFT +, new drum rhythm; held, on or off. On their pages white keys 1-3 choose
  voice (SHIFT +: new sound), PRESETS the rhythms. 21 sound recipes (7 a voice), 12 rhythm recipes. REVERB send
  a voice into SPACE; dry drums after reverb. Sessions keep them; older sessions load without touching them.
  They run on CPU1 with output stage.
- MASTER knob is read before any sound: splash played for about 1 second at half volume, knob down or
  not.
- tools/fm1_bench.py patches-drums: load with drums at their densest.

## Build 15 (2026-11-10)

- HOME and SHIFT + HOME as on Tonitor: **HOME** new patch from any recipe, always; **SHIFT + HOME** new patch from
  the recipe named in main view (playing patch's, or the one PRESETS turned to). PRESETS turns through
  recipes from the one playing. ANY is gone.

## Build 14 (2026-11-10)

- No more silence under RHYTHM: in host's test every recipe with every style (400 pairs) now sounds,
  quietest at -33 dB (build 13: 6 near silent, down to -58 dB). 3 causes, 3 fixes:
  - Channel in a ring was choked by KICK and CHOKE: ring died between hits and never grew back. Now its
    gate cuts what is heard, and ring runs on.
  - Sparse lane (a hit or 2 a bar, short) left its channel all but silent: it is chopped now, over a bed 14 dB
    down.
  - Trim measured a quiet output, chopped by RHYTHM, as silence, and never brought it up (32 dB short in
    1 case). Its measure is finer now.
- RHYTHM's test plays every recipe with every style, and fails under -40 dB (before: every 3rd recipe, -50).

## Build 13 (2026-11-10)

- SEQ now reaches chosen channel's motion: OUT, OUT EQ, OUT FX, then LFO 1, LFO 2, MOD 1 TO 4, MOD 5 TO 8, then
  back to channel's page. OCT- / OCT+ step through them there. Channel's own OCT pages end at FX.
- New recipe, **FREE**: no direction, anything goes as long as it sounds (every connection, voice, effect and space
  as likely as the next, over the widest ranges; its gestures to destinations drawn per channel). On PRESETS it sits
  last, beside ANY. Tested: 200 FREE patches, none quiet, none clipped.
- Muted channel is grey too, even while RHYTHM runs.
- Known: RHYTHM's KICK and CHOKE can choke a ring of channels to near silence (6 of 400 recipe x style pairs in
  host's test, in several recipes, FREE among them). Not changed yet.

## Build 12 (2026-11-10)

- Gesture loop for each channel, each on its own (as Tonitor's looper is a layer's): REC and PLAY act on
  chosen channel; channel's loop records only that channel (its parameters and its effect slot) and replaces the
  loop it had (no overdubs). Loops play together, each its own length. Only 1 records at a time: REC on another
  channel ends recording and arms that one. Sessions keep each channel's loop; session saved before, with 1
  loop for all, loads as a loop for each channel it touched (output's and RHYTHM's changes left out).
- HOME draws from **any** recipe until PRESETS chooses one (as on Tonitor). PRESETS has a position for that, **ANY**,
  before the first recipe and after the last. SHIFT + HOME: another recipe than the one playing. Session loaded:
  none chosen.
- Each start is a new patch, from any recipe, and its tempo is drawn between 91 and 299 BPM (before: always the same
  DRONE at 120). Randomness comes from ADCs' noise and clock at power-on.
- Channel not in the patch (muted, or its LEVEL and SENDs off with no RHYTHM) is drawn in grey: its number, its
  name, its meter, its pages; in OVERVIEW too.

## Build 11 (2026-11-10)

- PRESETS no longer flashes "RECIPE": turning it changes the name in main view, in place, to the recipe's (from
  another page it goes to main view). Before, the flash went and main view still named the patch playing.
  HOME draws from the recipe named; turned back to patch's own recipe, patch's name is shown again.

## Build 10 (2026-11-10)

- PRESETS are gone: there were 16 fixed "factory patches" and a page of them. Knob now shows the **RECIPE**
  (an archetype, never a fixed patch): its name, flashed, and nothing else changes. **HOME** draws new patch from
  the recipe shown, otherwise each time; **SHIFT + HOME** from another recipe, at random. No page of its own.
- 8 new recipes, 24 in all: CHOIR, ENGINE, INSECTS, ORGAN, MORSE, FURNACE, AURORA, SONAR. They reach
  for effects the first 16 use little (RESONATOR, XMOD, SAG, OCTAVER, REED, DIFFUSER, GATE, CUTS). Session
  keeps its recipe as before (the first 16 keep their numbers).
- RHYTHM's pages: OCT+ / OCT- between TEMPO SWING CHAOS LENGTH and a 2nd page, **MIX**: how much of the gates
  is heard against loops as they run on (100 %: all gate, as before; 0 %: loops alone). KICK's bursts and
  ACCENT's pushes follow it. Stored in rhythm's spare byte: older sessions load at 100 %.
- GATES is now LENGTH (gates' length; "GATE LENGTH" does not fit beside its bar).
- Start: a DRONE (the same patch as before, named after its recipe).

## Build 9 (2026-11-10)

- CPU0 made room: over the same 30 drawn patches (tools/fm1_bench.py), block's load is 64 % on average (build 8:
  76 %, build 7: 91 %), 82 % at worst (build 8: 102 %), and no patch runs late (build 8: 3 did, all 6 channels
  live). Sound is unchanged: host's renders are build 8's, bit for bit.
  - Each channel's strip runs inside sample loop, not called (the call, a channel a sample, cost more than the
    tube); its input is read through pointer set once a block; sends and pan worked out only when not 0.
  - Parameters copied each block a halfword at a time, in RAM (compiler's copy went bytewise, to flash).
  - USB's 6 channel lanes are made on CPU1 with output stage, and now line up with output (before, they
    came a block ahead of it).
  - The tanh, exp2, log2 and filter tables in RAM, off flash's cache.
- tools/fm1_bench.py costs-of I: drawn patch's costs, a block in parts (copies, modulation, coefficients, gains,
  samples, tail), and CPU's clock (324 MHz).
- Build stays -Os: built -O2, update loader never answers (FM-1 sits in update mode until power-cycled),
  and app comes up on USB but never answers there (UBOOT was the way back).

## Build 8 (2026-11-10)

- Screen reader removed, whole: its voice, its words, GLOBAL SETUP's SETTINGS (SCREEN READER, SPEECH RATE) and
  SHIFT + black key 8. Stored GLOBAL SETUP still loads (its 2 bytes kept, unused). Image 15 KiB smaller.
- CPU0, measured on FM-1 (tools/fm1_bench.py): every drawn patch ran at 85 .. 100 % (half of them late), CPU1 at
  20 %. Effects are not the cause (most ~3 % of a sample each; RESONATOR 8.6, DIFFUSER 4.8): the 6 channel
  strips are (~9 % each, run whether heard or not), and coefficients worked out anew each block (16 %).
  - Channel's coefficients now each anew only when what they are made of moves (modulation moved 1 value,
    the whole channel was redone).
  - Channel that reaches nothing (muted, or LEVEL and both SENDs off, no RHYTHM on it) is not run; back, it starts
    from silence. Its insert, if any, runs on.
  - Shelf or peak at 0 dB is not run, unless a modulation slot moves it.
  Sound is unchanged: host's renders are the same as build 7's, bit for bit.
- tools/fm1_bench.py: each effect's, each strip's and a block's cost measured on FM-1, and load of drawn
  patches as they play (firmware/src/bench.c, over USB MIDI).

## Build 7 (2026-11-10)

- Fixes builds 5 and 6 coming up without USB (nothing reached FM-1, not even the installer; UBOOT was the way
  back). Every-block part of the mixer (coefficients, modulation, rhythm, screen reader's voice (since removed),
  ~12 KiB) ran from flash, through its small cache; as it grew, render filled CPU0 (~100 %) and USB, served
  between renders, never started. It runs from RAM now, as Tonitor's renders do. Sound is unchanged, bit for bit.
- USB can no longer be shut out: if the renders leave it no gap for 2 ms, it is served inside one.
- Build fails if anything the render calls each block is left in flash, or RAM for code runs low.
- Includes build 6 (the RHYTHM), never installed.

## Build 6 (2026-11-10)

- RHYTHM, stage 1: a lane a channel (up to 32 steps, 8 step lengths, accents, gate, depth, chance, rolls,
  mutation), played CHOP, CHOKE, KICK or ACCENT; 16 styles (INDUSTRIAL, POWER NOISE, BROKEN, AKSAK, POLYMETER,
  POLYRHYTHM, MARCH, HALFTIME, GABBER, ROLLS, CHOP, TRIBAL, TURING, ERRATIC, SPARSE, CLOCKWORK) drawn for the patch
  as it sounds. 2nd black key from the right: on and off; SHIFT +: new rhythm; held: TEMPO page (10 .. 500
  BPM, SWING, CHAOS, GATES; PRESETS through styles).
- Kept with snapshots and sessions; new patch keeps it playing. Sessions saved by builds 4 and 5 load, rhythm
  off.

## Build 5 (2026-11-10)

- 6 effects in the rack, none moving pitch: CYCLES (WAVELOSS, SPUTTER, WAVESET), REED (clarinet's bore, its
  note a fixed step), OCTAVER (1 or 2 octaves down), DIFFUSER (4 allpasses), SAG (power supply starving),
  CUTS (BBCut-style phrases of cuts and rolls; GLITCH unchanged). Their notes, modes, sizes and windows are steps,
  out of modulation's reach. Families use them.
- Sessions saved by build 4 load as they were.

## Build 4 (2026-11-10)

- FOLD in every channel, after tube: wavefolder (x 1.2 .. x 16), more harmonics as it is driven, never more
  level. On AMP page (TUBE, GAIN, FOLD, OP-AMP; SHIFT: MODE), modulation destination, in families.
- 4 effects in the rack: RESONATOR (6 tuned resonators, root and chord, fixed: no modulation reaches its
  tuning), XMOD (ring, XOR, fold, compare with another channel or bus), DUCK and GATE (keyed by another channel or
  bus). Families use them.
- DELAY's and FLANGER's feedback keep their rounding error (after ChoralRoot's FM-1 delay): repeats fade as they
  should.
- No pitch shifting or frequency shifting added, on purpose.
- Build numbers only: release option gone; each build tagged build-N in git and kept as
  build/lilliput-build-N.fwsc.
- Sessions saved by build 3 are not read.

## Build 3 (2026-11-10)

- Modulation on pages of its own, the channel's last: LFO 1, LFO 2, and matrix -- now 8 slots -- on
  MOD 1 TO 4 and MOD 5 TO 8 (KNOB: amount; SHIFT + KNOB: destination; PRESETS: source of slot in hand).
- SHIFT + black key 1: chosen channel, every parameter at random (its mute kept).
- GESTURE loop: REC arms, first change records, REC ends and plays; PLAY plays or stops it; REC held empties
  it. Every change on any channel. Output's mute moves to SHIFT + PLAY.
- SNAPSHOTS: SAVE short opens them; 16 whole-mixer states on white keys (held: store; short: recall;
  SHIFT: empty), with morph time (KNOB 1, up to 30 s). Kept with session, as is the gesture loop.
- Screen: a 2nd canvas (1 band drawn while the last is sent), panel's link at 20 MHz (was 12), main
  view drawn without a wipe -- pages change about twice as fast.
- Sessions saved by build 2 are not read.

## Build 2 (2026-11-10)

- LEVEL: 1 dB a detent up to -5 dB, then 0.1 dB a detent; PAN: 1 step a detent, L 50 .. R 50.
- 16 families of patches, as Tonitor's: each its connections, its voicing, its effects, its space and its
  motion. HOME: another family; SHIFT + HOME: same family; SHIFT + PRESETS: family before or after; HOME held:
  nudge.
- Tonitor's LFOs and modulation matrix on every channel: MOD page (after FX), SHIFT + SEQ for a random one;
  sources LFO 1 and 2, channel's own loudness, output's, a random value at each onset.
- Pages redrawn: names small, values large on their own line, nothing over anything; SHIFT held shows a page's
  switches; text sized to fit, never squeezed. Test checks every word fits.
- Trim keeps peaks under -4 dB, and a guard brings main level down if output holds at its ceiling.
- Sessions saved by build 1 are not read.

## Build 1 (2026-11-10)

First build of Lilliput: a no-input mixer for the M-VAVE FM-1, after NoInputMixer, on Tonitor's screen and
platform (Tonitor build 74), with none of Tonitor's engines.

- 6 channels a sample at a time: input selector (PRE, POST, MIX L/R, SEND 1/2, RET 1/2, NONE, CH 1-6), noise
  generator (6 types), preamp, 6 tubes, 6 op-amps, overload protection, shelves or cuts, peak, notch or
  band-pass, brickwall limiter, 2 sends, level, pan, main mix.
- Rack of 7 slots: DELAY, FLANGER, CHORUS, TREMOLO, FORMANT, BITCRUSH, GLITCH.
- Output: returns, equaliser, insert, SPACE (6 reverb spaces), main level; on CPU1.
- HOME draws new patch from 6 recipes; SHIFT + HOME nudges it; each patch woken up and trimmed to a single loudness.
- 16 factory patches, 64 sessions (their own record: other firmwares' data left alone), GLOBAL SETUP, MIDI
  CCs, USB audio of output or the 6 channels, screen reader.
