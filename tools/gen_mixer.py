#!/usr/bin/env python3
# SPDX-License-Identifier: MIT -- Lilliput website: the mixer Lilliput models, drawn as a desk seen from above.
#
# A patch bay across the top (each channel's PRE and POST outs and its IN; the mix bus, the sends and the
# returns), 6 channel strips, the rack's 7 slots, the returns and the output. Each routing (ROUTINGS) is a set of
# cables in the bay, with each strip's INPUT display and the rack's slots set to match: the page's buttons
# (docs/js/mixer.js) show one at a time. tools/make_site.py puts figure() into the Concept page.
import html
import math
import random

W, H = 1000, 980
FONT = "Geist Mono, ui-monospace, SF Mono, Menlo, monospace"
FG, DIM, MID, LINE = "#f1f3f5", "#9aa1aa", "#5d636c", "#2a2f36"
CH = ["#ffd600", "#00d6ff", "#ff5cd2", "#60ff60", "#ff8c00", "#bea0ff"]
SX0, SW = 18, 90                       # the strips: the first's left edge, each one's width
BAY_IN, BAY_PRE, BAY_POST = 122, 150, 178
BUS_Y = 158
BUS_X = {"MIX L": 604, "MIX R": 650, "SEND 1": 716, "SEND 2": 762, "RET 1": 828, "RET 2": 874}
BUS_COL = {"MIX": FG, "SEND": "#b9c0c9", "RET": "#7d848e"}

# each routing: its button, its caption, each channel's INPUT, and the rack's 7 slots (effect, IN, OUT)
ROUTINGS = [
    ("drawn", "Drawn patch",
     "As HOME might draw it: channel 1 hears mix bus, 2 follows 1, 3 hears resonator on SEND 1's return, 4 hears "
     "SEND 2, 5 and 6 feed each other. 1 loop of each kind, all pulling at each other through mix bus.",
     ["MIX L", "CH 1", "RET 1", "SEND 2", "CH 6", "CH 5"],
     [("DELAY", "INSERT", "NONE"), None, ("RESONATOR", "SEND 1", "RET 1"), None, ("CUTS", "INSERT", "NONE"),
      ("GLITCH", "INSERT", "NONE"), ("SAG", "INSERT", "NONE")]),
    ("self", "Itself",
     "Each channel hears itself. PRE takes it from after its fader; POST from after its insert, so effect is "
     "inside loop. Its LEVEL decides whether it stays silent, sings or screams.",
     ["PRE", "POST", "PRE", "POST", "PRE", "POST"],
     [None, ("DELAY", "INSERT", "NONE"), None, ("FLANGER", "INSERT", "NONE"), None, ("BITCRUSH", "INSERT", "NONE"),
      None]),
    ("mix", "Mix bus",
     "Channels 1 to 4 hear mix bus: every channel feeding it, and returns. Every fader and PAN is part of loop. "
     "5 and 6 on NONE: their noise alone, through channel, into mix.",
     ["MIX L", "MIX R", "MIX L", "MIX R", "NONE", "NONE"],
     [None, None, ("CHORUS", "INSERT", "NONE"), None, None, None, ("DIFFUSER", "INSERT", "NONE")]),
    ("sends", "Sends and rack",
     "Loops through send buses and rack: SEND 1 into DELAY, on into REED (PREV), back on RET 1; SEND 2 into "
     "FORMANT, back on RET 2. Channels 2, 4 and 5 hear returns, so effects are inside loop.",
     ["SEND 1", "RET 1", "SEND 2", "RET 2", "RET 1", "PRE"],
     [("DELAY", "SEND 1", "NONE"), ("REED", "PREV", "RET 1"), ("FORMANT", "SEND 2", "RET 2"), None, None, None,
      None]),
    ("ring", "Ring",
     "6 channels in ring, each hearing the one before it, after its insert. Each fader round ring decides whether "
     "it sings; 1 loud channel drives all 6.",
     ["CH 6", "CH 1", "CH 2", "CH 3", "CH 4", "CH 5"],
     [("OCTAVER", "INSERT", "NONE"), None, None, ("TREMOLO", "INSERT", "NONE"), None, None, None]),
]

