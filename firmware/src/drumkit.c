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
/* (build 17: anew, after the genre -- rigid, militaristic 4/4 from a distorted kick, the strangeness in accents, ghosts
 * and metal rather than in denser kicks; every lane in sixteenths, 16 steps or a two-bar 32, so the pace is always
 * the tempo's. Sources: Bigorna's phrase.c, Attack Magazine's industrial and dark Berlin techno, the genre's own
 * descriptions -- see the CHANGELOG) */
enum { DS_POWER, DS_PISTON, DS_MARCH, DS_OFFBEAT, DS_RUMBLE, DS_BROKEN, DS_3V4, DS_TRIBAL, DS_SHUFFLE, DS_DBREAK,
       DS_RITUAL, DS_BURST, DS_N };
static const char *const DS_NAME[DS_N] = {"POWER 4/4", "PISTON", "MARCH", "OFFBEAT", "RUMBLE", "BROKEN", "3 AGAINST 4",
                                          "TRIBAL", "SHUFFLE", "DIST BREAK", "HALF RITUAL", "BURST FILL"};
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
}
#define PICK(...) ((const char *const[]){__VA_ARGS__})[rnd_below(&r, (int32_t)(sizeof((const char *const[]){__VA_ARGS__}) / sizeof(const char *)))]
/* new lanes for the four voices, of style st (-1: any), from seed */
static uint32_t drm_draw_rhythm(uint32_t seed, int st)
{
    rnd_t r;
    uint32_t i;
    rnd_seed(&r, seed);
    if (st < 0 || st >= DS_N)
        st = rnd_below(&r, DS_N);
    drm.swing = 0;
    switch (st) {
    case DS_POWER:                               /* the distorted kick in militaristic 4/4 */
        dl_set(DV_KICK, PICK("X...X...X...X...", "X...X...X...X.x.", "X...X...X..xX..."), 100);
        dl_set(DV_SNARE, PICK("....X.......X...", "....X.......X..o", "....X...o...X.o."), 100);
        dl_set(DV_HAT, PICK("..X...X...X...X.", "o.X.o.X.o.X.o.X.", "ooXoooXoooXoooXo"), 100);
        dl_set(DV_PERC, PICK("......x.......x.", "...x......x.....", "..........O.O..."), 90);
        break;
    case DS_PISTON:                              /* eighth-note kicks, the beat accented: rapid-fire machines */
        dl_set(DV_KICK, PICK("X.x.X.x.X.x.X.x.", "X.o.X.o.X.o.X.oo", "X.x.X.x.X.xxX.x."), 100);
        dl_set(DV_SNARE, PICK("................", "....O.......O...", "............X..."), 100);
        dl_set(DV_HAT, PICK("..O...O...O...O.", "..X..oX...X..oX."), 100);
        dl_set(DV_PERC, PICK("...o...o...o...o", "x..............."), 100);
        break;
    case DS_MARCH:                               /* the martial trot */
        dl_set(DV_KICK, PICK("X.......X.......", "X...x...X...x...", "X.......X...x.x."), 100);
        dl_set(DV_SNARE, PICK("....X..o....X.oo", "o.ooX.o.o.ooX.o.", "....X.......X.ox"), 80);
        dl_set(DV_HAT, PICK("x...x...x...x...", "x...x...x...x..."), 100);
        dl_set(DV_PERC, PICK("....O.......O...", "....O.....o.O..."), 100);
        break;
    case DS_OFFBEAT:                             /* 4/4, the off-beat crushed hat, a low-mid tom, a noise snare */
        drm.swing = (uint8_t)pr_in(&r, 0, 12);
        dl_set(DV_KICK, PICK("X...X...X...X...", "X...X...X...X..o"), 100);
        dl_set(DV_SNARE, PICK("....X.......X...", "....X..o....X..."), 100);
        dl_set(DV_HAT, PICK("..X...X...X...X.", "ooXoooXoooXoooXo", "o.X.o.XoooX.o.Xo"), 100);
        dl_set(DV_PERC, PICK("...x..x....x..x.", "......x..x....x.", "...x......x..x.."), 85);
        break;
    case DS_RUMBLE:                              /* the dark Berlin layout: a clap on two, a stab, a push every other bar */
        drm.swing = (uint8_t)pr_in(&r, 6, 18);
        dl_set(DV_KICK, PICK("X...X...X...X...", "X...X...X..oX..."), 100);
        dl_set(DV_SNARE, PICK("....X...........................", "....X...............X.........o."), 100);
        dl_set(DV_HAT, PICK("..X...X...X...X.", "..X...X...X...Xo"), 100);
        dl_set(DV_PERC, PICK("........X...............X..x....", "......x.......x."), 100);
        break;
    case DS_BROKEN:                              /* jagged, syncopated kicks over a backbeat; the hats on the beat */
        dl_set(DV_KICK, PICK("X..x..x...X..x..", "X..x...x..X.....", "X.x...x..x..x..."), 100);
        dl_set(DV_SNARE, PICK("....X.......X...", "....X.....o.X..."), 100);
        dl_set(DV_HAT, PICK("X.o.X.o.X.o.X.o.", "X...X...X...X..."), 100);
        dl_set(DV_PERC, PICK("..o....o..o....o", ".o.....o.o....o."), 100);
        drm.l[DV_KICK].prob = 100;
        break;
    case DS_3V4:                                 /* threes against the 4/4 bar, which stays 16 steps */
        dl_set(DV_KICK, "X...X...X...X...", 100);
        dl_set(DV_SNARE, PICK("....X.......X...", "........X......."), 100);
        dl_set(DV_HAT, PICK("XooXooXooXooXooX", "o..O..o..O..o..O"), 100);
        dl_set(DV_PERC, PICK("x....x....x....x", "..x..x..x..x..x."), 100);
        break;
    case DS_TRIBAL: {                            /* ritual toms out of the kick, an anvil on the beat */
        static const int8_t P1[16] = {0, 0, 0, 3, 0, 0, 7, 0, 0, 0, 0, 5, 0, 0, 3, 0};
        static const int8_t P2[16] = {0, 0, 0, 3, 0, 0, 5, 0, 0, 0, 7, 0, 3, 0, 0, 0};
        int two = rnd_chance(&r, 50);
        drm.swing = (uint8_t)pr_in(&r, 0, 10);
        dl_set(DV_KICK, two ? "X..x..x...x.x..." : "X..x..x.X..x..x.", 100);
        for (i = 0; i < 16u; i++)
            drm.l[DV_KICK].pitch[i] = two ? P2[i] : P1[i];
        dl_set(DV_SNARE, PICK("....x.......x...", "............x..."), 100);
        dl_set(DV_HAT, PICK("O.o.o.o.O.o.o.o.", "o.oxo.oxo.oxo.ox"), 90);
        dl_set(DV_PERC, PICK("X...x...X...x...", "X.......X..x...."), 100);
        break;
    }
    case DS_SHUFFLE:                             /* the lower-tempo shuffle: the only one that swings much */
        drm.swing = (uint8_t)pr_in(&r, 20, 35);
        dl_set(DV_KICK, PICK("X...X...X...X...", "X...X...X..xX..."), 100);
        dl_set(DV_SNARE, PICK("....X.......X...", "....X..o....X..o"), 100);
        dl_set(DV_HAT, PICK("ooXoooXoooXoooXo", "x.xxx.xxx.xxx.xx"), 90);
        dl_set(DV_PERC, PICK("..........x..x..", "...x.......x...."), 100);
        break;
    case DS_DBREAK:                              /* a distorted break, straight, never swung */
        dl_set(DV_KICK, PICK("X.x.......xx....", "X.x.......X.....", "X.......x.x....."), 100);
        dl_set(DV_SNARE, PICK("....X..o.o..X..o", "....X......oX...", "....X.o..o..X.oo"), 75);
        dl_set(DV_HAT, PICK("x.x.x.x.x.x.x.x.", "x.xox.x.x.xox.x."), 100);
        dl_set(DV_PERC, PICK("..............OO", "......o.......o."), 100);
        break;
    case DS_RITUAL:                              /* half time: the hats keep the tempo */
        dl_set(DV_KICK, PICK("X.........x.....", "X......x..x....."), 100);
        dl_set(DV_SNARE, PICK("........X.......", "........X.....o."), 100);
        dl_set(DV_HAT, PICK("..x...x...x...x.", "o.o.o.o.o.o.o.o."), 100);
        dl_set(DV_PERC, PICK("......x......x..", "..............x."), 100);
        break;
    default:                                     /* BURST FILL: two bars of 4/4, a noise burst ending the second */
        dl_set(DV_KICK, PICK("X...X...X...X...X...X...X...X.x.", "X...X...X...X...X...X...X......."), 100);
        dl_set(DV_SNARE, PICK("....X.......X.......X.......xxXX", "....X.......X.......X.......X..."), 100);
        dl_set(DV_HAT, PICK("..X...X...X...X...X...X...X.....", "..X...X...X...X...X...X...X...X."), 100);
        dl_set(DV_PERC, PICK("......x.......x.......x...oooOOX", "......x.......x.......x.......x."), 100);
        break;
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
