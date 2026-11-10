/* SPDX-License-Identifier: GPL-3.0-only
 * LILLIPUT: each channel's modulation -- Tonitor's matrix (its 0.53), its LFOs as they were, its sources and
 * destinations made the mixer's. Each channel has two LFOs and four slots; a slot takes a source to a destination by
 * an amount, -100 .. +100 of the destination's swing. It moves the channel's parameters as the ISR reads them each
 * block (1378 Hz): the values the panel shows stay where they were set, the sound moves round them.
 *
 * LFO: SHAPE (SINE, TRI, SAW UP, SAW DN, SQUARE, STEPS -- a new random value each cycle --, DRIFT -- smooth random),
 * RATE (0 .. 100: 0.01 Hz .. 20 Hz), MODE (FREE; ONSET: from the start each time the channel's loop bursts into sound;
 * ONCE: one cycle from each onset, then held), FADE (0 .. 100: in over 0 .. 10 s from each onset).
 * Sources: LFO 1, LFO 2, LEVEL (the channel's own loudness: a loop that moves itself as it grows), OUTPUT (the whole
 * mix's loudness), RANDOM (a value drawn at each onset). Tonitor's velocity and key have no notes here to come from.
 * Destinations: GAIN, NOISE, FREQ, MID, HIGH, LOW, LIMIT, PROTECT, LEVEL, PAN, SEND 1, SEND 2, the channel's slot's two
 * parameters and its presence, and either LFO's rate. */

/* (the types are mixer.c's: they are part of a patch) */
static const char *const MS_NAME[MS_N] = {"SINE", "TRI", "SAW UP", "SAW DN", "SQUARE", "STEPS", "DRIFT"};
static const char *const MM_NAME[MM_N] = {"FREE", "ONSET", "ONCE"};
static const char *const MSRC_NAME[MSRC_N] = {"OFF", "LFO 1", "LFO 2", "LEVEL", "OUTPUT", "RANDOM"};
static const char *const MD_NAME[MD_N] = {"GAIN", "NOISE", "FREQ", "MID", "HIGH", "LOW", "LIMIT", "PROTECT", "LEVEL",
                                          "PAN", "SEND 1", "SEND 2", "FX 1", "FX 2", "FX MIX", "LFO 1 RATE",
                                          "LFO 2 RATE"};
/* each destination's whole swing (amount 100), in its own units: half a parameter's range; LEVEL 12 dB; PAN the
 * whole way; an LFO's rate half its range */
static const int16_t MD_SPAN[MD_N] = {500, 500, 500, 500, 500, 500, 500, 500, 120, 50, 500, 500, 500, 500, 500, 50, 50};

