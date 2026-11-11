/* SPDX-License-Identifier: GPL-3.0-only
 * LILLIPUT: the DRUMS' recipes (build 16) -- their sounds (a kit: a recipe a voice), their rhythms, and what the
 * values mean to the voices (drm_coef: worked out in the main loop when a value moves). drums.c has the voices.
 *
 * A SOUND recipe is a voice's archetype -- a range for each value, drawn anew each time (SHIFT + a voice's white key
 * on the DRUMS pages): kicks deep, punchy, gabber-hard, soft (a slow attack), tom-like, sub, crunchy; snares snapping,
 * all noise, with a body, metallic, a rim, crushed, long; hats closed, open, metallic, a shaker, a tick, a cymbal,
 * dirty. Every voice has its DRIVE: this is rhythmic noise.
 *
 * A RHYTHM recipe draws the four lanes at once (SHIFT + the DRUMS key; PRESETS on their pages): POWER 4/4, PISTON,
 * MARCH, OFFBEAT, RUMBLE, BROKEN, 3 AGAINST 4, TRIBAL, SHUFFLE, DIST BREAK, HALF RITUAL, BURST FILL. As in Noir, steps
 * carry accents, ALT (the decay a fifth: closed hats among open ones, ghost snares) and pitches (toms from one kick). */

static const char *const DV_NAME[NDV] = {"KICK", "SNARE", "HATS", "PERC"};
static const char *const DN_NAME[DN_N] = {"LOW", "BAND", "HIGH"};
static const char *const DM_NAME[DM_N] = {"OFF", "1.41", "1.50", "2.76", "3.50", "5.40", "7.13"};
static const uint32_t DM_RAT[DM_N] = {0, 92682, 98304, 180879, 229376, 353894, 467272};   /* (Q16) */

/* ------------------------------------------------------------ the sounds --- */
/*                 TUNE     SWEEP    BEND     ATTACK   DECAY    NOISE    COLOR    CLICK    DRIVE    LEVEL    REVERB */
typedef struct {
    const char *name;
    uint8_t dv, filt, mlo, mhi;
    int16_t r[11][2];            /* each value's range, as dvoice_t has them (PAN: always the middle) */
} dkit_t;
static const dkit_t DKIT[] = {
    /* (TUNE: 20 Hz x 2^(v / 111) -- 130 ~45 Hz, 260 ~100 Hz, 370 ~200 Hz, 630 ~1 kHz, 910 ~6 kHz) */
    {"DEEP", DV_KICK, DN_LP, 0, 0,
     {{110, 150}, {300, 500}, {550, 700}, {0, 0}, {650, 780}, {0, 30}, {300, 500}, {100, 300}, {100, 300}, {750, 850}, {0, 100}}},
    {"PUNCH", DV_KICK, DN_LP, 0, 0,
     {{160, 200}, {450, 650}, {400, 520}, {0, 0}, {420, 560}, {0, 50}, {400, 600}, {400, 700}, {350, 550}, {750, 850}, {0, 80}}},
    {"GABBER", DV_KICK, DN_LP, 0, 0,
     {{170, 220}, {500, 750}, {450, 600}, {0, 0}, {450, 600}, {0, 60}, {400, 600}, {300, 600}, {800, 1000}, {620, 720}, {0, 60}}},
    {"SOFT", DV_KICK, DN_LP, 0, 0,
     {{130, 180}, {100, 300}, {500, 700}, {350, 550}, {550, 700}, {0, 40}, {300, 500}, {0, 0}, {0, 200}, {800, 900}, {0, 150}}},
    {"TOM", DV_KICK, DN_BP, 0, 1,
     {{230, 330}, {150, 350}, {550, 700}, {0, 100}, {450, 600}, {50, 150}, {500, 700}, {100, 300}, {200, 400}, {750, 850}, {50, 250}}},
    {"SUB", DV_KICK, DN_LP, 0, 0,
     {{90, 130}, {50, 200}, {600, 750}, {0, 80}, {750, 880}, {0, 20}, {300, 400}, {0, 100}, {50, 250}, {800, 900}, {0, 60}}},
    {"CRUNCH", DV_KICK, DN_BP, 0, 0,
     {{150, 210}, {600, 800}, {300, 450}, {0, 0}, {300, 450}, {100, 250}, {600, 800}, {600, 900}, {600, 800}, {650, 750}, {0, 80}}},
    {"SNAP", DV_SNARE, DN_BP, 0, 0,
     {{380, 430}, {100, 250}, {350, 500}, {0, 0}, {380, 480}, {600, 750}, {680, 780}, {300, 500}, {300, 500}, {700, 800}, {50, 200}}},
    {"NOISE", DV_SNARE, DN_BP, 0, 0,
     {{400, 450}, {0, 100}, {400, 500}, {0, 0}, {400, 520}, {850, 1000}, {650, 800}, {200, 400}, {200, 450}, {700, 800}, {50, 250}}},
    {"BODY", DV_SNARE, DN_BP, 0, 0,
     {{360, 420}, {200, 400}, {400, 550}, {0, 0}, {450, 580}, {400, 550}, {600, 720}, {100, 300}, {300, 500}, {720, 820}, {50, 200}}},
    {"METAL", DV_SNARE, DN_HP, 2, 4,
     {{420, 520}, {0, 100}, {400, 500}, {0, 0}, {400, 550}, {300, 500}, {750, 850}, {200, 400}, {300, 600}, {680, 780}, {100, 300}}},
    {"RIM", DV_SNARE, DN_BP, 0, 2,
     {{520, 600}, {0, 100}, {300, 400}, {0, 0}, {200, 300}, {150, 300}, {750, 850}, {500, 800}, {300, 500}, {680, 780}, {0, 150}}},
    {"CRUSH", DV_SNARE, DN_BP, 0, 0,
     {{380, 460}, {0, 200}, {350, 500}, {0, 0}, {350, 500}, {700, 900}, {600, 800}, {300, 600}, {800, 1000}, {600, 700}, {0, 150}}},
    {"LONG", DV_SNARE, DN_BP, 0, 0,
     {{380, 440}, {50, 200}, {400, 550}, {0, 100}, {600, 720}, {650, 800}, {650, 800}, {100, 300}, {200, 400}, {680, 780}, {300, 500}}},
    {"CLOSED", DV_HAT, DN_HP, 0, 0,
     {{800, 900}, {0, 0}, {300, 400}, {0, 0}, {220, 320}, {850, 1000}, {880, 950}, {100, 300}, {100, 300}, {600, 700}, {0, 120}}},
    {"OPEN", DV_HAT, DN_HP, 0, 0,
     {{800, 900}, {0, 0}, {300, 400}, {0, 0}, {500, 620}, {850, 1000}, {860, 940}, {50, 200}, {100, 300}, {580, 680}, {50, 250}}},
    {"METAL", DV_HAT, DN_HP, 4, 6,
     {{650, 760}, {0, 50}, {300, 400}, {0, 0}, {280, 450}, {300, 500}, {850, 930}, {100, 300}, {200, 400}, {580, 680}, {50, 200}}},
    {"SHAKER", DV_HAT, DN_BP, 0, 0,
     {{800, 900}, {0, 0}, {300, 400}, {250, 400}, {300, 400}, {1000, 1000}, {800, 880}, {0, 100}, {0, 200}, {620, 720}, {0, 150}}},
    {"TICK", DV_HAT, DN_HP, 0, 0,
     {{850, 950}, {0, 0}, {300, 400}, {0, 0}, {120, 200}, {700, 900}, {900, 980}, {600, 900}, {200, 400}, {620, 720}, {0, 100}}},
    {"CYMBAL", DV_HAT, DN_HP, 3, 6,
     {{700, 800}, {0, 50}, {300, 400}, {0, 50}, {650, 780}, {500, 700}, {850, 950}, {100, 300}, {100, 300}, {550, 650}, {200, 400}}},
    {"DIRT", DV_HAT, DN_HP, 0, 0,
     {{760, 860}, {0, 100}, {300, 400}, {0, 0}, {250, 400}, {800, 1000}, {850, 950}, {200, 500}, {700, 950}, {520, 620}, {0, 150}}},
    /* PERC (build 17): struck metal for rhythmic noise -- anvils, pipes, sheets, clanks, chains, springs, grinders */
    {"ANVIL", DV_PERC, DN_HP, 5, 6,
     {{620, 700}, {0, 80}, {300, 400}, {0, 0}, {300, 420}, {100, 250}, {800, 900}, {600, 900}, {500, 750}, {620, 720}, {50, 200}}},
    {"PIPE", DV_PERC, DN_BP, 1, 3,
     {{480, 560}, {0, 100}, {300, 400}, {0, 0}, {400, 520}, {0, 100}, {600, 750}, {200, 400}, {200, 400}, {650, 750}, {150, 300}}},
    {"SHEET", DV_PERC, DN_BP, 4, 6,
     {{560, 660}, {0, 100}, {300, 400}, {0, 50}, {550, 700}, {400, 600}, {750, 850}, {200, 500}, {300, 600}, {580, 680}, {200, 400}}},
    {"CLANK", DV_PERC, DN_BP, 3, 5,
     {{300, 380}, {100, 250}, {350, 450}, {0, 0}, {280, 380}, {100, 200}, {600, 750}, {500, 800}, {600, 850}, {620, 720}, {50, 200}}},
    {"CHAIN", DV_PERC, DN_HP, 5, 6,
     {{750, 850}, {0, 50}, {300, 400}, {0, 0}, {200, 300}, {500, 700}, {850, 930}, {300, 600}, {400, 700}, {580, 680}, {50, 150}}},
    {"SPRING", DV_PERC, DN_BP, 2, 4,
     {{420, 520}, {400, 600}, {600, 750}, {0, 0}, {400, 550}, {50, 200}, {600, 750}, {100, 300}, {300, 500}, {620, 720}, {100, 300}}},
    {"GRIND", DV_PERC, DN_BP, 0, 2,
     {{350, 450}, {0, 150}, {300, 450}, {0, 50}, {300, 450}, {800, 1000}, {600, 750}, {400, 700}, {850, 1000}, {560, 660}, {0, 150}}},
};
#define NDKIT NELEM(DKIT)

