/* SPDX-License-Identifier: GPL-3.0-only
 * LILLIPUT: the DRUMS' recipes (build 16) -- their sounds (a kit: a recipe a voice), their rhythms, and what the
 * values mean to the voices (drm_coef: worked out in the main loop when a value moves). drums.c has the voices.
 *
 * A SOUND recipe is a voice's archetype -- a range for each value, drawn anew each time (SHIFT + a voice's white key
 * on the DRUMS pages): kicks deep, punchy, gabber-hard, soft (a slow attack), tom-like, sub, crunchy; snares snapping,
 * all noise, with a body, metallic, a rim, crushed, long; hats closed, open, metallic, a shaker, a tick, a cymbal,
 * dirty. Every voice has its DRIVE: this is rhythmic noise.
 *
 * A RHYTHM recipe draws the three lanes at once (SHIFT + the DRUMS key; PRESETS on their pages): FOUR FLOOR,
 * BREAKBEAT, GABBER, HALFTIME, JUNGLE, MARCH, POWER, EUCLID, AKSAK, DUB, BLAST, TRIBAL. As in Noir, steps carry
 * accents, ALT (the decay a fifth: closed hats among open ones, ghost snares) and pitches (toms and bass lines from
 * one kick). */

static const char *const DV_NAME[NDV] = {"KICK", "SNARE", "HATS"};
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
enum { DS_FOUR, DS_BREAK, DS_GABBER, DS_HALF, DS_JUNGLE, DS_MARCH, DS_POWER, DS_EUCLID, DS_AKSAK, DS_DUB, DS_BLAST,
       DS_TRIBAL, DS_N };
static const char *const DS_NAME[DS_N] = {"FOUR FLOOR", "BREAKBEAT", "GABBER", "HALFTIME", "JUNGLE", "MARCH", "POWER",
                                          "EUCLID", "AKSAK", "DUB", "BLAST", "TRIBAL"};