static struct {
    uint32_t ph[2];
    int32_t a[2], b[2];                          /* STEPS' and DRIFT's values: the last, the next */
    int32_t fade[2];                             /* each LFO's fade in since the last onset, Q16 0 .. 1 */
    int32_t val[2];                              /* each LFO now, Q15 -1 .. 1 */
    uint8_t held[2];                             /* ONCE: done */
    uint8_t loud;                                /* the channel sounding (its onset: from silent to sounding) */
    uint16_t quiet;                              /* blocks it has been quiet */
    int16_t rnd;
    int16_t off[MD_N];                           /* each destination's offset, its units */
} mst[NCH];
static uint32_t mod_rs = 0x9E3779B9u;
static int32_t mod_rand(void)                    /* -Q15 .. Q15 */
{
    mod_rs = mod_rs * 1664525u + 1013904223u;
    return (int32_t)(mod_rs >> 16) - 32768;
}
static uint32_t mod_inc(int32_t rate)            /* 0.01 Hz x 2000^(rate / 100), a block's phase step */
{
    return (uint32_t)(((uint64_t)31166u * exp2_q16(clampi(rate, 0, 100) * 7189)) >> 16);
}
static int mod_on(const mod_t *m)                /* (any slot in use) */
{
    uint32_t i;
    for (i = 0; i < NMSLOT; i++)
        if (m->s[i].src && m->s[i].amt)
            return 1;
    return 0;
}
/* the block: channel c's onset watched, its LFOs a step on, the offsets worked out */
static void mod_block(uint32_t c)
{
    const mod_t *m = &par.ch[c].mod;
    uint32_t k, i;
    int32_t ro[2] = {mst[c].off[MD_RATE1], mst[c].off[MD_RATE2]};   /* (the LFOs' rates as the last block moved them) */
    int32_t lv = ch_meter[c];
    memset(mst[c].off, 0, sizeof mst[c].off);
    /* an onset: loud after 30 ms or more of quiet */
    if (lv > 1200 && !mst[c].loud) {
        mst[c].loud = 1;
        if (mst[c].quiet >= 40u) {
            mst[c].rnd = (int16_t)mod_rand();
            mst[c].fade[0] = mst[c].fade[1] = 0;
            for (k = 0; k < 2u; k++)
                if (m->lfo[k].mode != MM_FREE) {
                    mst[c].ph[k] = 0;
                    mst[c].held[k] = 0;
                }
        }
        mst[c].quiet = 0;
    } else if (lv < 400) {
        mst[c].loud = 0;
        if (mst[c].quiet < 0xFFFFu)
            mst[c].quiet++;
    }
    if (!mod_on(m))
        return;
    for (k = 0; k < 2u; k++) {
        const mlfo_t *f = &m->lfo[k];
        uint32_t p0 = mst[c].ph[k], inc = mod_inc((int32_t)f->rate + ro[k]), p1 = p0 + inc, t;
        int wrap = p1 < p0;
        int32_t v;
        if (mst[c].held[k])
            p1 = p0;
        else if (wrap && f->mode == MM_ONCE) {
            p1 = 0xFFFFFFFFu;                    /* (one cycle: held at its end) */
            mst[c].held[k] = 1;
        }
        if (wrap && !mst[c].held[k]) {           /* (a new cycle: STEPS' and DRIFT's next value) */
            mst[c].a[k] = mst[c].b[k];
            mst[c].b[k] = mod_rand();
        }
        mst[c].ph[k] = p1;
        t = p1 >> 16;
        switch (f->shape) {
        case MS_TRI:
            v = t < 32768u ? (int32_t)t * 2 - 32768 : 32767 - ((int32_t)t - 32768) * 2;
            break;
        case MS_SAWU:
            v = (int32_t)t - 32768;
            break;
        case MS_SAWD:
            v = 32767 - (int32_t)t;
            break;
        case MS_SQR:
            v = t < 32768u ? 32767 : -32768;
            break;
        case MS_STEPS:
            v = mst[c].b[k];
            break;
        case MS_DRIFT:
            v = mst[c].a[k] + (int32_t)(((int64_t)(mst[c].b[k] - mst[c].a[k]) * (int32_t)t) >> 16);
            break;
        default:
            v = sin_ph(p1);
            break;
        }
        if (mst[c].fade[k] < Q16)                /* (FADE: in over 0.1 s a step, 138 blocks) */
            mst[c].fade[k] += f->fade ? (int32_t)(65536u / (f->fade * 138u) + 1u) : Q16;
        if (mst[c].fade[k] > Q16)
            mst[c].fade[k] = Q16;
        mst[c].val[k] = (int32_t)(((int64_t)v * mst[c].fade[k]) >> 16);
    }
    for (i = 0; i < NMSLOT; i++) {
        const mslot_t *s = &m->s[i];
        int32_t v, o;
        if (!s->src || !s->amt || s->dst >= MD_N)
            continue;
        if (s->src == MSRC_L1 || s->src == MSRC_L2)
            v = mst[c].val[s->src - MSRC_L1];
        else if (s->src == MSRC_LEVEL)           /* (-1 silent .. +1 at the limiter's ceiling) */
            v = clampi(lv * 4 - Q15, -Q15, Q15);
        else if (s->src == MSRC_OUT)
            v = clampi(out_meter * 4 - Q15, -Q15, Q15);
        else
            v = mst[c].rnd;
        o = v * s->amt * MD_SPAN[s->dst] / (Q15 * 100);   /* (32 bits: at most 32768 x 100 x 500) */
        mst[c].off[s->dst] = (int16_t)clampi(mst[c].off[s->dst] + o, -1000, 1000);
    }
}
/* channel c's parameters as the sound has them now: the panel's, moved by the modulation */
static void mod_apply(uint32_t c, ch_par_t *e, slot_par_t *sl)
{
    const int16_t *o = mst[c].off;
#define MV(f, d, lo, hi) do { if (o[d]) (f) = (int16_t)clampi((f) + o[d], lo, hi); } while (0)
    MV(e->gain, MD_GAIN, 0, PMAX);
    if (e->noise)
        MV(e->noise, MD_NOISE, 1, PMAX);
    MV(e->mfreq, MD_FREQ, 0, PMAX);
    MV(e->mgain, MD_MID, 0, PMAX);
    MV(e->high, MD_HIGH, 0, PMAX);
    MV(e->low, MD_LOW, 0, PMAX);
    MV(e->lim, MD_LIM, 0, PMAX);
    MV(e->over, MD_OVER, 0, PMAX);
    if (e->level)
        MV(e->level, MD_LEVEL, 1, LVL_MAX);
    MV(e->pan, MD_PAN, -PAN_MAX, PAN_MAX);
    if (e->send1)
        MV(e->send1, MD_SEND1, 1, PMAX);
    if (e->send2)
        MV(e->send2, MD_SEND2, 1, PMAX);
    if (sl) {
        MV(sl->p1, MD_FX1, 0, PMAX);
        MV(sl->p2, MD_FX2, 0, PMAX);
        MV(sl->pres, MD_FXMIX, 0, PMAX);
    }
#undef MV
}
static void mod_check(mod_t *m)                  /* (as read from a session: each value in its range, or off) */
{
    uint32_t k;
    for (k = 0; k < 2u; k++)
        if (m->lfo[k].shape >= MS_N || m->lfo[k].rate > 100u || m->lfo[k].mode >= MM_N || m->lfo[k].fade > 100u)
            memset(&m->lfo[k], 0, sizeof m->lfo[k]);
    for (k = 0; k < NMSLOT; k++)
        if (m->s[k].src >= MSRC_N || m->s[k].dst >= MD_N || m->s[k].amt < -100 || m->s[k].amt > 100)
            memset(&m->s[k], 0, sizeof m->s[k]);
}