TUBES = ["12AX7", "EF86", "6L6", "12AT7", "300B", "6C16"]
OPAMPS = ["4558", "TL072", "741", "NE5532", "CA3140", "OP07"]
NTYPES = ["PINK", "WHITE", "CRACKLE", "RED", "GEIGER", "HUM"]


def e(s):
    return html.escape(str(s), quote=True)


def text(x, y, s, size=8.5, fill=DIM, anchor="middle", weight=None, ls=0.6):
    w = f' font-weight="{weight}"' if weight else ""
    return (f'<text x="{x:.1f}" y="{y:.1f}" font-size="{size}" fill="{fill}" text-anchor="{anchor}"'
            f' letter-spacing="{ls}"{w}>{e(s)}</text>')


def knob(x, y, v, label=None, r=10.5, ptr=FG):
    a = math.radians(-225 + 270 * v)
    out = []
    for i in range(11):                                   # the scale's dots
        t = math.radians(-225 + 27 * i)
        out.append(f'<circle cx="{x + (r + 4.2) * math.cos(t):.1f}" cy="{y + (r + 4.2) * math.sin(t):.1f}" r="0.7" '
                   f'fill="{MID}"/>')
    out.append(f'<circle cx="{x}" cy="{y}" r="{r}" fill="url(#kb)" stroke="#000" stroke-width="1"/>')
    out.append(f'<circle cx="{x}" cy="{y}" r="{r * 0.68:.1f}" fill="#16191d" stroke="#2c3138" stroke-width="0.6"/>')
    out.append(f'<line x1="{x + r * 0.2 * math.cos(a):.1f}" y1="{y + r * 0.2 * math.sin(a):.1f}" '
               f'x2="{x + r * 0.92 * math.cos(a):.1f}" y2="{y + r * 0.92 * math.sin(a):.1f}" stroke="{ptr}" '
               f'stroke-width="1.6" stroke-linecap="round"/>')
    if label:
        out.append(text(x, y + r + 12, label))
    return "".join(out)


def chip(x, y, w, s, label, fill=FG):
    """a selector shown as a small display: its value, its name under it"""
    return (f'<rect x="{x - w / 2:.1f}" y="{y - 8}" width="{w}" height="15" fill="#000" stroke="{LINE}"/>'
            + text(x, y + 3, s, 7.5, fill, ls=0.3) + text(x, y + 19, label))


def fader(x, y0, y1, v, cap_line, ticks=True, cw=22):
    out = [f'<rect x="{x - 2}" y="{y0}" width="4" height="{y1 - y0}" rx="2" fill="#000" stroke="#1d2126"/>']
    if ticks:
        for i in range(11):
            ty = y0 + (y1 - y0) * i / 10
            out.append(f'<line x1="{x - 14}" y1="{ty:.1f}" x2="{x - 8 if i % 5 else x - 5}" y2="{ty:.1f}" '
                       f'stroke="{MID}" stroke-width="0.7"/>')
    cy = y1 - (y1 - y0) * v
    out.append(f'<rect x="{x - cw / 2}" y="{cy - 8:.1f}" width="{cw}" height="16" rx="1.5" fill="url(#cap)" '
               f'stroke="#000"/>')
    out.append(f'<line x1="{x - cw / 2 + 2}" y1="{cy:.1f}" x2="{x + cw / 2 - 2}" y2="{cy:.1f}" stroke="{cap_line}" '
               f'stroke-width="2"/>')
    return "".join(out)


def meter(x, y0, y1, lit, col, n=16):
    out, h = [], (y1 - y0) / n
    for i in range(n):
        on = i < lit
        c = ("#ff5c5c" if i >= n - 2 else col) if on else "#15181c"
        out.append(f'<rect x="{x - 2.5}" y="{y1 - (i + 1) * h + 1:.1f}" width="5" height="{h - 2:.1f}" fill="{c}"'
                   + (' opacity="0.9"' if on else "") + "/>")
    return "".join(out)


