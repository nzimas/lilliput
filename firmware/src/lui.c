/* SPDX-License-Identifier: GPL-3.0-only
 * LILLIPUT: the panel and the screen. Tonitor's screen (its bands, its dials, its rows, its flashes, its rules for
 * low vision: a few words, very large, high contrast, each channel its own colour), the no-input mixer's controls.
 *
 *   FX SEL ENV LFO EDIT GLO   the top row: channels 1 to 6 (lit: the one chosen; glowing: sounding). On a channel's
 *                             page, another channel shows the same page; the chosen one again: back to its main view
 *   SELECT (knob)             the chosen channel's LEVEL (its fader: in most loops, what makes them sing)
 *   ALGORITHM (knob)          its PAN
 *   KNOB 1 .. 4               on the main view: GAIN, FREQ (the middle filter's), LIMIT (the loop's pitch), SEND 1;
 *                             SHIFT +: NOISE, MID (the middle filter's gain or width), PROTECT (the overload
 *                             protection's threshold), SEND 2 -- a dial over
 *                             the whole screen while turning. On a page: its four rows (SHIFT +: the page's switches)
 *   OCT- / OCT+               the channel's pages: MAIN, AMP, NOISE, EQ, SEND, FX (on SEQ's: through them;
 *                             on the RHYTHM's: TEMPO SWING CHAOS LENGTH (the gates'), then MIX)
 *   SEQ                       COMPRESSOR, OUT, OUT EQ, OUT FX, then the chosen channel's LFO 1, LFO 2, MOD 1 TO 4, MOD 5 TO 8,
 *                             then back to the channel's page
 *   ARP                       the OVERVIEW: the six channels, their inputs and levels, at once
 *   PRESETS (knob)            the recipe: its name replaces the patch's in the main view (nothing else changes)
 *   HOME                      a NEW PATCH from any recipe: the whole mixer drawn again -- its connections and its
 *                             settings, each time otherwise (patch.c). As on Tonitor
 *   SHIFT + HOME              a new patch from the recipe named in the main view (the one playing, or PRESETS')
 *   HOME held                 the patch nudged: every channel's tone a little elsewhere, the connections kept
 *   SAVE held                 save the session: over the one loaded at once, else a new one (asked)
 *   SHIFT + SAVE              the SESSIONS page (PRESETS chooses, SAVE loads); held 0.8 s: GLOBAL SETUP
 *   3rd black key from right  the DRUMS (drums.c): short, their pages (again: back); SHIFT + short, a new drum rhythm,
 *                             playing; held 0.8 s, the drums on or off. On their pages the first three white keys
 *                             choose KICK, SNARE, HATS; SHIFT + one, a new sound for it; PRESETS, the drum rhythms;
 *                             OCT- / OCT+ through the three pages
 *   REC / PLAY                the chosen channel's GESTURE loop: REC arms, ends; held, empties; PLAY plays or stops
 *                             it (one loop a channel, each on its own: perform.c)
 *   SHIFT + PLAY              the output muted, or back (a panic: the loops keep going, unheard)
 *   white keys 1 .. 15        the chosen channel's INPUT: PRE, POST, MIX L, MIX R, SEND 1, SEND 2, RET 1, RET 2, NONE,
 *                             CH 1 .. CH 6 (lit: the one it has)
 *   white key 16              its MAIN MIX on or off
 *   black keys 1 .. 6         mute channels 1 .. 6 (lit: muted)
 *   SHIFT + black key 7 held  the CPU page
 *   MASTER (knob)             the level after everything */

enum { B_FX, B_SEL, B_ENV, B_LFO, B_EDIT, B_GLO, B_HOME, B_SAVE, B_ARP, B_SEQ, B_PLAY, B_REC, B_OCTDN, B_OCTUP, NB };
enum { EN_SELECT, EN_ALGO, EN_PRESET, EN_K1, EN_K2, EN_K3, EN_K4, NE };
static const uint8_t PANEL_BTN[NB] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 0, 1};
static const uint8_t PANEL_ENC[NE] = {0, 1, 6, 2, 3, 4, 5};
static const uint8_t LAYER_BTN[NCH] = {B_FX, B_SEL, B_ENV, B_LFO, B_EDIT, B_GLO};
#define NKEY 27u
#define KEY_SHIFT 25u            /* the rightmost black key */
#define KEY_CPU 15u              /* the 7th black key: SHIFT + it held, the CPU page */
#define KEY_RHY 22u              /* the 2nd from the right: the RHYTHM -- on or off; SHIFT +: a new one; held: TEMPO */
#define KEY_DRM 20u              /* the 3rd from the right: the DRUMS -- their pages; SHIFT +: a new rhythm; held: on / off */
static const uint8_t KEY_MUTE[NCH] = {1, 3, 5, 8, 10, 13};   /* black keys 1 .. 6 */
#define WHITE_MAINMIX 15         /* the last white key */

static uint32_t boot_seed;                       /* (main.c, before ui_init: the ADCs' noise and the clock, mixed) */
static int32_t batt_raw;                         /* the battery's divider, smoothed (main.c), 0 .. 1023; 0 none yet */
#define SES_SLOTS 64u            /* (lses.c's: sessions 1 .. 64) */
static const uint16_t LAYER_COL[NCH] = {RGB(255, 214, 0), RGB(0, 214, 255), RGB(255, 92, 210),
                                        RGB(96, 255, 96), RGB(255, 140, 0), RGB(190, 160, 255)};
#define C_MONO RGB(190, 190, 190)        /* a channel not in the patch (nothing of it reaches anything): its screens */
#define C_OUT RGB(140, 190, 255)
/* a channel's colour; grey when it is not in the patch -- muted, or its LEVEL and SENDs off with no RHYTHM (the mixer
 * does not run it: chr[c].dead) */
static int ch_out(uint32_t c) { return par.ch[c].mute || chr[c].dead; }
static uint16_t ch_col(uint32_t c) { return ch_out(c) ? C_MONO : LAYER_COL[c]; }
#define C_SES RGB(120, 255, 200)
#define C_PRE RGB(255, 200, 120)          /* (the RECIPE's) */
#define C_OK RGB(60, 220, 90)
#define C_BAD RGB(255, 40, 40)
#define C_REC RGB(255, 30, 30)

/* the pages */
enum { PG_MAIN, PG_AMP, PG_DYN, PG_EQ, PG_SEND, PG_FX, PG_LFO1, PG_LFO2, PG_MTX1, PG_MTX2,   /* a channel's (OCT- / OCT+) */
       PG_OUT, PG_OEQ, PG_OFX, PG_COMP,                      /* the output's (SEQ: COMP first) */
       PG_TEMPO, PG_RHY2,                                    /* the RHYTHM's (its key held; OCT- / OCT+ between) */
       PG_DRUM, PG_DRUM2, PG_DRUM3,                          /* the DRUMS' (their key; OCT- / OCT+ between) */
       PG_VIEW, PG_SES, PG_ASK, PG_SNAP, PG_SET, PG_N };
#define PG_CH_LAST PG_MTX2       /* (the chosen channel's pages; OCT reaches them up to FX, SEQ the rest) */
/* SEQ's pages in turn: the output's, then the chosen channel's motion */
static const uint8_t SEQ_PG[] = {PG_COMP, PG_OUT, PG_OEQ, PG_OFX, PG_LFO1, PG_LFO2, PG_MTX1, PG_MTX2};
static int seq_ix(uint32_t pg)           /* (its place in SEQ's turn; -1: not one of them) */
{
    uint32_t i;
    for (i = 0; i < sizeof SEQ_PG; i++)
        if (SEQ_PG[i] == pg)
            return (int)i;
    return -1;
}
static const char *const PG_TITLE[PG_N] = {"", "AMP", "NOISE", "EQ", "SEND", "FX", "LFO 1", "LFO 2", "MOD 1 TO 4", "MOD 5 TO 8",
                                           "OUT", "OUT EQ", "OUT FX", "COMPRESSOR", "RHYTHM", "RHYTHM", "DRUMS", "DRUMS", "DRUMS",
                                           "", "", "", "",
                                           ""};

/* every parameter the panel reaches: on the chosen channel, its slot, or the output */
enum { P_GAIN, P_NOISE, P_NTYPE, P_TUBE, P_TMODE, P_OPAMP, P_OVER, P_LIM, P_FTYPE, P_HIGH, P_LOW, P_MTYPE, P_MGAIN, P_MFREQ,
       P_LEVEL, P_PAN, P_SEND1, P_SEND2, P_SRC, P_MAINMIX, P_MUTE,
       P_FX, P_P1, P_P2, P_PRES, P_LOFI, P_BYP, P_FIN, P_FOUT,
       P_RET1, P_RET2, P_OFTYPE, P_OHIGH, P_OLOW, P_OWIDE, P_OMGAIN, P_OMFREQ, P_SPACE, P_SPRES, P_OLEVEL,
       P_LSHAPE, P_LRATE, P_LMODE, P_LFADE,      /* (the LFO pages': LFO 1 or 2 as the page is) */
       P_FOLD,
       P_BPM, P_SWING, P_CHAOS, P_GSCALE,        /* (the TEMPO page's) */
       P_RMIX,                                   /* (the RHYTHM's second page) */
       P_DTUNE, P_DDECAY, P_DDRIVE, P_DLEVEL, P_DSWEEP, P_DATTACK, P_DNOISE, P_DVERB,   /* (the DRUMS' voice chosen) */
       P_DBEND, P_DCOLOR, P_DCLICK, P_DPAN, P_DMETAL, P_DFILT,
       P_DSWING,                                 /* (the DRUMS' own) */
       P_CAMT, P_CTHR, P_CREL, P_CSC, P_CATT, P_CRAT,   /* (the COMPRESSOR's) */
       P_NONE };
/* each page's four rows (KNOB 1 .. 4), and with SHIFT */
static const uint8_t PG_ROW[PG_N][4] = {
    {P_GAIN, P_MFREQ, P_LIM, P_SEND1}, {P_TUBE, P_GAIN, P_FOLD, P_OPAMP}, {P_NOISE, P_NTYPE, P_LIM, P_OVER},
    {P_HIGH, P_LOW, P_MGAIN, P_MFREQ}, {P_SRC, P_SEND1, P_SEND2, P_MAINMIX}, {P_FX, P_P1, P_P2, P_PRES},
    {P_LSHAPE, P_LRATE, P_LMODE, P_LFADE}, {P_LSHAPE, P_LRATE, P_LMODE, P_LFADE},
    {P_NONE, P_NONE, P_NONE, P_NONE}, {P_NONE, P_NONE, P_NONE, P_NONE},
    {P_OLEVEL, P_RET1, P_RET2, P_SPRES}, {P_OHIGH, P_OLOW, P_OMGAIN, P_OMFREQ}, {P_FX, P_P1, P_P2, P_PRES},
    {P_CAMT, P_CTHR, P_CREL, P_CSC},
    {P_BPM, P_SWING, P_CHAOS, P_GSCALE}, {P_RMIX, P_NONE, P_NONE, P_NONE},
    {P_DTUNE, P_DDECAY, P_DDRIVE, P_DLEVEL}, {P_DBEND, P_DCOLOR, P_DCLICK, P_DPAN}, {P_DSWING, P_NONE, P_NONE, P_NONE}};
static const uint8_t PG_SHROW[PG_N][4] = {
    {P_NOISE, P_MGAIN, P_OVER, P_SEND2}, {P_TMODE, P_NONE, P_NONE, P_NONE}, {P_NONE, P_NONE, P_NONE, P_NONE},
    {P_FTYPE, P_NONE, P_MTYPE, P_NONE}, {P_NONE, P_NONE, P_NONE, P_NONE}, {P_FIN, P_FOUT, P_LOFI, P_BYP},
    {P_NONE, P_NONE, P_NONE, P_NONE}, {P_NONE, P_NONE, P_NONE, P_NONE},
    {P_NONE, P_NONE, P_NONE, P_NONE}, {P_NONE, P_NONE, P_NONE, P_NONE},
    {P_NONE, P_NONE, P_NONE, P_SPACE}, {P_OFTYPE, P_NONE, P_OWIDE, P_NONE}, {P_FIN, P_FOUT, P_LOFI, P_BYP},
    {P_CATT, P_CRAT, P_NONE, P_NONE},
    {P_NONE, P_NONE, P_NONE, P_NONE}, {P_NONE, P_NONE, P_NONE, P_NONE},
    {P_DSWEEP, P_DATTACK, P_DNOISE, P_DVERB}, {P_DMETAL, P_DFILT, P_NONE, P_NONE}, {P_NONE, P_NONE, P_NONE, P_NONE}};

