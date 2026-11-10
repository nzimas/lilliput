/* SPDX-License-Identifier: GPL-3.0-only
 * LILLIPUT: the DRUMS (build 16) -- a drum section beside the no-input mixer, in the spirit of Bram Bos's Ruismaker
 * Noir: synthesized, punchy and dirty, for rhythmic noise. Three voices -- KICK, SNARE, HATS -- of one engine, told
 * apart by their sounds (drumkit.c's recipes), each with its own lane of steps on the RHYTHM's tempo.
 *
 * A voice: a tone (a sine, or a metallic one: a sine whose phase another sine bends) swept down in pitch from SWEEP
 * octaves above over BEND, and noise through a filter (low-, band- or high-pass at COLOR), mixed by NOISE; an envelope
 * that rises over ATTACK (0: at once -- the hard transient) and falls over DECAY; a CLICK (a burst of noise a couple of
 * milliseconds long) on top; then DRIVE (a tanh, up to 32 times over). Its LEVEL and PAN into the output, after the
 * reverb, so SPACE never washes the drums away; its REVERB send into SPACE's reverb.
 *
 * As in Noir, a step can carry more than a hit: an accent, a pitch of its own (semitones), and ALT -- the decay cut to a
 * fifth (a closed hat after open ones, a ghost snare, a choked kick) -- so one voice plays several sounds.
 *
 * The voices run in out_stage (CPU1, with the output stage, a block behind the channels); the steps on CPU0 beside the
 * RHYTHM's (drumseq.c). Everything slow -- pitches, times, filters, gains -- is worked out in the main loop when a
 * value moves (drumkit.c's drm_coef), so the audio does only adds and multiplies. */

enum { DV_KICK, DV_SNARE, DV_HAT, NDV };
enum { DN_LP, DN_BP, DN_HP, DN_N };
#define DM_N 7u                  /* METAL: OFF, then six ratios */
typedef struct {
    int16_t tune, sweep, bend, attack, decay, noise, color, click, drive, level, verb, pan;   /* 0 .. PMAX (pan +-PAN_MAX) */
    uint8_t filt, metal, rsv[2];
} dvoice_t;
#define DSTEPS 32u
typedef struct {
    uint32_t hits, acc, alt;     /* its steps: a hit; an accent; ALT (the decay a fifth) */
    int8_t pitch[DSTEPS];        /* each step's pitch, semitones (-12 .. +12) */
    uint8_t len, div, prob, rsv; /* 1 .. 32 steps; NDIV's step length; a hit's chance, % */
} dlane_t;
typedef struct {
    dvoice_t v[NDV];
    dlane_t l[NDV];
    uint8_t on, style, swing, rsv;
    uint8_t kit[NDV], rsv2;      /* each voice's sound, as drawn (its recipe: the name) */
    int16_t level, rsv3;         /* the drums' level, 0 .. PMAX (707: as drawn) */
} drm_par_t;
static drm_par_t drm;

/* a voice's coefficients (drumkit.c's drm_coef, the main loop) */
typedef struct {
    uint32_t inc0, sinc;         /* the tone's phase step; the sweep's, at its top */
    uint32_t pdec, adec, adec_alt, cdec;   /* a sample's fall, Q32: the sweep, the level (and ALT's), the click */
    uint32_t att;                /* the rise a sample, Q30 (0: at once) */
    svf_c nf;
    int32_t gtone, gnoise, gclick, drv;    /* Q15; DRIVE Q12 */
    int32_t gl, gr, gs;          /* into the output, left and right; into the reverb (Q15) */
    uint32_t mrat;               /* METAL: the bending sine's ratio, Q16 (0: a plain sine) */
    uint8_t filt, rsv[3];
} dcoef_t;
static dcoef_t drc[NDV];
typedef struct {
    uint32_t ph, mph, inc, sinc, penv, aenv, cenv, dec;
    int32_t acc;
    uint8_t up, live, rsv[2];
    svf_t ns;
} dvst_t;
static dvst_t dvs[NDV];
static uint32_t drm_rng = 0x2545F491u;
static volatile struct { uint8_t fire, acc, alt; int8_t pitch; } dtrig[NDV];   /* this block's hits (CPU0, for the stage) */
static int32_t drm_l[CTL], drm_r[CTL], drm_s[CTL];   /* the drums this block: dry left, right; into the reverb */
static uint8_t drm_wet;          /* (a voice sounding with a reverb send, or its tail: the stage's reverb runs with them) */
static uint32_t drm_tail;        /* (blocks the reverb runs on for the drums' tail) */
static uint8_t drm_reset = 1;    /* (the steps from the top: 1, on the RHYTHM's next beat if it plays; 2, now) */
static volatile uint8_t drm_live;   /* (a voice sounding: the flash is not blanked under it) */