def section(x0, x1, y, name):
    return (f'<line x1="{x0}" y1="{y}" x2="{x1}" y2="{y}" stroke="{LINE}" stroke-width="0.8"/>'
            + f'<rect x="{(x0 + x1) / 2 - len(name) * 3.2 - 5:.1f}" y="{y - 6}" width="{len(name) * 6.4 + 10:.1f}" '
              f'height="12" fill="#0e1013"/>' + text((x0 + x1) / 2, y + 3, name, 8, DIM, ls=1.4))


def jack(x, y):
    return (f'<circle cx="{x}" cy="{y}" r="6.5" fill="#2a2e34" stroke="#000"/>'
            f'<circle cx="{x}" cy="{y}" r="3.2" fill="#000"/>')


def cx(c):
    return SX0 + SW * c + SW / 2


def source_pos(src, c):
    if src == "PRE":
        return cx(c) + 16, BAY_PRE, CH[c]
    if src == "POST":
        return cx(c) + 16, BAY_POST, CH[c]
    if src.startswith("CH "):
        k = int(src[3:]) - 1
        return cx(k) + 16, BAY_POST, CH[k]
    return BUS_X[src], BUS_Y, BUS_COL[src.split()[0]]


def cable(x1, y1, x2, y2, col):
    """a patch cable hanging from out (x1, y1) to in (x2, y2), arched over the bay"""
    if abs(x2 - x1) < 30:                                 # its own channel: a short loop to the side
        d = f"M{x1},{y1} C{x1 + 26},{y1 - 6} {x2 + 30},{y2 - 34} {x2},{y2}"
    else:
        top = min(y1, y2) - min(104, 30 + 0.16 * abs(x2 - x1))
        d = f"M{x1},{y1} C{x1 + (x2 - x1) * 0.08:.1f},{top:.1f} {x2 - (x2 - x1) * 0.08:.1f},{top:.1f} {x2},{y2}"
    return (f'<path d="{d}" fill="none" stroke="#000" stroke-width="6.5" stroke-linecap="round" opacity="0.85"/>'
            f'<path d="{d}" fill="none" stroke="{col}" stroke-width="3.6" stroke-linecap="round"/>'
            f'<path d="{d}" fill="none" stroke="#fff" stroke-width="0.8" stroke-linecap="round" opacity="0.28" '
            f'transform="translate(-0.8,-0.8)"/>'
            + "".join(f'<circle cx="{px}" cy="{py}" r="5.6" fill="{col}" stroke="#000" stroke-width="1.4"/>'
                      f'<circle cx="{px}" cy="{py}" r="1.6" fill="#000" opacity="0.5"/>' for px, py in
                      ((x1, y1), (x2, y2))))


