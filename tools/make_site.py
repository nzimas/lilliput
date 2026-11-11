#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# LILLIPUT (as NORILSK's, TONITOR's and BIGORNA's): the website's generated parts, in docs/ (GitHub Pages):
#   docs/index.html, concept.html, screens.html, install.html, backup.html, projects.html, apps.html
#                        site/*.html in the common frame (the home page's demos: docs/assets/demo/, tools/make_demos.sh;
#                        their player js/demo.js)
#   docs/assets/screens/ the FM-1's screens as build/host screens draws them (run tests/run_tests.sh first)
#   docs/manual.html     the README's manual (from "# Manual" to "# Building from source")
#   docs/changelog.html  CHANGELOG.md
#   docs/firmware/       the package the browser installer writes, and latest.json (its file, build, SHA-256)
#   tools/make_site.py [PACKAGE.fwsc]        (default build/lilliput.fwsc; run tools/build.py first)
# The logo: assets/logo (tools/gen_logo.py), copied to docs/assets; the stylesheet and scripts: docs/css, docs/js,
# written by hand. The markdown is the little the README uses: headings, paragraphs, lists, tables, quotes, code,
# emphasis, links.
import hashlib
import html
import json
import re
import shutil
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DOCS = ROOT / "docs"
REPO = "https://github.com/nzimas/lilliput"
# the stylesheet's address changes with it, so browsers never keep an old one
CSS_V = hashlib.sha256((DOCS / "css/site.css").read_bytes()).hexdigest()[:10]
LOGO_V = hashlib.sha256((ROOT / "assets/logo/lilliput-logotype-white.svg").read_bytes()).hexdigest()[:10]


def slug(text):
    s = re.sub(r"<[^>]+>", "", text).lower()
    s = re.sub(r"[^\w\s-]", "", s, flags=re.U)
    return re.sub(r"\s+", "-", s.strip())


def inline(t):
    codes = []

    def keep(m):
        codes.append(f"<code>{html.escape(m[1])}</code>")
        return f"\0{len(codes) - 1}\0"
    t = re.sub(r"`([^`]+)`", keep, t)
    if not re.search(r"<[a-z/][^>]*>", t):
        t = html.escape(t, quote=False)

    def link(m):
        url = m[2]
        if not re.match(r"[a-z]+:|#", url):        # a file in the repository
            url = f"{REPO}/blob/master/{url}"
        return f'<a href="{html.escape(url)}">{m[1]}</a>'
    t = re.sub(r"\[([^\]]+)\]\(([^)]+)\)", link, t)
    t = re.sub(r"<(https?://[^>]+)>", r'<a href="\1">\1</a>', t)
    t = re.sub(r"\*\*(.+?)\*\*", r"<b>\1</b>", t)
    t = re.sub(r"(?<![\w*])\*([^*\s][^*]*?)\*(?![\w*])", r"<i>\1</i>", t)
    t = t.replace(" -- ", " — ").replace(" --\n", " —\n")      # (the changelog's dashes)
    return re.sub(r"\0(\d+)\0", lambda m: codes[int(m[1])], t)


