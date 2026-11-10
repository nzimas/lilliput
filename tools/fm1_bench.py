#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""LILLIPUT's costs measured on the FM-1 itself, over USB MIDI (firmware/src/bench.c).

  tools/fm1_bench.py costs            each effect, each channel strip, the strip's parts, a block's work
  tools/fm1_bench.py costs-of I        the same, with drawn patch I loaded (as patches draws it)
  tools/fm1_bench.py patches N [S]    N drawn patches (the host's check seeds: 0xC0FFEE + i x 7919), S s each:
                                      the load as they play (CSV in build/bench/patches.csv)

The output is muted while it measures. Needs mido with python-rtmidi (as the installer).
"""
import csv
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import fm1_install as fi  # noqa: E402

FX = ["DELAY", "FLANGER", "CHORUS", "TREMOLO", "FORMANT", "BITCRUSH", "GLITCH", "RESONATOR", "XMOD", "DUCK", "GATE",
      "CYCLES", "REED", "OCTAVER", "DIFFUSER", "SAG", "CUTS"]
VARIANTS = ["bare", "+ noise (pink)", "+ fold", "+ tube saturated", "+ shelves", "+ peak", "+ limiter slow", "all"]
TICKS_PER_SAMPLE = 24e6 / 44100          # CPU0's whole budget for one sample, in TIMER4 ticks
BN_N, CTL = 256, 32


def frame(cmd, body=b""):
    n = len(body)
    m = bytearray([0x00, 0x59, cmd, n & 0xFF, (n >> 8) & 0xFF, (n >> 16) & 0xFF]) + body
    m.append(~sum(body) & 0xFF)
    return b"\xF0" + fi.pack7(m) + b"\xF7"


def reply(link, cmd, timeout):
    end = time.monotonic() + timeout
    while (left := end - time.monotonic()) > 0:
        p = link.read(left)
        if p is None:
            break
        u = fi.unpack7(p[1:-1])
        if len(u) >= 7 and u[:3] == bytes([0x00, 0x59, cmd]):
            n = u[3] | u[4] << 8 | u[5] << 16
            body = u[6:6 + n]
            if ~sum(body) & 0xFF == u[6 + n]:
                return body
    raise SystemExit(f"no answer to 0x{cmd:02x}")


def u32s(b):
    return [int.from_bytes(b[i:i + 4], "little") for i in range(0, len(b) - 3, 4)]


def costs(link):
    link.send(frame(0x60))
    b = reply(link, 0x60, 30)
    v = u32s(b[1:1 + 4 * b[0]])
    nfx, nch = len(FX), 6

    def pct(t, n):                         # share of CPU0's per-sample budget
        return 100.0 * t / n / TICKS_PER_SAMPLE
    print(f"{'effect':12s} {'ns/sample':>10s} {'% of a sample':>14s}")
    for name, t in sorted(zip(FX, v[:nfx]), key=lambda x: -x[1]):
        print(f"{name:12s} {t / BN_N / 0.024:10.0f} {pct(t, BN_N):13.1f}%")
    print()
    for c, t in enumerate(v[nfx:nfx + nch]):
        print(f"channel {c + 1} as patched {t / BN_N / 0.024:8.0f} ns  {pct(t, BN_N):5.1f}%")
    print()
    for name, t in zip(VARIANTS, v[nfx + nch:nfx + nch + len(VARIANTS)]):
        print(f"strip {name:18s} {t / BN_N / 0.024:8.0f} ns  {pct(t, BN_N):5.1f}%")
    mx, out, coef, allb = v[nfx + nch + len(VARIANTS):][:4]
    parts = v[nfx + nch + len(VARIANTS) + 4:][:7]
    clk = v[nfx + nch + len(VARIANTS) + 11:][:2]
    print()
    for name, t in (("channels + slots (mx_samples)", mx), ("output stage", out),
                    ("coefficients + modulation", coef), ("mix_block", allb)):
        print(f"block: {name:32s} {t / CTL / 0.024:8.0f} ns/sample  {pct(t, CTL):5.1f}%")
    if len(parts) == 7:
        print()
        for name, t in zip(("copies + RHYTHM", "modulation", "channel coefficients", "channel gains",
                            "slot + output coefficients", "samples (mx_samples)", "tail (output, USB, meters)"), parts):
            print(f"part:  {name:32s} {t / CTL / 0.024:8.0f} ns/sample  {pct(t, CTL):5.1f}%")
    if clk and clk[0]:
        print(f"\nCPU clock: about {64000 / (clk[0] / 24):.0f} MHz (64000 adds in {clk[0] / 24:.0f} us, from the flash)")
    if len(clk) > 1 and clk[1]:
        print(f"the same adds from the RAM: {clk[1] / 24:.0f} us ({clk[1] / clk[0]:.2f} x)")


def patches(link, n, secs):
    out = Path(__file__).resolve().parent.parent / "build" / "bench"
    out.mkdir(parents=True, exist_ok=True)
    rows = []
    for i in range(n):
        seed = (0xC0FFEE + i * 7919) & 0xFFFFFFFF
        link.send(frame(0x61, seed.to_bytes(4, "little") + bytes([255, secs])))
        b = reply(link, 0x61, secs + 10)
        fam, (mean, peak, late, m0, m0pk, m1pk) = b[0], u32s(b[1:25])
        rows.append(dict(i=i, seed=seed, fam=fam, load=mean / 10, peak=peak / 10, late=late,
                         cpu0_block=round(100 * m0 / (CTL * 1e6 / 44100 * 24), 1),
                         cpu0_peak=round(100 * m0pk / (CTL * 1e6 / 44100 * 24), 1),
                         cpu1_peak=round(100 * m1pk / (CTL * 1e6 / 44100 * 24), 1)))
        r = rows[-1]
        print(f"{i:3d} fam {fam:2d}  load {r['load']:5.1f}%  peak {r['peak']:5.1f}%  late {late:3d}  "
              f"CPU0 block {r['cpu0_block']:5.1f}% (peak {r['cpu0_peak']:5.1f}%)  CPU1 peak {r['cpu1_peak']:5.1f}%",
              flush=True)
    with open(out / "patches.csv", "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=list(rows[0]))
        w.writeheader()
        w.writerows(rows)
    loads = sorted(r["peak"] for r in rows)
    print(f"\npeak load: median {loads[len(loads) // 2]:.1f}%, worst {loads[-1]:.1f}%, "
          f"{sum(x >= 90 for x in loads)} of {len(loads)} at 90% or more")


def main():
    if len(sys.argv) < 2 or sys.argv[1] not in ("costs", "patches", "costs-of"):
        raise SystemExit(__doc__)
    up = fi.Updater(fi.MidoBackend())
    dev = up.find(lambda i: not i.loader)
    if not dev:
        raise SystemExit("FM-1 not found")
    try:
        if sys.argv[1] == "costs":
            costs(dev.link)
        elif sys.argv[1] == "costs-of":          # costs-of I: drawn patch I loaded (muted, 1 s), then its costs
            seed = (0xC0FFEE + int(sys.argv[2]) * 7919) & 0xFFFFFFFF
            dev.link.send(frame(0x61, seed.to_bytes(4, "little") + bytes([255, 1])))
            reply(dev.link, 0x61, 15)
            costs(dev.link)
        else:
            patches(dev.link, int(sys.argv[2]) if len(sys.argv) > 2 else 40,
                    int(sys.argv[3]) if len(sys.argv) > 3 else 3)
    finally:
        dev.link.close()


if __name__ == "__main__":
    main()