def strip(c, rnd):
    x = cx(c)
    col = CH[c]
    l, r = x - 20, x + 20
    out = [f'<rect x="{SX0 + SW * c + 4}" y="216" width="{SW - 8}" height="3" fill="{col}"/>',
           text(x, 242, str(c + 1), 17, col, weight=400, ls=0),
           text(x, 289, "INPUT")]
    rows = [(318, ("NOISE", rnd.random() * 0.5), None),
            (362, ("GAIN", 0.45 + rnd.random() * 0.5), ("TUBE", TUBES[c])),
            (406, ("FOLD", rnd.random() * 0.6), ("OP-AMP", OPAMPS[c])),
            (450, ("PROTECT", 0.3 + rnd.random() * 0.6), ("LIMIT", rnd.random())),
            (514, ("HIGH", 0.2 + rnd.random() * 0.6), ("LOW", 0.2 + rnd.random() * 0.6)),
            (558, ("FREQ", rnd.random()), ("MID", 0.5 + rnd.random() * 0.5)),
            (620, ("SEND 1", rnd.random() * 0.8), ("SEND 2", rnd.random() * 0.8))]
    out.append(chip(r, 318, 38, NTYPES[c], "TYPE"))
    for y, a, b in rows:
        out.append(knob(l, y, a[1], a[0], ptr=col if a[0] == "GAIN" else FG))
        if b and isinstance(b[1], str):
            out.append(chip(r, y, 38, b[1], b[0]))
        elif b:
            out.append(knob(r, y, b[1], b[0]))
    out.append(section(SX0 + SW * c + 6, SX0 + SW * (c + 1) - 6, 486, "EQ"))
    out.append(section(SX0 + SW * c + 6, SX0 + SW * (c + 1) - 6, 592, "SENDS"))
    out.append(text(x, 680, "INSERT"))
    out.append(knob(x, 712, 0.25 + rnd.random() * 0.5, "PAN"))
    m = False
    out.append(f'<rect x="{x - 15}" y="{746}" width="30" height="13" rx="1.5" fill="{"#3a1414" if m else "#1b1e23"}" '
               f'stroke="#000"/>' + text(x, 755.5, "MUTE", 7, "#ff5c5c" if m else DIM, ls=0.8))
    out.append(fader(x - 8, 778, 930, 0.35 + rnd.random() * 0.5, col))
    out.append(text(x, 952, f"CH {c + 1}", 8.5, FG, ls=1.2))
    return "".join(out)


def rack_unit(i, y, slot):
    x0, x1 = 586, 774
    out = [f'<rect x="{x0}" y="{y}" width="{x1 - x0}" height="20" fill="#121418" stroke="#000"/>',
           f'<circle cx="{x0 + 5}" cy="{y + 10}" r="1.6" fill="{MID}"/><circle cx="{x1 - 5}" cy="{y + 10}" r="1.6" '
           f'fill="{MID}"/>',
           text(x0 + 17, y + 13.5, "OUT" if i == 6 else str(i + 1), 8, CH[i] if i < 6 else FG, ls=0.4)]
    return "".join(out)


def rack_text(i, y, slot):
    x0, x1 = 586, 774
    if not slot:
        return text(x0 + 36, y + 13.5, "OFF", 8, MID, "start", ls=0.6)
    fx, fin, fout = slot
    rt = fin if fout == "NONE" else f"{fin} > {fout}"
    led = f'<circle cx="{x0 + 28}" cy="{y + 10}" r="2" fill="#60ff60"/>'
    return (led + text(x0 + 36, y + 13.5, fx, 8.5, FG, "start", ls=0.6)
            + text(x1 - 12, y + 13.5, rt, 7.5, DIM, "end", ls=0.3))


