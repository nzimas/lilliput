# Changelog

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
