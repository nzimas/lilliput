/* SPDX-License-Identifier: GPL-3.0-only
 * LILLIPUT: playing over time -- the GESTURE loop and the SNAPSHOTS.
 *
 * GESTURES. A loop of what the hands do: every change to the mixer -- a knob, an input key, a mute, a new patch, a
 * channel thrown at random, a MIDI CC -- on any channel, recorded as it happens, then played back round and round.
 *   REC          arms it (a loop playing stops); the first change starts the recording
 *   REC again    ends it: the loop is as long as from that first change to now, and plays at once
 *   REC held 1 s empties it
 *   PLAY         the loop played, or stopped (stopped, everything stays where the loop left it)
 * Each round begins from the mixer as it was when the recording began (only what the loop touches), so it plays
 * the same way every time. The loop watches the mixer itself (its parameters, a 16-bit word at a time, against
 * what they were the pass before): whatever moves them is caught, and what the loop plays back is not recorded
 * again. PLAY's old job, the output's mute, is SHIFT + PLAY now; the mute is never recorded.
 *
 * SNAPSHOTS. Sixteen states of the whole mixer, kept with the session. SAVE (short) opens the SNAPSHOTS page: the
 * white keys are the sixteen snapshots -- a key let go soon recalls its snapshot, held 0.8 s stores the mixer in it,
 * SHIFT + it empties it. KNOB 1 there: the MORPH time, 0 .. 30 s: a recall glides every value that can glide (the
 * levels, gains, filters, sends, limits ...) from where it is to the snapshot's; the rest (inputs, types, routings)
 * changes at once. SAVE again, or a channel's button, back. (Tonitor's, after BIGORNA.) */

#define GEV_MAX 2048u            /* the loop's changes (8 bytes each: 16 KiB) */
#define NWORD ((uint32_t)sizeof(lil_par_t) / 2u)
typedef char lil_par_is_words[sizeof(lil_par_t) % 2u == 0u ? 1 : -1];
typedef struct { uint32_t t; uint16_t w, v; } gev_t;
static gev_t gev[GEV_MAX];
enum { GS_IDLE, GS_ARMED, GS_REC };
static struct {
    uint8_t st, play, was_play;
    uint16_t n;                  /* changes recorded */
    uint32_t len;                /* the loop's length, ms */
    uint32_t t0;                 /* the recording's start; the playing's */
    uint32_t cyc, pos;           /* the round being played; the next change in it */
} gs;
static lil_par_t g_prev;         /* the mixer as the last pass left it */
static lil_par_t g_s0;           /* .. as it was when the recording began */
static uint8_t g_touched[(NWORD + 7u) / 8u];
#define W_MUTE_OUT ((uint32_t)__builtin_offsetof(lil_par_t, out.mute) / 2u)   /* (the output's mute: never recorded) */

static void gest_sync(void) { g_prev = par; }    /* (the mixer replaced: no change to record, the loop stops) */
static void gest_halt(void)
{
    if (gs.st != GS_REC)
        gs.play = 0;
    if (gs.st == GS_ARMED)
        gs.st = GS_IDLE;
    if (gs.st != GS_REC)
        gest_sync();
}
static void gest_apply(uint32_t w, uint32_t v)
{
    ((uint16_t *)&par)[w] = (uint16_t)v;
    ((uint16_t *)&g_prev)[w] = (uint16_t)v;
}
static void gest_play_from_start(void)
{
    gs.play = 1;
    gs.t0 = fm1_ms;
    gs.cyc = 0xFFFFFFFFu;                        /* (the first pass begins a round: the start restored) */
    gs.pos = 0;
}
static void gest_stop_rec(void)
{
    uint32_t i;
    gs.st = GS_IDLE;
    gs.len = fm1_ms - gs.t0;
    if (gs.len < 200u)
        gs.len = 200u;
    memset(g_touched, 0, sizeof g_touched);
    for (i = 0; i < gs.n; i++)
        g_touched[gev[i].w >> 3] |= (uint8_t)(1u << (gev[i].w & 7u));
    gest_play_from_start();
}
static void gest_rec_press(void)                  /* REC */
{
    if (gs.st == GS_REC) {
        gest_stop_rec();
        say("PLAYING", 1000);
    } else if (gs.st == GS_ARMED) {
        gs.st = GS_IDLE;
        gs.play = gs.was_play;
        if (gs.play)
            gest_play_from_start();
        say("NOT ARMED", 900);
    } else {
        gs.was_play = gs.play;
        gs.play = 0;
        gs.st = GS_ARMED;
        gest_sync();
    }
}
static void gest_clear(void)                     /* REC held */
{
    gs.st = GS_IDLE;
    gs.play = 0;
    gs.n = 0;
    gs.len = 0;
    flash("GESTURES", "EMPTIED", RGB(255, 60, 60));
}
static void gest_play_press(void)                /* PLAY */
{
    if (!gs.n || gs.st == GS_REC) {
        say(gs.st == GS_REC ? "RECORDING" : "NO GESTURES", 900);
        return;
    }
    if (gs.play) {
        gs.play = 0;
        say("STOPPED", 800);
    } else {
        gest_play_from_start();
        say("PLAYING", 800);
    }
}
/* each pass of the main loop: the loop played up to now, then what changed since the last pass, recorded */
static void gest_poll(void)
{
    uint16_t *cur = (uint16_t *)&par, *prv = (uint16_t *)&g_prev;
    uint32_t w, now = fm1_ms;
    if (gs.play && gs.len && gs.n) {
        uint32_t el = now - gs.t0, cyc = el / gs.len, at = el - cyc * gs.len;
        if (cyc != gs.cyc) {                     /* a new round: what the loop touches as it was when it began */
            if (gs.cyc != 0xFFFFFFFFu)
                for (; gs.pos < gs.n; gs.pos++)  /* (the last round's end, not missed) */
                    gest_apply(gev[gs.pos].w, gev[gs.pos].v);
            for (w = 0; w < NWORD; w++)
                if (g_touched[w >> 3] >> (w & 7u) & 1u)
                    gest_apply(w, ((const uint16_t *)&g_s0)[w]);
            gs.cyc = cyc;
            gs.pos = 0;
        }
        for (; gs.pos < gs.n && gev[gs.pos].t <= at; gs.pos++)
            gest_apply(gev[gs.pos].w, gev[gs.pos].v);
    }
    if (gs.st == GS_ARMED) {                     /* the first change starts the recording */
        for (w = 0; w < NWORD; w++)
            if (cur[w] != prv[w] && w != W_MUTE_OUT)
                break;
        if (w < NWORD) {
            gs.st = GS_REC;
            gs.t0 = now;
            gs.n = 0;
            g_s0 = g_prev;
        }
    }
    for (w = 0; w < NWORD; w++) {
        if (cur[w] == prv[w])
            continue;
        if (gs.st == GS_REC && w != W_MUTE_OUT) {
            if (gs.n < GEV_MAX) {
                gev[gs.n].t = now - gs.t0;
                gev[gs.n].w = (uint16_t)w;
                gev[gs.n].v = cur[w];
                gs.n++;
            } else {                             /* (full: the loop ends here) */
                prv[w] = cur[w];
                gest_stop_rec();
                say("LOOP FULL", 1200);
                continue;
            }
        }
        prv[w] = cur[w];
    }
    if (gs.st == GS_REC && now - gs.t0 > 600000u) {   /* (ten minutes at most) */
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
