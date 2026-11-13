/* SPDX-License-Identifier: GPL-3.0-only
 * LILLIPUT: playing over time -- the GESTURE loop and the SNAPSHOTS.
 *
 * GESTURES. A loop of what the hands do, one for each channel, each on its own: every change to the channel -- its
 * knobs, its input, its mute, its effect slot, the channel thrown at random, a MIDI CC to it -- recorded as it happens,
 * then played back round and round. The keys act on the channel chosen:
 *   REC          arms its loop (the loop it had stops); the first change to the channel starts the recording, and
 *                replaces the loop it had (no overdubs: a channel has one loop at a time)
 *   REC again    ends it: the loop is as long as from that first change to now, and plays at once
 *   REC held 1 s empties the channel's loop
 *   PLAY         the channel's loop played, or stopped (stopped, the channel stays where the loop left it)
 * One channel records at a time: REC on another ends the recording (it plays) and arms that one. The loops play
 * together, each its own length. Each round begins from the channel as it was when its recording began (only what
 * the loop touches), so it plays the same way every time. The loops watch the mixer itself (its parameters, a 16-bit
 * word at a time, against what they were the pass before): whatever moves them is caught, and what a loop plays back
 * is not recorded again. The output's, the RHYTHM's and the output slot's settings belong to no channel and are
 * never recorded. A new patch (HOME, a preset) leaves the loops playing, over it (build 24). PLAY's old job, the
 * output's mute, is SHIFT + PLAY.
 *
 * SNAPSHOTS. Sixteen states of the whole machine -- the mixer, the DRUMS, the COMPRESSOR (build 27) -- kept with the
 * session. SAVE (short) opens the SNAPSHOTS page: the white keys are the sixteen snapshots -- a key let go soon
 * recalls its snapshot, held 0.8 s stores the machine in it, SHIFT + it empties it. SAVE again, or a channel's
 * button, back. (Tonitor's, after BIGORNA. Build 27: no MORPH -- a recall is at once.) */

#define GEV_MAX 1664u            /* the loops' changes, all channels' together (8 bytes each: 13 KiB; build 27: was
                                  * 2048 -- the room for the snapshots' DRUMS) */
#define NWORD ((uint32_t)sizeof(lil_par_t) / 2u)
typedef char lil_par_is_words[sizeof(lil_par_t) % 2u == 0u ? 1 : -1];
typedef char ch_par_is_words[sizeof(ch_par_t) % 2u == 0u && sizeof(slot_par_t) % 2u == 0u ? 1 : -1];
/* a channel's own words: its ch_par_t, then its slot's */
#define CH_W0 ((uint32_t)__builtin_offsetof(lil_par_t, ch) / 2u)
#define CHW ((uint32_t)sizeof(ch_par_t) / 2u)
#define SL_W0 ((uint32_t)__builtin_offsetof(lil_par_t, slot) / 2u)
#define SLW ((uint32_t)sizeof(slot_par_t) / 2u)
#define LW (CHW + SLW)
typedef struct { uint32_t t; uint16_t w, v; } gev_t;
static gev_t gev[GEV_MAX];       /* each channel's changes together, at gl[c].off */
enum { GS_IDLE, GS_ARMED, GS_REC };
typedef struct {
    uint8_t play, was_play;
    uint16_t off, n;             /* its changes in gev */
    uint32_t len;                /* the loop's length, ms (0: none) */
    uint32_t t0;                 /* the recording's start; the playing's */
    uint32_t cyc, pos;           /* the round being played; the next change in it */
} gloop_t;
static gloop_t gl[NCH];
static int8_t g_rec = -1;        /* the channel armed or recording (-1: none) */
static uint8_t g_st;             /* .. GS_ARMED or GS_REC */
static uint16_t g_used;          /* changes in gev, all loops' */
static lil_par_t g_prev;         /* the mixer as the last pass left it */
static uint16_t g_s0[NCH][LW];   /* each channel as it was when its recording began */
static uint8_t g_touched[NCH][(LW + 7u) / 8u];

