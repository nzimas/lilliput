/* SPDX-License-Identifier: GPL-3.0-only
 * LILLIPUT: patches -- the whole mixer drawn at once (HOME), from families, as Tonitor draws its sounds.
 *
 * A FAMILY is a kind of no-input sound: how its channels are connected (each one's input: a loop of its own, a ring
 * of channels, the mix bus, the sends; loops through the rack's effects on the returns), how they are voiced (the
 * limiter's speed -- the loops' register --, where their filters ring, their tubes and op-amps, their noise, how near
 * the overload protection), which effects it reaches for, its space -- and how it MOVES: each family has its
 * gestures, LFOs on the parameters that make its character (a siren's sweeping filter, a pulse's gated level, a
 * cavern's bending delay, a swarm's wandering pans), drawn anew for every channel so no two move alike.
 *
 *   HOME            a new patch, of a family drawn at random (another than the one playing)
 *   SHIFT + HOME    a new patch of the same family
 *   SHIFT + PRESETS a new patch of the family before or after
 *   HOME held       the patch nudged (the app's SHIFT): every channel's tone a little elsewhere, the rest kept
 *
 * Every patch's faders are set a little past where its loops start to sing -- worked out from the gains round each
 * loop -- so it sounds at once, and the faders are where the playing is; some channels come with theirs down, to be
 * brought in. Then the trim (below) wakes any loop still silent and sets the main level once. */

enum { RC_SOLO, RC_RING, RC_WEB, RC_SEND, RC_N };

/* a channel as it starts: a quiet, plain voice, its level down */
static void patch_ch_plain(ch_par_t *p, uint32_t c)
{
    static const int8_t PAN0[NCH] = {-30, 30, -15, 15, -45, 45};
    memset(p, 0, sizeof *p);
    p->src = SRC_PRE;
    p->gain = 250;
    p->noise = 450;
    p->ntype = NZ_PINK;
    p->tube = TB_12AX7;
    p->tmode = TM_LIN;
    p->opamp = OA_TL072;
    p->over = 500;
    p->lim = 400;
    p->ftype = FT_SHELF;
    p->high = 500;
    p->low = 500;
    p->mtype = MT_PEAK;
    p->mgain = 500;
    p->mfreq = 500;
    p->level = 0;
    p->pan = PAN0[c];
    p->mainmix = 1;
}
static void patch_plain(lil_par_t *q)
{
    uint32_t c;
    memset(q, 0, sizeof *q);
    for (c = 0; c < NCH; c++)
        patch_ch_plain(&q->ch[c], c);
    for (c = 0; c < NSLOT; c++) {
        q->slot[c].fx = (int16_t)(c % FX_N);
        q->slot[c].p1 = 500;
        q->slot[c].p2 = 300;
        q->slot[c].pres = 500;
        q->slot[c].in = FI_NON;
        q->slot[c].out = FO_NON;
    }
    q->out.ret1 = q->out.ret2 = 800;
    q->out.ftype = FT_SHELF;
    q->out.high = q->out.low = 500;
    q->out.wide = 1;
    q->out.mgain = 500;
    q->out.mfreq = 500;
    q->out.space = SP_HALL;
    q->out.spres = 250;
    q->out.level = 500;
}

/* ----------------------------------------------------------- families --- */
#define SH_SINE 1u
#define SH_TRI 2u
#define SH_SAWU 4u
#define SH_SAWD 8u
#define SH_SQR 16u
#define SH_STEPS 32u
#define SH_DRIFT 64u
#define MD_LOOP 0xFEu            /* (a gesture's destination: whichever control makes the channel's loop -- its LEVEL,
                                  * or the SEND its input comes back from) */
typedef struct {
    uint8_t p;                   /* % of the heard channels that make it */
    uint8_t dst;                 /* MD_*, or MD_LOOP */
    uint8_t shapes;              /* SH_* */
    uint8_t rlo, rhi;            /* rate, 0 .. 100 (0.01 .. 20 Hz) */
    uint8_t alo, ahi;            /* amount (either sign) */
    uint8_t fade;                /* % with a fade in */
} gesture_t;
typedef struct {
    const char *name;
    uint8_t topo[RC_N];          /* each heard channel's connection: SOLO, RING, WEB, SEND (weights) */
    uint8_t ret;                 /* % for loops through the rack (sends into an effect, its return into channels) */
    uint8_t nlo, nhi;            /* channels heard */
    int16_t lim_lo, lim_hi;      /* LIMIT: the loops' register (fast, high .. slow, low) */
    int16_t mf_lo, mf_hi;        /* where the middle filter rings */
    uint8_t mt[MT_N];            /* PEAK, NOTCH, BAND (weights) */
    int16_t mg_lo, mg_hi;        /* its gain (peak) or width */
    uint8_t cut;                 /* % with cuts rather than shelves */
    int16_t hi_lo, hi_hi, lo_lo, lo_hi;   /* HIGH, LOW */
    uint8_t sat;                 /* % with the tube saturated */
    uint8_t tube[TB_N], opamp[OA_N], noise[NZ_N];
    int16_t ov_lo, ov_hi;        /* PROTECT: low, the loops trip and crackle */
    uint8_t ins;                 /* % of heard channels with an insert */
    uint8_t fx[FX_N];
    uint8_t lofi;                /* % LO-FI */
    uint8_t space[SP_N];
    int16_t sp_lo, sp_hi;        /* SPACE MIX */
    int8_t m_lo, m_hi;           /* the margin past singing, dB x 10 / 2 */
    uint8_t cluster;             /* the middle filters drawn close together (beating) */
    uint8_t chaos;               /* % of channels whose modulation is thrown wide (SHIFT + SEQ's) */
    gesture_t g[3];
    uint8_t sync;                /* the channels' LFOs at one rate, in step: a rhythm, a tide, heard together */
} family_t;
/*        SOLO RING WEB SEND                                     PEAK NOTCH BAND                  12AX7 12AT7 6C16 EF86 6L6 300B
 *                                                               TL072 NE5532 4558 741 OP07 CA3140   WHITE PINK RED CRACKLE GEIGER HUM
 *        DELAY FLANGER CHORUS TREMOLO FORMANT BITCRUSH GLITCH   ROOM HALL PLATE CAVE ABYSS VOID */