def master(rnd):
    out = []
    # the rack
    out.append(section(580, 780, 228, "RACK"))
    for i in range(7):
        out.append(rack_unit(i, 240 + i * 24, None))
    # the returns
    out.append(section(580, 780, 430, "RETURNS"))
    out.append(knob(640, 456, 0.6, "RET 1", r=12))
    out.append(knob(720, 456, 0.45, "RET 2", r=12))
    # the output's path, printed on the panel
    out.append(section(580, 780, 512, "OUTPUT"))
    steps = ["CHANNELS: PAN", "MIX L / R + RET 1, 2", "OUT EQ", "OUT FX (SLOT 7)", "LEVEL", "SPACE", "COMPRESSOR",
             "OUT"]
    for i, s in enumerate(steps):
        y = 530 + i * 50
        out.append(f'<rect x="604" y="{y}" width="152" height="22" fill="none" stroke="{MID}" stroke-width="0.8"/>'
                   + text(680, y + 14.5, s, 8, FG if i == len(steps) - 1 else DIM, ls=0.8))
        if i < len(steps) - 1:
            out.append(f'<line x1="680" y1="{y + 22}" x2="680" y2="{y + 46}" stroke="{MID}" stroke-width="0.8"/>'
                       f'<path d="M676.5,{y + 42} L680,{y + 48} L683.5,{y + 42}" fill="{MID}"/>')
    # SPACE
    out.append(section(796, 984, 228, "SPACE"))
    sx, sy = 852, 284
    rooms = ["ROOM", "HALL", "PLATE", "CAVE", "ABYSS", "VOID"]
    for k, n in enumerate(rooms):
        t = math.radians(-210 + k * 48)
        out.append(text(sx + 29 * math.cos(t), sy + 29 * math.sin(t) + 3, n, 6.5, FG if k == 3 else DIM, ls=0.2))
    out.append(knob(sx, sy, 0.62, None, r=13))
    out.append(text(sx, sy + 40, "SPACE"))
    out.append(knob(940, 284, 0.35, "MIX", r=12))
    # the output's EQ
    out.append(section(796, 984, 346, "OUT EQ"))
    out.append(knob(846, 374, 0.55, "HIGH"))
    out.append(knob(934, 374, 0.5, "LOW"))
    out.append(knob(846, 418, 0.62, "GAIN"))
    out.append(knob(934, 418, 0.4, "FREQ"))
    # the compressor
    out.append(section(796, 984, 460, "COMPRESSOR"))
    out.append(knob(846, 488, 0.35, "AMOUNT"))
    out.append(knob(934, 488, 0.55, "THRESH"))
    out.append(knob(846, 532, 0.3, "RELEASE"))
    out.append(knob(934, 532, 0.5, "RATIO"))
    # the main level
    out.append(section(796, 984, 578, "MAIN"))
    out.append(fader(866, 604, 930, 0.62, FG, cw=26))
    out.append(meter(910, 604, 930, 12, "#60ff60", 20))
    out.append(meter(922, 604, 930, 11, "#60ff60", 20))
    out.append(text(910, 944, "L", 7, DIM) + text(922, 944, "R", 7, DIM))
    out.append(text(866, 952, "LEVEL", 8.5, FG, ls=1.2))
    return "".join(out)


def bay():
    out = [f'<rect x="10" y="96" width="980" height="102" rx="4" fill="#0b0c0f" stroke="{LINE}"/>',
           ]
    for c in range(6):
        x = cx(c)
        out.append(jack(x - 16, BAY_IN) + text(x - 2, BAY_IN + 3, "IN", 7, DIM, "start"))
        out.append(jack(x + 16, BAY_PRE) + text(x + 2, BAY_PRE + 3, "PRE", 7, DIM, "end"))
        out.append(jack(x + 16, BAY_POST) + text(x + 2, BAY_POST + 3, "POST", 7, DIM, "end"))
        out.append(f'<rect x="{x - 26}" y="{BAY_IN - 13}" width="4" height="4" fill="{CH[c]}"/>')
        if c:
            out.append(f'<line x1="{SX0 + SW * c}" y1="102" x2="{SX0 + SW * c}" y2="192" stroke="#16191d"/>')
    for name, x in BUS_X.items():
        out.append(jack(x, BUS_Y) + text(x, BUS_Y + 20, name, 7, DIM, ls=0.3))
    for name, (a, b) in (("MIX", ("MIX L", "MIX R")), ("SENDS", ("SEND 1", "SEND 2")), ("RETURNS", ("RET 1", "RET 2"))):
        x0, x1 = BUS_X[a] - 12, BUS_X[b] + 12
        out.append(f'<line x1="{x0}" y1="132" x2="{x1}" y2="132" stroke="{MID}" stroke-width="0.7"/>'
                   + text((x0 + x1) / 2, 127, name, 7, DIM, ls=1.2))
    out.append(text(976, 112, "PATCH BAY", 7.5, MID, "end", ls=1.4))
    return "".join(out)