/* the channel a word of the mixer belongs to (-1: none), and its place among the channel's */
static int g_owner(uint32_t w, uint32_t *li)
{
    if (w >= CH_W0 && w < CH_W0 + NCH * CHW) {
        *li = (w - CH_W0) % CHW;
        return (int)((w - CH_W0) / CHW);
    }
    if (w >= SL_W0 && w < SL_W0 + NCH * SLW) {
        *li = CHW + (w - SL_W0) % SLW;
        return (int)((w - SL_W0) / SLW);
    }
    return -1;
}
static uint32_t g_word(uint32_t c, uint32_t li) { return li < CHW ? CH_W0 + c * CHW + li : SL_W0 + c * SLW + li - CHW; }
static uint32_t gl_state(uint32_t c) { return g_rec == (int)c ? g_st : GS_IDLE; }

static void gest_sync(void) { g_prev = par; }    /* (the mixer replaced: no change to record) */
static void gest_halt(void)                      /* a new patch (build 24): the loops are the hands', not the patch's
                                                  * -- each plays on over the new patch, the same moves, so they may
                                                  * do something else there; one armed stays armed, a recording goes
                                                  * on. The patch's own change is never recorded */
{
    gest_sync();
}
static void gest_apply(uint32_t w, uint32_t v)
{
    ((uint16_t *)&par)[w] = (uint16_t)v;
    ((uint16_t *)&g_prev)[w] = (uint16_t)v;
}
static void gest_play_from_start(uint32_t c)
{
    gl[c].play = 1;
    gl[c].t0 = fm1_ms;
    gl[c].cyc = 0xFFFFFFFFu;                     /* (the first pass begins a round: the start restored) */
    gl[c].pos = 0;
}
static void gl_touch(uint32_t c)                 /* the words its loop touches */
{
    uint32_t i, li;
    memset(g_touched[c], 0, sizeof g_touched[c]);
    for (i = 0; i < gl[c].n; i++)
        if (g_owner(gev[gl[c].off + i].w, &li) == (int)c)
            g_touched[c][li >> 3] |= (uint8_t)(1u << (li & 7u));
}
static void gl_drop(uint32_t c)                  /* a channel's loop gone; the others' changes closed up behind it */
{
    uint32_t k, off = gl[c].off, n = gl[c].n;
    if (n) {
        for (k = off; k + n < g_used; k++)      /* (down: a forward copy is safe) */
            gev[k] = gev[k + n];
        for (k = 0; k < NCH; k++)
            if (k != c && gl[k].off > off)
                gl[k].off = (uint16_t)(gl[k].off - n);
        g_used = (uint16_t)(g_used - n);
    }
    gl[c].off = g_used;
    gl[c].n = 0;
    gl[c].len = 0;
    gl[c].play = 0;
}
static void gest_stop_rec(void)
{
    uint32_t c = (uint32_t)g_rec;
    g_rec = -1;
    gl[c].len = fm1_ms - gl[c].t0;
    if (gl[c].len < 200u)
        gl[c].len = 200u;
    gl_touch(c);
    gest_play_from_start(c);
}
static void gest_unarm(void)                     /* (armed, not begun: the loop it had as it was) */
{
    uint32_t c = (uint32_t)g_rec;
    g_rec = -1;
    if (gl[c].was_play && gl[c].n)
        gest_play_from_start(c);
}
static void gest_rec_press(void)                 /* REC: the chosen channel's */
{
    uint32_t c = ui.sel;
    if (g_rec == (int)c) {
        if (g_st == GS_REC) {
            gest_stop_rec();
            say("PLAYING", 1000);
        } else {
            gest_unarm();
            say("NOT ARMED", 900);
        }
        return;
    }
    if (g_rec >= 0) {                            /* (another channel's: ended, or let go) */
        if (g_st == GS_REC)
            gest_stop_rec();
        else
            gest_unarm();
    }
    gl[c].was_play = gl[c].play;
    gl[c].play = 0;
    g_rec = (int8_t)c;
    g_st = GS_ARMED;
    gest_sync();
}
static void gest_clear(void)                     /* REC held: the chosen channel's loop emptied */
{
    if (g_rec == (int)ui.sel)
        g_rec = -1;
    gl_drop(ui.sel);
    flash("GESTURES", "EMPTIED", RGB(255, 60, 60));
}
static void gest_play_press(void)                /* PLAY: the chosen channel's */
{
    gloop_t *g = &gl[ui.sel];
    if (!g->n || gl_state(ui.sel) == GS_REC) {
        say(gl_state(ui.sel) == GS_REC ? "RECORDING" : "NO GESTURES", 900);
        return;
    }
    if (g->play) {
        g->play = 0;
        say("STOPPED", 800);
    } else {
        gest_play_from_start(ui.sel);
        say("PLAYING", 800);
    }
}
/* each pass of the main loop: the loops played up to now, then what changed since the last pass, recorded */
static void gest_poll(void)
{
    uint16_t *cur = (uint16_t *)&par, *prv = (uint16_t *)&g_prev;
    uint32_t w, c, li, now = fm1_ms;
    for (c = 0; c < NCH; c++) {
        gloop_t *g = &gl[c];
        const gev_t *e = &gev[g->off];
        uint32_t el, cyc, at;
        if (!g->play || !g->len || !g->n)
            continue;
        el = now - g->t0;
        cyc = el / g->len;
        at = el - cyc * g->len;
        if (cyc != g->cyc) {                     /* a new round: what the loop touches as it was when it began */
            if (g->cyc != 0xFFFFFFFFu)
                for (; g->pos < g->n; g->pos++)  /* (the last round's end, not missed) */
                    gest_apply(e[g->pos].w, e[g->pos].v);
            for (li = 0; li < LW; li++)
                if (g_touched[c][li >> 3] >> (li & 7u) & 1u)
                    gest_apply(g_word(c, li), g_s0[c][li]);
            g->cyc = cyc;
            g->pos = 0;
        }
        for (; g->pos < g->n && e[g->pos].t <= at; g->pos++)
            gest_apply(e[g->pos].w, e[g->pos].v);
    }
    if (g_rec >= 0 && g_st == GS_ARMED) {        /* the first change to the channel starts the recording */
        for (w = 0; w < NWORD; w++)
            if (cur[w] != prv[w] && g_owner(w, &li) == g_rec)
                break;
        if (w < NWORD) {
            c = (uint32_t)g_rec;
            gl_drop(c);                          /* (its old loop: replaced) */
            g_st = GS_REC;
            gl[c].t0 = now;
            for (li = 0; li < LW; li++)
                g_s0[c][li] = prv[g_word(c, li)];
        }
    }
    for (w = 0; w < NWORD; w++) {
        if (cur[w] == prv[w])
            continue;
        if (g_rec >= 0 && g_st == GS_REC && g_owner(w, &li) == g_rec) {
            gloop_t *g = &gl[g_rec];
            if (g_used < GEV_MAX) {              /* (the loop recording is the last in gev) */
                gev[g_used].t = now - g->t0;
                gev[g_used].w = (uint16_t)w;
                gev[g_used].v = cur[w];
                g_used++;
                g->n++;
            } else {                             /* (full: the loop ends here) */
                prv[w] = cur[w];
                gest_stop_rec();
                say("LOOP FULL", 1200);
                continue;
            }
        }
        prv[w] = cur[w];
    }
    if (g_rec >= 0 && g_st == GS_REC && now - gl[g_rec].t0 > 600000u) {   /* (ten minutes at most) */
        gest_stop_rec();
        say("LOOP FULL", 1200);
    }
}