static const family_t FAM[] = {
    {"DRONE", {5, 2, 1, 1}, 25, 2, 4, 620, 1000, 80, 450, {8, 0, 2}, 600, 900, 20, 300, 650, 400, 750, 25,
     {2, 2, 1, 3, 3, 3}, {3, 1, 3, 2, 1, 1}, {1, 4, 3, 0, 0, 2}, 500, 950, 25, {4, 2, 3, 0, 1, 0, 0}, 10,
     {0, 2, 0, 3, 3, 1}, 250, 450, 10, 20, 0, 5,
     {{80, MD_FREQ, SH_DRIFT | SH_SINE, 8, 30, 4, 14, 40}, {50, MD_PAN, SH_DRIFT, 5, 20, 20, 60, 0},
      {30, MD_LOOP, SH_DRIFT, 5, 25, 6, 18, 50}}, 0},
    {"WHISTLE", {6, 1, 0, 1}, 10, 2, 4, 0, 230, 650, 960, {4, 0, 5}, 500, 900, 40, 650, 1000, 300, 700, 10,
     {2, 3, 1, 1, 1, 1}, {4, 4, 1, 0, 0, 2}, {4, 3, 0, 0, 0, 0}, 600, 1000, 20, {3, 0, 3, 3, 0, 0, 0}, 0,
     {2, 3, 3, 0, 0, 0}, 150, 350, 5, 15, 0, 5,
     {{60, MD_FREQ, SH_SINE | SH_TRI, 45, 66, 2, 6, 30}, {40, MD_LOOP, SH_DRIFT, 18, 40, 10, 25, 0},
      {25, MD_LIM, SH_DRIFT, 10, 30, 3, 10, 0}}, 0},
    {"SIREN", {5, 1, 0, 2}, 10, 1, 3, 200, 700, 300, 800, {7, 0, 3}, 650, 950, 20, 400, 800, 350, 700, 30,
     {3, 2, 2, 1, 1, 1}, {3, 3, 2, 1, 0, 1}, {3, 3, 1, 0, 0, 1}, 550, 950, 20, {4, 1, 1, 0, 0, 0, 0}, 0,
     {1, 3, 1, 1, 1, 0}, 200, 400, 15, 30, 0, 0,
     {{100, MD_FREQ, SH_SINE | SH_TRI | SH_SAWU, 40, 62, 30, 70, 20}, {50, MD_LIM, SH_TRI | SH_SINE, 30, 52, 10, 30, 0},
      {30, MD_PAN, SH_SINE, 35, 55, 30, 70, 0}}, 1},
    {"CHIRP", {5, 0, 2, 1}, 15, 2, 4, 0, 500, 400, 900, {6, 1, 3}, 550, 900, 30, 450, 900, 300, 700, 30,
     {3, 2, 2, 1, 1, 1}, {3, 3, 2, 1, 1, 2}, {3, 2, 0, 2, 1, 0}, 450, 900, 25, {3, 1, 0, 1, 0, 1, 2}, 15,
     {3, 1, 3, 0, 0, 0}, 120, 300, 10, 25, 0, 10,
     {{100, MD_LIM, SH_SAWU | SH_SAWD | SH_SQR, 70, 90, 20, 60, 0}, {70, MD_FREQ, SH_SAWD | SH_STEPS, 65, 88, 20, 50, 0},
      {30, MD_LOOP, SH_SQR, 60, 80, 15, 40, 0}}, 0},
    {"STATIC", {3, 0, 2, 3}, 50, 3, 5, 100, 700, 200, 950, {3, 3, 4}, 300, 900, 70, 450, 900, 450, 900, 50,
     {2, 1, 3, 2, 1, 1}, {1, 1, 3, 4, 4, 2}, {2, 1, 0, 4, 1, 1}, 0, 250, 30, {1, 0, 0, 1, 0, 4, 4}, 40,
     {3, 1, 2, 1, 0, 0}, 80, 250, 10, 25, 0, 25,
     {{70, MD_OVER, SH_STEPS, 55, 80, 20, 50, 0}, {50, MD_LOOP, SH_STEPS, 60, 85, 15, 40, 0},
      {30, MD_NOISE, SH_STEPS | SH_SQR, 40, 70, 20, 50, 0}}, 0},
    {"GEIGER", {6, 0, 0, 1}, 20, 2, 4, 0, 400, 500, 950, {3, 0, 6}, 500, 900, 30, 500, 1000, 300, 700, 20,
     {2, 2, 2, 1, 1, 1}, {3, 3, 1, 1, 1, 1}, {0, 0, 0, 1, 8, 0}, 300, 700, 30, {3, 0, 0, 0, 0, 2, 4}, 30,
     {4, 1, 4, 0, 0, 0}, 150, 350, -5, 5, 0, 10,
     {{80, MD_NOISE, SH_STEPS | SH_DRIFT, 40, 70, 20, 60, 0}, {40, MD_FREQ, SH_STEPS, 50, 70, 15, 40, 0},
      {30, MD_FX2, SH_STEPS, 45, 70, 20, 50, 0}}, 0},
    {"SWARM", {1, 0, 6, 0}, 10, 4, 6, 300, 800, 200, 800, {8, 1, 1}, 550, 850, 25, 350, 750, 350, 750, 60,
     {3, 1, 3, 2, 2, 1}, {2, 2, 3, 2, 1, 3}, {3, 3, 1, 1, 0, 1}, 450, 950, 15, {2, 3, 4, 0, 0, 0, 0}, 0,
     {0, 3, 1, 3, 1, 0}, 250, 450, 10, 25, 0, 10,
     {{90, MD_PAN, SH_DRIFT, 20, 45, 40, 90, 0}, {60, MD_FREQ, SH_DRIFT, 15, 35, 5, 20, 0},
      {30, MD_LIM, SH_DRIFT, 10, 30, 5, 15, 0}}, 0},
    {"BEATING", {2, 6, 1, 0}, 10, 2, 3, 400, 800, 250, 750, {8, 0, 2}, 650, 900, 20, 350, 700, 350, 700, 20,
     {3, 3, 1, 2, 2, 2}, {4, 4, 2, 1, 0, 1}, {2, 4, 2, 0, 0, 1}, 600, 1000, 10, {2, 2, 4, 0, 0, 0, 0}, 0,
     {0, 4, 2, 3, 0, 0}, 250, 450, 10, 20, 1, 0,
     {{70, MD_FREQ, SH_DRIFT, 5, 20, 1, 4, 0}, {40, MD_LOOP, SH_SINE | SH_DRIFT, 10, 30, 5, 15, 0},
      {20, MD_PAN, SH_SINE, 10, 30, 20, 50, 0}}, 0},
    {"PULSE", {4, 1, 0, 3}, 25, 2, 5, 100, 800, 250, 900, {6, 1, 3}, 550, 900, 30, 400, 850, 350, 750, 40,
     {3, 2, 3, 1, 1, 1}, {3, 3, 2, 1, 1, 2}, {3, 3, 1, 1, 1, 1}, 400, 950, 30, {5, 0, 1, 3, 0, 1, 1}, 15,
     {3, 2, 3, 1, 0, 0}, 150, 350, 15, 30, 0, 0,
     {{100, MD_LOOP, SH_SQR | SH_STEPS | SH_SAWD, 58, 82, 70, 100, 0}, {50, MD_FXMIX, SH_SQR | SH_SAWU, 55, 75, 30, 70, 0},
      {35, MD_FREQ, SH_STEPS, 55, 75, 10, 35, 0}}, 1},
    {"CAVERN", {2, 0, 0, 2}, 100, 2, 4, 300, 900, 150, 700, {7, 1, 2}, 550, 850, 30, 300, 650, 400, 750, 30,
     {2, 2, 1, 3, 3, 3}, {3, 2, 2, 2, 1, 1}, {2, 4, 3, 0, 0, 1}, 450, 950, 20, {6, 1, 2, 0, 0, 0, 0}, 20,
     {0, 1, 0, 3, 4, 3}, 350, 600, 15, 30, 0, 5,
     {{80, MD_FX1, SH_DRIFT | SH_SINE, 10, 35, 10, 40, 0}, {50, MD_LOOP, SH_DRIFT, 15, 35, 8, 25, 40},
      {30, MD_FREQ, SH_DRIFT, 5, 25, 5, 15, 0}}, 0},
    {"VOICES", {4, 2, 0, 1}, 50, 2, 4, 150, 750, 300, 800, {6, 0, 4}, 550, 850, 30, 400, 800, 350, 700, 30,
     {3, 2, 2, 2, 1, 2}, {3, 3, 3, 1, 1, 1}, {3, 4, 1, 0, 0, 1}, 500, 950, 70, {0, 0, 1, 0, 8, 0, 0}, 10,
     {1, 3, 2, 2, 1, 0}, 200, 400, 10, 25, 0, 10,
     {{100, MD_FX1, SH_SINE | SH_TRI | SH_DRIFT | SH_STEPS, 30, 65, 30, 80, 0}, {50, MD_FREQ, SH_DRIFT, 15, 40, 10, 30, 0},
      {30, MD_FX2, SH_DRIFT, 15, 40, 20, 50, 0}}, 0},
    {"BROKEN RADIO", {4, 1, 1, 2}, 30, 2, 5, 150, 750, 350, 850, {2, 1, 6}, 400, 900, 60, 450, 850, 450, 850, 40,
     {2, 2, 3, 2, 1, 1}, {2, 2, 3, 2, 2, 2}, {2, 1, 0, 4, 1, 3}, 150, 500, 40, {1, 0, 0, 4, 2, 3, 1}, 30,
     {3, 1, 2, 0, 0, 0}, 120, 300, 10, 25, 0, 15,
     {{80, MD_FREQ, SH_STEPS | SH_DRIFT, 35, 65, 15, 50, 0}, {60, MD_LOOP, SH_STEPS, 50, 75, 10, 30, 0},
      {40, MD_FX1, SH_SINE | SH_STEPS, 40, 70, 20, 50, 0}}, 0},
    {"SUBTERRANEAN", {5, 2, 1, 1}, 20, 2, 4, 750, 1000, 0, 260, {8, 0, 2}, 600, 900, 80, 0, 380, 700, 1000, 50,
     {1, 1, 1, 1, 4, 4}, {1, 1, 2, 4, 4, 1}, {1, 3, 4, 0, 0, 2}, 450, 950, 15, {4, 2, 2, 0, 0, 0, 0}, 0,
     {0, 1, 0, 4, 4, 2}, 250, 450, 15, 30, 0, 5,
     {{60, MD_LIM, SH_DRIFT, 5, 25, 5, 20, 0}, {40, MD_LOOP, SH_DRIFT, 5, 20, 10, 25, 50},
      {30, MD_FREQ, SH_DRIFT, 5, 20, 5, 15, 0}}, 0},
    {"SCREAM", {4, 2, 1, 1}, 15, 2, 4, 100, 650, 300, 900, {3, 5, 1}, 500, 900, 25, 450, 850, 350, 700, 90,
     {4, 1, 5, 2, 1, 1}, {1, 1, 4, 2, 1, 4}, {3, 2, 1, 1, 0, 1}, 700, 1000, 15, {2, 2, 0, 0, 0, 2, 0}, 10,
     {2, 2, 2, 1, 0, 0}, 100, 300, 25, 45, 0, 10,
     {{80, MD_FREQ, SH_SINE | SH_TRI | SH_SAWU, 45, 70, 20, 60, 0}, {40, MD_GAIN, SH_SINE | SH_DRIFT, 30, 60, 10, 30, 0},
      {30, MD_LOOP, SH_TRI, 35, 60, 10, 25, 0}}, 0},
    {"GLITCH", {4, 1, 1, 2}, 60, 2, 5, 0, 700, 300, 950, {4, 2, 3}, 400, 900, 50, 400, 900, 400, 900, 40,
     {2, 2, 2, 2, 1, 1}, {2, 2, 2, 2, 2, 2}, {3, 2, 1, 2, 1, 1}, 250, 800, 60, {1, 0, 0, 0, 0, 3, 6}, 50,
     {4, 1, 3, 0, 0, 0}, 80, 250, 10, 25, 0, 30,
     {{90, MD_LIM, SH_STEPS, 60, 85, 20, 60, 0}, {60, MD_FX2, SH_STEPS, 55, 80, 20, 60, 0},
      {40, MD_LOOP, SH_STEPS | SH_SQR, 55, 80, 15, 40, 0}}, 0},
    {"TIDES", {2, 0, 0, 6}, 30, 3, 6, 250, 900, 150, 800, {7, 1, 2}, 550, 850, 25, 350, 700, 350, 700, 30,
     {2, 2, 1, 3, 3, 3}, {3, 3, 2, 2, 1, 1}, {2, 4, 3, 0, 0, 1}, 450, 950, 15, {3, 2, 3, 0, 0, 0, 0}, 0,
     {0, 3, 1, 3, 3, 1}, 300, 500, 10, 25, 0, 0,
     {{100, MD_LOOP, SH_SINE | SH_TRI, 12, 30, 50, 90, 50}, {60, MD_PAN, SH_DRIFT, 5, 25, 30, 70, 0},
      {40, MD_FREQ, SH_DRIFT, 5, 20, 5, 20, 0}}, 1},
};
#define NFAM NELEM(FAM)