static const uint32_t DSEMI[25] TABLE_RAM = {32768, 34716, 36781, 38968, 41285, 43740, 46341, 49097, 52016, 55109, 58386, 61858, 65536, 69433, 73562, 77936, 82570, 87480, 92682, 98193, 104032, 110218, 116772, 123715, 131072};   /* (2^(n / 12), Q16, n -12 .. 12) */
AINL uint32_t mul32(uint32_t a, uint32_t b) { return (uint32_t)(((uint64_t)a * b) >> 32); }
/* the drums of a block, into drm_l / drm_r / drm_s (out_stage, CPU1) */
static FAST void drums_render(void)
{
    uint32_t c, i, live = 0, wet = 0;
    for (i = 0; i < CTL; i++)
        drm_l[i] = drm_r[i] = drm_s[i] = 0;
    for (c = 0; c < NDV; c++) {
        dvst_t *v = &dvs[c];
        const dcoef_t *k = &drc[c];
        if (dtrig[c].fire) {                     /* a hit: from the top, at its step's pitch */
            uint32_t r = DSEMI[clampi(dtrig[c].pitch, -12, 12) + 12];   /* (2^(semitones / 12)) */
            v->inc = (uint32_t)(((uint64_t)k->inc0 * r) >> 16);
            v->sinc = (uint32_t)(((uint64_t)k->sinc * r) >> 16);
            v->ph = v->mph = 0;
            v->penv = v->cenv = 1u << 30;
            v->aenv = k->att ? 0u : 1u << 30;
            v->up = (uint8_t)(k->att != 0u);
            v->dec = dtrig[c].alt ? k->adec_alt : k->adec;
            v->acc = dtrig[c].acc ? Q15 : 20000;
            v->live = 1;
        }
        if (!v->live)
            continue;
        for (i = 0; i < CTL; i++) {
            uint32_t inc = v->inc + (uint32_t)(((uint64_t)v->sinc * v->penv) >> 30);
            int32_t tone, n, f, bp, lp, hp, x;
            v->penv = mul32(v->penv, k->pdec);
            v->ph += inc;
            if (k->mrat) {                       /* METAL: the phase bent by another sine, at a ratio */
                v->mph += (uint32_t)(((uint64_t)inc * k->mrat) >> 16);
                tone = sin_ph(v->ph + ((uint32_t)sin_ph(v->mph) << 16));
            } else
                tone = sin_ph(v->ph);
            drm_rng = drm_rng * 1664525u + 1013904223u;
            n = (int32_t)drm_rng >> 16;
            hp = svf3(&v->ns, &k->nf, n, &bp, &lp);
            f = k->filt == DN_LP ? lp : k->filt == DN_BP ? mulr24(k->nf.k, bp) : hp;
            x = mulq15(tone, k->gtone) + mulq15(f, k->gnoise);
            if (v->up) {                         /* ATTACK: the rise */
                v->aenv += k->att;
                if (v->aenv >= 1u << 30) {
                    v->aenv = 1u << 30;
                    v->up = 0;
                }
            } else
                v->aenv = mul32(v->aenv, v->dec);
            x = mulq15(x, (int32_t)(v->aenv >> 15));
            x += mulq15(mulq15(n, k->gclick), (int32_t)(v->cenv >> 15));   /* CLICK */
            v->cenv = mul32(v->cenv, k->cdec);
            x = mulq15(tanh_q15((int32_t)(((int64_t)x * k->drv) >> 12)), v->acc);   /* DRIVE */
            drm_l[i] += mulq15(x, k->gl);
            drm_r[i] += mulq15(x, k->gr);
            drm_s[i] += mulq15(x, k->gs);
        }
        wet |= k->gs != 0;
        if (!v->up && v->aenv < (1u << 15))      /* (-90 dB: done) */
            v->live = 0;
        live |= v->live;
    }
    if (wet)                                     /* (the reverb runs on ~4 s after: the tail heard out) */
        drm_tail = 6000u;
    else if (drm_tail)
        drm_tail--;
    drm_wet = (uint8_t)(drm_tail != 0u);
    drm_live = (uint8_t)live;
}