/* a lane written out: x a hit, X accented, o a hit with ALT (short), O both, . a rest */
static void dl_set(uint32_t c, const char *s, uint8_t div, uint8_t prob)
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
    L->div = div;
    L->prob = prob;
}
static void dl_bits(uint32_t c, uint32_t hits, uint32_t acc, uint32_t alt, uint32_t len, uint8_t div, uint8_t prob)
{
    dlane_t *L = &drm.l[c];
    memset(L, 0, sizeof *L);
    L->hits = hits;
    L->acc = acc & hits;
    L->alt = alt & hits;
    L->len = (uint8_t)clampi((int32_t)len, 1, (int32_t)DSTEPS);
    L->div = div;
    L->prob = prob;
}
#define PICK(...) ((const char *const[]){__VA_ARGS__})[rnd_below(&r, (int32_t)(sizeof((const char *const[]){__VA_ARGS__}) / sizeof(const char *)))]
/* new lanes for the three voices, of style st (-1: any), from seed */
static uint32_t drm_draw_rhythm(uint32_t seed, int st)
{
    rnd_t r;
    uint32_t i;
    rnd_seed(&r, seed);
    if (st < 0 || st >= DS_N)
        st = rnd_below(&r, DS_N);
    drm.swing = 0;
    switch (st) {
    case DS_FOUR:
        dl_set(DV_KICK, "X...X...X...X...", DV_16, 100);
        dl_set(DV_SNARE, PICK("....X.......X...", "....X.......X..o", "....X.....o.X..."), DV_16, 100);
        dl_set(DV_HAT, PICK("o.X.o.X.o.X.o.X.", "ooXoooXoooXoooXo", "..x...x...x...x."), DV_16, 100);
        break;
    case DS_BREAK:
        drm.swing = (uint8_t)pr_in(&r, 15, 40);
        dl_set(DV_KICK, PICK("X.....x...x.....", "X.x.......x..x..", "X......x..x....."), DV_16, 100);
        dl_set(DV_SNARE, PICK("....X..o.o..X..o", "....X......oX...", "....X.o....oX.o."), DV_16, 100);
        dl_set(DV_HAT, PICK("x.x.x.x.x.x.x.x.", "x.xox.x.x.xox.x."), DV_16, 90);
        break;
    case DS_GABBER:
        dl_set(DV_KICK, "X...X...X...X...", DV_16, 100);
        dl_set(DV_SNARE, PICK("................", "............X...", "........X......."), DV_16, 100);
        dl_set(DV_HAT, PICK("..x...x...x...x.", "..X...X...X...X.", "..xo..xo..xo..xo"), DV_16, 100);
        break;
    case DS_HALF:
        dl_set(DV_KICK, PICK("X.........x.....", "X......x..x.....", "X.........x..x.."), DV_16, 100);
        dl_set(DV_SNARE, PICK("........X.......", "........X......o"), DV_16, 100);
        dl_set(DV_HAT, PICK("x.x.x.x.x.x.x.xo", "o.o.X.o.o.o.X.o.", "xxx.xxx.xxx.xxx."), DV_16, 95);
        break;
    case DS_JUNGLE:
        drm.swing = (uint8_t)pr_in(&r, 5, 25);
        dl_set(DV_KICK, PICK("X.........X.....", "X.....x...x.....", "X.x.......x....."), DV_16, 100);
        dl_set(DV_SNARE, PICK("....X..o.o..X...", "....X......oX..o", "....X.o..o..X.o."), DV_16, 100);
        dl_set(DV_HAT, "xoxoxoxoxoxoxoxo", DV_16, 85);
        break;
    case DS_MARCH:
        dl_set(DV_KICK, PICK("X...x...X...x...", "X.......X.......", "X...x...X..xx..."), DV_16, 100);
        dl_set(DV_SNARE, PICK("x.ooX.oox.ooX.oo", "X.o.x.ooX.o.x.oo", "xoooXoooxoooXooo"), DV_16, 100);
        dl_set(DV_HAT, PICK("x.......x.......", "................"), DV_16, 100);
        break;
    case DS_POWER:
        dl_set(DV_KICK, PICK("X.x.X.x.X.xxX.x.", "X.xxX.x.X.xxX.xx", "XxxxXxxxXxxxXxxx"), DV_16, 100);
        for (i = 0; i < 16u; i++) {              /* (a bass line out of the kick: its steps' pitches) */
            static const int8_t BL[6] = {0, 3, 5, 7, 12, -5};
            if ((i & 3u) && rnd_chance(&r, 45))
                drm.l[DV_KICK].pitch[i] = BL[rnd_below(&r, 6)];
        }
        dl_set(DV_SNARE, PICK("....X.......X..X", "....X.......X...", "....X..x....X..x"), DV_16, 100);
        dl_set(DV_HAT, "oOooOoooOoooOooo", DV_16, 100);
        break;
    case DS_EUCLID: {                            /* (each its own length: they drift against each other) */
        uint32_t n1 = (uint32_t)pr_in(&r, 12, 16), n2 = (uint32_t)pr_in(&r, 10, 14), n3 = (uint32_t)pr_in(&r, 7, 13);
        dl_bits(DV_KICK, euclid((uint32_t)pr_in(&r, 3, 5), n1, 0), 1u, 0, n1, DV_16, 100);
        dl_bits(DV_SNARE, euclid((uint32_t)pr_in(&r, 2, 4), n2, (uint32_t)pr_in(&r, 2, 5)), 0, 0, n2, DV_16, 100);
        dl_bits(DV_HAT, euclid((uint32_t)pr_in(&r, 4, 8), n3, 1), euclid(2, n3, 1), euclid(3, n3, 0), n3, DV_16, 95);
        break;
    }
    case DS_AKSAK:
        if (rnd_chance(&r, 50)) {                /* (7: 2 + 2 + 3) */
            dl_set(DV_KICK, PICK("X.x.X..", "X...X.."), DV_16, 100);
            dl_set(DV_SNARE, PICK("..X..X.", "....X.."), DV_16, 100);
            dl_set(DV_HAT, PICK("x.x.x.x", "xoxoxxo"), DV_16, 100);
        } else {                                 /* (9: 2 + 2 + 2 + 3) */
            dl_set(DV_KICK, PICK("X.x.x.X..", "X.....X.."), DV_16, 100);
            dl_set(DV_SNARE, PICK("......X..", "..X...X.."), DV_16, 100);
            dl_set(DV_HAT, PICK("x.x.x.xxx", "xoxoxoxoo"), DV_16, 100);
        }
        break;
    case DS_DUB:
        dl_set(DV_KICK, PICK("X.......x.......", "X.........x....."), DV_16, 100);
        drm.l[DV_KICK].pitch[rnd_below(&r, 16)] = (int8_t)pr_in(&r, -5, 7);
        dl_set(DV_SNARE, PICK("........X.......", "........X......o"), DV_16, 100);
        dl_set(DV_HAT, PICK("..x...x...x...x.", "..x...x...x...xo"), DV_16, 70);
        break;
    case DS_BLAST:
        dl_set(DV_KICK, PICK("xxxxxxxxxxxxxxxx", "XxXxXxXxXxXxXxXx"), DV_16, 100);
        dl_set(DV_SNARE, PICK(".x.x.x.x.x.x.x.x", "x.x.x.x.x.x.x.x."), DV_16, 100);
        dl_set(DV_HAT, "X...X...X...X...", DV_16, 100);
        break;
    default: {                                   /* TRIBAL: toms from the kick, on a scale; Noir's melodic drums */
        static const int8_t SC[6] = {0, 3, 5, 7, 10, 12};
        dl_bits(DV_KICK, euclid((uint32_t)pr_in(&r, 5, 7), 16, (uint32_t)pr_in(&r, 0, 3)), 1u | 1u << 8, 0, 16, DV_16, 100);
        for (i = 0; i < 16u; i++)
            drm.l[DV_KICK].pitch[i] = i ? SC[rnd_below(&r, 6)] : 0;
        dl_set(DV_SNARE, PICK("x..x..x...x..x..", "o..o..x...o..o.."), DV_16, 100);
        dl_bits(DV_HAT, euclid(3, 8, (uint32_t)pr_in(&r, 0, 2)), 0, 0, 8, DV_8, 90);
        break;
    }
    }
    drm.style = (uint8_t)st;
    return (uint32_t)st;
}
#undef PICK

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
