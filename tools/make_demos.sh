#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# LILLIPUT (as NORILSK's): the website's demos -- build/host demo renders three pieces from the machine (tests/host/host.c
# cmd_demo); here each is faded in and out, brought to about -16 LUFS and made an MP3 in docs/assets/demo/.
# Needs ffmpeg with libmp3lame. Run tests/run_tests.sh (or build the host) first.
set -e
cd "$(dirname "$0")/.."
mkdir -p build/demo docs/assets/demo
build/host demo build/demo > build/demo/demo.txt
for n in beating under-the-kick ritual; do
    ffmpeg -loglevel error -y -i build/demo/demo-$n.wav \
        -af "afade=t=in:d=2.5,afade=t=out:st=44:d=4,loudnorm=I=-16:TP=-1.5:LRA=11" \
        -ar 44100 -c:a libmp3lame -q:a 2 docs/assets/demo/$n.mp3
done
cat build/demo/demo.txt
ls -la docs/assets/demo