static struct {
    uint8_t sel, page, last_ch_page;
    int8_t touched;              /* the row just turned (-1 none), until touch_until */
    uint32_t touch_until;
    uint8_t dial;                /* the parameter whose dial covers the main view, until dial_until */
    uint32_t dial_until, dial_drawn_at;
    int32_t shown_dial;
    char msg[16];
    uint32_t msg_until;
    uint32_t home_until;
    uint32_t seed;
    char patch[16];              /* the patch's name: the recipe it was drawn from, or the session's */
    uint8_t fam;                 /* the recipe (family) of the patch playing */
    uint8_t recipe;              /* the recipe named in the main view (PRESETS; a new patch: its own): SHIFT + HOME's */
    uint8_t browse;              /* PRESETS turned since the patch came: the main view names the recipe, not the patch */
    uint8_t home_down, home_sh, home_done;   /* HOME down (with SHIFT) since home_t0; held: the nudge done */
    uint32_t home_t0;
    uint8_t rhy_down, rhy_sh, rhy_done;   /* the RHYTHM's key down (with SHIFT) since rhy_t0; held: TEMPO */
    uint32_t rhy_t0, beat_seen, beat_at;
    uint8_t dsel;                /* the DRUMS' voice chosen: KICK, SNARE, HATS */
    uint8_t drm_down, drm_sh, drm_done;
    uint32_t drm_t0;
    int8_t dkey;                 /* a voice's white key down on the DRUMS' pages (-1 none): held 0.7 s, muted or back */
    uint8_t dkey_done;
    uint32_t dkey_t0;
    uint8_t rec_down, rec_done;  /* REC down since rec_t0 (held 1 s: the channel's gesture loop emptied) */
    uint32_t rec_t0;
    int8_t snap_key;             /* the SNAPSHOTS page: the white key down (-1 none) since snap_t0; held: stored */
    uint8_t snap_done;
    uint32_t snap_t0;
    int32_t shown_snap;
    uint8_t mtx_focus;           /* the MOD pages' slot in hand (its row lit): PRESETS sets where it comes FROM */
    int32_t shown_mtx[4];
    uint32_t keys;               /* the keys down last time */
    uint32_t flash_t0;
    uint8_t flash_on, flash_phase;
    uint16_t flash_col;
    char flash_l1[12], flash_l2[16];
    uint8_t ses_ix, ses_yes, ses_over;   /* the SESSIONS page's slot; the question's answer, and whether it overwrites */
    uint16_t ses_new;
    uint32_t save_t0, ssv_t0;
    uint8_t save_down, save_long, ssv_down, ssv_long;
    uint8_t set_page;            /* GLOBAL SETUP up (setup.c) */
    uint8_t cpu_page, cpu_was;
    uint8_t midi_mask[128];      /* (setup.c: MIDI notes are not played here) */
    /* the screen as last drawn */
    uint8_t shown_page, shown_sel;
    int32_t shown_src, shown_msg, shown_meter, shown_level, shown_pan;
    char shown_text[16];
    int32_t shown_row[4];
    int8_t shown_touch;
    uint32_t meter_at, view_at;
    int32_t shown_view[NCH];
    uint8_t shown_dead;          /* the chosen channel drawn grey (not in the patch) */
    uint8_t force;
} ui;

static void say(const char *s, uint32_t ms)
{
    str_cpy(ui.msg, s, sizeof ui.msg);
    ui.msg_until = fm1_ms + ms;
}
static void flash(const char *l1, const char *l2, uint16_t c)
{
    str_cpy(ui.flash_l1, l1, sizeof ui.flash_l1);
    str_cpy(ui.flash_l2, l2, sizeof ui.flash_l2);
    ui.flash_col = c;
    ui.flash_t0 = fm1_ms;
    ui.flash_on = 1;
    ui.flash_phase = 0xFF;
}
static uint32_t next_seed(void)
{
    ui.seed = ui.seed * 1664525u + 1013904223u + fm1_ms;
    return ui.seed ^ (ui.seed >> 15);
}

/* ------------------------------------------------- the parameters' side --- */
static uint32_t pslot(void) { return ui.page == PG_OFX ? SLOT_OUT : ui.sel; }   /* (the slot the FX rows are on) */
static int32_t p_get(uint32_t id)
{
    const ch_par_t *c = &par.ch[ui.sel];
    const slot_par_t *s = &par.slot[pslot()];
    const out_par_t *o = &par.out;
    switch (id) {
    case P_GAIN: return c->gain;
    case P_NOISE: return c->noise;
    case P_NTYPE: return c->ntype;
    case P_TUBE: return c->tube;
    case P_TMODE: return c->tmode;
    case P_OPAMP: return c->opamp;
    case P_FOLD: return c->fold;
    case P_OVER: return c->over;
    case P_LIM: return c->lim;
    case P_FTYPE: return c->ftype;
    case P_HIGH: return c->high;
    case P_LOW: return c->low;
    case P_MTYPE: return c->mtype;
    case P_MGAIN: return c->mgain;
    case P_MFREQ: return c->mfreq;
    case P_LEVEL: return c->level;
    case P_PAN: return c->pan;
    case P_SEND1: return c->send1;
    case P_SEND2: return c->send2;
    case P_SRC: return c->src;
    case P_MAINMIX: return c->mainmix;
    case P_MUTE: return c->mute;
    case P_FX: return s->fx;
    case P_P1: return s->p1;
    case P_P2: return s->p2;
    case P_PRES: return s->pres;
    case P_LOFI: return s->lofi;
    case P_BYP: return s->bypass;
    case P_FIN: return s->in;
    case P_FOUT: return s->out;
    case P_RET1: return o->ret1;
    case P_RET2: return o->ret2;
    case P_OFTYPE: return o->ftype;
    case P_OHIGH: return o->high;
    case P_OLOW: return o->low;
    case P_OWIDE: return o->wide;
    case P_OMGAIN: return o->mgain;
    case P_OMFREQ: return o->mfreq;
    case P_SPACE: return o->space;
    case P_SPRES: return o->spres;
    case P_OLEVEL: return o->level;
    case P_BPM: return par.rhy.bpm;
    case P_SWING: return par.rhy.swing;
    case P_CHAOS: return par.rhy.chaos;
    case P_GSCALE: return par.rhy.gscale;
    case P_RMIX: return 100 - (par.rhy.dry > 100u ? 100 : par.rhy.dry);
    case P_LSHAPE: return c->mod.lfo[ui.page == PG_LFO2].shape;
    case P_LRATE: return c->mod.lfo[ui.page == PG_LFO2].rate;
    case P_LMODE: return c->mod.lfo[ui.page == PG_LFO2].mode;
    case P_LFADE: return c->mod.lfo[ui.page == PG_LFO2].fade;
    case P_DTUNE: return drm.v[ui.dsel].tune;
    case P_DDECAY: return drm.v[ui.dsel].decay;
    case P_DDRIVE: return drm.v[ui.dsel].drive;
    case P_DLEVEL: return drm.v[ui.dsel].level;
    case P_DSWEEP: return drm.v[ui.dsel].sweep;
    case P_DATTACK: return drm.v[ui.dsel].attack;
    case P_DNOISE: return drm.v[ui.dsel].noise;
    case P_DVERB: return drm.v[ui.dsel].verb;
    case P_DBEND: return drm.v[ui.dsel].bend;
    case P_DCOLOR: return drm.v[ui.dsel].color;
    case P_DCLICK: return drm.v[ui.dsel].click;
    case P_DPAN: return drm.v[ui.dsel].pan;
    case P_DMETAL: return drm.v[ui.dsel].metal;
    case P_DFILT: return drm.v[ui.dsel].filt;
    case P_DSWING: return drm.swing;
    case P_CAMT: return cmp.amount;
    case P_CTHR: return cmp.thresh;
    case P_CREL: return cmp.release;
    case P_CSC: return cmp.sc;
    case P_CATT: return cmp.attack;
    case P_CRAT: return cmp.ratio;
    default: return 0;
    }
}
/* a choice's count (0: a continuous value, 0 .. PMAX; -1: PAN) */
static int32_t p_count(uint32_t id)
{
    switch (id) {
    case P_NTYPE: return NZ_N;
    case P_TUBE: return TB_N;
    case P_TMODE: return TM_N;
    case P_OPAMP: return OA_N;
    case P_FTYPE: case P_OFTYPE: return FT_N;
    case P_MTYPE: return MT_N;
    case P_SRC: return SRC_N;
    case P_MAINMIX: case P_MUTE: case P_LOFI: case P_BYP: case P_OWIDE: return 2;
    case P_FX: return FX_N;
    case P_P1: return fx_pcount(par.slot[pslot()].fx, 0);
    case P_P2: return fx_pcount(par.slot[pslot()].fx, 1);
    case P_FIN: return FI_N;
    case P_FOUT: return FO_N;
    case P_SPACE: return SP_N;
    case P_LSHAPE: return MS_N;
    case P_LMODE: return MM_N;
    case P_PAN: case P_DPAN: return -1;          /* (-PAN_MAX .. PAN_MAX) */
    case P_DMETAL: return DM_N;
    case P_CSC: return 2;
    case P_CRAT: return 6;
    case P_DFILT: return DN_N;
    default: return 0;
    }
}
static void p_set(uint32_t id, int32_t v)
{
    ch_par_t *c = &par.ch[ui.sel];
    slot_par_t *s = &par.slot[pslot()];
    out_par_t *o = &par.out;
    int32_t n = p_count(id);
    v = n > 0 ? clampi(v, 0, n - 1) : n < 0 ? clampi(v, -PAN_MAX, PAN_MAX)
        : id == P_BPM ? clampi(v, 10, 500)
        : clampi(v, 0, id == P_LEVEL ? LVL_MAX : id == P_LRATE || id == P_LFADE || id == P_SWING || id == P_CHAOS ? 100
                       : id == P_GSCALE || id == P_RMIX || id == P_DSWING || id == P_CAMT ? 100 : PMAX);
    switch (id) {
    case P_GAIN: c->gain = (int16_t)v; break;
    case P_NOISE: c->noise = (int16_t)v; break;
    case P_NTYPE: c->ntype = (int16_t)v; break;
    case P_TUBE: c->tube = (int16_t)v; break;
    case P_TMODE: c->tmode = (int16_t)v; break;
    case P_OPAMP: c->opamp = (int16_t)v; break;
    case P_FOLD: c->fold = (int16_t)v; break;
    case P_OVER: c->over = (int16_t)v; break;
    case P_LIM: c->lim = (int16_t)v; break;
    case P_FTYPE: c->ftype = (int16_t)v; break;
    case P_HIGH: c->high = (int16_t)v; break;
    case P_LOW: c->low = (int16_t)v; break;
    case P_MTYPE: c->mtype = (int16_t)v; break;
    case P_MGAIN: c->mgain = (int16_t)v; break;
    case P_MFREQ: c->mfreq = (int16_t)v; break;
    case P_LEVEL: c->level = (int16_t)v; break;
    case P_PAN: c->pan = (int16_t)v; break;
    case P_SEND1: c->send1 = (int16_t)v; break;
    case P_SEND2: c->send2 = (int16_t)v; break;
    case P_SRC: c->src = (int16_t)v; break;
    case P_MAINMIX: c->mainmix = (uint8_t)v; break;
    case P_MUTE: c->mute = (uint8_t)v; break;
    case P_FX: s->fx = (int16_t)v; break;
    case P_P1: s->p1 = (int16_t)v; break;
    case P_P2: s->p2 = (int16_t)v; break;
    case P_PRES: s->pres = (int16_t)v; break;
    case P_LOFI: s->lofi = (uint8_t)v; break;
    case P_BYP: s->bypass = (uint8_t)v; break;
    case P_FIN: s->in = (uint8_t)v; break;
    case P_FOUT: s->out = (uint8_t)v; break;
    case P_RET1: o->ret1 = (int16_t)v; break;
    case P_RET2: o->ret2 = (int16_t)v; break;
    case P_OFTYPE: o->ftype = (int16_t)v; break;
    case P_OHIGH: o->high = (int16_t)v; break;
    case P_OLOW: o->low = (int16_t)v; break;
    case P_OWIDE: o->wide = (int16_t)v; break;
    case P_OMGAIN: o->mgain = (int16_t)v; break;
    case P_OMFREQ: o->mfreq = (int16_t)v; break;
    case P_SPACE: o->space = (int16_t)v; break;
    case P_SPRES: o->spres = (int16_t)v; break;
    case P_OLEVEL: o->level = (int16_t)v; break;
    case P_BPM: par.rhy.bpm = (uint16_t)v; break;
    case P_SWING: par.rhy.swing = (uint8_t)v; break;
    case P_CHAOS: par.rhy.chaos = (uint8_t)v; break;
    case P_GSCALE: par.rhy.gscale = (uint8_t)(v < 5 ? 5 : v); break;
    case P_RMIX: par.rhy.dry = (uint8_t)(100 - v); break;
    case P_LSHAPE: c->mod.lfo[ui.page == PG_LFO2].shape = (uint8_t)v; break;
    case P_LRATE: c->mod.lfo[ui.page == PG_LFO2].rate = (uint8_t)v; break;
    case P_LMODE: c->mod.lfo[ui.page == PG_LFO2].mode = (uint8_t)v; break;
    case P_LFADE: c->mod.lfo[ui.page == PG_LFO2].fade = (uint8_t)v; break;
    case P_DTUNE: drm.v[ui.dsel].tune = (int16_t)v; break;
    case P_DDECAY: drm.v[ui.dsel].decay = (int16_t)v; break;
    case P_DDRIVE: drm.v[ui.dsel].drive = (int16_t)v; break;
    case P_DLEVEL: drm.v[ui.dsel].level = (int16_t)v; break;
    case P_DSWEEP: drm.v[ui.dsel].sweep = (int16_t)v; break;
    case P_DATTACK: drm.v[ui.dsel].attack = (int16_t)v; break;
    case P_DNOISE: drm.v[ui.dsel].noise = (int16_t)v; break;
    case P_DVERB: drm.v[ui.dsel].verb = (int16_t)v; break;
    case P_DBEND: drm.v[ui.dsel].bend = (int16_t)v; break;
    case P_DCOLOR: drm.v[ui.dsel].color = (int16_t)v; break;
    case P_DCLICK: drm.v[ui.dsel].click = (int16_t)v; break;
    case P_DPAN: drm.v[ui.dsel].pan = (int16_t)v; break;
    case P_DMETAL: drm.v[ui.dsel].metal = (uint8_t)v; break;
    case P_DFILT: drm.v[ui.dsel].filt = (uint8_t)v; break;
    case P_DSWING: drm.swing = (uint8_t)v; break;
    case P_CAMT: cmp.amount = (uint8_t)v; break;
    case P_CTHR: cmp.thresh = (int16_t)v; break;
    case P_CREL: cmp.release = (int16_t)v; break;
    case P_CSC: cmp.sc = (uint8_t)v; break;
    case P_CATT: cmp.attack = (int16_t)v; break;
    case P_CRAT: cmp.ratio = (uint8_t)v; break;
    default: break;
    }
    if (id >= P_DTUNE && id <= P_DSWING)        /* (the DRUMS: what the values mean, worked out again) */
        drm_coef_all();
    if (id >= P_CAMT && id <= P_CRAT)            /* (the COMPRESSOR's) */
        cmp_coef();
}
static int32_t p_hi(uint32_t id)
{
    return id == P_LEVEL ? LVL_MAX : id == P_PAN || id == P_DPAN ? PAN_MAX : id == P_BPM ? 500 : id == P_DSWING || id == P_CAMT ? 100
           : id == P_LRATE || id == P_LFADE || id == P_SWING || id == P_CHAOS || id == P_GSCALE || id == P_RMIX ? 100
           : p_count(id) > 0 ? p_count(id) - 1 : PMAX;
}
static int32_t p_lo(uint32_t id) { return id == P_PAN || id == P_DPAN ? -PAN_MAX : id == P_BPM ? 10 : 0; }
/* LEVEL, a detent: 1 dB a step up to -5 dB (on whole dBs), 0.1 dB a step above it; below -60 dB, off */
static int32_t level_step(int32_t v, int up)
{
    int32_t db;
    if (!v)
        return up ? 1 : 0;
    db = v - LVL_0DB;
    if (up)
        db = db < -50 ? db - ((db % 10 + 10) % 10) + 10 : db + 1;
    else if (db > -50)
        db -= 1;
    else
        db = (db % 10 + 10) % 10 ? db - ((db % 10 + 10) % 10) : db - 10;
    return db < -600 ? 0 : db + LVL_0DB;
}
/* a knob's turn of s detents: a choice a step a detent; LEVEL by dB then tenths; PAN a step a detent; any other value
 * faster the faster it turns */