/* SHIFT + SEQ: channel c's modulation thrown anew -- both LFOs and two to four slots, from any source to any
 * destination, by amounts that may go far (Tonitor's) */
static void mod_random(mod_t *mo, uint32_t seed)
{
    static const uint8_t DW[MD_N] = {6, 4, 14, 8, 5, 5, 12, 6, 5, 9, 5, 5, 6, 5, 4, 3, 3};   /* (the destinations' odds) */
    static const uint8_t SW[MSRC_N] = {0, 35, 30, 15, 8, 12};
    rnd_t r;
    mod_t m;
    uint32_t k, n, i;
    memset(&m, 0, sizeof m);
    rnd_seed(&r, seed);
    for (k = 0; k < 2u; k++) {
        mlfo_t *f = &m.lfo[k];
        f->shape = (uint8_t)rnd_below(&r, MS_N);
        f->rate = (uint8_t)(k ? rnd_in(&r, 0, 55) : rnd_in(&r, 5, rnd_chance(&r, 25) ? 90 : 70));   /* (LFO 2 slower) */
        f->mode = (uint8_t)(rnd_chance(&r, 60) ? MM_FREE : rnd_chance(&r, 60) ? MM_ONSET : MM_ONCE);
        f->fade = (uint8_t)(rnd_chance(&r, 60) ? 0 : rnd_in(&r, 10, 60));
    }
    n = 2u + (uint32_t)rnd_below(&r, 3);
    for (k = 0; k < n; k++) {
        mslot_t *s = &m.s[k];
        uint32_t tries, d = 0;
        int32_t mag;
        s->src = (uint8_t)rnd_weighted(&r, SW, MSRC_N);
        for (tries = 0; tries < 6u; tries++) {   /* (a destination not yet taken, and no LFO on its own rate) */
            d = rnd_weighted(&r, DW, MD_N);
            if ((d == MD_RATE1 && s->src == MSRC_L1) || (d == MD_RATE2 && s->src == MSRC_L2))
                continue;
            for (i = 0; i < k && m.s[i].dst != d; i++)
                ;
            if (i == k)
                break;
        }
        s->dst = (uint8_t)d;
        mag = d == MD_LEVEL ? rnd_in(&r, 10, 40) : d == MD_PAN ? rnd_in(&r, 20, 70) : rnd_in(&r, 15, 70);
        s->amt = (int8_t)(mag * (rnd_chance(&r, 50) ? 1 : -1));
    }
    *mo = m;
}
