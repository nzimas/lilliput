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
 * never recorded. PLAY's old job, the output's mute, is SHIFT + PLAY.
 *
 * SNAPSHOTS. Sixteen states of the whole mixer, kept with the session. SAVE (short) opens the SNAPSHOTS page: the
 * white keys are the sixteen snapshots -- a key let go soon recalls its snapshot, held 0.8 s stores the mixer in it,
 * SHIFT + it empties it. KNOB 1 there: the MORPH time, 0 .. 30 s: a recall glides every value that can glide (the
 * levels, gains, filters, sends, limits ...) from where it is to the snapshot's; the rest (inputs, types, routings)
 * changes at once. SAVE again, or a channel's button, back. (Tonitor's, after BIGORNA.) */

#define GEV_MAX 2048u            /* the loops' changes, all channels' together (8 bytes each: 16 KiB) */
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
static void gest_halt(void)                      /* (a new patch: the loops stop, a recording goes on) */
{
    uint32_t c;
    for (c = 0; c < NCH; c++)
        if (gl_state(c) != GS_REC)
            gl[c].play = 0;
    if (g_rec >= 0 && g_st == GS_ARMED)
        g_rec = -1;
    if (g_rec < 0)
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
static uint16_t snap_used;       /* a bit a snapshot */
static int8_t snap_cur = -1;     /* the last recalled or stored */
static uint8_t morph_hs;         /* the morph time, half seconds 0 .. 60 */
static struct { uint8_t on; int8_t to; uint32_t t0, dur; lil_par_t from; } mo;
/* the values that glide: a channel's, a slot's, the output's */
static const uint8_t MORPH_CH[] = {
    __builtin_offsetof(ch_par_t, gain), __builtin_offsetof(ch_par_t, noise), __builtin_offsetof(ch_par_t, over),
    __builtin_offsetof(ch_par_t, lim), __builtin_offsetof(ch_par_t, high), __builtin_offsetof(ch_par_t, low),
    __builtin_offsetof(ch_par_t, mgain), __builtin_offsetof(ch_par_t, mfreq), __builtin_offsetof(ch_par_t, level),
    __builtin_offsetof(ch_par_t, pan), __builtin_offsetof(ch_par_t, send1), __builtin_offsetof(ch_par_t, send2)};
static const uint8_t MORPH_SL[] = {__builtin_offsetof(slot_par_t, p1), __builtin_offsetof(slot_par_t, p2),
                                   __builtin_offsetof(slot_par_t, pres)};
static const uint8_t MORPH_OUT[] = {__builtin_offsetof(out_par_t, ret1), __builtin_offsetof(out_par_t, ret2),
                                    __builtin_offsetof(out_par_t, high), __builtin_offsetof(out_par_t, low),
                                    __builtin_offsetof(out_par_t, mgain), __builtin_offsetof(out_par_t, mfreq),
                                    __builtin_offsetof(out_par_t, spres), __builtin_offsetof(out_par_t, level)};
static int snap_has(uint32_t i) { return i < NSNAP && (snap_used >> i & 1u); }
static void morph_step(uint32_t num, uint32_t den)   /* the gliding values, num / den of the way */
{
    uint32_t c, k;
#define MG(base_to, base_from, off) do { \
        int16_t *d_ = (int16_t *)((uint8_t *)(base_to) + (off)); \
        int32_t a_ = *(const int16_t *)((const uint8_t *)(base_from) + (off)), b_ = *(const int16_t *)((const uint8_t *)&snap[mo.to] + ((const uint8_t *)(d_) - (const uint8_t *)&par)); \
        *d_ = (int16_t)(a_ + (b_ - a_) * (int32_t)num / (int32_t)den); } while (0)
    for (c = 0; c < NCH; c++)
        for (k = 0; k < sizeof MORPH_CH; k++)
            MG(&par.ch[c], &mo.from.ch[c], MORPH_CH[k]);
    for (c = 0; c < NSLOT; c++)
        for (k = 0; k < sizeof MORPH_SL; k++)
            MG(&par.slot[c], &mo.from.slot[c], MORPH_SL[k]);
    for (k = 0; k < sizeof MORPH_OUT; k++)
        MG(&par.out, &mo.from.out, MORPH_OUT[k]);
#undef MG
}
static void snap_recall(uint32_t i)
{
    char n[4];
    if (!snap_has(i)) {
        say("EMPTY", 800);
        return;
    }
    snap_cur = (int8_t)i;
    fmt_int(n, (int32_t)i + 1);
    if (!morph_hs) {
        mo.on = 0;
        par = snap[i];
        ch_fresh_all();
        say("RECALLED", 800);
    } else {                                     /* the rest at once, the gliding values from where they are */
        mo.from = par;
        mo.to = (int8_t)i;
        mo.t0 = fm1_ms;
        mo.dur = morph_hs * 500u;
        mo.on = 1;
        par = snap[i];
        ch_fresh_all();
        morph_step(0, 1);
        say("MORPHING", 900);
    }
    ui.force = 1;
}
static void morph_poll(void)
{
    uint32_t el;
    if (!mo.on)
        return;
    el = fm1_ms - mo.t0;
    if (el >= mo.dur) {
        morph_step(1, 1);
        mo.on = 0;
        return;
    }
    morph_step(el, mo.dur);
}
static void snap_store(uint32_t i)
{
    char n[4];
    snap[i] = par;
    snap_used |= (uint16_t)(1u << i);
    snap_cur = (int8_t)i;
    fmt_int(n, (int32_t)i + 1);
    flash("STORED", n, RGB(120, 220, 255));
}
static void snap_clear(uint32_t i)
{
    snap_used &= (uint16_t)~(1u << i);
    if (snap_cur == (int8_t)i)
        snap_cur = -1;
    say("EMPTIED", 800);
}
