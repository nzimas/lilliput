#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# LILLIPUT host tests (no toolchain, no FM-1 needed): the generated tables and fonts, the whole firmware
# checked for a runtime the chip lacks, then the DSP, the sounds, the layers and the screen run on the
# computer. Renders land in build/sounds (WAV) and build/screens (PNG).
set -e
cd "$(dirname "$0")/.."
PY=python3
[ -x .venv/bin/python ] && PY=.venv/bin/python
mkdir -p build/gen build/sounds build/screens
$PY tools/gen_tables.py build/gen/tonitor_tables.h >/dev/null
$PY tools/gen_font.py build/gen/tonitor_font.h >/dev/null
$PY tools/gen_voice.py tools/voice_words.txt build/gen/voice_dict.h >/dev/null
tests/check_target_ir.sh
clang -O2 -std=gnu99 -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare -Wno-unused-function -Wno-format -Werror \
    -Ifirmware/src -Ibuild/gen tests/host/host.c -o build/host
build/host check
build/host fits
build/host effects
build/host flows
build/host screens build/screens
c2=$(build/host cores 2)                              # the two cores: the same sound
c1=$(build/host cores 1)
echo "$c2"
[ "$(echo "$c2" | sed 's/.*hash //')" = "$(echo "$c1" | sed 's/.*hash //')" ] || { echo "FAIL cores: two cores and one differ"; exit 1; }
build/host render build/sounds >/dev/null
clang -O2 -std=gnu99 -Wall -Wextra -Werror -Ifirmware/hal tests/host/flash_id.c -o build/flash_id
build/flash_id
clang -O2 -std=gnu99 -Wall -Wextra -Wno-unused-function -Werror -Ifirmware/src -Ibuild/gen tests/host/voice_demo.c \
    -o build/voice_demo
build/voice_demo -c
for f in build/screens/*.ppm; do $PY -c "from PIL import Image; import sys; Image.open(sys.argv[1]).save(sys.argv[1][:-4] + '.png')" "$f" 2>/dev/null || true; done
if [ -f build/lilliput.fwsc ] && command -v node >/dev/null 2>&1; then    # the browser installer (after a build)
    $PY tests/web/python_facts.py build/lilliput.fwsc > build/facts.json
    node tests/web/test_fm1ota.mjs build/lilliput.fwsc build/facts.json
fi
echo "tests: all passed"