def routing(key, inputs, slots):
    """one routing: its cables, each strip's INPUT, the rack's slots"""
    cab = []
    for c, src in enumerate(inputs):
        if src == "NONE":
            continue
        x1, y1, col = source_pos(src, c)
        cab.append((abs(x1 - (cx(c) - 16)), cable(x1, y1, cx(c) - 16, BAY_IN, col)))
    cab.sort(key=lambda t: -t[0])                         # the long ones underneath
    lcd = []
    for c, src in enumerate(inputs):
        x = cx(c)
        lcd.append(f'<rect x="{x - 34}" y="256" width="68" height="20" fill="#000" stroke="{LINE}"/>'
                   + text(x, 270, src, 9.5, MID if src == "NONE" else CH[c], weight=400, ls=1))
    rk = [rack_text(i, 240 + i * 24, s) for i, s in enumerate(slots)]
    for c in range(6):                                    # each strip's insert: its slot, when the slot is on INSERT
        s = slots[c]
        ins = s[0] if s and s[1] == "INSERT" else "-"
        rk.append(f'<rect x="{cx(c) - 30}" y="655" width="60" height="15" fill="#000" stroke="{LINE}"/>'
                  + text(cx(c), 665.5, ins, 7.5, FG if ins != "-" else MID, ls=0.3))
    return (f'<g data-p="{key}"' + ("" if key == ROUTINGS[0][0] else ' display="none"') + ">"
            + "".join(lcd) + "".join(rk) + "".join(t for _, t in cab) + "</g>")


def svg():
    rnd = random.Random(1964)
    defs = ('<defs>'
            '<radialGradient id="kb" cx="40%" cy="35%" r="70%"><stop offset="0" stop-color="#3a3f47"/>'
            '<stop offset="1" stop-color="#15181c"/></radialGradient>'
            '<linearGradient id="cap" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#e6e8eb"/>'
            '<stop offset="0.5" stop-color="#b6bbc2"/><stop offset="1" stop-color="#7b818a"/></linearGradient>'
            '<linearGradient id="desk" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#121418"/>'
            '<stop offset="1" stop-color="#0c0d10"/></linearGradient>'
            '</defs>')
    body = [f'<rect x="10" y="206" width="980" height="764" rx="6" fill="url(#desk)" stroke="{LINE}"/>',
            '<rect x="0" y="200" width="12" height="776" rx="4" fill="#1a1c20" stroke="#000"/>',
            '<rect x="988" y="200" width="12" height="776" rx="4" fill="#1a1c20" stroke="#000"/>']
    for c in range(1, 6):
        body.append(f'<line x1="{SX0 + SW * c}" y1="212" x2="{SX0 + SW * c}" y2="964" stroke="#000"/>'
                    f'<line x1="{SX0 + SW * c + 1}" y1="212" x2="{SX0 + SW * c + 1}" y2="964" stroke="#1b1e23"/>')
    body.append(f'<line x1="566" y1="212" x2="566" y2="964" stroke="#000" stroke-width="2"/>')
    body.append(f'<line x1="784" y1="224" x2="784" y2="964" stroke="#1b1e23"/>')
    for c in range(6):
        body.append(strip(c, rnd))
    body.append(master(rnd))
    body.append(bay())
    body += [routing(k, ins, sl) for k, _, _, ins, sl in ROUTINGS]
    return (f'<svg viewBox="0 0 {W} {H}" xmlns="http://www.w3.org/2000/svg" role="img" font-family="{FONT}" '
            f'aria-labelledby="mixer-t"><title id="mixer-t">Lilliput as a desk seen from above: a patch bay, 6 '
            f'channel strips, a rack of 7 effects, returns and output</title>{defs}{"".join(body)}</svg>')


def figure():
    btns = "".join(f'<button type="button" data-p="{k}" data-cap="{e(cap)}" aria-pressed="'
                   f'{"true" if i == 0 else "false"}">{e(name)}</button>' for i, (k, name, cap, _, _) in
                   enumerate(ROUTINGS))
    return (f'<figure class="mixer">\n<div class="mixer-routes" role="group" aria-label="Routings" hidden>{btns}</div>\n'
            f'<div class="mixer-desk">{svg()}</div>\n'
            f'<figcaption><span class="mixer-name">{e(ROUTINGS[0][1])}</span> '
            f'<span class="mixer-cap">{e(ROUTINGS[0][2])}</span></figcaption>\n</figure>')


if __name__ == "__main__":
    print(figure())
