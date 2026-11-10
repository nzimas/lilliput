/* SPDX-License-Identifier: GPL-3.0-only
 * LILLIPUT: the RHYTHM's styles -- a new rhythm drawn for the mixer as it is (the main loop; the clock and the gates
 * are mixer.c's, in the ISR).
 *
 * A rhythm is a lane a channel: its steps (up to 32), its step's length (1/2 .. 1/32, triplets, quintuplets), its
 * mode -- CHOP (what is heard gated: mutes and unmutes in time, the loop running on), CHOKE (the loop itself gated:
 * it dies and grows back, a swell), KICK (the loop choked and struck by a burst at each hit: the loop's own sound
 * made a drum), ACCENT (no gate: each hit pushes GAIN and FOLD up a moment) -- its accents, its gate length, its
 * depth, its chance, its rolls and its mutation. Lanes of different lengths drift against each other (polymeter);
 * lanes of different step lengths run in different time (polyrhythm).
 *
 * The channels get their parts by what they sound like: the darkest loop (a slow limiter, a low middle) the KICK's,
 * the brightest the hats', those between the snare's and the percussion's; a channel with its fader down is left
 * out; one drone may be left free under the rhythm. The styles, a palette for rhythmic noise and around it:
 *   INDUSTRIAL   four on the floor, the backbeat, off-beat hats            (Toussaint's evenness: Euclidean hits)
 *   POWER NOISE  4/4 kicks, a drone chopped in sixteenths, accents
 *   BROKEN       syncopated Euclidean kicks and snares, swung
 *   AKSAK        odd meters added up: 5 (3+2), 7 (2+2+3), 9 (2+2+2+3), 11 (3+3+3+2), 13 (3+3+3+2+2)
 *   POLYMETER    lanes of 16, 12, 10, 9, 7, 5, 3 steps against each other
 *   POLYRHYTHM   quarters against triplets against quintuplets against eighths
 *   MARCH        snare rolls, ratchets, a military kick
 *   HALFTIME     slow, heavy, long gates: a kick on one, the snare on three
 *   GABBER       a kick on every beat, noise bursts between
 *   ROLLS        sparse hits rolled into bursts
 *   CHOP         drones chopped in sixteenths and thirty-seconds, holes by chance
 *   TRIBAL       tresillo, cinquillo, clave, the 12/8 bell (Toussaint's timelines)
 *   TURING       random steps that repeat and slowly mutate
 *   ERRATIC      lengths, steps and chances all uneven, swung, ghosts
 *   SPARSE       few hits, long choked swells
 *   CLOCKWORK    dividers: every 2, 3, 4, 6, 8 steps, gated hard -- a machine */

enum { RS_INDUSTRIAL, RS_POWER, RS_BROKEN, RS_AKSAK, RS_POLYMETER, RS_POLYRHYTHM, RS_MARCH, RS_HALFTIME, RS_GABBER,
       RS_ROLLS, RS_CHOP, RS_TRIBAL, RS_TURING, RS_ERRATIC, RS_SPARSE, RS_CLOCKWORK, RS_N };
static const char *const RS_NAME[RS_N] = {"INDUSTRIAL", "POWER NOISE", "BROKEN", "AKSAK", "POLYMETER", "POLYRHYTHM",
                                          "MARCH", "HALFTIME", "GABBER", "ROLLS", "CHOP", "TRIBAL", "TURING", "ERRATIC",
                                          "SPARSE", "CLOCKWORK"};
static const char *const RM_NAME[RM_N] = {"OFF", "CHOP", "CHOKE", "KICK", "ACCENT"};
static const char *const DIV_NAME[NDIV] = {"1/2", "1/4", "1/8", "1/8 T", "1/16", "1/16 Q", "1/16 T", "1/32"};
enum { DV_2, DV_4, DV_8, DV_8T, DV_16, DV_16Q, DV_16T, DV_32 };