/* -------------------------------------------------------------- snapshots --- */
#define NSNAP 16u
static lil_par_t snap[NSNAP];
/* (build 27) the rest of the machine with each: the DRUMS -- their voices, their rhythm as drawn and its DENSITY,
 * SWING, VARIATION, mutes, on or off -- and the COMPRESSOR. A session is not in a snapshot. (No per-step pitch: the
 * drums have none since build 22.) */
typedef struct {
    uint32_t hits, acc, alt;     /* the rhythm as drawn (DENSITY 25) */
    uint8_t len, div, prob, var;
} snap_lane_t;
typedef struct {
    dvoice_t v[NDV];
    snap_lane_t l[NDV];
    uint8_t on, style, swing, mute;
    uint8_t kit[NDV];
    int16_t level, vary;
    uint32_t dseed;              /* DENSITY's orders */
    uint8_t dens, rel_auto, rsv[2];
    cmp_par_t cmp;
} snap_drm_t;
static snap_drm_t snap_d[NSNAP];
static uint16_t snap_used;       /* a bit a snapshot */
static uint16_t snap_dused;      /* .. and its DRUMS and COMPRESSOR (not in a session from before build 27) */
static int8_t snap_cur = -1;     /* the last recalled or stored */
static int snap_has(uint32_t i) { return i < NSNAP && (snap_used >> i & 1u); }
static void snap_drm_store(snap_drm_t *d)
{
    uint32_t c;
    memset(d, 0, sizeof *d);
    for (c = 0; c < NDV; c++) {
        d->v[c] = drm.v[c];
        d->l[c].hits = dbase[c].hits;
        d->l[c].acc = dbase[c].acc;
        d->l[c].alt = dbase[c].alt;
        d->l[c].len = drm.l[c].len;
        d->l[c].div = drm.l[c].div;
        d->l[c].prob = drm.l[c].prob;
        d->l[c].var = drm.l[c].var;
        d->kit[c] = drm.kit[c];
    }
    d->on = drm.on;
    d->style = drm.style;
    d->swing = drm.swing;
    d->mute = drm.mute;
    d->level = drm.level;
    d->vary = drm.vary;
    d->dseed = dens_seed;
    d->dens = dens;
    d->rel_auto = cmp_rel_auto;
    d->cmp = cmp;
}
static void snap_drm_recall(const snap_drm_t *d)
{
    uint32_t c;
    for (c = 0; c < NDV; c++) {
        dlane_t *L = &drm.l[c];
        drm.v[c] = d->v[c];
        memset(L, 0, sizeof *L);
        L->hits = dbase[c].hits = d->l[c].hits;
        L->acc = dbase[c].acc = d->l[c].acc;
        L->alt = dbase[c].alt = d->l[c].alt;
        L->len = d->l[c].len;
        L->div = d->l[c].div;
        L->prob = d->l[c].prob;
        L->var = d->l[c].var;
        drm.kit[c] = d->kit[c];
    }
    drm.style = d->style;
    drm.swing = d->swing;
    drm.mute = d->mute;
    drm.level = d->level;
    drm.vary = d->vary;
    dens_seed = d->dseed;
    dens = d->dens <= DENS_MAX ? d->dens : DENS_MID;
    dens_apply();                                /* (the rhythm at its DENSITY: VARIATION takes it as home) */
    drm_coef_all();
    if (d->on && !drm.on)
        drm_reset = 1;                           /* (started: from the top, on the beat) */
    drm.on = d->on;
    cmp = d->cmp;
    cmp_rel_auto = d->rel_auto;
    cmp_coef();
}
static void snap_recall(uint32_t i)
{
    if (!snap_has(i)) {
        say("EMPTY", 800);
        return;
    }
    snap_cur = (int8_t)i;
    par = snap[i];
    ch_fresh_all();
    if (snap_dused >> i & 1u)
        snap_drm_recall(&snap_d[i]);
    say("RECALLED", 800);
    ui.force = 1;
}
static void snap_store(uint32_t i)
{
    char n[4];
    snap[i] = par;
    snap_drm_store(&snap_d[i]);
    snap_used |= (uint16_t)(1u << i);
    snap_dused |= (uint16_t)(1u << i);
    snap_cur = (int8_t)i;
    fmt_int(n, (int32_t)i + 1);
    flash("STORED", n, RGB(120, 220, 255));
}
static void snap_clear(uint32_t i)
{
    snap_used &= (uint16_t)~(1u << i);
    snap_dused &= (uint16_t)~(1u << i);
    if (snap_cur == (int8_t)i)
        snap_cur = -1;
    say("EMPTIED", 800);
}