def to_html(md, toc):
    out, lines, i = [], md.splitlines(), 0

    def para(buf):
        if buf:
            out.append("<p>" + inline(" ".join(buf)) + "</p>")
            buf.clear()
    buf = []
    while i < len(lines):
        ln = lines[i]
        if not ln.strip():
            para(buf)
            i += 1
        elif ln.startswith("```"):
            para(buf)
            j = i + 1
            while j < len(lines) and not lines[j].startswith("```"):
                j += 1
            out.append("<pre><code>" + html.escape("\n".join(lines[i + 1:j])) + "</code></pre>")
            i = j + 1
        elif m := re.match(r"(#{1,4})\s+(.*)", ln):
            para(buf)
            lv, text = len(m[1]), m[2]
            sid = slug(text)
            out.append(f'<h{lv} id="{sid}">{inline(text)}</h{lv}>')
            if lv in (2, 3):
                toc.append((lv, sid, re.sub(r"<[^>]+>", "", inline(text))))
            i += 1
        elif ln.startswith("|"):
            para(buf)
            rows = []
            while i < len(lines) and lines[i].startswith("|"):
                cells = [c.strip() for c in re.split(r"(?<!\\)\|", lines[i].strip())[1:-1]]
                if not all(re.fullmatch(r":?-+:?", c) for c in cells):
                    rows.append(cells)
                i += 1
            t = ["<table>", "<tr>" + "".join(f"<th>{inline(c)}</th>" for c in rows[0]) + "</tr>"]
            t += ["<tr>" + "".join(f"<td>{inline(c)}</td>" for c in r) + "</tr>" for r in rows[1:]]
            out.append("\n".join(t + ["</table>"]))
        elif ln.startswith(">"):
            para(buf)
            q = []
            while i < len(lines) and lines[i].startswith(">"):
                q.append(lines[i][1:].strip())
                i += 1
            q = [x for x in q if not re.fullmatch(r"\[!\w+\]", x)]
            out.append("<blockquote><p>" + inline(" ".join(q)) + "</p></blockquote>")
        elif re.match(r"\s*[-*] ", ln):
            para(buf)
            items, depth0 = [], len(ln) - len(ln.lstrip())
            while i < len(lines) and (re.match(r"\s*[-*] ", lines[i]) or (lines[i].startswith("  ") and lines[i].strip())):
                d = len(lines[i]) - len(lines[i].lstrip())
                if re.match(r"\s*[-*] ", lines[i]):
                    items.append([d > depth0, lines[i].strip()[2:]])
                else:
                    items[-1][1] += " " + lines[i].strip()
                i += 1
            h, open_sub = ["<ul>"], False
            for sub, text in items:
                if sub and not open_sub:
                    h[-1] = h[-1].removesuffix("</li>")
                    h.append("<ul>")
                    open_sub = True
                elif not sub and open_sub:
                    h.append("</ul></li>")
                    open_sub = False
                h.append(f"<li>{inline(text)}</li>")
            if open_sub:
                h.append("</ul></li>")
            out.append("\n".join(h + ["</ul>"]))
        elif re.match(r"\d+\. ", ln):                 # (a numbered list: its items, each perhaps run on)
            para(buf)
            items = []
            while i < len(lines) and (re.match(r"\d+\. ", lines[i]) or (lines[i].startswith("   ") and lines[i].strip())):
                if re.match(r"\d+\. ", lines[i]):
                    items.append(re.sub(r"^\d+\. ", "", lines[i]))
                else:
                    items[-1] += " " + lines[i].strip()
                i += 1
            out.append("<ol>\n" + "\n".join(f"<li>{inline(t)}</li>" for t in items) + "\n</ol>")
        elif ln.lstrip().startswith("<"):
            para(buf)
            out.append(ln)
            i += 1
        else:
            buf.append(ln.strip())
            i += 1
    para(buf)
    return "\n".join(out)


DONATE = """<form class="donate {cls}" action="https://www.paypal.com/cgi-bin/webscr" method="post" target="_top">
      <input type="hidden" name="cmd" value="_xclick">
      <input type="hidden" name="business" value="nzimas.lv@gmail.com">
      <input type="hidden" name="item_name" value="Support Lilliput">
      <input type="hidden" name="currency_code" value="EUR">
      <input type="image" src="https://www.paypalobjects.com/en_US/i/btn/btn_donateCC_LG.gif" name="submit"
             title="PayPal - The safer, easier way to pay online!" alt="Donate with PayPal button">
    </form>"""


REDIRECT = """<!doctype html>
<html lang="en"><head><meta charset="utf-8"><title>Concept — Lilliput</title>
<meta http-equiv="refresh" content="0; url=concept.html"><link rel="canonical" href="concept.html"></head>
<body><p><a href="concept.html">Concept</a></p></body></html>
"""