/* ------------------------------------------------------------ drawing --- */
static int32_t pr_in(rnd_t *r, int32_t a, int32_t b) { return b <= a ? a : a + rnd_below(r, b - a + 1); }
static int16_t lvl_of_db10(int32_t db10) { return (int16_t)clampi(db10 + LVL_0DB, 40, LVL_MAX); }
static int16_t snd_of_db10(int32_t db10) { return (int16_t)clampi((db10 + 600) * PMAX / 660, 40, PMAX); }
/* the gain round a channel's own loop at its loudest frequency, dB x 10, its LEVEL left out: the preamp, the tube
 * (saturated: its drive), the op-amp (x 2), a resonant peak or a shelf lifted */
static int32_t loop_db10(const ch_par_t *p)
{
    static const int16_t TUBE_DB10[TB_N] = {95, 68, 136, 70, 40, 23};
    int32_t g = p->gain * 480 / PMAX + 60;
    if (p->tmode == TM_SAT)
        g += TUBE_DB10[clampi(p->tube, 0, TB_N - 1)];
    if (p->mtype == MT_PEAK && p->mgain > 500)
        g += peak_db10(p->mgain) * 8 / 10;
    if (p->ftype == FT_SHELF)
        g += maxi(0, maxi(shelf_db10(p->high), shelf_db10(p->low))) / 2;
    if (p->mtype == MT_BAND)
        g -= 45;                                 /* (measured: a band-pass's peak seldom where the loop's phase lets it ring) */
    return g;
}
/* a channel's voice, of family f: everything but its connections and levels */
static void draw_voice(rnd_t *r, const family_t *f, ch_par_t *p, int32_t mf_base)
{
    p->tube = (int16_t)rnd_weighted(r, f->tube, TB_N);
    p->tmode = (int16_t)(rnd_chance(r, f->sat) ? TM_SAT : TM_LIN);
    p->opamp = (int16_t)rnd_weighted(r, f->opamp, OA_N);
    p->ntype = (int16_t)rnd_weighted(r, f->noise, NZ_N);
    p->noise = (int16_t)(p->ntype == NZ_GEIGER ? pr_in(r, 250, 650) : pr_in(r, 330, 680));
    p->over = (int16_t)pr_in(r, f->ov_lo, f->ov_hi);
    p->lim = (int16_t)pr_in(r, f->lim_lo, f->lim_hi);
    p->ftype = (int16_t)(rnd_chance(r, f->cut) ? FT_CUT : FT_SHELF);
    p->high = (int16_t)pr_in(r, f->hi_lo, f->hi_hi);
    p->low = (int16_t)pr_in(r, f->lo_lo, f->lo_hi);
    if (p->ftype == FT_CUT) {                    /* (a cut: nearer open than a shelf's middle) */
        p->high = (int16_t)clampi(p->high + 250, 0, PMAX);
        p->low = (int16_t)clampi(p->low + 250, 0, PMAX);
    }
    p->mtype = (int16_t)rnd_weighted(r, f->mt, MT_N);
    p->mgain = (int16_t)pr_in(r, f->mg_lo, f->mg_hi);
    p->mfreq = (int16_t)(f->cluster ? clampi(mf_base + pr_in(r, -18, 18), 0, PMAX) : pr_in(r, f->mf_lo, f->mf_hi));
    p->pan = (int16_t)pr_in(r, -42, 42);
    p->mute = 0;
    p->mainmix = 1;
    p->send1 = p->send2 = 0;
    memset(&p->mod, 0, sizeof p->mod);
}
static void draw_slot(rnd_t *r, const family_t *f, slot_par_t *s, uint32_t in, uint32_t out)
{
    s->fx = (int16_t)rnd_weighted(r, f->fx, FX_N);
    s->p1 = (int16_t)pr_in(r, 100, 900);
    s->p2 = (int16_t)pr_in(r, 100, 850);
    s->pres = (int16_t)(in == FI_INS ? pr_in(r, 300, 850) : PMAX);
    s->lofi = (uint8_t)rnd_chance(r, f->lofi);
    s->bypass = 0;
    s->in = (uint8_t)in;
    s->out = (uint8_t)out;
}
/* the family's gestures on channel c: each LFO a gesture, each its slot (or the channel's modulation thrown wide) */
static struct { uint8_t on, rate[3], shape[3]; } motion_sync;   /* (a synced family's rate and shape a gesture) */
static void draw_motion(rnd_t *r, const family_t *f, ch_par_t *p, uint32_t seed)
{
    uint32_t i, used = 0;
    if (rnd_chance(r, f->chaos)) {
        mod_random(&p->mod, seed);
        return;
    }
    for (i = 0; i < 3u && used < 2u; i++) {
        const gesture_t *g = &f->g[i];
        uint32_t k, d;
        if (!g->p || !rnd_chance(r, g->p))
            continue;
        d = g->dst;
        if (d == MD_LOOP)                        /* (the control that makes this channel's loop) */
            d = p->src == SRC_SND1 || p->src == SRC_RET1 ? MD_SEND1 : p->src == SRC_SND2 || p->src == SRC_RET2 ? MD_SEND2
                                                                                                         : MD_LEVEL;
        k = used++;
        {
            uint8_t sh[7];
            uint32_t n = 0, b;
            for (b = 0; b < MS_N; b++)
                if (g->shapes >> b & 1u)
                    sh[n++] = (uint8_t)b;
            p->mod.lfo[k].shape = n ? sh[rnd_below(r, (int32_t)n)] : MS_SINE;
        }
        p->mod.lfo[k].rate = (uint8_t)pr_in(r, g->rlo, g->rhi);
        if (motion_sync.on) {                    /* (in step with the other channels) */
            p->mod.lfo[k].rate = motion_sync.rate[i];
            p->mod.lfo[k].shape = motion_sync.shape[i];
        }
        p->mod.lfo[k].mode = (uint8_t)(rnd_chance(r, 15) && !motion_sync.on ? MM_ONSET : MM_FREE);
        p->mod.lfo[k].fade = (uint8_t)(rnd_chance(r, g->fade) ? pr_in(r, 20, 70) : 0);
        p->mod.s[k].src = (uint8_t)(MSRC_L1 + k);
        p->mod.s[k].dst = (uint8_t)d;
        p->mod.s[k].amt = (int8_t)(pr_in(r, g->alo, g->ahi) * (rnd_chance(r, 50) ? 1 : -1));
    }
    if (used && rnd_chance(r, 12)) {             /* (now and then the loop moves itself: its loudness on its tone) */
        p->mod.s[2].src = MSRC_LEVEL;
        p->mod.s[2].dst = (uint8_t)(rnd_chance(r, 50) ? MD_FREQ : MD_LIM);
        p->mod.s[2].amt = (int8_t)(pr_in(r, 8, 30) * (rnd_chance(r, 50) ? 1 : -1));
    }
}