/* a voice's values from its recipe (kit; -1: any of the voice's), drawn from seed */
static uint32_t drm_draw_voice(uint32_t c, uint32_t seed, int kit)
{
    rnd_t r;
    uint32_t i, n = 0, ix[NDKIT];
    dvoice_t *v = &drm.v[c];
    const dkit_t *k;
    int16_t *f[11] = {&v->tune, &v->sweep, &v->bend, &v->attack, &v->decay, &v->noise, &v->color, &v->click,
                      &v->drive, &v->level, &v->verb};
    rnd_seed(&r, seed);
    for (i = 0; i < NDKIT; i++)
        if (DKIT[i].dv == c)
            ix[n++] = i;
    if (kit < 0 || kit >= (int)NDKIT || DKIT[kit].dv != c)
        kit = (int)ix[rnd_below(&r, (int32_t)n)];
    k = &DKIT[kit];
    for (i = 0; i < 11u; i++)
        *f[i] = (int16_t)pr_in(&r, k->r[i][0], k->r[i][1]);
    v->pan = 0;
    v->filt = k->filt;
    v->metal = (uint8_t)pr_in(&r, k->mlo, k->mhi);
    drm.kit[c] = (uint8_t)kit;
    return (uint32_t)kit;
}

/* ----------------------------------------------------------- the rhythms --- */
/* (build 21: anew -- away from four on the floor. Each style a way of building a rhythm, drawn afresh from the
 * seed, so no two draws are alike: odd meters added up, cells phasing against themselves, Euclidean lanes of
 * unequal lengths, triplets against quintuplets against sixteenths, timelines, nested cycles, bouncing balls,
 * canons in one tempo and in several, hockets. Lanes of different lengths drift against each other (polymeter);
 * lanes of different step lengths run in different time (polyrhythm). And VARIATION (drm.vary): now and then a
 * lane's round is played otherwise -- a hit dropped, a pick-up, a step pushed, the lane turned, a round's rest --
 * and the next comes home (drm_evolve). Sources: Toussaint's "The Geometry of Musical Rhythm" (Euclidean rhythms,
 * timelines), Reich's phasing, Nancarrow's tempo canons, the gamelan's colotomic cycles and kotekan, aksak meters,
 * Bigorna's phrase.c) */
enum { DS_AKSAK, DS_PHASE, DS_MESH, DS_POLY, DS_TIMELINE, DS_COLOTOMIC, DS_CASCADE, DS_LIMP, DS_DUST, DS_CANON,
       DS_HOCKET, DS_RITUAL, DS_BROKEN, DS_TRIBAL, DS_DBREAK, DS_TCANON, DS_N };
static const char *const DS_NAME[DS_N] = {"AKSAK", "PHASING", "EUCLID MESH", "POLYRHYTHM", "TIMELINE", "COLOTOMIC",
                                          "CASCADE", "LIMP", "DUST", "CANON", "HOCKET", "RITUAL", "BROKEN", "TRIBAL",
                                          "DIST BREAK", "TEMPO CANON"};
