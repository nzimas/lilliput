/* SPDX-License-Identifier: GPL-3.0-only
 * LILLIPUT: BREAK (build 19) -- on the DRUMS' pages, while the last white key is held, the drums play a variation of
 * their own rhythm: a fill, a break, flams, a beat cut shorter and shorter... Each press draws one (never the one before),
 * kept while the key is held. Nothing is written: each lane's own step goes on underneath, so let go, the rhythm plays on
 * from where it would have been.
 *
 * It is quantized: a lane goes into the break on its next beat (its next half bar for HALF TIME and MIRROR), playing as
 * written until then; the fills are measured against the bar, so they land on its next downbeat however late they
 * start. Every way is made from the rhythm playing -- its hits, accents, pitches -- and from what each voice is: the
 * kick holds the floor, the snare rolls and flams, the hats ghost, the perc tumbles.
 *
 *   STUTTER    the bar's busiest beat (its hits, every voice's), repeated: a beat, then half, then a step,
 *              ratcheting faster
 *   AVALANCHE  a fill over the bar: snare from ghosts on 8ths to accented 16ths and doubles, rising in pitch, the kick on
 *              the beats; it lands on the downbeat, everything accented, and builds again
 *   FLAMS      the rhythm as written, every kick, snare and perc hit flammed (a soft grace note before it); accented
 *              snares dragged (two)
 *   DROP       kick and snare out; hats and perc on, the hats' gaps filled with ghosts
 *   HALF TIME  kick, snare and perc at half speed from where it starts; the hats at full speed over them
 *   CELL n     every lane loops a cell of 3, 5, 6 or 7 steps of itself against the bar; the kick keeps each downbeat
 *   SWAP       the voices trade lanes: the kick plays the snare's steps and the snare the kick's, the hats the perc's
 *   MIRROR     each half bar played backwards; the kick keeps its first step
 *   TUMBLE     the perc on every step, falling an octave and more over 2 beats, again and again; the kick as written
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
    e->fire = 1;
    e->acc = (uint8_t)acc;
    e->alt = (uint8_t)alt;
    e->pitch = (int8_t)pitch;
    e->rat = e->flam = 0;
}
/* the hit lane c plays n steps into the break, from its anchor a (spb steps a beat); sb: STUTTER's beat */
static void brk_way(uint32_t c, uint32_t a, uint32_t n, uint32_t spb, uint32_t sb, dhit_t *e)
{
    uint32_t bar = 4u * spb, s = a + n, pos = s % bar, w;
    const brk_plan_t *P = &brk_plan;
    if (P->way == BK_STUTTER)                    /* (its window: the bar's busiest beat) */
        a = a - a % bar + sb * spb;
    switch (P->way) {
    case BK_STUTTER:                             /* a beat, half, a step; ratchets */
        w = n < 2u * spb ? spb : n < 3u * spb ? (spb > 1u ? spb / 2u : 1u) : 1u;
        brk_lane(c, a + n % w, e);
        if (n % w == 0u && e->fire)
            e->acc = 1;
        if (w == 1u && c != DV_KICK && e->fire)
            e->rat = (uint8_t)(n < 2u * bar ? 2u : n < 3u * bar ? 3u : 4u);
        break;
    case BK_AVALANCHE: {                         /* a fill over the bar, landing on its downbeat */
        uint32_t qb = pos / spb, in = pos % spb;
        e->fire = 0;
        if (c == DV_KICK) {
            if (in == 0u)
                brk_hit(e, pos == 0u, 0, pos == 0u ? 0 : -2);
        } else if (c == DV_SNARE) {
            if (qb == 0u && !(in & 1u) && pos)
                brk_hit(e, 2, 1, 0);
            else if (qb == 1u && !(in & 1u))
                brk_hit(e, 0, 1, 1);
            else if (qb == 2u)
                brk_hit(e, 0, 0, 3);
            else if (qb == 3u) {
                brk_hit(e, 1, 0, (int32_t)(4u + in * 4u / spb));
                if (pos + 2u >= bar)
                    e->rat = 2;
            }
        } else if (c == DV_HAT) {
            if (pos == 0u)
                brk_hit(e, 1, 0, 0);             /* (the downbeat, open) */
            else if (qb < 2u && in == spb / 2u)
                brk_hit(e, 2, 1, 0);
        } else {
            if (pos == 0u)
                brk_hit(e, 1, 0, 0);
            else if (qb == 3u && !(in & 1u))
                brk_hit(e, 1, 1, (int32_t)(7u - in));
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
    case BK_HALF:
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
            brk_hit(e, 1, 0, e->fire ? e->pitch : 0);
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
        e->pitch = (int8_t)(e->pitch / 2);
        break;
    }
    case BK_MIRROR: {
        uint32_t h = 2u * spb, p = s % h;
        brk_lane(c, c == DV_KICK && p == 0u ? s : s - p + (h - 1u - p), e);
        break;
    }
    case BK_TUMBLE: {
        uint32_t t = n % (2u * spb);
        if (c == DV_PERC)
            brk_hit(e, t == 0u, t + 1u < 2u * spb, 12 - (int32_t)(t * 24u / (2u * spb - 1u)));
        else if (c == DV_KICK)
            brk_lane(c, s, e);
        else if (c == DV_SNARE && t + 1u == 2u * spb)
            brk_hit(e, 1, 0, 0);
        else
            e->fire = 0;
        break;
    }
    case BK_INTERLOCK: {
        uint32_t lo = c == DV_KICK || c == DV_SNARE, half = spb > 1u ? spb / 2u : 1u;
        brk_lane(c, s, e);
        if (c == DV_KICK && pos == 0u)
            break;
        if ((n < bar ? (pos % spb) / half : (pos / spb) & 1u) == lo)   /* (the low voices the first half of a beat,
                                                  * the others the second; then whole beats: 1 and 3, 2 and 4) */
            e->fire = 0;
        else if (e->fire)
            e->acc = 1;
        break;
    }
    case BK_SCATTER:
        brk_lane(c, bar == 16u ? s - pos + P->ord[pos / 2u & 7u] * 2u + (pos & 1u) : s, e);
        break;
    case BK_BUZZ:
        brk_lane(c, s, e);
        if (e->fire && c != DV_KICK)
            e->rat = (uint8_t)(c == DV_HAT ? (n < bar ? 3u : 4u) : (n < bar ? 2u : 3u));
        break;
    case BK_GHOSTS:
        brk_lane(c, s, e);
        if (c == DV_HAT) {
            static const uint16_t A332 = 0x4949u;   /* (steps 0 3 6 8 11 14) */
            if (!e->fire)
                brk_hit(e, 2, 1, 0);
            else
                e->acc = (uint8_t)(bar == 16u && (A332 >> pos & 1u));
        } else if (c == DV_SNARE && !e->fire && (pos & 1u) && (brk_rnd() >> 31))
            brk_hit(e, 2, 1, 0);
        else if (c == DV_KICK && !e->fire && bar == 16u && (pos == 10u || pos == 14u))
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
/* a press: each lane's table, from the step its quantum falls on next; then the ISR is told (brk_id) */
static void brk_fill(void)
{
    uint32_t c, i, sb = 0, best = 0, bt, l;
    {                                            /* STUTTER's beat: the busiest of the kick's bar, every voice's hits */
        uint32_t spb = brk_spb(DV_KICK), st = dlr[DV_KICK].step == 0xFF || drm_reset ? 0u : dlr[DV_KICK].step, b0 = st - st % (4u * spb);
        for (bt = 0; bt < 4u; bt++) {
            uint32_t cnt = 0;
            for (l = 0; l < NDV; l++)
                for (i = 0; i < spb; i++)
                    cnt += drm.l[l].hits >> ((b0 + bt * spb + i) % (uint32_t)clampi(drm.l[l].len, 1, (int32_t)DSTEPS)) & 1u;
            if (cnt >= best) {                   /* (a tie: the later beat, nearer the bar's turn) */
                best = cnt;
                sb = bt;
            }
        }
    }
    for (c = 0; c < NDV; c++) {
        uint32_t spb = brk_spb(c), q = brk_plan.q == 8u ? 2u * spb : spb, len = (uint32_t)clampi(drm.l[c].len, 1, (int32_t)DSTEPS);
        uint32_t st = dlr[c].step, a = st == 0xFF || drm_reset ? 0u : (st / q + 1u) * q % len;   /* (from the top: 0) */
        brk_a[c] = (uint8_t)a;
        for (i = 0; i < BRK_TN; i++) {
            dhit_t e;
            brk_way(c, a, i, spb, sb, &e);
            brk_tab[c][i] = (uint16_t)(e.fire | (e.acc & 3u) << 1 | (e.alt & 1u) << 3 | (e.rat & 7u) << 4 | (e.flam & 3u) << 7 |
                                       (uint32_t)(clampi(e.pitch, -12, 12) + 12) << 9);
        }
    }
    __asm__ volatile("" ::: "memory");
    brk_id++;
}
/* ---- the ISR's: the hit lane c plays on its step s (of len; spb steps a beat) */
static FAST void brk_step(uint32_t c, uint32_t s, uint32_t len, uint32_t spb, dhit_t *e)
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
    if (!bks[c].on) {
        if (s % (brk_plan.q == 8u ? 2u * spb : spb)) {
            brk_written(c, s, e);
            return;
        }
        bks[c].on = 1;
        bks[c].n = (uint16_t)((s + len - brk_a[c]) % len);   /* (0, unless the press fell on the step before it) */
        if (bks[c].n >= 2u * spb)                /* (not where the table was made for: from its start) */
            bks[c].n = 0;
    } else if (bks[c].n < 60000u)
        bks[c].n++;
    i = bks[c].n < BRK_TN ? bks[c].n : BRK_TN / 2u + (bks[c].n - BRK_TN / 2u) % (BRK_TN / 2u);
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