/* a channel set to sing with its fader at lvl (dB x 10): its preamp's gain worked out for the loop's gain to pass
 * 0 dB by margin (+ extra: what the loop loses elsewhere -- a pan, a bus shared). If the preamp cannot reach it
 * (0 .. +48 dB), the fader moves instead. Returns the fader's value */
static int16_t sing_at(ch_par_t *p, int32_t lvl, int32_t margin, int32_t extra)
{
    int32_t need, have;
    p->gain = 0;
    have = loop_db10(p);                         /* (with no preamp gain) */
    need = -lvl + margin + extra;                /* the loop's gain wanted, the fader left out */
    p->gain = (int16_t)clampi((need - have) * PMAX / 480, 0, PMAX);
    have = loop_db10(p);
    if (have > need + 10 || have < need - 10)    /* (the preamp at an end: the fader takes up the rest) */
        lvl = -have + margin + extra;
    return lvl_of_db10(lvl);
}
/* .. a send's value, the same way, for a loop through a bus */
static int16_t sing_send(ch_par_t *p, int32_t snd, int32_t margin, int32_t extra)
{
    int32_t need, have;
    p->gain = 0;
    have = loop_db10(p);
    need = -snd + margin + extra;
    p->gain = (int16_t)clampi((need - have) * PMAX / 480, 0, PMAX);
    have = loop_db10(p);
    if (have > need + 10 || have < need - 10)
        snd = -have + margin + extra;
    return snd_of_db10(snd);
}

