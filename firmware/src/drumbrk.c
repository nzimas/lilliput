/* SPDX-License-Identifier: GPL-3.0-only
 * LILLIPUT: BREAK (build 19) -- on the DRUMS' pages, while the last white key is held, the drums play a variation of
 * their own rhythm: a fill, a break, flams, a beat cut shorter and shorter... Each press draws one (never the one before),
 * kept while the key is held. Nothing is written: each lane's own step goes on underneath, so let go, the rhythm plays on
 * from where it would have been.
 *
 * Build 36: every lane by its own meter. A lane's bar is its cycle -- its own length, 5, 7, 12, 13 steps as it is, not
 * a 4/4 bar of four beats -- and its beats are found in it (brk_meter): groups near a beat long, laid where its own
 * accents and hits fall (an aksak lane's 2 + 2 + 3, a 12-step lane's three fours, a 16-step lane's four fours). It is
 * quantized to them: a lane goes into the break on its next beat (its next half cycle for HALF TIME and MIRROR),
 * playing as written until then; the fills are measured against its cycle, so they land on its own downbeat however
 * late they start, and a break held long goes round in the lane's cycle. Lanes of different lengths keep drifting
 * against each other, each breaking in its own time. (Before, every lane was cut in 4/4 bars of four beats: on a
 * polymeter the break came in off the lane's beat and ran on as if in 4/4.) Every way is made from the rhythm playing -- its hits, accents, pitches -- and from what each voice is: the
 * kick holds the floor, the snare rolls and flams, the hats ghost, the perc tumbles.
 *
 *   STUTTER    the bar's busiest beat (its hits, every voice's), repeated: a beat, then half, then a step,
 *              ratcheting faster
 *   AVALANCHE  a fill over the bar: snare from ghosts on 8ths to accented 16ths and doubles, the kick on
 *              the beats; it lands on the downbeat, everything accented, and builds again
 *   FLAMS      the rhythm as written, every kick, snare and perc hit flammed (a soft grace note before it); accented
 *              snares dragged (two)
 *   DROP       kick and snare out; hats and perc on, the hats' gaps filled with ghosts
 *   HALF TIME  kick, snare and perc at half speed from where it starts; the hats at full speed over them
 *   CELL n     every lane loops a cell of 3, 5, 6 or 7 steps of itself against the bar; the kick keeps each downbeat
 *   SWAP       the voices trade lanes: the kick plays the snare's steps and the snare the kick's, the hats the perc's
 *   MIRROR     each half bar played backwards; the kick keeps its first step
 *   TUMBLE     the perc on every step over 2 beats, falling from an accent to ghosts, again and again; the kick as written
 *   INTERLOCK  kick and snare keep the first half of each beat, hats and perc the second; after a bar they trade
 *              whole beats (1 and 3, 2 and 4): the voices answer each other, what is left accented; the kick keeps
 *              the downbeat
 *   SCATTER    the bar's 8 pairs of steps in another order (drawn each press), the first in place
 *   BUZZ       the rhythm as written, hats in threes then fours, snare and perc in doubles, faster the longer it is held
 *   GHOSTS     the rhythm with every gap filled with ghosts, hats accented 3 3 2, the kick pushing ahead of beat 4 */
enum { BK_STUTTER, BK_AVALANCHE, BK_FLAMS, BK_DROP, BK_HALF, BK_CELL, BK_SWAP, BK_MIRROR, BK_TUMBLE, BK_INTERLOCK, BK_SCATTER,
       BK_BUZZ, BK_GHOSTS, BK_N };
static const char *const BK_NAME[BK_N] = {"STUTTER", "AVALANCHE", "FLAMS", "DROP", "HALF TIME", "CELL", "SWAP", "MIRROR",
                                          "TUMBLE", "INTERLOCK", "SCATTER", "BUZZ", "GHOSTS"};
typedef struct { uint8_t way, cell, q, rsv; uint8_t ord[8]; } brk_plan_t;
static brk_plan_t brk_plan;                      /* (the main loop's: written before brk_id moves on) */
static volatile uint8_t brk_hold;                /* (the main loop's: the key held, on the DRUMS' pages) */
static volatile uint32_t brk_id;                 /* (.. one a press) */
static struct {
    uint32_t id, p0;
    uint16_t n;                                  /* its step in the break's table (from the anchor the table is for) */
    uint8_t on, rn, rk, fw, fk, rsv;             /* in it; a ratchet's hits and the next; a flam's wait (blocks) and its
                                                  * graces left */
    dhit_t rev, fev;                             /* the ratchet's hit; the flam's main hit */
} bks[NDV];
/* the way, worked out by the main loop at the press (brk_fill), a lane's 64 steps from its anchor -- the step its
 * quantum falls on next -- as the ISR is to play them (the last 32 again and again, held longer): so only playing them
 * is in RAM, on the audio's path. A step: fire, ACC (2 bits), ALT, RAT (3), FLAM (2), pitch + 12 (5) */