def page(title, current, body, desc, head=""):
    """every page: the menu down the left (above the content on a narrow screen), the content, the footer"""
    nav = [("./", "Home"), ("concept.html", "Concept"), ("screens.html", "Screens"), ("install.html", "Install"),
           ("backup.html", "Backup"), ("manual.html", "Manual"), ("changelog.html", "Changelog"),
           ("projects.html", "Other projects"), ("apps.html", "Apps"), (REPO, "GitHub")]
    links = "\n      ".join(f'<a href="{h}"' + (' aria-current="page"' if n == current else "") + f">{n}</a>"
                           for h, n in nav)
    return f"""<!doctype html>
<!-- SPDX-License-Identifier: MIT -- Lilliput website (generated by tools/make_site.py from site/, README.md, CHANGELOG.md) -->
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>{title}</title>
<meta name="description" content="{desc}">
<link rel="icon" href="assets/mark.svg?v={LOGO_V}" type="image/svg+xml">
<link rel="preconnect" href="https://fonts.googleapis.com">
<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
<link href="https://fonts.googleapis.com/css2?family=Geist:wght@300;400;600&family=Geist+Mono:wght@400&display=swap" rel="stylesheet">
<link rel="stylesheet" href="css/site.css?v={CSS_V}">{head}
</head>
<body>
<div class="layout">
  <aside class="side">
    <div class="top">
      <a class="brand" href="./"><img src="assets/logo-dark.svg?v={LOGO_V}" alt="Lilliput" width="182" height="108"></a>
      <button class="menu-btn" type="button" aria-label="Menu" aria-controls="site-nav" aria-expanded="false"><span></span><span></span><span></span></button>
    </div>
    <nav id="site-nav" aria-label="Site">
      <button class="menu-close" type="button" aria-label="Close the menu">&times;</button>
      {links}
    </nav>
    <div class="menu-shade" hidden></div>
    {DONATE.format(cls="donate-side")}
  </aside>
  <div class="content">
    <main>
{body}
    </main>
    <footer>
      <p>Developed by <a href="https://nunozimas.com">Nuno Zimas</a>. Free software (GPL-3.0); not made or endorsed by M-VAVE.</p>
      {DONATE.format(cls="donate-foot")}
    </footer>
  </div>
</div>
<script src="js/menu.js?v={CSS_V}"></script>
</body>
</html>
"""


DESC = ("Lilliput: a firmware for the M-VAVE FM-1 that turns it into a no-input mixer, six channels feeding back into "
        "themselves and each other, with a four-voice drum synthesizer for rhythmic noise.")


def fragments():
    src = ROOT / "site"
    demo = DOCS / "js" / "demo.js"
    demo_v = hashlib.sha256(demo.read_bytes()).hexdigest()[:10]
    (DOCS / "index.html").write_text(page(
        "Lilliput — a no-input mixer for the M-VAVE FM-1", "Home", (src / "home.html").read_text(), DESC,
        f'\n<script src="js/demo.js?v={demo_v}" defer></script>'))
    sys.path.insert(0, str(ROOT / "tools"))
    import gen_mixer
    mixer_v = hashlib.sha256((DOCS / "js" / "mixer.js").read_bytes()).hexdigest()[:10]
    (DOCS / "concept.html").write_text(page(
        "Concept — Lilliput", "Concept", (src / "concept.html").read_text().replace("<!-- MIXER -->", gen_mixer.figure()),
        "What a no-input mixer is, where the technique comes from, who plays it, and how Lilliput brings it together "
        "with rhythm, and the desk Lilliput models, drawn.",
        f'\n<script src="js/mixer.js?v={mixer_v}" defer></script>'))
    (DOCS / "noinput.html").write_text(REDIRECT)
    (DOCS / "screens.html").write_text(page(
        "Lilliput on the FM-1's screen", "Screens", (src / "screens.html").read_text(),
        "Lilliput on the M-VAVE FM-1's screen: the main view, a channel's pages, the overview, the drums, the "
        "compressor, the rhythm, the snapshots and the gesture loops."))
    (DOCS / "install.html").write_text(page(
        "Install Lilliput", "Install", (src / "install.html").read_text(),
        "Install Lilliput on an M-VAVE FM-1 from your browser, over USB MIDI.",
        '\n<script type="module" src="js/install.js"></script>'))
    (DOCS / "backup.html").write_text(page(
        "Back up Lilliput sessions", "Backup", (src / "backup.html").read_text(),
        "Save the sessions on an M-VAVE FM-1 running Lilliput to a file, and restore them, from your browser.",
        '\n<script type="module" src="js/backup.js"></script>'))
    (DOCS / "projects.html").write_text(page(
        "Other projects by Nuno Zimas", "Other projects", (src / "projects.html").read_text(),
        "Firmware and instruments by Nuno Zimas for the M-VAVE FM-1, the Elektron Model:Cycles and Digitone II, the "
        "Ableton Move, the Akai MPC One and macOS."))
    (DOCS / "apps.html").write_text(page(
        "Apps by Nuno Zimas", "Apps", (src / "apps.html").read_text(),
        "Music apps by Nuno Zimas for phones and tablets."))