/* the main level, so that patches come out alike in loudness: each channel's share estimated (its limiter's
 * ceiling, its level, its pan) and the returns' (what is sent, the return's level), summed as powers */
static int32_t pw_of_db10(int32_t db10) { return (int32_t)exp2_q16(clampi(db10 + 400, -400, 400) * 2177); }   /* (re -40 dB) */
static void patch_balance(lil_par_t *q)
{
    int64_t sum = 0;
    int32_t est, c, nsnd1 = 0, nsnd2 = 0;
    for (c = 0; c < NCH; c++) {
        const ch_par_t *p = &q->ch[c];
        if (p->level && p->mainmix && !p->mute)
            sum += pw_of_db10(lvl_db10(p->level) - 180);
        nsnd1 += p->level && p->send1;
        nsnd2 += p->level && p->send2;
    }
    for (c = 0; c < NSLOT; c++) {
        const slot_par_t *s = &q->slot[c];
        int32_t n = s->in == FI_SEN1 ? nsnd1 : s->in == FI_SEN2 ? nsnd2 : s->in == FI_PREV ? 1 : 0;
        if (!n || s->out == FO_NON)
            continue;
        sum += (int64_t)pw_of_db10(snd_db10(s->out == FO_RET1 ? q->out.ret1 : q->out.ret2) - 60 + 30 * (n - 1)) * 4;
    }
    if (sum <= 0)
        return;
    if (sum > 0x7FFFFFFF)
        sum = 0x7FFFFFFF;
    est = (int32_t)(((int64_t)(log2_q16((uint32_t)sum) - (16 << 16)) * 1972830) >> 32) - 400;   /* dB x 10 (3.0103 dB an octave of power) */
    q->out.level = (int16_t)clampi((-220 - est + 300) * PMAX / 600, 100, PMAX);   /* (to about -22 dB: the trim brings it up, safer than down) */
}

