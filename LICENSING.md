# Lilliput licensing

The Lilliput **firmware** is free software, licensed under the GNU General Public License, version 3 only
(`GPL-3.0-only`, full text in `LICENSE`). If you distribute it, or firmware built from it, give your
recipients its complete corresponding source under the same licence.

The **documentation** written for Lilliput is under the MIT licence (`LICENSES/MIT.txt`).

Lilliput is a work based on **Tonitor** (Nuno Zimas), itself based on **Felucca** (Copyright (C) 2026 Leo
Kuroshita (@kurogedelic), Hügelton Instruments) and **SLOOP** (isod89), all GPL-3.0-only. Files taken from
them keep their copyright headers; changes are noted in each file. No sound engine of Tonitor or Bigorna is
in it.

Lilliput follows the signal flow and the controls of **NoInputMixer** (Igor Vasiliev), as its manual and
block diagram describe them. No code of that app is in this tree: the mixer is written anew, in fixed point,
for the FM-1.

| What | Licence | Where |
| --- | --- | --- |
| Lilliput's own code: `firmware/src/{lilliput,mixer,patch,lui,lses,logo}.c`, `tests/host/host.c` | GPL-3.0-only | |
| From Tonitor: `firmware/src/{reverb,gfx,voice,setup,backup,midi,main}.c`, `dsp.h`, `tools/{build,gen_tables,gen_font,gen_voice,sdk_from_package}.py`, `tests/`, the scripts (adapted) | GPL-3.0-only | |
| From **Melodee** 1.4.5 (Copyright (C) 2026 Kerem Kilic (Ellic Studio), keremimo/melodee), as adapted in BIGORNA (Nuno Zimas): the dual-core start, hold and fault handling, its job mailbox and the stock supply setup -- `firmware/hal/{fm1_multicore,fm1_core1_stop,fm1_core1_bar,fm1_power}.h`, `firmware/src/audio_worker.h`, the CPU1 parts of `firmware/hal/{fm1_irq,fm1_sys}.h`, `fm1_vec.S` and `crt0.S` | GPL-3.0-only | <https://github.com/keremimo/melodee> |
| The browser installer, a port of `tools/fm1_install.py` (Felucca / SLOOP) to JavaScript | GPL-3.0-only | `docs/js/fm1ota.js` |
| From Felucca / SLOOP: `firmware/hal/`, `firmware/loader/`, `firmware/crt0.S`, `firmware/app.ld`, `firmware/src/{usb,ota,recovery,lcd,libc,midi_uart}.c`, `firmware/src/uac_fir.h`, `bootguard.h`, `tools/{fm1pkg_make,lz4blk,fm1_install,gen_uac_fir}.py`, `tools/get_toolchain.sh`, parts of `tools/build.py` and `tools/gen_tables.py` | GPL-3.0-only | |
| The documentation's text | MIT | `README.md`, `CHANGELOG.md`, `LICENSES/MIT.txt` |
| Terminus Bold 8x16 (the screen's fallback font) | SIL OFL 1.1 | `assets/fonts/ter-u16b.bdf`, `assets/fonts/Terminus-LICENSE.txt` |
| Atkinson Hyperlegible Bold (the screen's type, if `./setup.sh --font` fetched it) | SIL OFL 1.1 | `assets/fonts/` |
| JieLi AC79 SDK: `uboot.boot`, `cfg_tool.bin`, `eq_cfg_hw.bin` are read from your SDK checkout at build time and placed in the package; no SDK file is in this tree | Apache-2.0 | <https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK> |
