#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""LILLIPUT's logo (build 18): the name drawn on one grid -- "lilliput" from bare strokes, squares and two arcs, every
stroke one gap from the next, so the word reads as a pulse train. The icon is "li". Writes assets/logo/*.svg; the
splash (firmware/src/logo.c) draws the same shapes from the same numbers.

  tools/gen_logo.py
"""
from pathlib import Path

OUT = Path(__file__).resolve().parent.parent / "assets" / "logo"
CH = ["#FFD600", "#00D6FF", "#FF5CD2", "#60FF60", "#FF8C00", "#BEA0FF"]   # the channels' colours (lui.c LAYER_COL)

# the grid (units; y down): the stroke, the gap between strokes, the baseline, x-height, ascender, descender
SW, GAP, BASE, XH, ASC, DESC = 6.0, 9.0, 80.0, 44.0, 8.0, 104.0


def logotype(fg, dots=None):
    """The shapes of "lilliput": ("rect", x, y, w, h), ("ring", cx, cy, r) and ("arc", x0, x1, y, r) -- an arc under
    two stems -- all stroked or filled at SW. Returns the shapes and the width."""
    rc = (BASE - XH) / 2 - SW / 2                # a bowl's radius, on its stroke's middle
    s = []
    n = 0

    def stem(x, top, bot):
        s.append(("rect", x - SW / 2, top, SW, bot - top, fg))

    def dot(x):
        nonlocal n
        s.append(("rect", x - SW / 2, ASC, SW, SW, dots[n % len(dots)] if dots else fg))
        n += 1

    x = SW / 2
    for k in "lilli":                            # l i l l i: the pulse
        stem(x, ASC if k == "l" else XH, BASE)
        if k == "i":
            dot(x)
        x += SW + GAP
    stem(x, XH, DESC)                            # p: down to the descender, a round bowl against it
    s.append(("ring", x + rc, (XH + BASE) / 2, rc, fg))
    x = x + 2 * rc + SW + GAP
    u0, u1, mid = x, x + 2 * rc, BASE - SW / 2 - rc   # u: two stems, a bowl under them
    stem(u0, XH, mid)
    stem(u1, XH, BASE)
    s.append(("arc", u0, u1, mid, rc, fg))
    x = u1 + SW + GAP
    stem(x, ASC + 12, BASE)                      # t: a stem from above the x-height, its bar
    s.append(("rect", x - SW / 2 - 7, XH, SW + 14, SW, fg))
    return s, x + SW / 2 + 7


def icon(fg, dot=None):
    """"li": a tall stroke, a short one and its square (on a 100 x 100 field)."""
    return [("rect", 31, 18, 12, 64, fg), ("rect", 57, 42, 12, 40, fg), ("rect", 57, 22, 12, 12, dot or fg)]


def svg_of(shapes):
    out = []
    for sh in shapes:
        if sh[0] == "rect":
            _, x, y, w, h, c = sh
            out.append(f'<rect x="{x:.2f}" y="{y:.2f}" width="{w:.2f}" height="{h:.2f}" fill="{c}"/>')
        elif sh[0] == "ring":
            _, cx, cy, r, c = sh
            out.append(f'<circle cx="{cx:.2f}" cy="{cy:.2f}" r="{r:.2f}" fill="none" stroke="{c}" stroke-width="{SW}"/>')
        else:
            _, x0, x1, y, r, c = sh
            out.append(f'<path d="M{x0:.2f} {y:.2f} A{r:.2f} {r:.2f} 0 0 0 {x1:.2f} {y:.2f}" fill="none" stroke="{c}" '
                       f'stroke-width="{SW}"/>')
    return "".join(out)


def doc(vb, body, bg=None):
    x, y, w, h = vb
    rect = f'<rect x="{x}" y="{y}" width="{w}" height="{h}" fill="{bg}"/>' if bg else ""
    return (f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="{x} {y} {w:g} {h}" role="img" aria-label="Lilliput">'
            f'<title>Lilliput</title>{rect}{body}</svg>\n')


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    _, w = logotype("#fff")
    vb = (-2, 0, round(w + 4, 2), 108)
    files = {
        "lilliput-logotype-white.svg": doc(vb, svg_of(logotype("#FFFFFF")[0])),
        "lilliput-logotype-black.svg": doc(vb, svg_of(logotype("#0B0C0E")[0])),
        "lilliput-logotype-white-channels.svg": doc(vb, svg_of(logotype("#FFFFFF", CH)[0])),
        "lilliput-icon-white.svg": doc((0, 0, 100, 100), svg_of(icon("#FFFFFF"))),
        "lilliput-icon-black.svg": doc((0, 0, 100, 100), svg_of(icon("#0B0C0E"))),
        "lilliput-icon-on-black.svg": doc((0, 0, 100, 100), svg_of(icon("#FFFFFF")), "#0B0C0E"),
    }
    for name, text in files.items():
        (OUT / name).write_text(text)
    print(f"logo: {len(files)} files in {OUT} (logotype {w:g} x 108 units)")
    for sh in logotype("#fff")[0]:               # (the numbers logo.c draws, for checking it)
        print("  ", sh[0], *(f"{v:.2f}" if isinstance(v, float) else v for v in sh[1:-1]))


if __name__ == "__main__":
    main()