#define BRK_TN 64u
static uint16_t brk_tab[NDV][BRK_TN] POOL;
static uint8_t brk_a[NDV];                       /* (each table's anchor: the lane's step it starts on) */
static uint8_t brk_lp[NDV];                      /* (the part held longer goes round in this many steps: the most
                                                  * whole cycles of the lane in 32) */
/* a lane's meter (the main loop's): its cycle's beats -- each step's beat and place in it, each beat's start and
 * length; where its second half begins */
typedef struct {
    uint8_t L, nb, half, rsv;
    uint8_t bq[DSTEPS], bo[DSTEPS], bs[DSTEPS], bl[DSTEPS];
} brk_meter_t;
static brk_meter_t brk_m[NDV] POOL;
#define BRK_FLAM_BLK ((uint32_t)FS * 11u / (1000u * CTL))   /* (a grace note 11 ms before its hit) */

/* lane l's step s, as written (its chance taken): as the ISR plays it, not under a break */
static FAST void brk_written(uint32_t l, uint32_t s, dhit_t *e)
{
    const dlane_t *L = &drm.l[l];
    s %= (uint32_t)clampi(L->len, 1, (int32_t)DSTEPS);
    e->fire = (uint8_t)((L->hits >> s & 1u) && (rnd_next(&rhy_r) >> 25) < (uint32_t)(L->prob ? L->prob : 100) * 128u / 100u);
    e->acc = (uint8_t)(L->acc >> s & 1u);
    e->alt = (uint8_t)(L->alt >> s & 1u);
    e->pitch = L->pitch[s];
    e->rat = e->flam = 0;
}
/* ---- the main loop's: the ways */
static uint32_t brk_r = 0x9E3779B9u;
static uint32_t brk_rnd(void)
{
    brk_r = brk_r * 1664525u + 1013904223u;
    return brk_r;
}
static void brk_lane(uint32_t l, uint32_t s, dhit_t *e)   /* (as written, its chance taken here) */
{
    const dlane_t *L = &drm.l[l];
    s %= (uint32_t)clampi(L->len, 1, (int32_t)DSTEPS);
    e->fire = (uint8_t)((L->hits >> s & 1u) && (brk_rnd() >> 25) < (uint32_t)(L->prob ? L->prob : 100) * 128u / 100u);
    e->acc = (uint8_t)(L->acc >> s & 1u);
    e->alt = (uint8_t)(L->alt >> s & 1u);
    e->pitch = L->pitch[s];
    e->rat = e->flam = 0;
}
static void brk_hit(dhit_t *e, uint32_t acc, uint32_t alt, int32_t pitch)
{
    (void)pitch;                                 /* (build 22: no pitch moves in the drums -- each voice keeps its own) */
    e->fire = 1;
    e->acc = (uint8_t)acc;
    e->alt = (uint8_t)alt;
    e->pitch = 0;
    e->rat = e->flam = 0;
}
/* the hit lane c plays n steps into the break, from its anchor a, by its meter M; sb: STUTTER's beat (the lane's
 * busiest) */