/* a lane written out: x a hit, X accented, o a hit with ALT (short), O both, . a rest; a sixteenth a step */
static void dl_set(uint32_t c, const char *s, uint8_t prob)
{
    dlane_t *L = &drm.l[c];
    uint32_t i;
    memset(L, 0, sizeof *L);
    for (i = 0; s[i] && i < DSTEPS; i++) {
        char ch = s[i];
        if (ch == 'x' || ch == 'X' || ch == 'o' || ch == 'O')
            L->hits |= 1u << i;
        if (ch == 'X' || ch == 'O')
            L->acc |= 1u << i;
        if (ch == 'o' || ch == 'O')
            L->alt |= 1u << i;
    }
    L->len = (uint8_t)(i ? i : 1u);
    L->div = DV_16;
    L->prob = prob;
    L->var = 30;
}
AINL uint32_t dmask(uint32_t n) { return n >= 32u ? 0xFFFFFFFFu : (1u << n) - 1u; }
/* a lane from its masks: n steps of div; its chance; how readily it varies (of drm.vary, %) */
static void dl_bits(uint32_t c, uint32_t n, uint32_t div, uint32_t hits, uint32_t acc, uint32_t alt, uint8_t prob,
                    uint8_t var)
{
    dlane_t *L = &drm.l[c];
    memset(L, 0, sizeof *L);
    n = n < 1u ? 1u : n > DSTEPS ? DSTEPS : n;
    L->len = (uint8_t)n;
    L->div = (uint8_t)div;
    L->hits = hits & dmask(n);
    L->acc = acc & L->hits;
    L->alt = alt & L->hits;
    L->prob = prob;
    L->var = var;
}
static uint32_t brot(uint32_t m, uint32_t n, uint32_t k)   /* n steps turned k later */
{
    k %= n;
    return k ? ((m << k) | (m >> (n - k))) & dmask(n) : m & dmask(n);
}
static uint32_t bsome(rnd_t *r, uint32_t m, uint32_t k)    /* k of m's hits, at random */
{
    uint32_t o = 0, n = (uint32_t)__builtin_popcount(m);
    while (k-- && n) {
        uint32_t j = (uint32_t)rnd_below(r, (int32_t)n), b = m;
        while (j--)
            b &= b - 1u;
        b &= ~(b - 1u);                          /* (the j-th hit) */
        o |= b;
        m &= ~b;
        n--;
    }
    return o;
}
/* each hit of lane c a pitch from the set, walking (a step up or down the set, now and then a leap) */
static void dl_tune(uint32_t c, rnd_t *r, const int8_t *set, uint32_t ns)
{
    dlane_t *L = &drm.l[c];
    uint32_t i, at = (uint32_t)rnd_below(r, (int32_t)ns);
    for (i = 0; i < L->len; i++) {
        if (!(L->hits >> i & 1u))
            continue;
        L->pitch[i] = set[at];
        if (rnd_chance(r, 20))
            at = (uint32_t)rnd_below(r, (int32_t)ns);
        else
            at = (at + ns + (rnd_chance(r, 50) ? 1u : ns - 1u)) % ns;
    }
}
static const int8_t SC_PENT[6] = {-5, 0, 3, 5, 7, 10};
static const int8_t SC_PELOG[6] = {-12, -11, -9, -5, 0, 1};
static const int8_t SC_LOW[5] = {-12, -7, -5, 0, 3};
/* groups added up to an odd meter: their sizes, 2 or 3 */
static const char *const AKSAK_G[] = {"32", "23", "223", "322", "232", "2223", "3222", "333", "3332", "22322",
                                      "33322", "32323", "2232", "3223"};