/* a new patch into q, of family fam (-1: one at random), from seed; returns the family */
static uint32_t patch_draw(lil_par_t *q, uint32_t seed, int fam)
{
    rnd_t r;
    const family_t *f;
    uint32_t c, n, i, act[NCH], nring = 0, ring[NCH];
    uint8_t rc[NCH];
    int32_t margin, mf_base;
    rnd_seed(&r, seed);
    patch_plain(q);
    if (fam < 0 || fam >= (int)NFAM)
        fam = rnd_below(&r, (int32_t)NFAM);
    f = &FAM[fam];
    mf_base = pr_in(&r, f->mf_lo, f->mf_hi);
    for (c = 0; c < NCH; c++)
        draw_voice(&r, f, &q->ch[c], mf_base);
    n = (uint32_t)pr_in(&r, f->nlo, f->nhi);     /* the channels heard at once; the rest drawn too, down */
    for (c = 0; c < NCH; c++)
        act[c] = c;
    for (c = NCH - 1u; c > 0u; c--) {
        uint32_t j = (uint32_t)rnd_below(&r, (int32_t)c + 1), t = act[c];
        act[c] = act[j];
        act[j] = t;
    }
    for (i = 0; i < NCH; i++) {                  /* each one's connection */
        rc[i] = (uint8_t)rnd_weighted(&r, f->topo, RC_N);
        if (rc[i] == RC_RING && i < n)
            ring[nring++] = act[i];
    }
    if (nring == 1u)                             /* (a ring of one is a loop of its own) */
        for (i = 0; i < n; i++)
            if (act[i] == ring[0])
                rc[i] = RC_SOLO;
    margin = pr_in(&r, f->m_lo, f->m_hi) * 2;
    for (i = 0; i < NCH; i++) {
        uint32_t k = act[i], heard = i < n;
        ch_par_t *p = &q->ch[k];
        int32_t lv = -pr_in(&r, 0, 140);         /* (the fader where a player would like it) */
        switch (rc[i]) {
        case RC_RING:
            if (nring >= 2u && heard) {
                uint32_t j;
                for (j = 0; j < nring && ring[j] != k; j++)
                    ;
                p->src = (int16_t)(SRC_CH1 + ring[(j + nring - 1u) % nring]);
            } else
                p->src = (int16_t)(SRC_CH1 + act[rnd_below(&r, (int32_t)(n ? n : 1))]);
            p->level = sing_at(p, lv, margin, 15);
            break;
        case RC_WEB:
            p->src = (int16_t)(p->pan < 0 ? SRC_MIXL : SRC_MIXR);
            p->level = sing_at(p, lv, margin, 30 - (int32_t)n * 10);
            break;
        case RC_SEND: {
            int b = rnd_chance(&r, 50);
            p->src = (int16_t)(b ? SRC_SND2 : SRC_SND1);
            if (b)
                p->send2 = sing_send(p, -pr_in(&r, 20, 120), margin, 0);
            else
                p->send1 = sing_send(p, -pr_in(&r, 20, 120), margin, 0);
            p->level = lvl_of_db10(-pr_in(&r, 0, 120));
            break;
        }
        default:
            if (heard && rnd_chance(&r, f->ins)) {
                draw_slot(&r, f, &q->slot[k], FI_INS, FO_NON);
                p->src = (int16_t)(rnd_chance(&r, 70) ? SRC_POST : SRC_PRE);
            } else
                p->src = SRC_PRE;
            p->level = sing_at(p, lv, margin, 0);
            break;
        }
        if (!heard)
            p->level = 0;
    }
    if (rnd_chance(&r, f->ret)) {
        /* loops through the rack: a free slot on send 1 into return 1, sometimes a second, chained or on send 2 */
        uint32_t s1 = SLOT_OUT, s2 = SLOT_OUT;
        for (i = NCH; i-- > 0u;)
            if (q->slot[act[i]].in == FI_NON) {
                s1 = act[i];
                break;
            }
        if (s1 == SLOT_OUT)
            s1 = act[NCH - 1u];
        draw_slot(&r, f, &q->slot[s1], FI_SEN1, FO_RET1);
        if (q->slot[s1].fx == FX_BITCRUSH && rnd_chance(&r, 50))
            q->slot[s1].fx = FX_DELAY;
        if (s1 + 1u < NCH && q->slot[s1 + 1u].in == FI_NON && rnd_chance(&r, 40)) {
            s2 = s1 + 1u;
            draw_slot(&r, f, &q->slot[s2], FI_PREV, FO_RET2);
        } else if (rnd_chance(&r, 40)) {
            for (i = 0; i < NCH; i++)
                if (act[i] != s1 && q->slot[act[i]].in == FI_NON) {
                    s2 = act[i];
                    draw_slot(&r, f, &q->slot[s2], FI_SEN2, FO_RET2);
                    break;
                }
        }
        for (i = 0; i < NCH; i++) {              /* some channels into the loop through the rack */
            uint32_t k = act[i];
            ch_par_t *p = &q->ch[k];
            if (!rnd_chance(&r, f->ret == 100 ? 75 : 45))
                continue;
            p->src = (int16_t)(s2 != SLOT_OUT && rnd_chance(&r, 40) ? SRC_RET2 : SRC_RET1);
            p->send1 = sing_send(p, -pr_in(&r, 20, 100), margin, 30);
            if (s2 != SLOT_OUT && q->slot[s2].in == FI_SEN2)
                p->send2 = (int16_t)clampi(p->send1 - 30, 40, PMAX);
            p->level = i < n ? lvl_of_db10(-pr_in(&r, 0, 120)) : 0;
        }
        q->out.ret1 = (int16_t)pr_in(&r, 420, 640);
        q->out.ret2 = (int16_t)pr_in(&r, 420, 640);
    }
    motion_sync.on = f->sync;
    for (i = 0; i < 3u; i++) {                   /* (a synced family: one rate and shape a gesture, for all) */
        uint32_t b, n = 0;
        uint8_t sh[7];
        motion_sync.rate[i] = (uint8_t)pr_in(&r, f->g[i].rlo, f->g[i].rhi);
        for (b = 0; b < MS_N; b++)
            if (f->g[i].shapes >> b & 1u)
                sh[n++] = (uint8_t)b;
        motion_sync.shape[i] = n ? sh[rnd_below(&r, (int32_t)n)] : MS_SINE;
    }
    for (i = 0; i < NCH; i++)                    /* how each one moves */
        draw_motion(&r, f, &q->ch[act[i]], seed * 2654435761u + i * 40503u);
    if (f->sync)                                 /* (in step: their LFOs from one phase -- the ISR's, at the start) */
        memset(mst, 0, sizeof mst);
    q->out.space = (int16_t)rnd_weighted(&r, f->space, SP_N);
    q->out.spres = (int16_t)pr_in(&r, f->sp_lo, f->sp_hi);
    q->out.mgain = (int16_t)pr_in(&r, 420, 580);
    q->out.mfreq = (int16_t)pr_in(&r, 250, 800);
    if (rnd_chance(&r, 15)) {                    /* now and then the output's own insert */
        draw_slot(&r, f, &q->slot[SLOT_OUT], FI_INS, FO_NON);
        q->slot[SLOT_OUT].pres = (int16_t)pr_in(&r, 200, 600);
    }
    patch_balance(q);
    return (uint32_t)fam;
}