/* k hits spread as evenly as can be over n steps, turned by rot (the Euclidean rhythm E(k, n), Bresenham's way) */
static uint32_t euclid(uint32_t k, uint32_t n, uint32_t rot)
{
    uint32_t i, m = 0;
    if (!n || n > 32u)
        return 0;
    if (k > n)
        k = n;
    for (i = 0; i < n; i++)
        if ((i * k) % n < k)
            m |= 1u << ((i + rot) % n);
    return m;
}
/* steps written out: x a hit, X an accented one, . a rest (the accents into *acc) */
static uint32_t steps(const char *s, uint32_t *acc)
{
    uint32_t i, m = 0, a = 0;
    for (i = 0; s[i] && i < 32u; i++) {
        if (s[i] == 'x' || s[i] == 'X')
            m |= 1u << i;
        if (s[i] == 'X')
            a |= 1u << i;
    }
    if (acc)
        *acc = a;
    return m;
}
static uint32_t slen(const char *s) { uint32_t n = 0; while (s[n] && n < 32u) n++; return n; }

/* the parts, as the channels sound: their order from darkest to brightest (the heard ones) */
enum { RP_KICK, RP_SNARE, RP_HAT, RP_PERC, RP_N };
static uint32_t rhy_order(const lil_par_t *q, uint8_t *ord)
{
    uint32_t c, n = 0, i, j;
    int32_t key[NCH];
    for (c = 0; c < NCH; c++) {
        if (!q->ch[c].level || q->ch[c].mute)
            continue;
        key[n] = q->ch[c].lim * 2 - q->ch[c].mfreq;   /* (a slow limiter, a low middle: dark) */
        ord[n++] = (uint8_t)c;
    }
    for (i = 1; i < n; i++)                      /* (darkest first) */
        for (j = i; j > 0 && key[j] > key[j - 1]; j--) {
            int32_t t = key[j];
            uint8_t o = ord[j];
            key[j] = key[j - 1];
            key[j - 1] = t;
            ord[j] = ord[j - 1];
            ord[j - 1] = o;
        }
    return n;
}
static void lane_set(lane_t *L, uint8_t mode, uint32_t len, uint8_t div, uint32_t hits, uint32_t acc, uint8_t gate,
                     uint8_t depth)
{
    memset(L, 0, sizeof *L);
    L->mode = mode;
    L->len = (uint8_t)(len < 1u ? 1u : len > 32u ? 32u : len);
    L->div = div;
    L->hits = hits;
    L->acc = acc;
    L->gate = gate;
    L->depth = depth;
    L->prob = 100;
}

/* a new rhythm into q's lanes, of style st (-1: any), from seed; the tempo, the swing and the rest of the
 * performance's settings kept but for a style's own swing; returns the style */