static rnd_t dvr_r = {0x9E3779B9u};           /* (VARIATION's draws: drm_evolve) */
#define PICK(...) ((const char *const[]){__VA_ARGS__})[rnd_below(&r, (int32_t)(sizeof((const char *const[]){__VA_ARGS__}) / sizeof(const char *)))]
/* new lanes for the four voices, of style st (-1: any), from seed */
static uint32_t drm_draw_rhythm(uint32_t seed, int st)
{
    rnd_t r;
    uint32_t i, c;
    rnd_seed(&r, seed);
    rnd_seed(&dvr_r, seed * 2654435761u + 7u);   /* (its VARIATION's draws: from the same seed) */
    if (st < 0 || st >= DS_N)
        st = rnd_below(&r, DS_N);
    drm.swing = 0;
    drm.vary = (int16_t)pr_in(&r, 20, 40);
    switch (st) {
    case DS_AKSAK: {                             /* groups of 2 and 3 added up: 5, 7, 9, 11, 13 -- once, or twice with a
                                                  * changed second half */
        const char *g = AKSAK_G[rnd_below(&r, NELEM(AKSAK_G))];
        uint32_t starts = 0, threes = 0, n = 0, rep, k;
        for (i = 0; g[i]; i++) {
            starts |= 1u << n;
            if (g[i] == '3')
                threes |= 1u << (n + 2u);
            n += (uint32_t)(g[i] - '0');
        }
        rep = n <= 7u && rnd_chance(&r, 60) ? 2u : 1u;   /* (a short meter twice: room for a second half) */
        for (k = 1; k < rep; k++) {
            starts |= starts << n;
            threes |= threes << n;
        }
        n *= rep;
        {
            uint32_t kick = 1u | bsome(&r, starts & ~1u, (uint32_t)__builtin_popcount(starts) / 3u);
            uint32_t late = starts & ~dmask(n / 2u), sn = late ? bsome(&r, late, 1u + (uint32_t)rnd_chance(&r, 30)) : 0;
            dl_bits(DV_KICK, n, DV_16, kick, 1u, 0, 100, 20);
            dl_bits(DV_SNARE, n, DV_16, sn | bsome(&r, ~starts & ~kick & dmask(n), (uint32_t)rnd_below(&r, 3)), sn, ~sn, 100, 35);
            if (rnd_chance(&r, 30))              /* (the hats in 16 against it: the meter shears) */
                dl_bits(DV_HAT, 16, DV_16, euclid((uint32_t)pr_in(&r, 5, 11), 16, 0), euclid(3, 16, 0), 0, 95, 45);
            else
                dl_bits(DV_HAT, n, DV_16, dmask(n) & ~(rnd_chance(&r, 50) ? starts : 0u), starts, ~starts, (uint8_t)pr_in(&r, 80, 100), 45);
            dl_bits(DV_PERC, n, DV_16, threes | bsome(&r, starts, 1u), threes, 0, 100, 50);
            dl_tune(DV_PERC, &r, SC_PENT, 6);
        }
        break;
    }
    case DS_PHASE: {                             /* one cell on two voices, one a step longer: they drift apart and
                                                  * meet again (Reich's phasing) */
        uint32_t n = (uint32_t)pr_in(&r, 6, 12), cell = 1u | euclid((uint32_t)pr_in(&r, 3, (int32_t)n - 2), n, (uint32_t)rnd_below(&r, (int32_t)n));
        uint32_t acc = bsome(&r, cell, 2u);
        dl_bits(DV_HAT, n, DV_16, cell, acc, ~acc, 100, 15);
        dl_bits(DV_PERC, n + 1u, DV_16, cell, acc, 0, 100, 15);
        dl_tune(DV_PERC, &r, SC_PENT, 6);
        dl_bits(DV_KICK, n * 2u <= 32u ? n * 2u : n, DV_16, 1u | bsome(&r, cell << n, 1u), 1u, 0, 100, 15);
        dl_bits(DV_SNARE, n + 2u + (uint32_t)rnd_below(&r, 3), DV_16, 1u << (n / 2u), 1u << (n / 2u), 0, 100, 25);
        drm.vary = (int16_t)pr_in(&r, 10, 25);
        break;
    }
    case DS_MESH: {                              /* Euclidean lanes of unequal lengths, each turned its own way */
        static const uint8_t LN[] = {5, 7, 8, 9, 10, 11, 12, 13, 14, 16};
        uint32_t n[NDV], used = 0;
        for (c = 0; c < NDV; c++) {
            do
                n[c] = LN[rnd_below(&r, NELEM(LN))];
            while (used >> n[c] & 1u);
            used |= 1u << n[c];
        }
        dl_bits(DV_KICK, n[0], DV_16, euclid((uint32_t)pr_in(&r, 2, (int32_t)n[0] / 3), n[0], 0), 1u, 0, 100, 15);
        dl_bits(DV_SNARE, n[1], DV_16, euclid((uint32_t)pr_in(&r, 1, 3), n[1], (uint32_t)pr_in(&r, 2, (int32_t)n[1] - 1)), 0xFFFFFFFFu, 0, 100, 25);
        dl_bits(DV_HAT, n[2], rnd_chance(&r, 20) ? DV_32 : DV_16, euclid((uint32_t)pr_in(&r, (int32_t)n[2] / 2, (int32_t)n[2] - 1), n[2], (uint32_t)rnd_below(&r, (int32_t)n[2])),
                euclid(2, n[2], 0), euclid(3, n[2], 1), 95, 40);
        dl_bits(DV_PERC, n[3], DV_16, euclid((uint32_t)pr_in(&r, 2, (int32_t)n[3] / 2), n[3], (uint32_t)rnd_below(&r, (int32_t)n[3])), euclid(1, n[3], 0), 0, 100, 40);
        dl_tune(DV_PERC, &r, rnd_chance(&r, 50) ? SC_PENT : SC_PELOG, 6);
        break;
    }
    case DS_POLY: {                              /* twos, threes and fives at once: each voice its own subdivision */
        static const uint8_t DV3[4][4] = {       /* kick, snare, hats, perc: their step lengths */
            {DV_8T, DV_4, DV_16Q, DV_16}, {DV_4, DV_8T, DV_16, DV_16Q}, {DV_8, DV_8T, DV_16Q, DV_16T},
            {DV_8T, DV_8, DV_16T, DV_16Q}};
        static const uint8_t PER[NDIV] = {2, 4, 8, 6, 16, 20, 12, 32};   /* (steps a bar) */
        const uint8_t *d = DV3[rnd_below(&r, 4)];
        uint32_t nk = PER[d[0]], ns = PER[d[1]], nh = PER[d[2]], np = PER[d[3]];
        dl_bits(DV_KICK, nk, d[0], euclid((uint32_t)pr_in(&r, 2, (int32_t)nk / 2), nk, 0), 1u, 0, 100, 15);
        dl_bits(DV_SNARE, ns, d[1], euclid((uint32_t)pr_in(&r, 1, 2), ns, (uint32_t)pr_in(&r, 1, (int32_t)ns - 1)), 0xFFFFFFFFu, 0, 100, 25);
        dl_bits(DV_HAT, nh, d[2], euclid((uint32_t)pr_in(&r, (int32_t)nh / 3, (int32_t)nh * 2 / 3), nh, (uint32_t)rnd_below(&r, 3)),
                nh % 5u == 0 ? euclid(nh / 5u, nh, 0) : euclid(nh / 4u, nh, 0), 0, 95, 40);
        dl_bits(DV_PERC, np, d[3], euclid((uint32_t)pr_in(&r, 2, (int32_t)np / 3), np, (uint32_t)rnd_below(&r, (int32_t)np)), 0, 0, 100, 40);
        dl_tune(DV_PERC, &r, SC_PENT, 6);
        break;
    }
    case DS_TIMELINE: {                          /* a bell's timeline -- the 12/8 bell, clave, tresillo, gahu, bossa,
                                                  * shiko, soukous (Toussaint's) -- turned, against a pulse of the
                                                  * other kind */
        static const char *const T12[] = {"x.x.xx.x.x.x", "x.x.x.xx.x.x", "x.xx.x.x.xx.", "x..x.x..x.x."};
        static const char *const T16[] = {"x..x..x...x.x...", "x..x...x..x.x...", "x..x..x...x..x..", "x..x..x...x...x.",
                                          "x...x.x...x.x...", "x..x..x...xx....", "x..x..x.x..x..x."};
        int twelve = rnd_chance(&r, 50);
        const char *t = twelve ? T12[rnd_below(&r, NELEM(T12))] : T16[rnd_below(&r, NELEM(T16))];
        uint32_t n = (uint32_t)str_len(t), tl = 0, rt, bell;
        for (i = 0; i < n; i++)
            tl |= (uint32_t)(t[i] == 'x') << i;
        rt = rnd_chance(&r, 40) ? (uint32_t)rnd_below(&r, (int32_t)n) : 0u;   /* (another timeline: the same, turned) */
        tl = brot(tl, n, rt);
        bell = rnd_chance(&r, 60) ? DV_PERC : DV_HAT;
        dl_bits(bell, n, twelve ? DV_8T : DV_16, tl, tl & 1u, 0, 100, 10);
        if (bell == DV_PERC) {                   /* (the bell on PERC: the hats the other pulse) */
            for (i = 0; i < 16u; i++)
                drm.l[DV_PERC].pitch[i] = (int8_t)(rnd_chance(&r, 25) ? 7 : 0);
            dl_bits(DV_HAT, twelve ? 16u : 12u, twelve ? DV_16 : DV_8T, euclid(twelve ? 8u : 6u, twelve ? 16u : 12u, 1), 0, 0xFFFFFFFFu, 85, 45);
        } else
            dl_bits(DV_PERC, twelve ? 16u : 12u, twelve ? DV_16 : DV_8T, euclid((uint32_t)pr_in(&r, 2, 4), twelve ? 16u : 12u, 2), 0, 0, 100, 40);
        dl_bits(DV_KICK, n, twelve ? DV_8T : DV_16, 1u | bsome(&r, tl & ~1u, (uint32_t)pr_in(&r, 0, 2)), 1u, 0, 100, 20);
        dl_bits(DV_SNARE, n, twelve ? DV_8T : DV_16, bsome(&r, ~tl & dmask(n) & ~dmask(n / 3u), (uint32_t)pr_in(&r, 1, 2)), 0xFFFFFFFFu, 0, 100, 30);
        break;
    }
    case DS_COLOTOMIC: {                         /* cycles in cycles (the gamelan's): a gong, a gong's half, ticks
                                                  * between; the hats and snare interlocking (kotekan) */
        uint32_t n = rnd_chance(&r, 50) ? 32u : 16u, ken = 0, pol;
        for (i = 4; i < n; i += 4)
            ken |= 1u << i;
        dl_bits(DV_KICK, n, DV_8, 1u | (rnd_chance(&r, 40) ? 1u << (n / 2u) : 0u), 1u, 0, 100, 5);
        dl_bits(DV_PERC, n, DV_8, ken & ~(1u << (n / 2u)), 0, 0, 100, 20);
        dl_tune(DV_PERC, &r, SC_PELOG, 6);
        pol = euclid((uint32_t)pr_in(&r, 3, 5), 8, (uint32_t)rnd_below(&r, 8));
        dl_bits(DV_HAT, 8, DV_16, pol, pol & 0x11u, 0, 100, 40);
        dl_bits(DV_SNARE, 8, DV_16, ~pol & 0xFFu & ~1u, 0, 0xFFFFFFFFu, (uint8_t)pr_in(&r, 60, 90), 40);
        drm.vary = (int16_t)pr_in(&r, 15, 30);
        break;
    }
    case DS_CASCADE: {                           /* a bouncing ball: hits ever closer, the snare's ever farther */
        uint32_t k = 0, s = 0, at = 0, gap = (uint32_t)pr_in(&r, 6, 9), sn = 0;
        int32_t p = 12;
        dl_bits(DV_KICK, 32, DV_16, 0, 1u, 0, 100, 10);
        dl_bits(DV_PERC, 32, DV_16, 0, 0, 0, 100, 10);
        for (s = 0; at < 32u && s < 4u; ) {     /* (once the gap is a step: four more, then the ball lies still) */
            k |= 1u << at;
            drm.l[DV_PERC].pitch[at] = (int8_t)p;
            p = p > -12 ? p - 3 : -12;
            s += gap == 1u;
            at += gap;
            gap = gap > 1u ? (gap * 7u + 5u) / 10u : 1u;
        }
        s = 0;
        for (at = 31u, gap = 1u; s < 6u && at > 0u && at < 32u; s++, gap += (uint32_t)pr_in(&r, 1, 3)) {
            sn |= 1u << at;
            at = at > gap ? at - gap : 0u;
        }
        drm.l[DV_KICK].hits = k & 0x0000FFFFu ? k & 0x0000FFFFu : 1u;
        drm.l[DV_KICK].acc = 1u;
        drm.l[DV_PERC].hits = k;
        drm.l[DV_PERC].acc = 1u;
        dl_bits(DV_SNARE, 32, DV_16, sn & ~1u, 1u << 31, 0xFFFFFFFFu & ~(1u << 31), 100, 10);
        dl_bits(DV_HAT, 32, DV_16, euclid((uint32_t)pr_in(&r, 5, 9), 32, 1), 0, 0xFFFFFFFFu, 80, 30);
        drm.vary = (int16_t)pr_in(&r, 10, 20);
        break;
    }
    case DS_LIMP: {                              /* a long odd bar -- 5/4, 7/4, 9/8 in sixteenths -- the backbeat late, a
                                                  * beat missing */
        static const uint8_t LN[] = {18, 20, 22, 26, 28};
        uint32_t n = LN[rnd_below(&r, NELEM(LN))], h = 0, sn;
        for (i = 0; i < n; i += 2)
            h |= 1u << i;
        h &= ~(1u << (n - 2u - 2u * (uint32_t)rnd_below(&r, 3)));   /* (the limp: an eighth left out) */
        sn = 1u << (n / 2u + (uint32_t)pr_in(&r, 0, 2));
        dl_bits(DV_KICK, n, DV_16, 1u | bsome(&r, euclid((uint32_t)pr_in(&r, 3, 5), n, 3) & ~dmask(1), (uint32_t)pr_in(&r, 1, 3)), 1u, 0, 100, 25);
        dl_bits(DV_SNARE, n, DV_16, sn | (rnd_chance(&r, 50) ? sn >> 3 : 0u), sn, sn >> 3, 100, 30);
        dl_bits(DV_HAT, n, DV_16, h, euclid(3, n, 0) & h, 0, 95, 40);
        dl_bits(DV_PERC, n, DV_16, euclid((uint32_t)pr_in(&r, 2, 4), n, (uint32_t)rnd_below(&r, (int32_t)n)), 0, 0, 100, 45);
        dl_tune(DV_PERC, &r, SC_LOW, 5);
        drm.swing = (uint8_t)pr_in(&r, 0, 12);
        break;
    }
    case DS_DUST: {                              /* a cloud: lanes of uneven lengths, every hit a chance */
        uint32_t nk = (uint32_t)pr_in(&r, 13, 32), ns = (uint32_t)pr_in(&r, 9, 24), nh = (uint32_t)pr_in(&r, 17, 32), np = (uint32_t)pr_in(&r, 7, 24);
        dl_bits(DV_KICK, nk, DV_16, 1u | euclid((uint32_t)pr_in(&r, 3, 6), nk, (uint32_t)rnd_below(&r, (int32_t)nk)), 1u, 0, (uint8_t)pr_in(&r, 45, 70), 30);
        dl_bits(DV_SNARE, ns, DV_16, euclid((uint32_t)pr_in(&r, 2, 5), ns, (uint32_t)rnd_below(&r, (int32_t)ns)), euclid(1, ns, 0), 0xFFFFFFFFu, (uint8_t)pr_in(&r, 35, 60), 40);
        dl_bits(DV_HAT, nh, rnd_chance(&r, 50) ? DV_32 : DV_16, euclid((uint32_t)pr_in(&r, (int32_t)nh / 2, (int32_t)nh - 2), nh, 0), euclid(3, nh, 1), 0, (uint8_t)pr_in(&r, 45, 75), 60);
        dl_bits(DV_PERC, np, DV_16T, euclid((uint32_t)pr_in(&r, 3, (int32_t)np / 2), np, (uint32_t)rnd_below(&r, (int32_t)np)), 0, 0, (uint8_t)pr_in(&r, 35, 65), 60);
        dl_tune(DV_PERC, &r, SC_PELOG, 6);
        drm.vary = (int16_t)pr_in(&r, 35, 55);
        break;
    }
    case DS_CANON: {                             /* one figure, each voice entering it at another place */
        uint32_t n = rnd_chance(&r, 50) ? 12u : 16u, cell = 1u | euclid((uint32_t)pr_in(&r, 4, 7), n, (uint32_t)rnd_below(&r, (int32_t)n));
        uint32_t o1 = (uint32_t)pr_in(&r, 2, (int32_t)n / 2), o2 = o1 + (uint32_t)pr_in(&r, 1, 3), o3 = (uint32_t)pr_in(&r, 1, (int32_t)n - 1);
        uint32_t acc = bsome(&r, cell, 2u) | 1u;
        dl_bits(DV_KICK, n, DV_16, cell & ~bsome(&r, cell & ~1u, (uint32_t)__builtin_popcount(cell) / 2u), acc, 0, 100, 20);
        dl_bits(DV_SNARE, n, DV_16, bsome(&r, brot(cell, n, o1), 2u), 0xFFFFFFFFu, 0, 100, 30);
        dl_bits(DV_HAT, n, DV_16, brot(cell, n, o2), brot(acc, n, o2), 0, 100, 30);
        dl_bits(DV_PERC, n, DV_16, brot(cell, n, o3), brot(acc, n, o3), 0, 100, 35);
        dl_tune(DV_PERC, &r, SC_PENT, 6);
        for (i = 0; i < n; i++)                  /* (the kick's toms: the perc's line, an octave down) */
            drm.l[DV_KICK].pitch[i] = (int8_t)clampi(drm.l[DV_PERC].pitch[(i + o3) % n] - 12, -12, 12);
        break;
    }
    case DS_HOCKET: {                            /* one dense line shared out: no two voices on one step */
        static const uint8_t LN[] = {12, 14, 15, 16, 18, 20};
        uint32_t n = LN[rnd_below(&r, NELEM(LN))], line = 1u | euclid((uint32_t)pr_in(&r, (int32_t)n * 6 / 10, (int32_t)n * 85 / 100), n, 0), m[NDV] = {0};
        static const uint8_t W[NDV] = {20, 25, 25, 30};   /* (each voice's share) */
        for (i = 0; i < n; i++)
            if (line >> i & 1u) {
                uint32_t v = 0, t = (uint32_t)rnd_below(&r, 100);
                if (i == 0u)
                    v = DV_KICK;
                else
                    while (v < NDV - 1u && t >= W[v])
                        t -= W[v++];
                m[v] |= 1u << i;
            }
        for (c = 0; c < NDV; c++)
            dl_bits(c, n, DV_16, m[c], euclid(3, n, 0), 0, 100, 30);
        dl_tune(DV_PERC, &r, SC_PENT, 6);
        dl_tune(DV_KICK, &r, SC_LOW, 5);
        break;
    }
    case DS_RITUAL: {                            /* half time, slow: a kick, a snare far off, triplet hats */
        dl_bits(DV_KICK, 32, DV_16, 1u | (rnd_chance(&r, 60) ? 1u << pr_in(&r, 9, 13) : 0u), 1u, 0, 100, 15);
        dl_bits(DV_SNARE, 32, DV_16, 1u << (rnd_chance(&r, 50) ? 16 : 24), 0xFFFFFFFFu, 0, 100, 20);
        dl_bits(DV_HAT, 12, DV_8T, euclid((uint32_t)pr_in(&r, 4, 7), 12, (uint32_t)rnd_below(&r, 12)), 0, 0xFFFFFFFFu, 85, 40);
        dl_bits(DV_PERC, (uint32_t)pr_in(&r, 20, 28), DV_16, euclid((uint32_t)pr_in(&r, 1, 3), 24, 5), 0, 0, 100, 40);
        dl_tune(DV_PERC, &r, SC_LOW, 5);
        break;
    }
    case DS_BROKEN: {                            /* kicks off the beat, the backbeat pushed or pulled, ghosts */
        uint32_t n = rnd_chance(&r, 50) ? 32u : 16u, k = 1u, sn = 0, b;
        for (b = 0; b < n; b += 16u) {
            uint32_t off = (uint32_t)pr_in(&r, 2, 3);
            k |= bsome(&r, (0x2A6Cu & ~0x1111u) << b, off);   /* (syncopated places: none on a beat but the first) */
            sn |= 1u << (b + 4u + (uint32_t)(rnd_chance(&r, 30) ? 1 : 0));
            sn |= 1u << (b + 12u - (uint32_t)(rnd_chance(&r, 30) ? 1 : 0));
        }
        dl_bits(DV_KICK, n, DV_16, k, 1u, 0, 100, 25);
        dl_bits(DV_SNARE, n, DV_16, sn | bsome(&r, ~sn & ~k & dmask(n), (uint32_t)pr_in(&r, 1, 3)), sn, ~sn, 100, 35);
        dl_bits(DV_HAT, n, DV_16, dmask(n) & ~bsome(&r, dmask(n), n / 4u), 0x44444444u, 0x11111111u, 85, 45);
        dl_bits(DV_PERC, n, DV_16, euclid((uint32_t)pr_in(&r, 2, 5), n, (uint32_t)rnd_below(&r, (int32_t)n)), 0, 0, 100, 45);
        dl_tune(DV_PERC, &r, SC_PENT, 6);
        drm.swing = (uint8_t)pr_in(&r, 0, 15);
        break;
    }
    case DS_TRIBAL: {                            /* pitched toms out of the kick, Euclidean; an anvil three against four */
        uint32_t k = 1u | euclid((uint32_t)pr_in(&r, 5, 7), 16, (uint32_t)rnd_below(&r, 16));
        dl_bits(DV_KICK, 16, DV_16, k, bsome(&r, k, 2u) | 1u, 0, 100, 30);
        dl_tune(DV_KICK, &r, SC_LOW, 5);
        dl_bits(DV_SNARE, 16, DV_16, bsome(&r, ~k & 0xFFF0u, (uint32_t)pr_in(&r, 1, 2)), 0xFFFFFFFFu, 0, 100, 30);
        dl_bits(DV_HAT, rnd_chance(&r, 50) ? 12u : 16u, DV_16, euclid((uint32_t)pr_in(&r, 5, 9), 16, 0), euclid(2, 16, 0), 0xFFFFFFFFu, 90, 40);
        dl_bits(DV_PERC, 12, DV_16, euclid(4, 12, 0) | (rnd_chance(&r, 40) ? euclid(1, 12, 7) : 0u), 1u, 0, 100, 35);
        drm.swing = (uint8_t)pr_in(&r, 0, 10);
        break;
    }
    case DS_DBREAK:                              /* a distorted break, straight, never swung */
        dl_set(DV_KICK, PICK("X.x.......xx....", "X.x.......X.....", "X.......x.x....."), 100);
        dl_set(DV_SNARE, PICK("....X..o.o..X..o", "....X......oX...", "....X.o..o..X.oo"), 75);
        dl_set(DV_HAT, PICK("x.x.x.x.x.x.x.x.", "x.xox.x.x.xox.x."), 100);
        dl_set(DV_PERC, PICK("..............OO", "......o.......o."), 100);
        break;
    default: {                                   /* TEMPO CANON (Nancarrow's): one figure in several tempi at once --
                                                  * eighths, triplets, sixteenths, quintuplets -- meeting now and then */
        uint32_t n = (uint32_t)pr_in(&r, 5, 9), cell = 1u | euclid((uint32_t)pr_in(&r, 2, (int32_t)n / 2 + 1), n, (uint32_t)rnd_below(&r, (int32_t)n));
        static const uint8_t TD[NDV] = {DV_8, DV_8T, DV_16, DV_16Q};
        uint32_t sh = (uint32_t)rnd_below(&r, 4);
        for (c = 0; c < NDV; c++)
            dl_bits(c, n, TD[(c + sh) % NDV], c == DV_KICK ? cell & ~bsome(&r, cell & ~1u, 1u) : cell, 1u, 0, 100, 15);
        dl_tune(DV_PERC, &r, SC_PENT, 6);
        drm.vary = (int16_t)pr_in(&r, 10, 20);
        break;
    }
    }
    drm.style = (uint8_t)st;
    return (uint32_t)st;
}
#undef PICK