static void brk_way(uint32_t c, uint32_t a, uint32_t n, const brk_meter_t *M, uint32_t sb, dhit_t *e)
{
    uint32_t L = M->L, s = a + n, pos = s % L, qb = M->bq[pos], in = M->bo[pos], bl = M->bl[qb], w;
    const brk_plan_t *P = &brk_plan;
    switch (P->way) {
    case BK_STUTTER: {                           /* the busiest beat: whole, half, a step; ratchets */
        uint32_t b = M->bl[sb];
        w = n < 2u * b ? b : n < 3u * b ? (b > 1u ? b / 2u : 1u) : 1u;
        brk_lane(c, M->bs[sb] + n % w, e);
        if (n % w == 0u && e->fire)
            e->acc = 1;
        if (w == 1u && c != DV_KICK && e->fire)
            e->rat = (uint8_t)(n < 2u * L ? 2u : n < 3u * L ? 3u : 4u);
        break;
    }
    case BK_AVALANCHE: {                         /* a fill over the cycle, in four parts, landing on its downbeat */
        uint32_t ph = pos * 4u / L;
        e->fire = 0;
        if (c == DV_KICK) {
            if (in == 0u)
                brk_hit(e, pos == 0u, 0, 0);
        } else if (c == DV_SNARE) {
            if (ph == 0u && !(in & 1u) && pos)
                brk_hit(e, 2, 1, 0);
            else if (ph == 1u && !(in & 1u))
                brk_hit(e, 0, 1, 0);
            else if (ph == 2u)
                brk_hit(e, 0, 0, 0);
            else if (ph == 3u) {
                brk_hit(e, 1, 0, 0);
                if (pos + 2u >= L)
                    e->rat = 2;
            }
        } else if (c == DV_HAT) {
            if (pos == 0u)
                brk_hit(e, 1, 0, 0);             /* (the downbeat, open) */
            else if (ph < 2u && in == bl / 2u)
                brk_hit(e, 2, 1, 0);
        } else {
            if (pos == 0u)
                brk_hit(e, 1, 0, 0);
            else if (ph == 3u && !(in & 1u))
                brk_hit(e, 1, 1, 0);
        }
        break;
    }
    case BK_FLAMS:
        brk_lane(c, s, e);
        if (e->fire && c != DV_HAT)
            e->flam = (uint8_t)(c == DV_SNARE && e->acc ? 2u : 1u);
        break;
    case BK_DROP:
        if (c == DV_KICK || c == DV_SNARE)
            e->fire = 0;
        else {
            brk_lane(c, s, e);
            if (c == DV_HAT && !e->fire && (brk_rnd() >> 31))
                brk_hit(e, 2, 1, 0);
        }
        break;
    case BK_HALF:                                /* (from a half cycle: half speed, so a cycle takes two) */
        if (c == DV_HAT)
            brk_lane(c, s, e);
        else if (n & 1u)
            e->fire = 0;
        else {
            brk_lane(c, a + n / 2u, e);
            e->alt = 0;                          /* (long: half time) */
        }
        break;
    case BK_CELL:
        brk_lane(c, a + n % (P->cell ? P->cell : 3u), e);
        if (c == DV_KICK && pos == 0u)
            brk_hit(e, 1, 0, 0);
        break;
    case BK_SWAP: {
        uint32_t o = c ^ 1u;                     /* (kick and snare, hats and perc) */
        brk_lane(o, s, e);
        if (c == DV_KICK && pos == 0u) {
            dhit_t k;
            brk_lane(c, s, &k);
            if (k.fire)
                *e = k;
        }
        break;
    }
    case BK_MIRROR: {                            /* each half of the cycle backwards */
        uint32_t h = M->half, m = pos < h ? h - 1u - pos : h + (L - 1u - pos);
        brk_lane(c, c == DV_KICK && pos == 0u ? s : s - pos + m, e);
        break;
    }
    case BK_TUMBLE: {                            /* (over about two of its beats) */
        uint32_t T = M->nb > 2u ? (2u * L + M->nb / 2u) / M->nb : L, t;
        T = T < 2u ? 2u : T;
        t = n % T;
        if (c == DV_PERC)
            brk_hit(e, t == 0u ? 1u : t < T / 2u ? 0u : 2u, t + 1u < T, 0);   /* (its loudness falls, not its pitch) */
        else if (c == DV_KICK)
            brk_lane(c, s, e);
        else if (c == DV_SNARE && t + 1u == T)
            brk_hit(e, 1, 0, 0);
        else
            e->fire = 0;
        break;
    }
    case BK_INTERLOCK: {
        uint32_t lo = c == DV_KICK || c == DV_SNARE;
        brk_lane(c, s, e);
        if (c == DV_KICK && pos == 0u)
            break;
        if ((n < L ? 2u * in >= bl : qb & 1u) == lo)   /* (the low voices the first half of each beat, the others
                                                  * the second; a cycle on, whole beats: the even ones, the odd) */
            e->fire = 0;
        else if (e->fire)
            e->acc = 1;
        break;
    }
    case BK_SCATTER: {                           /* the cycle's beats (or, with fewer than three, its pairs of steps)
                                                  * in another order, the first in place */
        uint32_t byb = M->nb >= 3u, nu = byb ? M->nb : (L + 1u) / 2u, u = byb ? qb : pos / 2u, o = byb ? in : pos & 1u,
                 k, j = 0, src = u, ss, sl;
        if (u < 8u && nu > 1u) {                 /* (the drawn order of eight, those within the cycle's units) */
            uint32_t rank = 0;
            for (k = 0; k < 8u; k++)
                if (P->ord[k] < nu && P->ord[k] < 8u) {
                    if (rank++ == u) {
                        j = P->ord[k];
                        break;
                    }
                }
            src = j;
        }
        ss = byb ? M->bs[src] : src * 2u;
        sl = byb ? M->bl[src] : (src * 2u + 1u < L ? 2u : 1u);
        brk_lane(c, s - pos + ss + (o < sl ? o : sl - 1u), e);
        break;
    }
    case BK_BUZZ:
        brk_lane(c, s, e);
        if (e->fire && c != DV_KICK)
            e->rat = (uint8_t)(c == DV_HAT ? (n < L ? 3u : 4u) : (n < L ? 2u : 3u));
        break;
    case BK_GHOSTS:
        brk_lane(c, s, e);
        if (c == DV_HAT) {                       /* (accented 3 3 2 over its cycle, whatever its length) */
            uint32_t g = 0, k = 0, acc = 0;
            static const uint8_t G332[3] = {3, 3, 2};
            while (g < L) {
                if (g == pos && L - g > 1u)
                    acc = 1;
                g += G332[k++ % 3u];
            }
            if (!e->fire)
                brk_hit(e, 2, 1, 0);
            else
                e->acc = (uint8_t)acc;
        } else if (c == DV_SNARE && !e->fire && (pos & 1u) && (brk_rnd() >> 31))
            brk_hit(e, 2, 1, 0);
        else if (c == DV_KICK && !e->fire && M->nb > 1u &&
                 (pos + 2u == M->bs[M->nb - 1u] || pos + 2u == L))   /* (pushing ahead of the last beat, of the
                                                  * downbeat) */
            brk_hit(e, 0, 1, 0);
        break;
    default:
        brk_lane(c, s, e);
        break;
    }
}
static uint32_t brk_spb(uint32_t c)              /* (a lane's steps a beat) */
{
    uint32_t d2 = DIV2[drm.l[c].div < NDIV ? drm.l[c].div : 4];
    return d2 >= 2u ? d2 / 2u : 1u;
}
/* lane c's meter: its cycle cut in beats -- a cycle of whole beats in even beats; any other in groups of half a beat
 * to a beat and a step long, where its accents and its hits after rests fall most, the fewer steps from a beat's length the better (a 16-step lane in fours, 12 in fours or
 * threes as its hits say, 7 in 2 + 2 + 3 or 4 + 3, 5 in 2 + 3) */