static uint32_t rhythm_draw(lil_par_t *q, uint32_t seed, int st)
{
    rnd_t r;
    uint8_t ord[NCH];
    uint32_t n, i, k, acc;
    rhy_par_t *R = &q->rhy;
    rnd_seed(&r, seed);
    if (st < 0 || st >= RS_N)
        st = rnd_below(&r, RS_N);
    for (i = 0; i < NCH; i++)
        memset(&R->lane[i], 0, sizeof R->lane[i]);
    n = rhy_order(q, ord);
    R->style = (uint8_t)st;
    R->swing = 0;
    if (!n)
        return (uint32_t)st;
#define PART(p) (n > (p) ? &R->lane[ord[(p) < n ? (p) : 0]] : (lane_t *)0)
#define LANE_OF(i) (&R->lane[ord[(i)]])
    switch (st) {
    case RS_INDUSTRIAL: {
        static const char *const SN[3] = {"....X.......X...", "....X.......X..x", "....X..x....X..."};
        lane_set(LANE_OF(0), RM_KICK, 16, DV_16, steps("X...x...X...x...", &acc), acc, 25, 100);
        if (n > 1) {
            lane_set(LANE_OF(n - 1), RM_CHOP, 16, DV_16, euclid(pr_in(&r, 4, 8), 16, 2), 0, 30, 100);
            LANE_OF(n - 1)->hits = LANE_OF(n - 1)->hits & 0xAAAAu ? LANE_OF(n - 1)->hits : 0x4444u;   /* (off beats) */
        }
        if (n > 2)
            lane_set(LANE_OF(1), rnd_chance(&r, 50) ? RM_KICK : RM_CHOP, 16, DV_16, steps(SN[rnd_below(&r, 3)], &acc), acc, 30, 100);
        for (i = 2; i + 1u < n; i++)
            lane_set(LANE_OF(i), RM_CHOP, 16, DV_16, euclid(pr_in(&r, 3, 6), 16, pr_in(&r, 0, 15)), 0, 40, pr_in(&r, 60, 100));
        break;
    }
    case RS_POWER:
        lane_set(LANE_OF(0), RM_KICK, 16, DV_16, steps(rnd_chance(&r, 50) ? "X...x...X...x.x." : "X...x...X...x...", &acc), acc, 30, 100);
        for (i = 1; i < n; i++) {
            if (i == 1u)
                lane_set(LANE_OF(i), RM_CHOP, 16, DV_16, euclid(pr_in(&r, 10, 14), 16, 0), steps("X...X...X...X...", 0), 45, pr_in(&r, 70, 100));
            else
                lane_set(LANE_OF(i), rnd_chance(&r, 50) ? RM_CHOP : RM_ACCENT, 16, DV_16, steps("....X.......X...", &acc) | euclid(2, 16, 7), acc, 30, 100);
            LANE_OF(i)->ratchet = (uint8_t)pr_in(&r, 0, 20);
        }
        break;
    case RS_BROKEN:
        R->swing = (uint8_t)pr_in(&r, 10, 40);
        lane_set(LANE_OF(0), RM_KICK, 16, DV_16, euclid(pr_in(&r, 4, 6), 16, 0), steps("X...............", 0), 25, 100);
        for (i = 1; i < n; i++)
            lane_set(LANE_OF(i), i == 1u ? RM_KICK : RM_CHOP, 16, DV_16, euclid(i == 1u ? pr_in(&r, 2, 4) : pr_in(&r, 5, 11), 16, pr_in(&r, 2, 13)), 0,
                     (uint8_t)pr_in(&r, 20, 50), 100);
        break;
    case RS_AKSAK: {                             /* an odd meter as groups: the kick on each group, the snare on the last */
        static const uint8_t G[5][5] = {{3, 2}, {2, 2, 3}, {2, 2, 2, 3}, {3, 3, 3, 2}, {3, 3, 3, 2, 2}};
        uint32_t m = (uint32_t)rnd_below(&r, 5), len = 0, starts = 0, last = 0;
        for (k = 0; k < 5u && G[m][k]; k++) {
            starts |= 1u << len;
            last = len;
            len += G[m][k];
        }
        lane_set(LANE_OF(0), RM_KICK, len, DV_8, starts & ~(1u << last), 1u, 30, 100);
        if (n > 1)
            lane_set(LANE_OF(1), RM_KICK, len, DV_8, 1u << last, 1u << last, 35, 100);
        if (n > 2)
            lane_set(LANE_OF(2), RM_CHOP, len * 2u, DV_16, euclid(len, len * 2u, 1), starts, 35, 100);
        for (i = 3; i < n; i++)
            lane_set(LANE_OF(i), RM_ACCENT, len, DV_8, starts, starts, 60, 0);
        break;
    }
    case RS_POLYMETER: {
        static const uint8_t PL[7] = {16, 12, 10, 9, 7, 5, 3};
        for (i = 0; i < n; i++) {
            uint32_t len = i ? PL[rnd_below(&r, 7)] : 16u;
            lane_set(LANE_OF(i), i == 0u ? RM_KICK : rnd_chance(&r, 60) ? RM_CHOP : RM_KICK, len, DV_16,
                     euclid(i ? pr_in(&r, 2, (int32_t)len * 2 / 3) : 4u, len, i ? pr_in(&r, 0, (int32_t)len - 1) : 0u), 1u, (uint8_t)pr_in(&r, 20, 45), 100);
        }
        break;
    }
    case RS_POLYRHYTHM: {
        static const uint8_t PD[4] = {DV_4, DV_8T, DV_16Q, DV_8};
        for (i = 0; i < n; i++) {
            uint8_t dv = i ? PD[rnd_below(&r, 4)] : DV_4;
            uint32_t per = DIV2[dv] / 2u ? DIV2[dv] / 2u : 1u, len = per * (uint32_t)pr_in(&r, 1, 4);
            if (len > 32u)
                len = 32u;
            lane_set(LANE_OF(i), i == 0u ? RM_KICK : rnd_chance(&r, 50) ? RM_KICK : RM_CHOP, len, dv,
                     i ? euclid(pr_in(&r, (int32_t)len / 3 + 1, (int32_t)len), len, 0) : 0xFFFFFFFFu >> (32u - len), 1u,
                     (uint8_t)pr_in(&r, 20, 40), 100);
        }
        break;
    }
    case RS_MARCH:
        lane_set(LANE_OF(0), RM_KICK, 16, DV_16, steps("X.......x.......", &acc), acc, 30, 100);
        for (i = 1; i < n; i++) {
            lane_set(LANE_OF(i), RM_CHOP, 16, DV_16, steps(rnd_chance(&r, 50) ? "x.xxX.x.x.xxX.xx" : "X.x.x.xxX.x.x.xx", &acc), acc,
                     (uint8_t)pr_in(&r, 20, 35), 100);
            LANE_OF(i)->ratchet = (uint8_t)pr_in(&r, 15, 45);
        }
        break;
    case RS_HALFTIME:
        lane_set(LANE_OF(0), RM_KICK, 32, DV_16, steps("X.........x.......x.............", &acc), acc, 60, 100);
        if (n > 1)
            lane_set(LANE_OF(1), RM_KICK, 32, DV_16, steps("................X...............", &acc), 1u << 16, 70, 100);
        for (i = 2; i < n; i++)
            lane_set(LANE_OF(i), RM_CHOKE, 32, DV_16, euclid(pr_in(&r, 3, 6), 32, pr_in(&r, 0, 31)), 0, (uint8_t)pr_in(&r, 60, 100), (uint8_t)pr_in(&r, 50, 90));
        break;
    case RS_GABBER:
        lane_set(LANE_OF(0), RM_KICK, 4, DV_4, 0xFu, 0x1u, 35, 100);
        for (i = 1; i < n; i++)
            lane_set(LANE_OF(i), RM_CHOP, 16, DV_16, i == 1u ? steps("..x...x...x...x.", 0) : euclid(pr_in(&r, 6, 12), 16, 1), 0,
                     (uint8_t)pr_in(&r, 15, 30), 100);
        break;
    case RS_ROLLS:
        for (i = 0; i < n; i++) {
            lane_set(LANE_OF(i), i ? RM_CHOP : RM_KICK, 16, DV_16, euclid(pr_in(&r, 2, 5), 16, pr_in(&r, 0, 15)), 0, 30, 100);
            LANE_OF(i)->ratchet = (uint8_t)pr_in(&r, 40, 80);
        }
        break;
    case RS_CHOP:
        for (i = 0; i < n; i++) {
            uint8_t dv = rnd_chance(&r, 70) ? DV_16 : DV_32;
            uint32_t len = dv == DV_32 ? 32u : 16u;
            lane_set(LANE_OF(i), RM_CHOP, len, dv, euclid(pr_in(&r, (int32_t)len / 3, (int32_t)len * 5 / 6), len, pr_in(&r, 0, 7)), 0,
                     (uint8_t)pr_in(&r, 35, 75), (uint8_t)pr_in(&r, 70, 100));
            LANE_OF(i)->prob = (uint8_t)pr_in(&r, 60, 95);
        }
        break;
    case RS_TRIBAL: {
        static const char *const TL[5] = {"x..x..x.", "x.xx.xx.", "x..x..x...x.x...", "x..x...x..x.....", "x.x.xx.x.x.x"};
        static const uint8_t TD[5] = {DV_8, DV_8, DV_16, DV_16, DV_8T};
        R->swing = (uint8_t)pr_in(&r, 0, 20);
        for (i = 0; i < n; i++) {
            uint32_t t = (uint32_t)rnd_below(&r, 5);
            lane_set(LANE_OF(i), rnd_chance(&r, 70) ? RM_KICK : RM_CHOKE, slen(TL[t]), TD[t], steps(TL[t], 0), 1u, (uint8_t)pr_in(&r, 30, 60), 100);
        }
        break;
    }
    case RS_TURING:
        for (i = 0; i < n; i++) {
            uint32_t len = rnd_chance(&r, 50) ? 8u : 16u;
            lane_set(LANE_OF(i), i ? (rnd_chance(&r, 50) ? RM_CHOP : RM_KICK) : RM_KICK, len, rnd_chance(&r, 70) ? DV_16 : DV_8,
                     rnd_next(&r) & rnd_next(&r) & (0xFFFFFFFFu >> (32u - len)), rnd_next(&r), (uint8_t)pr_in(&r, 20, 50), 100);
            LANE_OF(i)->hits |= 1u;
            LANE_OF(i)->mut = (uint8_t)pr_in(&r, 4, 15);
        }
        break;
    case RS_ERRATIC:
        R->swing = (uint8_t)pr_in(&r, 20, 60);
        for (i = 0; i < n; i++) {
            uint32_t len = (uint32_t)pr_in(&r, 5, 19);
            lane_set(LANE_OF(i), (uint8_t)(RM_CHOP + rnd_below(&r, 3)), len, (uint8_t)pr_in(&r, DV_8, DV_16T),
                     euclid(pr_in(&r, 2, (int32_t)len - 1), len, pr_in(&r, 0, (int32_t)len - 1)), rnd_next(&r), (uint8_t)pr_in(&r, 15, 70),
                     (uint8_t)pr_in(&r, 60, 100));
            LANE_OF(i)->prob = (uint8_t)pr_in(&r, 55, 90);
            LANE_OF(i)->ratchet = (uint8_t)pr_in(&r, 0, 30);
        }
        break;
    case RS_SPARSE:
        for (i = 0; i < n; i++) {
            uint32_t len = rnd_chance(&r, 50) ? 32u : 24u;
            lane_set(LANE_OF(i), i ? RM_CHOKE : RM_KICK, len, DV_16, euclid(pr_in(&r, 1, 3), len, pr_in(&r, 0, (int32_t)len - 1)), 1u,
                     (uint8_t)pr_in(&r, 70, 100), (uint8_t)pr_in(&r, 50, 90));
        }
        break;
    default: {                                   /* CLOCKWORK: dividers -- every 2, 3, 4, 6, 8 steps, gated hard */
        static const uint8_t DVS[5] = {2, 3, 4, 6, 8};
        uint32_t o = (uint32_t)rnd_below(&r, 5);
        for (i = 0; i < n; i++) {
            uint32_t d = DVS[(i + o) % 5u], len = d * 4u > 32u ? d * 2u : d * 4u, m = 0;
            for (k = 0; k < len; k += d)
                m |= 1u << k;
            lane_set(LANE_OF(i), i ? (rnd_chance(&r, 75) ? RM_CHOP : RM_ACCENT) : RM_KICK, len, DV_16, m, 1u, (uint8_t)pr_in(&r, 15, 35), 100);
        }
        break;
    }
    }
    for (i = 0; i < NCH; i++) {                  /* a loop slow to grow -- a slow limiter, a narrow band-pass -- is
                                                  * not choked: it would never come back between the hits. CHOP it
                                                  * instead: it rings on, and the gate cuts it */
        lane_t *L = &R->lane[i];
        const ch_par_t *p = &q->ch[i];
        if ((L->mode == RM_KICK || L->mode == RM_CHOKE) &&
            (p->lim > 620 || (p->mtype == MT_BAND && p->mgain > 650) || p->src == SRC_SND1 || p->src == SRC_SND2 ||
             p->src == SRC_RET1 || p->src == SRC_RET2)) {
            L->mode = RM_CHOP;
            L->depth = 100;
        }
    }
    if (n >= 4u && rnd_chance(&r, 35))           /* (now and then a drone left free under it) */
        R->lane[ord[n / 2u]].mode = RM_OFF;
#undef PART
#undef LANE_OF
    return (uint32_t)st;
}
static void rhy_defaults(rhy_par_t *R)
{
    memset(R, 0, sizeof *R);
    R->bpm = 120;
    R->gscale = 50;
}