static void p_turn(uint32_t id, int32_t s)
{
    int32_t n = p_count(id), a = s < 0 ? -s : s;
    if (!s || id == P_NONE)
        return;
    if (id == P_LEVEL) {
        int32_t v = p_get(id);
        for (; a > 0; a--)
            v = level_step(v, s > 0);
        p_set(id, v);
    } else if (id == P_PAN || p_hi(id) <= 100)   /* (PAN, an LFO's RATE and FADE: a step a detent) */
        p_set(id, p_get(id) + s);
    else if (id == P_BPM)                        /* (the tempo: a BPM a detent, faster turned: five) */
        p_set(id, p_get(id) + s * (a >= 2 ? 5 : 1));
    else if (n > 0)
        p_set(id, p_get(id) + (s > 0 ? 1 : -1));
    else
        p_set(id, p_get(id) + s * (a >= 3 ? 14 : a == 2 ? 6 : 2));
}

/* a parameter's name as the rows show it */
static const char *p_name(uint32_t id)
{
    const ch_par_t *c = &par.ch[ui.sel];
    const slot_par_t *s = &par.slot[pslot()];
    switch (id) {
    case P_GAIN: return "GAIN";
    case P_NOISE: return "NOISE";
    case P_NTYPE: return "TYPE";
    case P_TUBE: return "TUBE";
    case P_TMODE: return "MODE";
    case P_OPAMP: return "OP-AMP";
    case P_FOLD: return "FOLD";
    case P_OVER: return "PROTECT";
    case P_LIM: return "LIMIT";
    case P_FTYPE: case P_OFTYPE: return "FILTER";
    case P_HIGH: return c->ftype == FT_CUT ? "HI CUT" : "HIGH";
    case P_LOW: return c->ftype == FT_CUT ? "LO CUT" : "LOW";
    case P_OHIGH: return par.out.ftype == FT_CUT ? "HI CUT" : "HIGH";
    case P_OLOW: return par.out.ftype == FT_CUT ? "LO CUT" : "LOW";
    case P_MTYPE: return "MIDDLE";
    case P_MGAIN: return c->mtype == MT_PEAK ? "PEAK" : c->mtype == MT_NOTCH ? "NOTCH Q" : "BAND Q";
    case P_MFREQ: case P_OMFREQ: return "FREQ";
    case P_LEVEL: return "LEVEL";
    case P_PAN: return "PAN";
    case P_SEND1: return "SEND 1";
    case P_SEND2: return "SEND 2";
    case P_SRC: return "INPUT";
    case P_MAINMIX: return "MAIN MIX";
    case P_MUTE: return "MUTE";
    case P_FX: return "EFFECT";
    case P_P1: return FX_P1[clampi(s->fx, 0, FX_N - 1)];
    case P_P2: return FX_P2[clampi(s->fx, 0, FX_N - 1)];
    case P_PRES: return "PRESENCE";
    case P_LOFI: return "LO-FI";
    case P_BYP: return "BYPASS";
    case P_FIN: return "IN";
    case P_FOUT: return "OUT";
    case P_RET1: return "RET 1";
    case P_RET2: return "RET 2";
    case P_OWIDE: return "PEAK";
    case P_OMGAIN: return "GAIN";
    case P_SPACE: return "SPACE";
    case P_SPRES: return "SPACE MIX";
    case P_OLEVEL: return "LEVEL";
    case P_BPM: return "TEMPO";
    case P_SWING: return "SWING";
    case P_CHAOS: return "CHAOS";
    case P_GSCALE: return "LENGTH";              /* (the gates': "GATE LENGTH" does not fit beside its bar) */
    case P_RMIX: return "MIX";
    case P_LSHAPE: return "SHAPE";
    case P_LRATE: return "RATE";
    case P_LMODE: return "MODE";
    case P_LFADE: return "FADE";
    case P_DTUNE: return "TUNE";
    case P_DDECAY: return "DECAY";
    case P_DDRIVE: return "DRIVE";
    case P_DLEVEL: return "LEVEL";
    case P_DSWEEP: return "SWEEP";
    case P_DATTACK: return "ATTACK";
    case P_DNOISE: return "NOISE";
    case P_DVERB: return "REVERB";
    case P_DBEND: return "BEND";
    case P_DCOLOR: return "COLOR";
    case P_DCLICK: return "CLICK";
    case P_DPAN: return "PAN";
    case P_DMETAL: return "METAL";
    case P_DFILT: return "FILTER";
    case P_DSWING: return "SWING";
    case P_CAMT: return "AMOUNT";
    case P_CTHR: return "THRESH";               /* ("THRESHOLD" does not fit beside its bar) */
    case P_CREL: return "RELEASE";
    case P_CSC: return "SIDECHAIN";
    case P_CATT: return "ATTACK";
    case P_CRAT: return "RATIO";
    default: return "";
    }
}
/* dB x 10 as text: -12.5 DB, +3 DB, OFF */
static void db_text(char *b, int32_t db10)
{
    int32_t a = db10 < 0 ? -db10 : db10;
    b[0] = db10 < 0 ? '-' : db10 > 0 ? '+' : ' ';
    fmt_int(b + 1, a / 10);                      /* (whole dBs as they are: -12 DB; tenths when there are: -4.9 DB) */
    if (a % 10) {
        uint32_t n = str_len(b);
        b[n] = '.';
        b[n + 1] = (char)('0' + a % 10);
        b[n + 2] = 0;
    }
    if (b[0] == ' ')
        str_cpy(b, b + 1, 12);
    str_cpy(b + str_len(b), " DB", 4);
}
static void hz_text(char *b, int32_t hz)
{
    if (hz < 1000) {
        fmt_int(b, hz);
        str_cpy(b + str_len(b), " HZ", 4);
    } else {
        fmt_fix(b, hz / 100, 1);
        str_cpy(b + str_len(b), " KHZ", 5);
    }
}
static void drm_ms_text(char *b, uint32_t samples)   /* the DRUMS' times, from samples: 2.5 MS, 36 MS, 1.2 S */
{
    uint32_t t10 = samples * 100u / 441u;        /* (tenths of a ms) */
    if (t10 < 100u)
        fmt_fix(b, (int32_t)t10, 1);
    else if (t10 < 10000u)
        fmt_int(b, (int32_t)((t10 + 5u) / 10u));
    else {
        fmt_fix(b, (int32_t)((t10 + 500u) / 1000u), 1);
        str_cpy(b + str_len(b), " S", 3);
        return;
    }
    str_cpy(b + str_len(b), " MS", 4);
}
static void ms_text(char *b, int32_t samples)    /* a time from samples: 0.4 MS, 36 MS, 1.2 S */
{
    int32_t us10 = samples * 2268 / 1000;        /* (tenths of a ms: 1 / 44.1) */
    if (us10 < 100)
        fmt_fix(b, us10, 1);
    else
        fmt_int(b, (us10 + 5) / 10);
    str_cpy(b + str_len(b), " MS", 4);
}
static void p_text(uint32_t id, int32_t v, char *b)
{
    const ch_par_t *c = &par.ch[ui.sel];
    const slot_par_t *s = &par.slot[pslot()];
    switch (id) {
    case P_GAIN: db_text(b, v * 480 / PMAX); break;
    case P_NOISE:
        if (!v)
            str_cpy(b, "OFF", 12);
        else if (c->ntype == NZ_GEIGER) {
            int32_t mhz = hz_log(v, 300, 2550);
            fmt_fix(b, mhz / 100, 1);
            str_cpy(b + str_len(b), " HZ", 4);
        } else
            db_text(b, -900 + v * 720 / PMAX);
        break;
    case P_NTYPE: str_cpy(b, NZ_NAME[v], 12); break;
    case P_TUBE: str_cpy(b, TB_NAME[v], 12); break;
    case P_TMODE: str_cpy(b, TM_NAME[v], 12); break;
    case P_OPAMP: str_cpy(b, OA_NAME[v], 12); break;
    case P_OVER: db_text(b, -60 + v * 300 / PMAX); break;    /* (0.5 .. 16 x full scale) */
    case P_LIM: ms_text(b, (int32_t)lim_len(v)); break;
    case P_FTYPE: case P_OFTYPE: str_cpy(b, v == FT_CUT ? "CUT" : "SHELF", 12); break;
    case P_HIGH: case P_OHIGH:
        if ((id == P_HIGH ? c->ftype : par.out.ftype) == FT_CUT)
            hz_text(b, hcut_hz(v));
        else
            db_text(b, shelf_db10(v));
        break;
    case P_LOW: case P_OLOW:
        if ((id == P_LOW ? c->ftype : par.out.ftype) == FT_CUT)
            hz_text(b, lcut_hz(v));
        else
            db_text(b, shelf_db10(v));
        break;
    case P_MTYPE: str_cpy(b, MT_NAME[v], 12); break;
    case P_MGAIN:
        if (c->mtype == MT_PEAK)
            db_text(b, peak_db10(v));
        else
            fmt_fix(b, bw_q12(v) * 10 / 4096, 1);
        break;
    case P_OMGAIN: db_text(b, peak_db10(v)); break;
    case P_MFREQ: case P_OMFREQ: hz_text(b, mid_hz(v)); break;
    case P_LEVEL:
        if (!v)
            str_cpy(b, "OFF", 12);
        else
            db_text(b, lvl_db10(v));
        break;
    case P_SEND1: case P_SEND2: case P_RET1: case P_RET2:
        if (!v)
            str_cpy(b, "OFF", 12);
        else
            db_text(b, snd_db10(v));
        break;
    case P_OLEVEL: db_text(b, main_db10(v)); break;
    case P_DTUNE: hz_text(b, (int32_t)((drm_hz8(v) + 128u) >> 8)); break;
    case P_DDECAY: drm_ms_text(b, drm_dec_tau(v) * 7u); break;   /* (to -42 dB: about what is heard) */
    case P_DATTACK:
        if (!v)
            str_cpy(b, "HARD", 12);
        else
            drm_ms_text(b, drm_att_len(v));
        break;
    case P_DCOLOR: hz_text(b, (int32_t)drm_log(v, 60, 528)); break;
    case P_DMETAL: str_cpy(b, DM_NAME[clampi(v, 0, DM_N - 1)], 12); break;
    case P_DFILT: str_cpy(b, DN_NAME[clampi(v, 0, DN_N - 1)], 12); break;
    case P_DSWING: fmt_int(b, v); break;
    case P_CAMT:
        if (!v)
            str_cpy(b, "OFF", 12);
        else {
            fmt_int(b, v);
            str_cpy(b + str_len(b), " %", 3);
        }
        break;
    case P_CTHR: db_text(b, cmp_thr_db10(v)); break;
    case P_CSC: str_cpy(b, v ? "KICK" : "OFF", 12); break;
    case P_CRAT: str_cpy(b, CMP_RNAME[clampi(v, 0, 5)], 12); break;
    case P_CATT: case P_CREL: {                  /* (tenths of a ms) */
        int32_t t = id == P_CATT ? cmp_att_x10(v) : cmp_rel_x10(v);
        if (t < 100)
            fmt_fix(b, t, 1);
        else
            fmt_int(b, (t + 5) / 10);
        str_cpy(b + str_len(b), " MS", 4);
        break;
    }
    case P_PAN: case P_DPAN:
        if (!v)
            str_cpy(b, "CENTRE", 12);
        else {
            b[0] = v < 0 ? 'L' : 'R';
            b[1] = ' ';
            fmt_int(b + 2, v < 0 ? -v : v);
        }
        break;
    case P_SRC: str_cpy(b, SRC_NAME[v], 12); break;
    case P_MAINMIX: case P_MUTE: case P_LOFI: case P_BYP: str_cpy(b, v ? "ON" : "OFF", 12); break;
    case P_OWIDE: str_cpy(b, v ? "WIDE" : "NARROW", 12); break;
    case P_FX: str_cpy(b, FX_NAME[v], 12); break;
    case P_FIN: str_cpy(b, FI_NAME[v], 12); break;
    case P_FOUT: str_cpy(b, FO_NAME[v], 12); break;
    case P_SPACE: str_cpy(b, SP_NAME[v], 12); break;
    case P_BPM:
        fmt_int(b, v);
        str_cpy(b + str_len(b), " BPM", 5);
        break;
    case P_RMIX:                                 /* (0: the loop as it runs, the gate unheard; 100: all gate) */
        fmt_int(b, v);
        str_cpy(b + str_len(b), " %", 3);
        break;
    case P_GSCALE:                               /* (as drawn: 100 %) */
        fmt_int(b, v * 2);
        str_cpy(b + str_len(b), " %", 3);
        break;
    case P_SWING:
    case P_CHAOS:
        fmt_int(b, v);
        break;
    case P_LSHAPE: str_cpy(b, MS_NAME[v], 12); break;
    case P_LMODE: str_cpy(b, MM_NAME[v], 12); break;
    case P_LRATE: {                              /* 0.01 Hz x 2000^(rate / 100) */
        uint32_t ch = (uint32_t)(((uint64_t)100u * exp2_q16(v * 7189)) >> 16);   /* (Hz x 100) */
        if (ch < 1000u) {
            b[0] = (char)('0' + ch / 100u);
            b[1] = '.';
            b[2] = (char)('0' + ch / 10u % 10u);
            b[3] = (char)('0' + ch % 10u);
            b[4] = 0;
        } else
            fmt_fix(b, (int32_t)(ch / 10u), 1);
        str_cpy(b + str_len(b), " HZ", 4);
        break;
    }
    case P_LFADE:
        if (!v)
            str_cpy(b, "OFF", 12);
        else {
            b[0] = (char)('0' + v / 10);
            b[1] = '.';
            b[2] = (char)('0' + v % 10);
            str_cpy(b + 3, " S", 3);
        }
        break;
    case P_FOLD:
        if (!v)
            str_cpy(b, "OFF", 12);
        else {                                   /* the drive into the fold: X 1.2 .. X 16 */
            int32_t d10 = 10 + v * 150 / PMAX;
            b[0] = 'X';
            b[1] = ' ';
            fmt_int(b + 2, d10 / 10);
            if (d10 < 100) {
                uint32_t n = str_len(b);
                b[n] = '.';
                b[n + 1] = (char)('0' + d10 % 10);
                b[n + 2] = 0;
            }
        }
        break;
    case P_P1:
    case P_P2:
        if (fx_pcount(s->fx, id == P_P2)) {      /* a choice: its name */
            int32_t n = clampi(v, 0, fx_pcount(s->fx, id == P_P2) - 1);
            static const char *const NOTE[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
            if ((s->fx == FX_RESON || s->fx == FX_REED) && id == P_P1) {   /* ROOT, NOTE: C2 .. B5 */
                str_cpy(b, NOTE[n % 12], 4);
                b[str_len(b) + 1] = 0;
                b[str_len(b)] = (char)('2' + n / 12);
            } else if (s->fx == FX_RESON)
                str_cpy(b, CHORD_NAME[n], 12);
            else if (s->fx == FX_XMOD && id == P_P1)
                str_cpy(b, XM_NAME[n], 12);
            else if (s->fx == FX_CYCLES)
                str_cpy(b, CY_NAME[n], 16);
            else if (s->fx == FX_OCTAVE)
                str_cpy(b, OC_NAME[n], 12);
            else if (s->fx == FX_DIFFUSE) {      /* SIZE 1 .. 8 */
                str_cpy(b, "SIZE ", 12);
                b[5] = (char)('1' + n);
                b[6] = 0;
            } else if (s->fx == FX_SAG)          /* TIME: its window */
                ms_text(b, 64 << n);
            else
                str_cpy(b, XSRC_NAME[n], 12);
            break;
        }
        if (id == P_P2) {
            fmt_int(b, (v + 5) / 10);
            break;
        }
        if (s->fx == FX_CUTS) {                  /* UNIT: 10 .. 46 ms */
            ms_text(b, 441 + v * 1607 / PMAX);
            break;
        }
        if (s->fx == FX_DELAY) {
            ms_text(b, hz_log(v, 44, 2183) * (s->lofi ? 2 : 1));
            break;
        }
        /* fallthrough */
    default:                                     /* 0 .. 100 */
        fmt_int(b, (v + 5) / 10);
        break;
    }
}
static int p_is_bar(uint32_t id) { return p_count(id) == 0; }
static int32_t p_bar(uint32_t id, int32_t v, int32_t w) { return (v - p_lo(id)) * w / (p_hi(id) - p_lo(id)); }

/* ---------------------------------------------------------- patches --- */
static void ch_fresh_all(void)                    /* (a new patch: what the ISR worked out, out of date) */
{
    uint32_t c;
    for (c = 0; c < NCH; c++)
        chr[c].seen.src = -1;
    for (c = 0; c < NSLOT; c++)
        slr[c].seen.fx = -1;
}
static void gest_halt(void);
static void goto_page(uint32_t pg);
static int rhy_page(void) { return ui.page == PG_TEMPO || ui.page == PG_RHY2; }
static int drm_page(void) { return ui.page >= PG_DRUM && ui.page <= PG_DRUM3; }
/* a new drum rhythm, of style st (-1: another than the one drawn), playing */
static void drm_new(int st)
{
    if (st < 0) {
        st = (int)(next_seed() % (DS_N - 1u));
        if (st >= (int)drm.style)
            st++;
    }
    drm_draw_rhythm(next_seed(), st);
    drm.on = 1;
    drm_reset = 1;
    say(DS_NAME[drm.style % DS_N], 1200);
    ui.force = 1;
}
static void mtx_knob(uint32_t j, int32_t s, int sh);
static void mtx_from(int32_t s);
static void patch_set(const lil_par_t *q, const char *name)
{
    par = *q;
    ch_fresh_all();
    gest_halt();                                 /* (a loop playing stops: it was another patch's) */
    str_cpy(ui.patch, name, sizeof ui.patch);
    ui.browse = 0;
    trim_start();
    ui.force = 1;
}
#include "perform.c"                     /* (the GESTURE loop and the SNAPSHOTS) */

/* a new rhythm of style st (-1: another than the one playing), for the patch as it is, and playing from the top */
static void rhy_new(int st)
{
    uint32_t got;
    if (st < 0) {
        st = (int)(next_seed() % (RS_N - 1u));
        if (st >= (int)par.rhy.style)
            st++;
    }
    got = rhythm_draw(&par, next_seed(), st);
    if (!par.rhy.bpm)
        par.rhy.bpm = 120;
    if (!par.rhy.gscale)
        par.rhy.gscale = 50;
    par.rhy.on = 1;
    rhy_reset = 1;
    say(RS_NAME[got % RS_N], 1200);
    ui.force = 1;
}

/* a new patch: from recipe fam, or (NP_ANY) from any; the main view then names its recipe */
enum { NP_ANY = -1 };
static void new_patch(int fam)
{
    static lil_par_t q;
    uint32_t f;
    if (fam < 0 || fam >= (int)NFAM)
        fam = (int)(next_seed() % NFAM);
    f = patch_draw(&q, next_seed(), fam);
    q.rhy = par.rhy;                             /* (the RHYTHM plays on over a new patch) */
    ui.fam = ui.recipe = (uint8_t)f;             /* (the main view names it: PRESETS goes on from it) */
    patch_set(&q, FAM[f].name);
    say("NEW PATCH", 1200);
    ui.home_until = fm1_ms + 400u;
}

/* ------------------------------------------------------------ sessions --- */
enum { SES_OK, SES_NOFLASH, SES_FULL, SES_FAIL };
static int ses_save(uint32_t num);
static int ses_load(uint32_t num);
static uint32_t ses_next_number(void);
static int ses_has(uint32_t num);
static int ses_current(void);
static void ses_done(int rc, const char *what, uint32_t num)
{
    char n[8];
    fmt_int(n, (int32_t)num);
    if (rc == SES_OK)
        flash(what, n, C_SES);
    else
        flash(rc == SES_FULL ? "FULL" : rc == SES_NOFLASH ? "NO FLASH" : "FAILED", "", C_BAD);
}
static void save_long(void)                      /* SAVE held: over the session loaded, or a new one (asked) */
{
    if (ui.page == PG_ASK)
        return;
    if (ses_current()) {
        ses_done(ses_save((uint32_t)ses_current()), "SAVED", (uint32_t)ses_current());
        return;
    }
    ui.ses_new = (uint16_t)ses_next_number();
    if (!ui.ses_new) {
        flash("FULL", "", C_BAD);
        return;
    }
    ui.ses_yes = 1;
    ui.ses_over = 0;
    ui.last_ch_page = ui.page <= PG_CH_LAST ? ui.page : ui.last_ch_page;
    ui.page = PG_ASK;
    ui.force = 1;
}
static void page_back(void)
{
    ui.page = ui.last_ch_page;
    ui.force = 1;
}
static void save_short(void)
{
    if (ui.page == PG_ASK) {                     /* the question answered */
        uint32_t n = ui.ses_new;
        page_back();
        if (ui.ses_yes) {
            int rc = ses_save(n);
            ses_done(rc, "SAVED", n);
        }
        return;
    }
    if (ui.page == PG_SES) {                     /* the session shown: loaded */
        if (ses_has(ui.ses_ix)) {
            int rc = ses_load(ui.ses_ix);
            ses_done(rc, "LOADED", ui.ses_ix);
        } else
            say("EMPTY", 1000);
        page_back();
        return;
    }
    if (ui.page == PG_SNAP)                      /* SAVE, short: the SNAPSHOTS page, or back */
        page_back();
    else {
        ui.snap_key = -1;
        goto_page(PG_SNAP);
    }
}

/* ------------------------------------------------------------- the panel --- */
static int32_t enc_take(uint32_t role) { return fm1_enc_take(PANEL_ENC[role]); }
static int btn_bit(uint32_t mask, uint32_t label) { return (int)((mask >> PANEL_BTN[label]) & 1u); }
static int32_t white_ix(uint32_t k)
{
    static const int8_t W[12] = {0, -1, 1, -1, 2, 3, -1, 4, -1, 5, -1, 6};
    uint32_t n = 53u + k, o = n / 12u;
    int32_t w = W[n % 12u];
    if (w < 0)
        return -1;
    return (int32_t)o * 7 + w - (4 * 7 + 3);     /* F3 = 0 */
}
static void setup_open(void);
static void setup_press(int sh);
static void setup_preset(int32_t s);
static void setup_draw(void);
static void midi_take(void);
static void touch(int32_t row)
{
    ui.touched = (int8_t)row;
    ui.touch_until = fm1_ms + 1200u;
}
static void goto_page(uint32_t pg)
{
    if (ui.page <= PG_FX)                        /* (the page SEQ's turn comes back to) */
        ui.last_ch_page = ui.page;
    ui.page = (uint8_t)pg;
    ui.touched = -1;
    ui.dial_until = 0;
    ui.force = 1;
}
static void ui_input(void)
{
    uint32_t rel, press = fm1_input_edges(&rel), keys, down, up, c, k;
    int32_t s;
    int sh;
    (void)rel;
    {   /* CPU1 retired as it plays: said, once */
        static uint8_t was_on;
        if (was_on && !audio_worker_online)
            say("1 CORE", 4000);
        was_on = (uint8_t)audio_worker_online;
    }
    trim_poll();
    keys = fm1_in.notes;
    down = keys & ~ui.keys;
    up = ui.keys & ~keys;
    ui.keys = keys;
    sh = (int)((keys >> KEY_SHIFT) & 1u);
    midi_take();
    /* the channels */
    for (c = 0; c < NCH; c++)
        if (btn_bit(press, LAYER_BTN[c])) {
            if (ui.set_page) {                   /* (GLOBAL SETUP left: the channel's main view) */
                ui.set_page = 0;
                ui.page = PG_MAIN;
            } else if (ui.page <= PG_CH_LAST) {         /* a channel's page: the same page on another; this one again: MAIN */
                if (c == ui.sel && ui.page != PG_MAIN)
                    ui.page = PG_MAIN;
            } else {
                ui.set_page = 0;
                ui.page = PG_MAIN;
            }
            ui.sel = (uint8_t)c;
            ui.dial_until = 0;
            ui.touched = -1;
            ui.force = 1;
        }
    if (ui.set_page) {                           /* GLOBAL SETUP: its own (setup.c) */
        if (btn_bit(press, B_SAVE))
            setup_press(sh);
        s = enc_take(EN_PRESET);
        if (s)
            setup_preset(s);
        (void)enc_take(EN_K1);
        (void)enc_take(EN_K2);
        (void)enc_take(EN_K3);
        (void)enc_take(EN_K4);
        goto knobs_lp;
    }
    /* HOME: a new patch from any recipe; SHIFT + HOME: from the recipe named; held 0.8 s: the patch nudged */
    if (btn_bit(press, B_HOME)) {
        ui.home_down = 1;
        ui.home_sh = (uint8_t)sh;
        ui.home_done = 0;
        ui.home_t0 = fm1_ms;
    }
    if (ui.home_down) {
        if (!btn_bit(fm1_in.buttons, B_HOME)) {
            ui.home_down = 0;
            if (!ui.home_done)
                new_patch(ui.home_sh ? (int)ui.recipe : NP_ANY);
        } else if (!ui.home_done && (uint32_t)(fm1_ms - ui.home_t0) >= 800u) {
            ui.home_done = 1;
            patch_shift(&par, next_seed());
            say("NUDGED", 1000);
            ui.home_until = fm1_ms + 250u;
        }
    }
    /* the pages */
    if (btn_bit(press, B_OCTDN) || btn_bit(press, B_OCTUP)) {
        int up = btn_bit(press, B_OCTUP);
        int k = seq_ix(ui.page);
        if (k >= 0)                              /* SEQ's pages: through them */
            goto_page(SEQ_PG[up ? (k + 1 < (int)sizeof SEQ_PG ? k + 1 : k) : (k ? k - 1 : 0)]);
        else if (ui.page <= PG_FX)
            goto_page(up ? (ui.page < PG_FX ? ui.page + 1u : ui.page) : (ui.page ? ui.page - 1u : 0u));
        else if (drm_page())                     /* the DRUMS': the voice's two pages, then the rhythm's */
            goto_page(up ? (ui.page < PG_DRUM3 ? ui.page + 1u : ui.page) : (ui.page > PG_DRUM ? ui.page - 1u : ui.page));
        else if (rhy_page())                     /* the RHYTHM's: TEMPO .., then MIX */
            goto_page(up ? PG_RHY2 : PG_TEMPO);
    }
    if (btn_bit(press, B_SEQ) && sh) {          /* SHIFT + SEQ: the chosen channel's modulation thrown anew */
        mod_random(&par.ch[ui.sel].mod, next_seed());
        say("NEW MOTION", 1000);
        ui.force = 1;
    } else if (btn_bit(press, B_SEQ)) {         /* COMP, OUT, OUT EQ, OUT FX, LFO 1, LFO 2, MOD 1 TO 4, MOD 5 TO 8, back */
        int k = seq_ix(ui.page);
        if (k >= 0 && k + 1 < (int)sizeof SEQ_PG)
            goto_page(SEQ_PG[k + 1]);
        else if (k >= 0)
            goto_page(ui.last_ch_page <= PG_FX ? ui.last_ch_page : PG_MAIN);
        else
            goto_page(SEQ_PG[0]);
    }
    if (btn_bit(press, B_ARP)) {
        if (ui.page == PG_VIEW)
            page_back();
        else
            goto_page(PG_VIEW);
    }
    if (btn_bit(press, B_PLAY)) {                /* PLAY: the channel's gesture loop; SHIFT + PLAY: the output muted */
        if (sh) {
            par.out.mute = (uint8_t)!par.out.mute;
            say(par.out.mute ? "OUT MUTED" : "OUT ON", 1000);
        } else
            gest_play_press();
    }
    if (btn_bit(press, B_REC)) {                 /* REC: arm, end; held 1 s: empty */
        ui.rec_down = 1;
        ui.rec_done = 0;
        ui.rec_t0 = fm1_ms;
    }
    if (ui.rec_down) {
        if (!btn_bit(fm1_in.buttons, B_REC)) {
            ui.rec_down = 0;
            if (!ui.rec_done)
                gest_rec_press();
        } else if (!ui.rec_done && (uint32_t)(fm1_ms - ui.rec_t0) >= 1000u) {
            ui.rec_done = 1;
            gest_clear();
        }
    }
    /* SAVE (and SHIFT + SAVE) */
    if (btn_bit(press, B_SAVE)) {
        if (sh) {
            ui.ssv_down = 1;
            ui.ssv_long = 0;
            ui.ssv_t0 = fm1_ms;
        } else {
            ui.save_down = 1;
            ui.save_long = 0;
            ui.save_t0 = fm1_ms;
        }
    }
    if (ui.save_down) {
        if (!btn_bit(fm1_in.buttons, B_SAVE)) {
            ui.save_down = 0;
            if (!ui.save_long)
                save_short();
        } else if (!ui.save_long && (uint32_t)(fm1_ms - ui.save_t0) >= 800u) {
            ui.save_long = 1;
            save_long();
        }
    }
    if (ui.ssv_down) {
        if (!btn_bit(fm1_in.buttons, B_SAVE)) {
            ui.ssv_down = 0;
            if (!ui.ssv_long) {                  /* the SESSIONS page, or back */
                if (ui.page == PG_SES)
                    page_back();
                else {
                    ui.ses_ix = (uint8_t)(ses_current() ? ses_current() : 1);
                    goto_page(PG_SES);
                }
            }
        } else if (!ui.ssv_long && (uint32_t)(fm1_ms - ui.ssv_t0) >= 800u) {
            ui.ssv_long = 1;
            if (ui.page <= PG_CH_LAST)
                ui.last_ch_page = ui.page;
            setup_open();
        }
    }
    /* the keys */
    if ((down >> KEY_RHY) & 1u) {                /* the RHYTHM's key */
        ui.rhy_down = 1;
        ui.rhy_sh = (uint8_t)sh;
        ui.rhy_done = 0;
        ui.rhy_t0 = fm1_ms;
    }
    if (ui.rhy_down) {
        if ((up >> KEY_RHY) & 1u) {
            ui.rhy_down = 0;
            if (!ui.rhy_done) {
                if (ui.rhy_sh)                   /* SHIFT + it: a new rhythm (another style), playing */
                    rhy_new(-1);
                else {                           /* on, or off */
                    uint32_t c, any = 0;
                    for (c = 0; c < NCH; c++)
                        any |= par.rhy.lane[c].mode != RM_OFF;
                    if (!par.rhy.on && !any)
                        rhy_new(-1);             /* (none drawn yet: one) */
                    else {
                        par.rhy.on = (uint8_t)!par.rhy.on;
                        rhy_reset = 1;
                        say(par.rhy.on ? "RHYTHM ON" : "RHYTHM OFF", 900);
                    }
                }
            }
        } else if (!ui.rhy_done && !ui.rhy_sh && (uint32_t)(fm1_ms - ui.rhy_t0) >= 800u) {
            ui.rhy_done = 1;                     /* held: the TEMPO page, or back */
            if (rhy_page())
                page_back();
            else
                goto_page(PG_TEMPO);
        }
    }
    if ((down >> KEY_DRM) & 1u) {                /* the DRUMS' key */
        ui.drm_down = 1;
        ui.drm_sh = (uint8_t)sh;
        ui.drm_done = 0;
        ui.drm_t0 = fm1_ms;
    }
    if (ui.drm_down) {
        if ((up >> KEY_DRM) & 1u) {
            ui.drm_down = 0;
            if (!ui.drm_done) {
                if (ui.drm_sh)                   /* SHIFT + it: a new drum rhythm (another style), playing */
                    drm_new(-1);
                else if (drm_page())             /* short: their pages, or back */
                    page_back();
                else {
                    if (ui.page <= PG_FX)
                        ui.last_ch_page = ui.page;
                    ui.set_page = 0;
                    goto_page(PG_DRUM);
                }
            }
        } else if (!ui.drm_done && !ui.drm_sh && (uint32_t)(fm1_ms - ui.drm_t0) >= 800u) {
            ui.drm_done = 1;                     /* held: on, or off */
            drm.on = (uint8_t)!drm.on;
            drm_reset = 1;
            say(drm.on ? "DRUMS ON" : "DRUMS OFF", 900);
            ui.force = 1;
        }
    }
    if (ui.dkey >= 0) {                          /* a DRUMS voice's key held 0.7 s: muted, or back */
        if (!((keys >> (uint32_t)ui.dkey) & 1u))
            ui.dkey = -1;
        else if (!ui.dkey_done && (uint32_t)(fm1_ms - ui.dkey_t0) >= 700u) {
            ui.dkey_done = 1;
            drm.mute ^= (uint8_t)(1u << ui.dsel);
            say(drm.mute >> ui.dsel & 1u ? "MUTED" : "UNMUTED", 900);
            ui.force = 1;
        }
    }
    if (ui.page == PG_SNAP && ui.snap_key >= 0) {   /* a snapshot's key: let go soon, recalled; held, stored */
        uint32_t kk = (uint32_t)ui.snap_key, sw = (uint32_t)white_ix(kk);
        if ((up >> kk) & 1u) {
            if (!ui.snap_done)
                snap_recall(sw);
            ui.snap_key = -1;
        } else if (!ui.snap_done && (uint32_t)(fm1_ms - ui.snap_t0) >= 800u) {
            ui.snap_done = 1;
            snap_store(sw);
        }
    }
    for (k = 0; k < NKEY; k++) {
        int32_t w = white_ix(k);
        if (!((down >> k) & 1u) || k == KEY_SHIFT)
            continue;
        if (ui.page == PG_SNAP && w >= 0) {      /* the SNAPSHOTS page: the white keys are the sixteen */
            if (sh)
                snap_clear((uint32_t)w);
            else {
                ui.snap_key = (int8_t)k;
                ui.snap_t0 = fm1_ms;
                ui.snap_done = 0;
            }
            ui.force = 1;
            continue;
        }
        if (k == KEY_RHY || k == KEY_DRM)
            continue;                            /* (their own, above) */
        if (drm_page() && w >= 0) {              /* the DRUMS' pages: white keys 1 .. 3 the voices; SHIFT + one, a new
                                                  * sound for it; the rest nothing here */
            if (w < (int32_t)NDV) {
                ui.dsel = (uint8_t)w;
                ui.dkey = (int8_t)k;             /* (held: muted, or back) */
                ui.dkey_t0 = fm1_ms;
                ui.dkey_done = (uint8_t)sh;
                if (sh) {
                    drm_draw_voice((uint32_t)w, next_seed(), -1);
                    drm_coef_all();
                    say(DKIT[drm.kit[w] % NDKIT].name, 1000);
                }
                if (ui.page == PG_DRUM3)
                    goto_page(PG_DRUM);
                ui.force = 1;
            }
            continue;
        }
        if (k == KEY_MUTE[0] && sh) {            /* SHIFT + black key 1: the chosen channel, every parameter at random */
            patch_random_channel(&par, ui.sel, next_seed());
            say("CHANNEL RANDOM", 1000);
            ui.force = 1;
            continue;
        }
        if (w >= 0 && w < SRC_N) {               /* the chosen channel's input */
            par.ch[ui.sel].src = (int16_t)w;
            say(SRC_NAME[w], 900);
        } else if (w == WHITE_MAINMIX) {
            par.ch[ui.sel].mainmix = (uint8_t)!par.ch[ui.sel].mainmix;
            say(par.ch[ui.sel].mainmix ? "MAIN MIX ON" : "MAIN MIX OFF", 1000);
        } else
            for (c = 0; c < NCH; c++)
                if (k == KEY_MUTE[c]) {
                    par.ch[c].mute = (uint8_t)!par.ch[c].mute;
                    if (c == ui.sel || ui.page == PG_VIEW)
                        ui.force = 1;
                }
    }
    ui.cpu_page = (uint8_t)(sh && ((keys >> KEY_CPU) & 1u));
    if (ui.page == PG_SNAP) {                    /* KNOB 1 on the SNAPSHOTS page: the morph time */
        s = enc_take(EN_K1);
        if (s) {
            morph_hs = (uint8_t)clampi(morph_hs + s, 0, 60);
            ui.force = 1;
        }
    }
    /* PRESETS: the RECIPE, or the SESSIONS page's slot, or the question's answer */
    s = enc_take(EN_PRESET);
    if (s) {
        if (ui.page == PG_MTX1 || ui.page == PG_MTX2)
            mtx_from(s);
        else if (drm_page())                     /* the DRUMS' pages: through their rhythms, one of each, playing */
            drm_new((int)((drm.style + DS_N + (s > 0 ? 1u : DS_N - 1u)) % DS_N));
        else if (rhy_page())                     /* the RHYTHM's pages: PRESETS through the styles, a rhythm of each */
            rhy_new((int)((par.rhy.style + RS_N + (s > 0 ? 1u : RS_N - 1u)) % RS_N));
        else if (ui.page == PG_SES)
            ui.ses_ix = (uint8_t)clampi(ui.ses_ix + (s > 0 ? 1 : -1), 1, (int32_t)SES_SLOTS);
        else if (ui.page == PG_ASK)
            ui.ses_yes = (uint8_t)(s > 0);
        else {                                   /* the recipe: its name in the main view, in place; HOME draws from it */
            if (ui.page != PG_MAIN)
                goto_page(PG_MAIN);
            ui.recipe = (uint8_t)((ui.recipe + (s > 0 ? 1u : NFAM - 1u)) % NFAM);
            ui.msg_until = fm1_ms;               /* (a message showing gives way to the name) */
            ui.browse = ui.recipe != ui.fam || !str_eq(ui.patch, FAM[ui.fam].name);
        }
    }
    /* KNOB 1 .. 4 */
    if (ui.page == PG_MTX1 || ui.page == PG_MTX2) {
        for (k = 0; k < 4u; k++) {
            s = enc_take(EN_K1 + k);
            if (s)
                mtx_knob(k, s, sh);
        }
    } else if (ui.page <= PG_COMP || rhy_page() || drm_page()) {
        for (k = 0; k < 4u; k++) {
            uint32_t id = sh ? PG_SHROW[ui.page][k] : PG_ROW[ui.page][k];
            s = enc_take(EN_K1 + k);
            if (!s || id == P_NONE)
                continue;
            p_turn(id, s);
            if (ui.page == PG_MAIN) {            /* the main view: its dial over the screen */
                ui.dial = (uint8_t)id;
                ui.dial_until = fm1_ms + 1000u;
            } else {
                touch((int32_t)k);
                if (p_count(id) > 0)             /* (a choice: the rows may name things anew -- HI CUT, NOTCH Q) */
                    ui.force = 1;
            }
        }
    } else
        for (k = 0; k < 4u; k++)
            (void)enc_take(EN_K1 + k);
knobs_lp:
    /* SELECT, ALGORITHM: the chosen channel's level and pan, everywhere */
    s = enc_take(EN_SELECT);
    if (s) {
        char m[16] = "LEVEL ", v[12];
        p_turn(P_LEVEL, s);
        p_text(P_LEVEL, par.ch[ui.sel].level, v);
        str_cpy(m + 6, v, sizeof m - 6);
        say(m, 900);
    }
    s = enc_take(EN_ALGO);
    if (s) {
        char m[16] = "PAN ", v[12];
        p_turn(P_PAN, s);
        p_text(P_PAN, par.ch[ui.sel].pan, v);
        str_cpy(m + 4, v, sizeof m - 4);
        say(m, 900);
    }
    morph_poll();                                /* (a snapshot's morph, a step on) */
    gest_poll();                                 /* (the gesture loops: played, and what changed this pass recorded) */
}

/* ------------------------------------------------------------ the lights --- */
static void led_dim_key(uint32_t id, int on)
{
    uint32_t p, r;
    for (p = 0; p < FM1_NCOL; p++)
        for (r = 1; r < 5u; r++)
            if (FM1_KEYMAP[r][p] == (int8_t)id) {
                if (on)
                    fm1_led_dim[p] |= (uint8_t)(1u << r);
                else
                    fm1_led_dim[p] &= (uint8_t)~(1u << r);
            }
}
static void ui_leds(void)
{
    uint32_t c, k, blink = (fm1_ms / 250u) & 1u;
    const ch_par_t *p = &par.ch[ui.sel];
    for (c = 0; c < NCH; c++) {
        fm1_led_key(PANEL_BTN[LAYER_BTN[c]], c == ui.sel);
        led_dim_key(PANEL_BTN[LAYER_BTN[c]], ch_sounding(c) && !par.ch[c].mute);
    }
    fm1_led_key(PANEL_BTN[B_HOME], (int32_t)(ui.home_until - fm1_ms) > 0);
    fm1_led_key(PANEL_BTN[B_SEQ], seq_ix(ui.page) >= 0);
    fm1_led_key(PANEL_BTN[B_ARP], ui.page == PG_VIEW);
    fm1_led_key(PANEL_BTN[B_SAVE], ui.set_page || ui.page == PG_SES || ui.page == PG_ASK ||
                                   ui.page == PG_SNAP);
    fm1_led_key(PANEL_BTN[B_PLAY], gl[ui.sel].play);   /* (the chosen channel's loop) */
    fm1_led_key(PANEL_BTN[B_REC], gl_state(ui.sel) == GS_REC || (gl_state(ui.sel) == GS_ARMED && blink));
    fm1_led_key(PANEL_BTN[B_OCTDN], (ui.page > PG_MAIN && ui.page <= PG_FX) || seq_ix(ui.page) > 0);
    fm1_led_key(PANEL_BTN[B_OCTUP], ui.page < PG_FX || (seq_ix(ui.page) >= 0 && seq_ix(ui.page) + 1 < (int)sizeof SEQ_PG));
    for (k = 0; k < NKEY; k++) {
        int32_t w = white_ix(k);
        if (ui.page == PG_SNAP && w >= 0) {      /* the SNAPSHOTS page: stored ones glow, the last lit */
            fm1_led_key(14u + k, w == snap_cur && snap_has((uint32_t)w));
            led_dim_key(14u + k, snap_has((uint32_t)w));
            continue;
        }
        if (drm_page() && w >= 0) {              /* the DRUMS' pages: the voices, the one chosen lit; a muted one
                                                  * blinking, dim */
            int muted = w < (int32_t)NDV && (drm.mute >> w & 1u);
            fm1_led_key(14u + k, w == ui.dsel && !muted);
            led_dim_key(14u + k, muted ? blink : w < (int32_t)NDV);
            continue;
        }
        if (w >= 0 && w < SRC_N) {               /* the inputs: the one chosen lit, the others glowing */
            int self = w >= SRC_CH1 && (uint32_t)(w - SRC_CH1) == ui.sel;
            fm1_led_key(14u + k, p->src == w);
            led_dim_key(14u + k, !self);
        } else if (w == WHITE_MAINMIX) {
            fm1_led_key(14u + k, p->mainmix);
            led_dim_key(14u + k, 1);
        } else if (w < 0) {
            fm1_led_key(14u + k, 0);
            led_dim_key(14u + k, 0);
        }
    }
    for (c = 0; c < NCH; c++) {                  /* the mutes: lit muted, glowing otherwise */
        fm1_led_key(14u + KEY_MUTE[c], par.ch[c].mute);
        led_dim_key(14u + KEY_MUTE[c], 1);
    }
    fm1_led_key(14u + KEY_SHIFT, (ui.keys >> KEY_SHIFT) & 1u);
    if (ui.beat_seen != rhy_beat) {              /* the RHYTHM's key: lit while it plays, dark a moment on each beat */
        ui.beat_seen = rhy_beat;
        ui.beat_at = fm1_ms;
    }
    fm1_led_key(14u + KEY_RHY, par.rhy.on && (uint32_t)(fm1_ms - ui.beat_at) >= 70u);
    led_dim_key(14u + KEY_RHY, 1);
    fm1_led_key(14u + KEY_DRM, drm.on && (uint32_t)(fm1_ms - ui.beat_at) >= 70u);   /* (the DRUMS': lit playing) */
    led_dim_key(14u + KEY_DRM, 1);
}

/* ------------------------------------------------------------ the screen --- */
#define BAND_A 0                 /* the channel's number */
#define BAND_B 98                /* its input */
#define BAND_C 146               /* the patch, or a message */
#define BAND_M 192               /* its meter */
#define BAND_D 216               /* its pan */
static int32_t batt_key(void)
{
    if (!batt_raw)
        return -1;
    if (usb.config && !usb.suspended)
        return 4;
    return batt_raw >= 591 ? 3 : batt_raw >= 561 ? 2 : batt_raw >= 531 ? 1 : 0;
}
static int32_t draw_batt(int32_t x, int32_t y)
{
    int32_t k = batt_key(), w = 18, h = 10, x0 = x - w - 3, i;
    uint16_t c = k >= 2 ? C_OK : k == 1 ? RGB(255, 170, 0) : RGB(255, 40, 40);
    if (k < 0)
        return x;
    cv_rect(x0, y, w, 2, C_WHITE);
    cv_rect(x0, y + h - 2, w, 2, C_WHITE);
    cv_rect(x0, y, 2, h, C_WHITE);
    cv_rect(x0 + w - 2, y, 2, h, C_WHITE);
    cv_rect(x0 + w, y + 3, 3, h - 6, C_WHITE);
    if (k == 4) {
        cv_rect(x0 + 8, y + 2, 3, 3, C_WHITE);
        cv_rect(x0 + 6, y + 4, 6, 2, C_WHITE);
        cv_rect(x0 + 7, y + 5, 3, 3, C_WHITE);
    } else
        for (i = 0; i < (k ? k : 1); i++)
            cv_rect(x0 + 3 + i * 4, y + 3, 3, 4, c);
    return x0 - 8;
}
/* the largest size, from big down, that takes t whole in w pixels without squeezing it (XS at the least) */
static const font_t *fit_font(const char *t, int32_t w, const font_t *big)
{
    static const font_t *const F[5] = {&FONT_XL, &FONT_L, &FONT_M, &FONT_S, &FONT_XS};
    uint32_t i = 0;
    while (i < 4u && F[i] != big)
        i++;
    for (; i < 4u; i++)
        if (text_w(F[i], t) <= w)
            return F[i];
    return &FONT_XS;
}
/* t centred in the canvas at y, in the largest size up to big that fits; y is the top of a band hb high */
static void text_fit_c(int32_t y, int32_t hb, const char *t, const font_t *big, uint16_t c)
{
    const font_t *f = fit_font(t, (int32_t)cv_w - 4, big);
    if (text_w(f, t) <= (int32_t)cv_w - 4)
        cv_text_c(y + (hb - (int32_t)f->h) / 2, f, t, c);
    else
        cv_text_fit(y + (hb - (int32_t)f->h) / 2, f, t, c);   /* (still too wide at XS: squeezed, the last resort) */
}
static void draw_number(void)
{
    char d[2] = {(char)('1' + ui.sel), 0};
    CV_AREA(0, BAND_A, 240, BAND_B - BAND_A, C_BLACK) {
        draw_batt(237, 2 + ((int32_t)FONT_S.h - 10) / 2);
        cv_text_c(3, &FONT_XL, d, ch_col(ui.sel));
        if (!par.ch[ui.sel].mainmix)
            cv_text(4, 2, &FONT_S, "NO MIX", RGB(175, 175, 175));
    }
}
static void draw_src(void)
{
    const char *e = SRC_NAME[clampi(par.ch[ui.sel].src, 0, SRC_N - 1)];
    cv_begin(240, 48, C_BLACK);
    cv_text_c(2, &FONT_L, e, C_WHITE);
    cv_blit(0, BAND_B);
}
/* 0 the patch (channel colour), 1 a message (black on white), 2 MUTED (white on red) */
static void draw_band_c(const char *t, int kind)
{
    static const uint16_t BG[3] = {C_BLACK, C_WHITE, RGB(220, 0, 0)};
    uint16_t fg = kind == 0 ? ch_col(ui.sel) : kind == 1 ? C_BLACK : C_WHITE;
    cv_begin(240, 44, BG[kind]);
    text_fit_c(0, 44, t, &FONT_M, fg);
    cv_blit(0, BAND_C);
}
#define METER_X 6
#define METER_LEN 228
static int32_t meter_px_db10(int32_t db10) { return clampi((db10 + 480) * METER_LEN / 540, 0, METER_LEN); }
static int32_t meter_px(int32_t peak)
{
    int32_t db10;
    if (peak < 130)
        return 0;
    db10 = (int32_t)(((int64_t)(log2_q16((uint32_t)peak) - (15 << 16)) * 602) >> 16) / 10;
    return meter_px_db10(db10);
}
static void draw_meter(int32_t px)              /* (its band from the end of band C to the pan's: no gap left) */
{
    int32_t lv = par.ch[ui.sel].level;
    cv_begin(240, BAND_D - BAND_C - 44, C_BLACK);
    cv_frame(METER_X - 2, 5, METER_LEN + 4, 16, 2, C_GREY);
    if (px)
        cv_rect(METER_X, 7, px, 12, par.ch[ui.sel].mute ? C_GREY : ch_col(ui.sel));
    if (lv)                                      /* the fader: a white bar across, -60 .. +12 dB along the meter */
        cv_rect(METER_X + lv * (METER_LEN - 2) / LVL_MAX - 1, 2, 5, 22, C_WHITE);
    else
        cv_rect(METER_X - 2, 2, 5, 22, C_REC);
    cv_blit(0, BAND_C + 44);
    ui.shown_meter = px;
    ui.shown_level = lv;
}
static void draw_pan(void)
{
    int32_t pn = par.ch[ui.sel].pan, x = 120 + pn * 104 / PAN_MAX;
    cv_begin(240, 240 - BAND_D, C_BLACK);
    cv_rect(6, 10, 228, 4, C_GREY);
    cv_rect(6, 4, 3, 16, C_GREY);
    cv_rect(231, 4, 3, 16, C_GREY);
    cv_rect(119, 2, 3, 20, C_WHITE);
    cv_rect(x - 9, 0, 18, 24, C_WHITE);
    cv_rect(x - 6, 3, 12, 18, ch_col(ui.sel));
    cv_blit(0, BAND_D);
    ui.shown_pan = pn;
}

/* a page: its title (and what its switches are), then four rows -- name, value, a bar under a value */
#define C_RHY RGB(255, 120, 80)
#define C_DRM RGB(255, 196, 40)
static uint16_t page_col(void) { return rhy_page() ? C_RHY : drm_page() ? C_DRM : ui.page >= PG_OUT ? C_OUT : ch_col(ui.sel); }
#define TITLE_H 36               /* a page's title band, then four rows of ROW_H */
#define ROW_H 51
static int show_shift(void)              /* SHIFT held on a page with switches: its rows show them */
{
    uint32_t j;
    if (!((ui.keys >> KEY_SHIFT) & 1u) || (ui.page > PG_COMP && !drm_page()) || (ui.page >= PG_LFO1 && ui.page <= PG_MTX2))
        return 0;
    for (j = 0; j < 4u; j++)
        if (PG_SHROW[ui.page][j] != P_NONE)
            return 1;
    return 0;
}
static uint32_t row_id(uint32_t j)       /* the parameter row j shows now */
{
    return show_shift() && PG_SHROW[ui.page][j] != P_NONE ? PG_SHROW[ui.page][j] : PG_ROW[ui.page][j];
}
static void draw_title(void)
{
    char t[24];
    str_cpy(t, PG_TITLE[ui.page], sizeof t);
    if (ui.page <= PG_CH_LAST) {                 /* the channel's number after the page's name */
        uint32_t n = str_len(t);
        str_cpy(t + n, "  CH ", 6);
        t[n + 5] = (char)('1' + ui.sel);
        t[n + 6] = 0;
    }
    if (rhy_page()) {                            /* the RHYTHM: its style, or OFF */
        str_cpy(t, par.rhy.on ? RS_NAME[par.rhy.style % RS_N] : "RHYTHM OFF", sizeof t);
    }
    if (ui.page == PG_DRUM3)                     /* the DRUMS' rhythm: its style, or OFF */
        str_cpy(t, drm.on ? DS_NAME[drm.style % DS_N] : "DRUMS OFF", sizeof t);
    else if (drm_page()) {                       /* a voice's: its name and its sound's (SHIFT held: the voice's) */
        str_cpy(t, DV_NAME[ui.dsel % NDV], sizeof t);
        if (!show_shift()) {
            str_cpy(t + str_len(t), "  ", 3);
            str_cpy(t + str_len(t), drm.mute >> ui.dsel & 1u ? "MUTED" : DKIT[drm.kit[ui.dsel % NDV] % NDKIT].name,
                    sizeof t - str_len(t));
        }
    }
    if (show_shift())                            /* (SHIFT held: the switches) */
        str_cpy(t + str_len(t), "  SHIFT", 8);
    cv_begin(240, TITLE_H, C_BLACK);
    text_fit_c(0, TITLE_H - 2, t, &FONT_M, page_col());
    cv_rect(0, TITLE_H - 1, 240, 1, RGB(60, 60, 60));
    cv_blit(0, 0);
}
/* a row, on two lines: the name small and grey, a bar beside it (0 none; 1 from the left, v 0 .. 1000; 2 from the
 * middle, v -1000 .. 1000), the value large under it -- the value always whole, never sharing its line */
static void draw_row2(uint32_t j, const char *name, const char *val, int bar, int32_t v, int touched)
{
    uint16_t bg = touched ? C_WHITE : C_BLACK, fg = touched ? C_BLACK : C_WHITE, col = page_col();
    cv_begin(240, ROW_H, bg);
    cv_text(6, 2, &FONT_XS, name, touched ? C_BLACK : RGB(175, 175, 175));
    if (bar) {
        int32_t x0 = 6 + text_w(&FONT_XS, name) + 12, w0 = 234 - x0, y0 = 8;
        cv_rect(x0, y0, w0, 7, touched ? C_GREY : RGB(60, 60, 60));
        if (bar == 1)
            cv_rect(x0, y0, clampi(v, 0, 1000) * w0 / 1000, 7, touched ? C_BLACK : col);
        else {
            int32_t c = x0 + w0 / 2, w = clampi(v, -1000, 1000) * (w0 / 2) / 1000;
            cv_rect(w >= 0 ? c : c + w, y0, w >= 0 ? w : -w, 7, touched ? C_BLACK : col);
            cv_rect(c - 1, y0 - 2, 2, 11, touched ? C_BLACK : C_WHITE);
        }
    }
    if (val && val[0])
        cv_text(6, ROW_H - (int32_t)FONT_S.h - 1, fit_font(val, 228, &FONT_S), val, fg);
    cv_rect(0, ROW_H - 1, 240, 1, RGB(50, 50, 50));
    cv_blit(0, TITLE_H + ROW_H * j);
}
static void draw_row(uint32_t j, int touched)
{
    uint32_t id = row_id(j);
    int32_t v = p_get(id);
    char t[16];
    int bar = id == P_PAN ? 2 : p_is_bar(id) || id == P_LEVEL ? 1 : 0;
    if (id == P_NONE) {                          /* (a row with nothing on it: blank) */
        cv_begin(240, ROW_H, C_BLACK);
        cv_blit(0, TITLE_H + j * ROW_H);
        ui.shown_row[j] = v;
        return;
    }
    p_text(id, v, t);
    draw_row2(j, p_name(id), t, bar, bar == 2 ? v * 1000 / PAN_MAX : p_bar(id, v, 1000), touched);
    ui.shown_row[j] = v;
}
/* the MOD pages: the chosen channel's eight slots, four a page -- each row a slot: where it comes FROM and its
 * number small, where it goes TO and by how much large, the amount a bar from the middle. KNOB 1 .. 4: the AMOUNT;
 * SHIFT + KNOB: TO; PRESETS: FROM, of the slot in hand (the last turned: its row lit) */
static uint32_t mtx_base(void) { return ui.page == PG_MTX2 ? 4u : 0u; }
static void mtx_knob(uint32_t j, int32_t s, int sh)
{
    mslot_t *q = &par.ch[ui.sel].mod.s[mtx_base() + j];
    ui.mtx_focus = (uint8_t)j;
    if (sh)
        q->dst = (uint8_t)clampi(q->dst + (s > 0 ? 1 : -1), 0, MD_N - 1);
    else {
        q->amt = (int8_t)clampi(q->amt + s, -100, 100);
        if (!q->src && q->amt)                   /* (an amount on a slot from nowhere: from LFO 1) */
            q->src = MSRC_L1;
    }
}
static void mtx_from(int32_t s)
{
    mslot_t *q = &par.ch[ui.sel].mod.s[mtx_base() + ui.mtx_focus % 4u];
    q->src = (uint8_t)clampi(q->src + (s > 0 ? 1 : -1), 0, MSRC_N - 1);
}
static int32_t mtx_key(uint32_t j)
{
    const mslot_t *q = &par.ch[ui.sel].mod.s[mtx_base() + j];
    return q->src | q->dst << 4 | (q->amt + 128) << 10 | (int32_t)(j == ui.mtx_focus) << 20;
}
static void draw_mtx_page(void)
{
    uint32_t j;
    int all = ui.force || ui.shown_page != ui.page || ui.shown_sel != ui.sel;
    if (all)
        draw_title();
    for (j = 0; j < 4u; j++) {
        const mslot_t *q = &par.ch[ui.sel].mod.s[mtx_base() + j];
        char nm[24], v[24];
        if (!all && ui.shown_mtx[j] == mtx_key(j))
            continue;
        ui.shown_mtx[j] = mtx_key(j);
        nm[0] = (char)('1' + mtx_base() + j);
        str_cpy(nm + 1, "  FROM ", 8);
        str_cpy(nm + 8, MSRC_NAME[q->src % MSRC_N], 10);
        if (!q->src || !q->amt)
            str_cpy(v, "OFF", sizeof v);
        else {
            str_cpy(v, MD_NAME[q->dst % MD_N], sizeof v);
            str_cpy(v + str_len(v), " ", 2);
            if (q->amt > 0)
                str_cpy(v + str_len(v), "+", 2);
            fmt_int(v + str_len(v), q->amt);
        }
        draw_row2(j, nm, v, 2, q->amt * 10, j == ui.mtx_focus);
    }
}
static void draw_page(void)
{
    uint32_t j;
    int32_t t = (int32_t)(ui.touch_until - fm1_ms) > 0 ? ui.touched : -1;
    static int8_t shown_shift;
    if (ui.force || ui.shown_page != ui.page || ui.shown_sel != ui.sel || shown_shift != show_shift()) {
        shown_shift = (int8_t)show_shift();
        draw_title();
        for (j = 0; j < 4u; j++)
            draw_row(j, (int32_t)j == t % 4);
        ui.shown_touch = (int8_t)t;
        return;
    }
    for (j = 0; j < 4u; j++)
        if (ui.shown_row[j] != p_get(row_id(j)) || (((int32_t)j == t % 4) != ((int32_t)j == ui.shown_touch % 4)))
            draw_row(j, (int32_t)j == t % 4);
    ui.shown_touch = (int8_t)t;
}

/* the main view's dial: a knob's value over the whole screen while it turns (Tonitor's) */
#define DIAL_CX 120
#define DIAL_CY 124
#define DIAL_R 76
static void dial_dot(int32_t oy, int32_t x, int32_t y, int32_t r, uint16_t c) { cv_rect(x - r, y - r - oy, 2 * r + 1, 2 * r + 1, c); }
static void dial_at(uint32_t a, int32_t r, int32_t *x, int32_t *y)
{
    *x = DIAL_CX + mulq15(r, sin_ph(a));
    *y = DIAL_CY - mulq15(r, sin_ph(a + 0x40000000u));
}
static void draw_dial(void)
{
    static const int32_t BAND[3][2] = {{0, 112}, {112, 112}, {224, 16}};
    uint32_t id = ui.dial;
    int32_t v = p_get(id), b, k, to;
    uint32_t av;
    char t[16], m[16];
    uint16_t col = ch_col(ui.sel);
    const font_t *tf;
    str_cpy(t, p_name(id), sizeof t - 3);
    str_cpy(t + str_len(t), "  ", 3);
    t[str_len(t) - 1] = (char)('1' + ui.sel);
    tf = text_w(&FONT_M, t) <= 236 ? &FONT_M : &FONT_S;
    to = -45 + v * 90 / PMAX;                    /* from 7 o'clock round */
    av = 0xA0000000u + (uint32_t)((uint64_t)(0xC0000000u / PMAX) * (uint32_t)v);
    p_text(id, v, m);
    for (b = 0; b < 3; b++) {
        int32_t oy = BAND[b][0], x, y;
        CV_AREA(0, (uint32_t)oy, 240, (uint32_t)BAND[b][1], C_BLACK) {
            cv_text(((int32_t)cv_w - text_w(tf, t)) / 2, 2 + ((int32_t)FONT_M.h - (int32_t)tf->h) / 2 - oy, tf, t, col);
            for (k = -45; k <= 45; k++) {
                dial_at((uint32_t)(k * 35791394), DIAL_R, &x, &y);
                dial_dot(oy, x, y, 4, C_GREY);
            }
            for (k = -45; k <= to; k++) {
                dial_at((uint32_t)(k * 35791394), DIAL_R, &x, &y);
                dial_dot(oy, x, y, 7, col);
            }
            for (k = 0; k <= DIAL_R - 16; k += 4) {
                dial_at(av, k, &x, &y);
                dial_dot(oy, x, y, 4, C_WHITE);
            }
            dial_dot(oy, DIAL_CX, DIAL_CY, 10, C_WHITE);
            if (b >= 1)
                cv_text_c(200 - oy, fit_font(m, 232, &FONT_M), m, C_WHITE);
        }
    }
    ui.shown_dial = v + (int32_t)id * 4096;
    ui.dial_drawn_at = fm1_ms;
}

/* the OVERVIEW: a row a channel -- its number, its input, its level as it sounds */
static void draw_view_row(uint32_t c)
{
    const ch_par_t *p = &par.ch[c];
    int32_t px = meter_px(ch_meter[c]) * 120 / METER_LEN;
    char n[2] = {(char)('1' + c), 0};
    uint16_t col = p->mute ? C_GREY : ch_col(c);
    cv_begin(240, 40, c == ui.sel ? RGB(40, 40, 40) : C_BLACK);
    cv_text(4, (40 - (int32_t)FONT_S.h) / 2, &FONT_S, n, col);
    cv_text(30, (40 - (int32_t)FONT_S.h) / 2, &FONT_S, p->mute ? "MUTED" : SRC_NAME[clampi(p->src, 0, SRC_N - 1)], col);
    cv_frame(112, 12, 124, 16, 2, C_GREY);
    if (px && !p->mute)
        cv_rect(114, 14, px, 12, col);
    cv_blit(0, 40 * c);
    ui.shown_view[c] = px * 4 + p->mute * 2 + (c == ui.sel) + p->src * 1024 + ch_out(c) * 0x100000;
}
static void draw_view(void)
{
    uint32_t c;
    int all = ui.force || ui.shown_page != PG_VIEW;
    if (!all && (uint32_t)(fm1_ms - ui.view_at) < 40u)
        return;
    ui.view_at = fm1_ms;
    for (c = 0; c < NCH; c++) {
        const ch_par_t *p = &par.ch[c];
        int32_t key = meter_px(ch_meter[c]) * 120 / METER_LEN * 4 + p->mute * 2 + (c == ui.sel) + p->src * 1024 +
                      ch_out(c) * 0x100000;
        if (all || key != ui.shown_view[c])
            draw_view_row(c);
    }
}

static int draw_flash(void)
{
    uint32_t t = fm1_ms - ui.flash_t0, ph = t / 250u, on;
    uint16_t bg, fg;
    if (!ui.flash_on)
        return 0;
    if (t >= 1500u) {
        ui.flash_on = 0;
        ui.force = 1;
        return 0;
    }
    if (ph == ui.flash_phase)
        return 1;
    ui.flash_phase = (uint8_t)ph;
    on = !(ph & 1u);
    bg = on ? ui.flash_col : C_BLACK;
    fg = on ? C_BLACK : ui.flash_col;
    CV_AREA(0, 0, 240, 100, bg)
        cv_text_c(ui.flash_l2[0] ? 40 : 52, &FONT_L, ui.flash_l1, fg);
    CV_AREA(0, 100, 240, 112, bg) {
        const char *s = ui.flash_l2;
        int digits = *s != 0;
        for (; *s; s++)
            digits &= *s >= '0' && *s <= '9';
        if (digits)
            cv_text_c(4, &FONT_XL, ui.flash_l2, fg);
        else
            text_fit_c(0, 112, ui.flash_l2, &FONT_L, fg);
    }
    cv_begin(240, 28, bg);
    cv_blit(0, 212);
    return 1;
}
static void draw_noyes(int yes)
{
    cv_frame(6, 10, 108, 60, 3, yes ? C_GREY : C_WHITE);
    cv_frame(126, 10, 108, 60, 3, yes ? C_WHITE : C_GREY);
    if (!yes)
        cv_rect(9, 13, 102, 54, C_WHITE);
    else
        cv_rect(129, 13, 102, 54, C_WHITE);
    cv_bg_set(yes ? C_BLACK : C_WHITE);
    cv_text(60 - text_w(&FONT_M, "NO") / 2, 22, &FONT_M, "NO", yes ? C_GREY : C_BLACK);
    cv_bg_set(yes ? C_WHITE : C_BLACK);
    cv_text(180 - text_w(&FONT_M, "YES") / 2, 22, &FONT_M, "YES", yes ? C_BLACK : C_GREY);
    cv_bg_set(C_BLACK);
}
/* the SESSIONS and question pages: a title, the one shown very large, a line under it */
static int32_t shown_list;
static void draw_list_page(void)
{
    int32_t key;
    char n[8];
    if (ui.page == PG_SES)
        key = 2000 + ui.ses_ix * 2 + ses_has(ui.ses_ix);
    else
        key = 3000 + ui.ses_yes;
    if (!ui.force && ui.shown_page == ui.page && key == shown_list)
        return;
    shown_list = key;
    if (ui.page == PG_SES) {
        int has = ses_has(ui.ses_ix);
        CV_AREA(0, 0, 240, 60, C_BLACK)
            cv_text_c(4, &FONT_L, "SESSIONS", C_SES);
        fmt_int(n, ui.ses_ix);
        CV_AREA(0, 60, 240, 112, C_BLACK)
            cv_text_c(0, &FONT_XL, n, has ? C_WHITE : C_GREY);
        CV_AREA(0, 172, 240, 68, C_BLACK)
            cv_text_c(8, &FONT_M, has ? (ses_current() == ui.ses_ix ? "LOADED" : "SAVED") : "EMPTY", has ? C_SES : C_GREY);
        return;
    }
    CV_AREA(0, 0, 240, 60, C_BLACK)
        cv_text_c(4, &FONT_L, "SAVE AS", C_SES);
    fmt_int(n, ui.ses_new);
    CV_AREA(0, 60, 240, 100, C_BLACK)
        cv_text_c(0, &FONT_XL, n, C_WHITE);
    cv_begin(240, 80, C_BLACK);
    draw_noyes(ui.ses_yes);
    cv_blit(0, 160);
}

/* the CPU page (SHIFT + black key 7 held): the load, its peak, the channels' share and the output's */
static struct { uint32_t avg, t, shown; uint8_t win[4], wi; } cpu;
static void draw_cpu_page(void)
{
    uint32_t now = fm1_ms, pk = load_pk, avg, i;
    uint16_t col;
    char t[24];
    if ((uint32_t)(now - cpu.t) >= 500u) {
        cpu.t = now;
        cpu.win[cpu.wi++ & 3u] = load_pk;
        load_pk = 0;
    }
    cpu.avg = (cpu.avg * 7u + (uint32_t)load_pct * 256u) / 8u;
    for (i = 0; i < 4u; i++)
        pk = cpu.win[i] > pk ? cpu.win[i] : pk;
    if (!ui.force && (uint32_t)(now - cpu.shown) < 250u)
        return;
    cpu.shown = now;
    avg = (cpu.avg + 128u) >> 8;
    avg = avg > 999u ? 999u : avg;
    pk = pk < avg ? avg : pk > 999u ? 999u : pk;
    col = pk < 60u ? C_OK : pk < 85u ? RGB(255, 200, 0) : C_BAD;
    cv_begin(240, 50, C_BLACK);
    str_cpy(t, "CPU ", 5);
    fmt_int(t + 4, (int32_t)avg);
    str_cpy(t + str_len(t), "%", 2);
    cv_text_fit(2, &FONT_L, t, col);
    cv_blit(0, 0);
    cv_begin(240, 50, C_BLACK);
    cv_rect(20, 6, 200, 12, RGB(50, 50, 50));
    cv_rect(20, 6, (int32_t)(avg > 100u ? 100u : avg) * 2, 12, col);
    cv_rect(19 + (int32_t)(pk > 100u ? 100u : pk) * 2, 4, 3, 16, C_WHITE);
    str_cpy(t, "PEAK ", 6);
    fmt_int(t + 5, (int32_t)pk);
    cv_text_c(26, &FONT_XS, t, RGB(175, 175, 175));
    cv_blit(0, 50);
    CV_AREA(0, 100, 240, 140, C_BLACK) {
        str_cpy(t, "CHANNELS ", 10);
        fmt_int(t + 9, (int32_t)(mx_ticks * 100u / BLK_TICKS));
        str_cpy(t + str_len(t), "%", 2);
        cv_text(6, 10, &FONT_S, t, C_WHITE);
        str_cpy(t, "OUTPUT ", 8);
        if (audio_worker_online)
            str_cpy(t + 7, "CPU1", 5);
        else {
            fmt_int(t + 7, (int32_t)(out_ticks * 100u / BLK_TICKS));
            str_cpy(t + str_len(t), "%", 2);
        }
        cv_text(6, 60, &FONT_S, t, C_WHITE);
    }
}

/* the SNAPSHOTS page: the sixteen as the white keys lie, four by four -- stored ones filled, the last recalled
 * framed white; the morph time under them */
static void draw_snap_page(void)
{
    int32_t key = snap_used | (snap_cur + 1) << 16 | morph_hs << 21 | (ui.snap_key >= 0) << 28, r, c;
    char t[16];
    if (!ui.force && ui.shown_page == PG_SNAP && key == ui.shown_snap)
        return;
    ui.shown_snap = key;
    cv_begin(240, TITLE_H, C_BLACK);
    text_fit_c(0, TITLE_H - 2, "SNAPSHOTS", &FONT_M, RGB(120, 220, 255));
    cv_rect(0, TITLE_H - 1, 240, 1, RGB(60, 60, 60));
    cv_blit(0, 0);
    for (r = 0; r < 4; r++) {
        cv_begin(240, 40, C_BLACK);
        for (c = 0; c < 4; c++) {
            int32_t i = r * 4 + c, x = 4 + c * 59, has = snap_has((uint32_t)i);
            char n[4];
            fmt_int(n, i + 1);
            if (i == snap_cur)
                cv_frame(x, 1, 55, 38, 3, C_WHITE);
            if (has)
                cv_rect(x + 3, 4, 49, 32, RGB(120, 220, 255));
            cv_bg_set(has ? RGB(120, 220, 255) : C_BLACK);
            cv_text(x + (55 - text_w(&FONT_S, n)) / 2, (40 - (int32_t)FONT_S.h) / 2, &FONT_S, n, has ? C_BLACK : C_GREY);
            cv_bg_set(C_BLACK);
        }
        cv_blit(0, (uint32_t)(TITLE_H + r * 40));
    }
    cv_begin(240, 240 - TITLE_H - 160, C_BLACK);
    if (morph_hs) {
        str_cpy(t, "MORPH ", sizeof t);
        fmt_int(t + 6, morph_hs / 2);
        if (morph_hs & 1)
            str_cpy(t + str_len(t), ".5", 3);
        str_cpy(t + str_len(t), " S", 3);
    } else
        str_cpy(t, "MORPH OFF", sizeof t);
    text_fit_c(0, 240 - TITLE_H - 160, t, &FONT_S, C_WHITE);
    cv_blit(0, TITLE_H + 160);
}

static void ui_draw(void)
{
    int msg = (int32_t)(ui.msg_until - fm1_ms) > 0;
    int32_t px;
    if (draw_flash())
        return;
    if (ch_out(ui.sel) != ui.shown_dead) {       /* (the channel in the patch, or out: its colours, or grey) */
        ui.shown_dead = (uint8_t)ch_out(ui.sel);
        ui.force = 1;
    }
    if (ui.cpu_page) {
        if (ui.shown_page != 0xF0)
            ui.force = 1;
        draw_cpu_page();
        ui.shown_page = 0xF0;
        ui.force = 0;
        ui.cpu_was = 1;
        return;
    }
    if (ui.cpu_was) {
        ui.cpu_was = 0;
        ui.shown_page = 0xFF;
        ui.force = 1;
    }
    if (ui.set_page) {
        if (ui.shown_page != PG_SET)
            ui.force = 1;
        setup_draw();
        ui.shown_page = PG_SET;
        ui.force = 0;
        return;
    }
    if (ui.page == PG_VIEW) {
        draw_view();
        ui.shown_page = PG_VIEW;
        ui.force = 0;
        return;
    }
    if (ui.page == PG_SNAP) {
        draw_snap_page();
        ui.shown_page = PG_SNAP;
        ui.force = 0;
        return;
    }
    if (ui.page == PG_SES || ui.page == PG_ASK) {
        draw_list_page();
        ui.shown_page = ui.page;
        ui.force = 0;
        return;
    }
    if (ui.page == PG_MTX1 || ui.page == PG_MTX2) {
        draw_mtx_page();
        ui.shown_page = ui.page;
        ui.shown_sel = ui.sel;
        ui.force = 0;
        return;
    }
    if (ui.page != PG_MAIN) {
        if (rhy_page() && ui.shown_page == ui.page && (int32_t)ui.shown_snap != par.rhy.on * 256 + par.rhy.style) {
            ui.shown_snap = par.rhy.on * 256 + par.rhy.style;   /* (the style or OFF changed: the title anew) */
            ui.force = 1;
        }
        draw_page();
        ui.shown_page = ui.page;
        ui.shown_sel = ui.sel;
        ui.force = 0;
        return;
    }
    if ((int32_t)(ui.dial_until - fm1_ms) > 0) { /* a knob turning on the main view: its dial */
        if (ui.shown_page != 0xF1 || (ui.shown_dial != p_get(ui.dial) + (int32_t)ui.dial * 4096 &&
                                      (uint32_t)(fm1_ms - ui.dial_drawn_at) >= 40u))
            draw_dial();
        ui.shown_page = 0xF1;
        return;
    }
    if (ui.shown_page != PG_MAIN)
        ui.force = 1;
    ui.shown_page = PG_MAIN;
    if (ui.force || ui.shown_sel != ui.sel) {
        draw_number();
        ui.shown_sel = ui.sel;
        ui.shown_src = ui.shown_msg = ui.shown_pan = ui.shown_level = -100;
    }
    if (ui.shown_src != par.ch[ui.sel].src) {
        draw_src();
        ui.shown_src = par.ch[ui.sel].src;
    }
    {
        char rs[16];
        uint32_t st = gl_state(ui.sel);
        int rec = st != GS_IDLE;
        int kind = msg ? 1 : rec || par.out.mute || par.ch[ui.sel].mute ? 2 : 0;
        const char *t = msg ? ui.msg : rec ? rs : par.out.mute ? "OUT MUTED" : par.ch[ui.sel].mute ? "MUTED" : ui.browse ? FAM[ui.recipe].name : ui.patch;
        if (st == GS_ARMED)
            str_cpy(rs, (fm1_ms / 250u) & 1u ? "ARMED" : "", sizeof rs);
        else if (st == GS_REC) {                 /* REC and the seconds so far */
            str_cpy(rs, "REC ", sizeof rs);
            fmt_int(rs + 4, (int32_t)((fm1_ms - gl[ui.sel].t0) / 1000u));
            str_cpy(rs + str_len(rs), " S", 3);
        }
        if (ui.shown_msg != kind || !str_eq(t, ui.shown_text)) {
            draw_band_c(t, kind);
            str_cpy(ui.shown_text, t, sizeof ui.shown_text);
            ui.shown_msg = kind;
        }
    }
    px = meter_px(ch_meter[ui.sel]);
    if (ui.shown_level != par.ch[ui.sel].level || ((uint32_t)(fm1_ms - ui.meter_at) >= 30u && px != ui.shown_meter)) {
        draw_meter(px);
        ui.meter_at = fm1_ms;
    }
    if (ui.shown_pan != par.ch[ui.sel].pan)
        draw_pan();
    ui.force = 0;
}

static void ui_init(void)
{
    static lil_par_t q;
    memset(&ui, 0, sizeof ui);
    ui.touched = -1;
    ui.snap_key = -1;
    ui.dkey = -1;
#if TELEMETRY
    ui.seed = 0x5EEDu;                           /* (a diagnostic build: the same patch each start) */
#else
    ui.seed = boot_seed ^ fm1_ticks() ^ 0x5EEDu; /* (boot_seed: the ADCs' noise, main.c) */
#endif
    {                                            /* a patch drawn from any recipe, otherwise each start; its tempo
                                                  * drawn too, 91 .. 299 BPM */
        uint32_t f = patch_draw(&q, next_seed(), (int)(next_seed() % NFAM));
        q.rhy.bpm = (uint16_t)(91u + next_seed() % 209u);
        ui.fam = (uint8_t)f;
        ui.recipe = (uint8_t)f;
        patch_set(&q, FAM[f].name);
    }
    drm_init(next_seed());                       /* the DRUMS: a kit and a rhythm drawn, stopped */
    cmp_coef();                                  /* (the COMPRESSOR: as it stands -- off) */
    ui.shown_sel = 0xFF;
    ui.force = 1;
}