/* SHIFT + black key 1: every parameter of channel c drawn anew -- its input, its voice, its filters, its sends, its
 * slot, its LFOs and matrix, its fader and pan (set together with its gain, so it sings); only its mute kept. From
 * any family's ranges, so anything the families can do, it can */
static void patch_random_channel(lil_par_t *q, uint32_t c, uint32_t seed)
{
    rnd_t r;
    const family_t *f;
    ch_par_t *p = &q->ch[c];
    uint8_t mute = p->mute;
    int32_t margin, lv = 0, src, k;
    int ret1 = 0, ret2 = 0;
    rnd_seed(&r, seed);
    f = &FAM[rnd_below(&r, (int32_t)NFAM)];
    draw_voice(&r, f, p, pr_in(&r, f->mf_lo, f->mf_hi));
    p->noise = (int16_t)pr_in(&r, 250, 750);
    p->pan = (int16_t)pr_in(&r, -PAN_MAX, PAN_MAX);
    p->mainmix = (uint8_t)!rnd_chance(&r, 10);
    for (k = 0; k < NSLOT; k++) {                /* (the returns something feeds) */
        ret1 |= q->slot[k].out == FO_RET1 && q->slot[k].in != FI_NON;
        ret2 |= q->slot[k].out == FO_RET2 && q->slot[k].in != FI_NON;
    }
    do                                           /* its input: anything but itself, a return only if it is fed */
        src = rnd_below(&r, SRC_N);
    while (src == SRC_CH1 + (int32_t)c || src == SRC_NONE || (src == SRC_RET1 && !ret1) || (src == SRC_RET2 && !ret2));
    p->src = (int16_t)src;
    margin = pr_in(&r, 10, 70);
    if (rnd_chance(&r, 45))                      /* its slot: an insert, or left as it was */
        draw_slot(&r, f, &q->slot[c], FI_INS, rnd_chance(&r, 25) ? (uint32_t)(FO_RET1 + rnd_below(&r, 2)) : FO_NON);
    else if (q->slot[c].in == FI_INS)
        q->slot[c].in = FI_NON;
    lv = -pr_in(&r, 0, 160);
    switch (src) {
    case SRC_SND1: case SRC_RET1:
        p->send1 = sing_send(p, -pr_in(&r, 20, 120), margin, src == SRC_RET1 ? 30 : 0);
        p->send2 = (int16_t)(rnd_chance(&r, 30) ? pr_in(&r, 200, 700) : 0);
        p->level = lvl_of_db10(lv);
        break;
    case SRC_SND2: case SRC_RET2:
        p->send2 = sing_send(p, -pr_in(&r, 20, 120), margin, src == SRC_RET2 ? 30 : 0);
        p->send1 = (int16_t)(rnd_chance(&r, 30) ? pr_in(&r, 200, 700) : 0);
        p->level = lvl_of_db10(lv);
        break;
    default:
        p->send1 = (int16_t)(rnd_chance(&r, 30) ? pr_in(&r, 200, 700) : 0);
        p->send2 = (int16_t)(rnd_chance(&r, 30) ? pr_in(&r, 200, 700) : 0);
        p->level = sing_at(p, lv, margin, src == SRC_MIXL || src == SRC_MIXR ? 30 : src >= SRC_CH1 ? 15 : 0);
        break;
    }
    motion_sync.on = 0;
    if (rnd_chance(&r, 50))                      /* its motion: a family's gestures, or thrown wide */
        draw_motion(&r, f, p, seed ^ 0xA5A5u);
    else
        mod_random(&p->mod, seed ^ 0x5A5Au);
    p->mute = mute;
}