static void brk_meter(uint32_t c, brk_meter_t *M)
{
    const dlane_t *Ln = &drm.l[c];
    uint32_t L = (uint32_t)clampi(Ln->len, 1, (int32_t)DSTEPS), spb = brk_spb(c), lo = (spb + 1u) / 2u, hi = spb + 1u;
    int32_t best[DSTEPS + 1];
    uint8_t from[DSTEPS + 1];
    uint32_t p, g, k, b;
    if (lo < 1u)
        lo = 1u;
    if (hi < 2u)
        hi = 2u;
    best[0] = 0;
    for (p = 1; p <= L; p++) {                   /* (the best cut of the first p steps, a group ending at p) */
        best[p] = -1000000;
        from[p] = 0;
        for (g = lo; g <= hi && g <= p; g++) {
            uint32_t q = p - g;
            int32_t v, d = (int32_t)g - (int32_t)spb;
            if (best[q] <= -1000000)
                continue;
            v = best[q] - 4 * (d < 0 ? -d : d) / (int32_t)spb;   /* (a group off a beat's length: a cost) */
            if (q)                               /* (a group starting on an accent, on a hit after a rest: a gain --
                                                  * a lane hit on every step says nothing by its hits) */
                v += (int32_t)(6u * ((Ln->hits >> q & 1u) && !(Ln->hits >> (q - 1u) & 1u)) + 8u * (Ln->acc >> q & 1u));
            if (v > best[p]) {
                best[p] = v;
                from[p] = (uint8_t)q;
            }
        }
        if (best[p] <= -1000000 && p == L && L < lo) {   /* (a cycle shorter than a group: one group) */
            best[p] = 0;
            from[p] = 0;
        }
    }
    if (L % spb == 0u) {                         /* a cycle of whole beats: even beats -- of a beat, or of another
                                                  * size its onsets plainly keep (3 of 4 group starts on one, 3 of 4 of
                                                  * them on a group start: a 12-step lane in threes -- unless its beats already begin with them). Off-beat accents
                                                  * are a lane's syncopation, not its meter */
        uint32_t g, eg = spb, no = 0, ob = 0;
        for (p = 0; p < L; p++) {
            uint32_t o = ((Ln->hits >> p & 1u) && !(Ln->hits >> (p ? p - 1u : L - 1u) & 1u)) || (Ln->acc >> p & 1u);
            no += o;
            ob += o && p % spb == 0u;
        }
        for (g = lo; g <= hi && 4u * ob < 3u * (L / spb); g++) {   /* (its beats kept if 3 of 4 begin with an onset:
                                                  * steps between them only fill them in -- even eighths are 4/4) */
            uint32_t on = 0;
            if (g == spb || L % g || L / g < 2u)
                continue;
            for (p = 0; p < L; p += g)
                on += ((Ln->hits >> p & 1u) && !(Ln->hits >> (p ? p - 1u : L - 1u) & 1u)) || (Ln->acc >> p & 1u);
            if (4u * on >= 3u * (L / g) && 4u * on >= 3u * no)
                eg = g;
        }
        for (p = eg; p <= L; p += eg)
            from[p] = (uint8_t)(p - eg);
    }
    {   /* the groups, back from the end */
        uint8_t st[DSTEPS];
        uint32_t n = 0;
        p = L;
        while (p > 0u && n < DSTEPS) {
            st[n++] = from[p];
            p = from[p];
        }
        M->L = (uint8_t)L;
        M->nb = (uint8_t)n;
        for (b = 0; b < n; b++) {
            uint32_t s0 = st[n - 1u - b], s1 = b + 1u < n ? st[n - 2u - b] : L;
            M->bs[b] = (uint8_t)s0;
            M->bl[b] = (uint8_t)(s1 - s0);
            for (k = s0; k < s1; k++) {
                M->bq[k] = (uint8_t)b;
                M->bo[k] = (uint8_t)(k - s0);
            }
        }
    }
    {   /* the second half: the beat nearest the middle (one beat: the middle step) */
        uint32_t h = L / 2u, bd = 99;
        for (b = 1; b < M->nb; b++) {
            uint32_t d = M->bs[b] > L / 2u ? M->bs[b] - L / 2u : L / 2u - M->bs[b];
            if (d < bd) {
                bd = d;
                h = M->bs[b];
            }
        }
        M->half = (uint8_t)(h ? h : 1u);
    }
}
/* a press: each lane's meter and table, from the step its quantum falls on next; then the ISR is told (brk_id) */
static void brk_fill(void)
{
    uint32_t c, i;
    for (c = 0; c < NDV; c++) {
        brk_meter_t *M = &brk_m[c];
        uint32_t len, st, a, sb = 0, best = 0, b, k;
        brk_meter(c, M);
        len = M->L;
        st = dlr[c].step == 0xFF || drm_reset ? 0xFFu : dlr[c].step % len;
        if (st == 0xFFu)
            a = 0;                               /* (from the top: 0) */
        else {                                   /* its next beat (HALF TIME, MIRROR: its next half cycle) */
            a = (st + 1u) % len;
            for (k = 0; k < len; k++, a = (a + 1u) % len)
                if (brk_plan.q == 8u ? a == 0u || a == M->half : M->bo[a] == 0u)
                    break;
        }
        for (b = 0; b < M->nb; b++) {            /* STUTTER's beat: the lane's busiest (a tie: the later) */
            uint32_t cnt = 0;
            for (k = M->bs[b]; k < (uint32_t)M->bs[b] + M->bl[b]; k++)
                cnt += (drm.l[c].hits >> k & 1u) + (drm.l[c].acc >> k & 1u);
            if (cnt >= best) {
                best = cnt;
                sb = b;
            }
        }
        brk_a[c] = (uint8_t)a;
        brk_lp[c] = (uint8_t)(32u / len * len);
        for (i = 0; i < BRK_TN; i++) {
            dhit_t e;
            brk_way(c, a, i, M, sb, &e);
            brk_tab[c][i] = (uint16_t)(e.fire | (e.acc & 3u) << 1 | (e.alt & 1u) << 3 | (e.rat & 7u) << 4 | (e.flam & 3u) << 7 |
                                       (uint32_t)(clampi(e.pitch, -12, 12) + 12) << 9);
        }
    }
    __asm__ volatile("" ::: "memory");
    brk_id++;
}
/* ---- the ISR's: the hit lane c plays on its step s (of len) */
static FAST void brk_step(uint32_t c, uint32_t s, uint32_t len, dhit_t *e)
{
    uint32_t i, t;
    if (!brk_hold || !drm.on) {
        bks[c].on = 0;
        brk_written(c, s, e);
        return;
    }
    if (bks[c].id != brk_id) {                   /* (a new press: into it on its quantum) */
        bks[c].id = brk_id;
        bks[c].on = 0;
    }
    if (!bks[c].on) {                            /* (into it on its anchor -- its next beat, by its meter: brk_fill) */
        i = (s + len - brk_a[c]) % len;          /* (0; 1 if the press fell on the step before it) */
        if (i > 1u) {
            brk_written(c, s, e);
            return;
        }
        bks[c].on = 1;
        bks[c].n = (uint16_t)i;
    } else if (bks[c].n < 60000u)
        bks[c].n++;
    i = bks[c].n < BRK_TN ? bks[c].n : BRK_TN - brk_lp[c] + (bks[c].n - BRK_TN) % brk_lp[c];   /* (the last whole
                                                  * cycles again and again: in step with the lane) */
    t = brk_tab[c][i];
    e->fire = (uint8_t)(t & 1u);
    e->acc = (uint8_t)(t >> 1 & 3u);
    e->alt = (uint8_t)(t >> 3 & 1u);
    e->rat = (uint8_t)(t >> 4 & 7u);
    e->flam = (uint8_t)(t >> 7 & 3u);
    e->pitch = (int8_t)((int32_t)(t >> 9 & 31u) - 12);
}
/* a voice hit, now (and its ratchet and its flam, to come) */
static FAST void drm_emit(uint32_t c, const dhit_t *e)
{
    if (!e->fire || (drm.mute >> c & 1u))        /* (muted: its steps run on, unheard) */
        return;
    if (e->flam) {                               /* a grace note now, soft and short; the hit after it */
        bks[c].fev = *e;
        bks[c].fev.flam = 0;
        bks[c].fk = (uint8_t)(e->flam - 1u);
        bks[c].fw = (uint8_t)BRK_FLAM_BLK;
        dtrig[c].acc = 2;
        dtrig[c].alt = 1;
        dtrig[c].pitch = e->pitch;
        dtrig[c].fire = 1;
        return;
    }
    dtrig[c].acc = e->acc;
    dtrig[c].alt = e->alt;
    dtrig[c].pitch = e->pitch;
    dtrig[c].fire = 1;
    bks[c].rn = e->rat > 1u ? e->rat : 0u;
    if (bks[c].rn) {
        bks[c].rk = 1;
        bks[c].rev = *e;
        bks[c].rev.rat = 0;
        bks[c].rev.acc = e->acc == 1u ? 0u : e->acc;
        bks[c].p0 = dlr[c].ph;
    }
}
/* a block: the ratchets' and the flams' hits that fall in it */
static FAST void brk_block(uint32_t c)
{
    if (bks[c].fw && !--bks[c].fw) {
        if (bks[c].fk) {
            bks[c].fk--;
            bks[c].fw = (uint8_t)BRK_FLAM_BLK;
            dtrig[c].acc = 2;
            dtrig[c].alt = 1;
            dtrig[c].fire = 1;
        } else
            drm_emit(c, &bks[c].fev);
        return;
    }
    if (bks[c].rn && bks[c].rk < bks[c].rn) {
        uint32_t p0 = bks[c].p0, thr = p0 + (0xFFFFFFFFu - p0) / bks[c].rn * bks[c].rk;
        if (dlr[c].ph >= thr) {
            bks[c].rk++;
            if (!(drm.mute >> c & 1u)) {
                dtrig[c].acc = bks[c].rev.acc;
                dtrig[c].alt = bks[c].rev.alt;
                dtrig[c].pitch = bks[c].rev.pitch;
                dtrig[c].fire = 1;
            }
        }
    }
}
static FAST void brk_clear(void)
{
    uint32_t c;
    for (c = 0; c < NDV; c++)
        bks[c].rn = bks[c].fw = 0;
}