/* ------------------------------------------------------- VARIATION --- *
 * (build 21, the main loop's) each time a lane comes round, by VARIATION's chance (the lane's share of it), its
 * next round is played otherwise -- from its own pattern (home), never from the last variant, so it never wanders
 * off: a hit dropped, a pick-up before a hit, a hit pushed a step, an accent moved, a hit doubled, the lane turned,
 * its second half backwards, or a round's rest. A new rhythm, a session, an edit: the new pattern is home */
static struct {
    uint32_t hits, acc, alt;     /* home */
    uint32_t ph, pa, pl;         /* what was last written (else: someone else wrote it -- the new home) */
    uint8_t last;                /* the step it was on */
} dvr[NDV];
static uint32_t dvr_free(uint32_t m, uint32_t n) { return ~m & dmask(n); }
static void drm_vary_lane(uint32_t c)
{
    dlane_t *L = &drm.l[c];
    uint32_t n = L->len ? L->len : 1u, h = dvr[c].hits, a = dvr[c].acc, l = dvr[c].alt, op, b, t;
    uint32_t pct = (uint32_t)clampi(drm.vary, 0, 100) * L->var / 100u;
    if (drm.on && pct && rnd_chance(&dvr_r, (int32_t)pct) && h) {
        op = (uint32_t)rnd_below(&dvr_r, 100);
        if (op < 20) {                           /* a hit dropped (not the first, not an accent) */
            b = bsome(&dvr_r, h & ~a & ~1u, 1u);
            h &= ~b;
        } else if (op < 40) {                    /* a pick-up: a short hit just before one */
            b = bsome(&dvr_r, h, 1u);
            t = brot(b, n, n - 1u);
            if (t & dvr_free(h, n)) {
                h |= t;
                l |= t;
                a &= ~t;
            }
        } else if (op < 55) {                    /* a hit pushed or pulled a step */
            b = bsome(&dvr_r, h & ~1u, 1u);
            t = brot(b, n, rnd_chance(&dvr_r, 50) ? 1u : n - 1u);
            if (b && (t & dvr_free(h, n))) {
                h = (h & ~b) | t;
                a = (a & b) ? (a & ~b) | t : a;
                l = (l & b) ? (l & ~b) | t : l & ~t;
            }
        } else if (op < 68) {                    /* an accent moved */
            a = bsome(&dvr_r, h, (uint32_t)__builtin_popcount(a) ? (uint32_t)__builtin_popcount(a) : 1u);
        } else if (op < 82) {                    /* a hit doubled: another just after it */
            b = bsome(&dvr_r, h, 1u);
            t = brot(b, n, 1u);
            if (t & dvr_free(h, n)) {
                h |= t;
                l |= t;
            }
        } else if (op < 90) {                    /* the lane turned */
            t = (uint32_t)pr_in(&dvr_r, 1, (int32_t)n - 1);
            h = brot(h, n, t);
            a = brot(a, n, t);
            l = brot(l, n, t);
        } else if (op < 96 && n >= 4u) {         /* its second half backwards */
            uint32_t m = n / 2u, i, h2 = h & dmask(m), a2 = a & dmask(m), l2 = l & dmask(m);
            for (i = m; i < n; i++) {
                uint32_t j = n - 1u - (i - m);
                h2 |= (h >> j & 1u) << i;
                a2 |= (a >> j & 1u) << i;
                l2 |= (l >> j & 1u) << i;
            }
            h = h2, a = a2, l = l2;
        } else if (c != DV_KICK)                 /* a round's rest (never the kick's) */
            h = 0;
    }
    L->hits = dvr[c].ph = h;
    L->acc = dvr[c].pa = a;
    L->alt = dvr[c].pl = l;
}
static void drm_evolve(void)                     /* (the main loop: mx_poll) */
{
    uint32_t c;
    for (c = 0; c < NDV; c++) {
        dlane_t *L = &drm.l[c];
        uint8_t s = dlr[c].step;
        if (L->hits != dvr[c].ph || L->acc != dvr[c].pa || L->alt != dvr[c].pl) {   /* (written elsewhere: home) */
            dvr[c].hits = dvr[c].ph = L->hits;
            dvr[c].acc = dvr[c].pa = L->acc;
            dvr[c].alt = dvr[c].pl = L->alt;
        }
        if (s != 0xFF && dvr[c].last != 0xFF && s < dvr[c].last)   /* (round again: the next round chosen) */
            drm_vary_lane(c);
        dvr[c].last = s;
    }
}