SCREENS = ["00_splash", "01_main", "02_dial_limit", "03_amp", "05_eq", "07_fx", "09c_lfo1", "09d_mod_1_4", "10_overview",
           "07e_compressor", "08_out", "10b_drums_kick", "10d_drums_kick_2", "10e_drums_rhythm", "10f_drums_perc_muted", "10g_drums_break",
           "18_tempo", "19_rhythm_mix", "16_snapshots", "17_rec", "12_recipe", "13_new_patch", "15_save_ask"]


def screens():
    d = DOCS / "assets" / "screens"
    d.mkdir(parents=True, exist_ok=True)
    for n in SCREENS:
        shutil.copyfile(ROOT / "build" / "screens" / f"{n}.png", d / f"{n}.png")


def logo():
    d = DOCS / "assets"
    d.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(ROOT / "assets/logo/lilliput-logotype-white.svg", d / "logo-dark.svg")
    shutil.copyfile(ROOT / "assets/logo/lilliput-logotype-black.svg", d / "logo-light.svg")
    shutil.copyfile(ROOT / "assets/logo/lilliput-icon-on-black.svg", d / "mark.svg")


def manual():
    md = (ROOT / "README.md").read_text()
    part = md[md.index("\n# Manual\n"):md.index("\n# Building from source\n")]
    part = part.replace("\n# Manual\n", "\n", 1)
    toc = []
    body = to_html(part, toc)
    t = "\n".join(f'<a href="#{s}"' + (' class="sub"' if lv == 3 else "") + f">{html.escape(n)}</a>" for lv, s, n in toc)
    intro = ('<h1>Manual</h1>\n<p>Everything the FM-1 does under Lilliput. New here? Start with '
             '<a href="#playing-it">Playing it</a>; to install, see <a href="install.html">Install</a>.</p>\n'
             f'<nav class="toc" aria-label="Contents">\n{t}\n</nav>')
    (DOCS / "manual.html").write_text(page("Lilliput manual", "Manual", intro + "\n" + body,
                                           "The Lilliput manual: every control of the FM-1 under Lilliput."))


def changelog():
    md = (ROOT / "CHANGELOG.md").read_text()
    body = to_html(md, [])
    (DOCS / "changelog.html").write_text(page("Lilliput changelog", "Changelog", body, "What changed in each Lilliput build."))


def firmware(pkg):
    raw = pkg.read_bytes()
    sys.path.insert(0, str(ROOT / "tools"))
    import fm1_install as fi
    product, _ = fi.load_package(str(pkg), False)
    ver = "BUILD " + re.search(r'^VERSION = "(\d+)"', (ROOT / "tools" / "build.py").read_text(), re.M)[1]
    d = DOCS / "firmware"
    d.mkdir(exist_ok=True)
    for old in d.glob("*.fwsc"):
        old.unlink()
    name = "lilliput.fwsc"
    shutil.copyfile(pkg, d / name)
    (d / "latest.json").write_text(json.dumps({"file": name, "version": ver, "product": product,
                                               "size": len(raw), "sha256": hashlib.sha256(raw).hexdigest()}, indent=1) + "\n")
    print(f"site: firmware {ver} ({product}, {len(raw)} B)")


if __name__ == "__main__":
    logo()
    fragments()
    screens()
    manual()
    changelog()
    firmware(Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "build" / "lilliput.fwsc")
    print("site: docs/index.html, concept.html, screens.html, install.html, backup.html, manual.html, changelog.html, "
          "projects.html, apps.html")