/* SHIFT + HOME: every channel's tone a little elsewhere */
static void patch_shift(lil_par_t *q, uint32_t seed)
{
    rnd_t r;
    uint32_t c;
    rnd_seed(&r, seed);
    for (c = 0; c < NCH; c++) {
        ch_par_t *p = &q->ch[c];
        int16_t *f[8] = {&p->gain, &p->noise, &p->high, &p->low, &p->mgain, &p->mfreq, &p->lim, &p->over};
        uint32_t j;
        for (j = 0; j < 8u; j++)
            if (*f[j] > 0 || j > 1u)
                *f[j] = (int16_t)clampi(*f[j] + pr_in(&r, -45, 45), j == 1u ? 1 : 0, PMAX);
    }
}

/* ------------------------------------------------------- the factory's --- */
typedef struct { const char *name; uint8_t fam; uint32_t seed; } factory_t;
static const factory_t FACTORY[] = {
    {"FIRST LIGHT", 0, 101}, {"LONELY RADIO", 11, 7342}, {"CIRCLE", 7, 23}, {"STATIC SKY", 4, 911},
    {"TANGLE", 6, 52}, {"UNDERCURRENT", 15, 3}, {"ECHO CHAMBER", 9, 77}, {"RELAY", 8, 4410},
    {"STATIC FIELD", 5, 1312}, {"NIGHT BIRDS", 1, 808}, {"DEEP WIRE", 12, 2718}, {"MIRROR HALL", 10, 31415},
    {"SIGNAL LOSS", 14, 66}, {"AIR RAID", 2, 2026}, {"CRICKETS", 3, 999}, {"FAULTY MACHINE", 13, 4096}};
#define NFACTORY NELEM(FACTORY)
static void factory_load(lil_par_t *q, uint32_t i)
{
    patch_draw(q, FACTORY[i % NFACTORY].seed, FACTORY[i % NFACTORY].fam);
}

/* the trim: a patch just drawn or loaded is played into for a moment, as a player would before letting it run.
 * For the first two seconds, a channel with its fader up that is still not sounding has its preamp turned up
 * 3 dB at a time (every 150 ms) until it sings -- a loop's pitch may fall between what its filters let through,
 * and no estimate knows that; then the output is listened to for a second, and its main level moved once, so
 * that it comes out near -16 dB. The faders are never touched. Nothing more after that */
static struct { uint8_t on; uint32_t t0, woke_at, hot_since, guard_at; int32_t pk; } trim;
static void trim_start(void)
{
    trim.on = 1;
    trim.t0 = fm1_ms;
    trim.woke_at = fm1_ms;
    trim.pk = 0;
}
/* dB x 10 of a level x / Q15 (x > 0) */
static int32_t db10_of(uint32_t x) { return (int32_t)(((int64_t)(log2_q16(x) - (15 << 16)) * 3945661) >> 32); }
static void trim_poll(void)
{
    uint32_t el = fm1_ms - trim.t0, c;
    /* the guard, always: the output held at its ceiling for a third of a second, the main level a dB down */
    if (out_meter > 29000) {
        if (!trim.hot_since)
            trim.hot_since = fm1_ms | 1u;
        else if ((uint32_t)(fm1_ms - trim.hot_since) >= 330u && (uint32_t)(fm1_ms - trim.guard_at) >= 330u) {
            trim.guard_at = fm1_ms;
            par.out.level = (int16_t)clampi(par.out.level - PMAX / 60, 0, PMAX);   /* (-1 dB) */
        }
    } else
        trim.hot_since = 0;
    if (!trim.on)
        return;
    if (el < 2400u) {
        if (el >= 600u && (uint32_t)(fm1_ms - trim.woke_at) >= 150u) {
            trim.woke_at = fm1_ms;
            for (c = 0; c < NCH; c++) {
                ch_par_t *p = &par.ch[c];
                if (p->level && !p->mute && ch_meter[c] < 600 && p->gain < PMAX)
                    p->gain = (int16_t)clampi(p->gain + 63, 0, PMAX);   /* (+3 dB) */
            }
        }
        out_pw = 0;
        out_pw_n = 0;
        trim.pk = 0;
        return;
    }
    if (out_meter > trim.pk)
        trim.pk = out_meter;
    if (el < 3600u || !out_pw_n)
        return;
    trim.on = 0;
    {
        uint32_t mean = out_pw / out_pw_n;       /* (mean of (|x| / 64)^2 over a block / 32: x 32 back) */
        int32_t db10, d;
        if (mean < 1u)
            return;                              /* (silent: nothing to go by) */
        /* dB of the rms: 10 log10(mean) + 20 log10(64 / 32768), and 20 log10(1 / 512) is 18 octaves of power */
        db10 = (int32_t)(((int64_t)(log2_q16(mean) - (18 << 16)) * 1972830) >> 32);
        d = -160 - db10;                         /* to about -16 dB .. */
        if (trim.pk > 0 && d > -40 - db10_of((uint32_t)trim.pk))
            d = -40 - db10_of((uint32_t)trim.pk);   /* (.. its peaks under -4 dB) */
        par.out.level = (int16_t)clampi(par.out.level + d * PMAX / 600, 40, PMAX);
    }
}