/* ------------------------------------------- what the values mean, worked out --- */
static uint32_t drm_log(int32_t v, uint32_t lo, int32_t oct_k)   /* lo x 2^(v x oct_k / 65536) */
{
    return (uint32_t)(((uint64_t)lo * exp2_q16(clampi(v, 0, PMAX) * oct_k)) >> 16);
}
static uint32_t drm_fall(uint32_t tau)                            /* a sample's fall, Q32: halved every tau samples */
{
    return 0xFFFFFFFFu - 2977044472u / (tau < 4u ? 4u : tau);    /* (1 - ln 2 / tau) */
}
static uint32_t drm_hz8(int32_t tune) { return drm_log(tune, 20u * 256u, 590); }   /* (Hz x 256: 20 Hz .. 10 kHz) */
static uint32_t drm_dec_tau(int32_t decay) { return drm_log(decay, 88, 539); }     /* (2 ms .. 600 ms, halved) */
static uint32_t drm_att_len(int32_t attack) { return drm_log(attack, 44, 435); }   /* (1 ms .. 100 ms) */
static void drm_coef(uint32_t c)
{
    const dvoice_t *p = &drm.v[c];
    dcoef_t k;
    int32_t g, m, s;
    uint32_t ph, tau = drm_dec_tau(p->decay);
    uint64_t sw;
    memset(&k, 0, sizeof k);
    k.inc0 = drm_hz8(p->tune) * 380u;            /* (2^32 / 44100 / 256) */
    sw = ((uint64_t)k.inc0 * (uint32_t)(clampi(p->sweep, 0, PMAX) * 524)) >> 16;   /* (SWEEP: up to 8 x the pitch over) */
    k.sinc = sw > 0x7FFFFFFFu ? 0x7FFFFFFFu : (uint32_t)sw;
    k.pdec = drm_fall(drm_log(p->bend, 13, 564));   /* (BEND: halved in 0.3 .. 120 ms) */
    k.adec = drm_fall(tau);
    k.adec_alt = drm_fall(tau / 5u);
    k.cdec = drm_fall(53);                       /* (CLICK: halved every 1.2 ms) */
    k.att = p->attack > 0 ? (1u << 30) / drm_att_len(p->attack) : 0u;
    svf_coef(&k.nf, (int32_t)drm_log(p->color, 60, 528), p->filt == DN_BP ? 6144 : 2896);   /* (60 Hz .. 16 kHz) */
    k.filt = p->filt;
    k.gnoise = clampi(p->noise, 0, PMAX) * 32767 / PMAX;
    k.gtone = 32767 - k.gnoise;
    k.gclick = clampi(p->click, 0, PMAX) * 32767 / PMAX;
    k.drv = (int32_t)(exp2_q16(clampi(p->drive, 0, PMAX) * 328) >> 4);   /* (1 .. 32 x, Q12) */
    m = clampi(drm.level, 0, PMAX);
    m = m * m / PMAX * 65534 / PMAX;             /* (the drums' level: 707 -> 1, Q15) */
    g = clampi(p->level, 0, PMAX);
    g = mulq15(g * g / PMAX * 32767 / PMAX, m);
    ph = (uint32_t)(clampi(p->pan, -PAN_MAX, PAN_MAX) + PAN_MAX) * (0x40000000u / (2u * PAN_MAX));
    k.gl = mulq15(g, sin_ph(ph + 0x40000000u));
    k.gr = mulq15(g, sin_ph(ph));
    s = clampi(p->verb, 0, PMAX);
    k.gs = mulq15(s * s / PMAX * 32767 / PMAX, m);
    k.mrat = DM_RAT[p->metal < DM_N ? p->metal : 0];
    drc[c] = k;
}
static void drm_coef_all(void)
{
    uint32_t c;
    for (c = 0; c < NDV; c++)
        drm_coef(c);
}
/* the drums as a start has them: a kit drawn and a rhythm, stopped */
static void drm_init(uint32_t seed)
{
    uint32_t c;
    memset(&drm, 0, sizeof drm);
    drm.level = 707;
    for (c = 0; c < NDV; c++)
        drm_draw_voice(c, seed * 2654435761u + c * 40503u, -1);
    drm_draw_rhythm(seed ^ 0x5A5A5A5Au, -1);
    drm_coef_all();
}

