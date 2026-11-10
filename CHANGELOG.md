# Changelog

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