/* the COMPRESSOR's values, worked out for the output stage (mixer.c) */
static const char *const CMP_RNAME[6] = {"2:1", "3:1", "4:1", "6:1", "10:1", "20:1"};
static const int32_t CMP_SLOPE[6] = {16384, 21845, 24576, 27307, 29491, 31130};   /* (1 - 1 / ratio, Q15) */
static int32_t cmp_thr_db10(int32_t v) { return -480 + clampi(v, 0, PMAX) * 480 / PMAX; }
static int32_t cmp_att_x10(int32_t v) { return (int32_t)((2u * exp2_q16(clampi(v, 0, PMAX) * 524)) >> 16); }    /* (0.2 .. 51 ms) */
static int32_t cmp_rel_x10(int32_t v) { return (int32_t)((200u * exp2_q16(clampi(v, 0, PMAX) * 370)) >> 16); }  /* (20 ms .. 1 s) */
static void cmp_coef(void)
{
    cmp_coef_t k;
    int32_t a = cmp.amount > 100u ? 100 : cmp.amount;
    memset(&k, 0, sizeof k);
    k.thr = cmp_thr_db10(cmp.thresh) * 1089;     /* (dB x 10 to octaves, Q16) */
    k.slope = CMP_SLOPE[cmp.ratio < 6u ? cmp.ratio : 2] * a / 100;   /* (AMOUNT: how much of RATIO; 100 %: all) */
    k.makeup = mulq15(-k.thr, k.slope);          /* (a full-scale peak back at full scale: the rest brought forward) */
    k.depth = a * (3 << 16) / 100;               /* (100 %: 18 dB under the kick) */
    k.ka = clampi(237773 / maxi(cmp_att_x10(cmp.attack), 1), 1, Q15);   /* (a block, 0.73 ms, of the time) */
    k.kr = clampi(237773 / maxi(cmp_rel_x10(cmp.release), 1), 1, Q15);
    k.sc = cmp.sc;
    k.on = a > 0;
    cmpk = k;
}
