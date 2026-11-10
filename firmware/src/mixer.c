/* SPDX-License-Identifier: GPL-3.0-only
 * LILLIPUT: the no-input mixer -- six channels, a rack of seven effect slots, two send and two return buses, the
 * main mix and the output. After NoInputMixer (Igor Vasiliev), its signal flow and its controls; written anew for
 * the FM-1, in fixed point.
 *
 * A channel, left to right (as the app's block diagram):
 *
 *   input selector -> + noise -> (AC coupling) -> GAIN -> tube -> FOLD -> op-amp (with its overload protection) ->
 *   HIGH and LOW shelf / cut -> peak / notch / band-pass -> brickwall limiter -> mute ->
 *     SEND 1, SEND 2 (before the level) -> LEVEL -> [insert effect: its slot] -> PAN -> main mix on / off
 *
 * The input selector is what makes the feedback loop:
 *   PRE      the channel's own output after its level, before its insert: a loop of the channel alone
 *   POST     .. after its insert effect: the effect inside the loop
 *   MIX L/R  a side of the mix bus: every channel feeding the mix, and the returns, in one loop
 *   SEND 1/2 a send bus: every channel feeding it
 *   RET 1/2  a return bus: through the rack's effects on the sends
 *   NONE     no input: the noise generator alone
 *   CH 1..6  (Lilliput's own) another channel's output, after its insert: channels patched in a ring or a chain
 *
 * EVERYTHING RUNS A SAMPLE AT A TIME. Every bus and every channel's output is read as it was one sample before
 * (z^-1): the shortest loop is a sample and the limiter's look-ahead, so a loop's pitch is free from the lowest to
 * the highest, as the app's. (Blocks of 32 would put every loop's pitch at 1.4 kHz and its harmonics.) The
 * limiter's look-ahead is the variable delay that sets a loop's base tone: LIMIT, from fast (short, high) to slow
 * (long, low), as the app's manual has it.
 *
 * The rack: slots 1..6, each its channel's insert (IN INS) or fed from a send bus (SEN 1 / SEN 2) or from the slot
 * before it (PREV); its output to a return bus (RET 1 / RET 2) or nowhere. Slot 7 is the output's insert. Effects:
 * DELAY, FLANGER, CHORUS, TREMOLO, FORMANT, BITCRUSH, GLITCH, and (build 4) RESONATOR, XMOD, DUCK, GATE -- two
 * parameters each, a presence, HI-RES or LO-FI. The new four's tunings and choices are steps, never glided: no
 * modulation reaches them (no pitch instability added).
 *
 * The output: the mix bus and the two returns, an equaliser, slot 7 as insert, the SPACE reverb (six spaces,
 * Dattorro's plate set for each: reverb.c), the MASTER knob, a ceiling. It runs a block late, on CPU1 if it is
 * there: it is in no loop.
 *
 * The main loop owns the parameters (par); the audio ISR reads them each block, works out what they mean
 * (coefficients, gains) when they change, and glides the gains across the block. */

#define NCH 6
#define NLAYER NCH               /* (the platform's files count tracks: midi.c, setup.c, usb.c) */
#define NSLOT 7                  /* six channel slots, then the output's */
#define SLOT_OUT 6u

enum { SRC_PRE, SRC_POST, SRC_MIXL, SRC_MIXR, SRC_SND1, SRC_SND2, SRC_RET1, SRC_RET2, SRC_NONE,
       SRC_CH1, SRC_CH2, SRC_CH3, SRC_CH4, SRC_CH5, SRC_CH6, SRC_N };
enum { NZ_WHITE, NZ_PINK, NZ_RED, NZ_CRACKLE, NZ_GEIGER, NZ_HUM, NZ_N };
enum { TB_12AX7, TB_12AT7, TB_6C16, TB_EF86, TB_6L6, TB_300B, TB_N };
enum { TM_LIN, TM_SAT, TM_N };
enum { OA_TL072, OA_NE5532, OA_4558, OA_741, OA_OP07, OA_3140, OA_N };
enum { FT_SHELF, FT_CUT, FT_N };
enum { MT_PEAK, MT_NOTCH, MT_BAND, MT_N };
enum { FX_DELAY, FX_FLANGER, FX_CHORUS, FX_TREMOLO, FX_FORMANT, FX_BITCRUSH, FX_GLITCH,
       FX_RESON, FX_XMOD, FX_DUCK, FX_GATE,
       FX_CYCLES, FX_REED, FX_OCTAVE, FX_DIFFUSE, FX_SAG, FX_CUTS, FX_N };
enum { CY_LOSS, CY_DISINT, CY_WAVESET, CY_N };   /* CYCLES' modes */
enum { OC_ONE, OC_TWO, OC_BOTH, OC_N };          /* OCTAVER's */
#define NDIFF 8                  /* DIFFUSER's sizes */
#define NSAGW 9                  /* SAG's windows: 64 .. 16384 samples, doubling */
#define NCHORD 8                 /* RESONATOR's chords */
#define NXSRC 10                 /* XMOD's, DUCK's and GATE's other signal: CH 1 .. 6, MIX L, MIX R, SEND 1, SEND 2 */
enum { XM_RING, XM_XOR, XM_FOLD, XM_COMP, XM_N };
enum { FI_NON, FI_INS, FI_SEN1, FI_SEN2, FI_PREV, FI_N };
enum { FO_NON, FO_RET1, FO_RET2, FO_N };
enum { SP_ROOM, SP_HALL, SP_PLATE, SP_CAVE, SP_ABYSS, SP_VOID, SP_N };

/* ------------------------------------------------------------ parameters --- *
 * The continuous ones 0 .. PMAX (a thousand steps: the smallest turn can matter here, as the app's manual says);
 * the choices their enum. The main loop writes them; the ISR reads each as a whole (16 bits). */
#define PMAX 1000
#define LVL_MAX 721              /* LEVEL in tenths of a dB: 1 = -60.0 dB .. 601 = 0 dB .. 721 = +12.0 dB; 0 off */
#define LVL_0DB 601
#define PAN_MAX 50               /* PAN: -50 (left) .. +50 (right), a step a detent */

/* each channel's modulation (mod.c): two LFOs, four slots */
enum { MS_SINE, MS_TRI, MS_SAWU, MS_SAWD, MS_SQR, MS_STEPS, MS_DRIFT, MS_N };
enum { MM_FREE, MM_ONSET, MM_ONCE, MM_N };
enum { MSRC_OFF, MSRC_L1, MSRC_L2, MSRC_LEVEL, MSRC_OUT, MSRC_RAND, MSRC_N };
enum { MD_GAIN, MD_NOISE, MD_FREQ, MD_MID, MD_HIGH, MD_LOW, MD_LIM, MD_OVER, MD_LEVEL, MD_PAN, MD_SEND1, MD_SEND2,
       MD_FX1, MD_FX2, MD_FXMIX, MD_RATE1, MD_RATE2, MD_FOLD, MD_N };
typedef struct { uint8_t shape, rate, mode, fade; } mlfo_t;
typedef struct { uint8_t src, dst; int8_t amt; } mslot_t;
#define NMSLOT 8u
typedef struct {
    mlfo_t lfo[2];
    mslot_t s[NMSLOT];
} mod_t;                                         /* (a channel's: 32 bytes) */

typedef struct {
    int16_t src;                 /* SRC_* */
    int16_t gain;                /* the preamp: 0 .. +48 dB */
    int16_t noise;               /* the generator: 0 off, then -90 .. -18 dB (GEIGER: the clicks' rate) */
    int16_t ntype;               /* NZ_* */
    int16_t tube, tmode;         /* TB_*, TM_* */
    int16_t opamp;               /* OA_* */
    int16_t fold;                /* the wavefolder after the tube: 0 off, then x 1 .. x 16 into the fold */
    int16_t over;                /* the overload protection's threshold: 0.5 .. 16 x full scale */
    int16_t lim;                 /* the limiter's speed: fast (short look-ahead, a high loop) .. slow */
    int16_t ftype;               /* FT_*: HIGH and LOW as shelves or as cuts */
    int16_t high, low;           /* shelf: 500 flat, -15 .. +15 dB; cut: 1000 open */
    int16_t mtype;               /* MT_* */
    int16_t mgain;               /* peak: 500 flat, -18 .. +18 dB; notch, band-pass: wide .. narrow */
    int16_t mfreq;               /* 30 Hz .. 16 kHz */
    int16_t level;               /* 0 off, then tenths of a dB from -60 (LVL_MAX, LVL_0DB) */
    int16_t pan;                 /* -PAN_MAX (left) .. PAN_MAX (right) */
    int16_t send1, send2;        /* 0 off, -60 .. +6 dB */
    uint8_t mute, mainmix;       /* mainmix 0: nothing to the mix bus (the sends still fed) */
    mod_t mod;                   /* its LFOs and modulation slots */
} ch_par_t;
typedef struct {
    int16_t fx, p1, p2, pres;    /* FX_*; its two parameters; how much of it is heard */
    uint8_t lofi, bypass, in, out;   /* FI_*, FO_* */
} slot_par_t;
typedef struct {
    int16_t ret1, ret2;          /* the returns to the output: 0 off, -60 .. +6 dB */
    int16_t ftype, high, low;    /* the equaliser, as a channel's */
    int16_t wide;                /* the peak: 0 narrow, 1 wide */
    int16_t mgain, mfreq;
    int16_t space, spres;        /* SP_*; the reverb's presence (at the top: the reverb alone) */
    int16_t level;               /* the main level: -30 .. +30 dB (500: 0 dB), before the MASTER knob */
    uint8_t mute, rsv;
} out_par_t;
/* the RHYTHM (build 6): a lane a channel -- its steps, played on a clock, gating, choking, kicking or accenting it */
enum { RM_OFF, RM_CHOP, RM_CHOKE, RM_KICK, RM_ACCENT, RM_N };
#define NDIV 8                   /* a lane's step: 1/2, 1/4, 1/8, 1/8 T, 1/16, 1/16 Q (5), 1/16 T, 1/32 */
typedef struct {
    uint32_t hits, acc;          /* its steps (a bit each, 32 at most): a hit; an accent */
    uint8_t mode, len, div, gate;   /* RM_*; 1 .. 32 steps; NDIV's; how long a hit holds, % of a step (1 .. 100) */
    uint8_t depth, prob, ratchet, mut;   /* how far the gate closes, %; a hit's chance, %; a roll's chance, %; the
                                          * steps' chance to flip each round, % (a Turing machine's) */
} lane_t;
typedef struct {
    lane_t lane[NCH];
    uint16_t bpm;                /* 10 .. 500 */
    uint8_t on, style;           /* playing; the style it was drawn in */
    uint8_t swing, chaos, gscale, rsv;   /* 0 .. 100: the off steps late; hits skipped and ghosts added; the gates'
                                          * length (50: as drawn) */
} rhy_par_t;
typedef struct {
    ch_par_t ch[NCH];
    slot_par_t slot[NSLOT];
    out_par_t out;
    rhy_par_t rhy;               /* (build 6: at the end -- an older record, shorter, reads with the rhythm off) */
} lil_par_t;
static lil_par_t par;

/* ------------------------------------------------- the ISR's side: state --- */
#define LIM_T 16384              /* the limiter's ceiling: half of full scale (the buses have room for six) */
#define LIM_LEN 2048u            /* its look-ahead line (16 bits, x 1/2) */
#define LIM_MASK (LIM_LEN - 1u)
#define SLOT_LEN 16384u          /* an effect slot's line: 0.37 s (HI-RES), 0.74 s (LO-FI) */
#define SLOT_MASK (SLOT_LEN - 1u)
static int16_t lim_mem[NCH][LIM_LEN] POOL;
static int16_t slot_mem[NSLOT][SLOT_LEN] POOL;

typedef struct {
    /* the coefficients, from the parameters (each block, when they change) */
    svf_c fh, fl, fm;            /* the HIGH, LOW and middle filters */
    int32_t ah, al, am;          /* shelf / peak: A - 1 (Q15) */
    uint8_t ft, mt, src, ntype, opamp, tube, tmode, rsv;
    int32_t drive, bias, kpos, kneg, tb0;   /* the tube (Q12, Q15) */
    int32_t opg, rail, railn, slew, oplp;   /* the op-amp (Q12, Q15) */
    int32_t irail, irailn;       /* .. 1 / its rails (Q15 out of Q15) */
    int32_t fgain;               /* the fold's drive (Q12), 0: no fold */
    int32_t kick;                /* the RHYTHM's KICK: a burst of noise into the input, decaying (Q15) */
    int32_t thr;                 /* the overload's threshold (Q15) */
    uint32_t lahead;             /* the limiter's look-ahead, samples */
    int32_t rel;                 /* .. its release a sample (Q15) */
    int32_t namp;                /* the noise's amplitude (Q15), GEIGER: its chance a sample (of 2^32) */
    /* gains gliding across the block: now, and the step a sample */
    int32_t gp, gl, gpl, gpr, gs1, gs2;   /* preamp; level; pan L R (level and main mix in); sends */
    int32_t dgp, dgl, dgpl, dgpr, dgs1, dgs2;
    /* the running state */
    svf_t sh, sl, sm;
    int32_t dcx, dcy;            /* the AC coupling (dcy Q23) */
    int32_t pk0, pk1, pk2, red;  /* the pink and red noises */
    int32_t click;               /* GEIGER's click, decaying */
    uint32_t hum;
    int32_t opy;                 /* the op-amp's last output (its slew and bandwidth) */
    int32_t trip;                /* samples left with the overload protection open */
    int32_t env, g;              /* the limiter's envelope, its gain (Q15) */
    uint32_t hold, lw, gcnt;
    rnd_t r;
    int32_t pk;                  /* the output's peak this block (the meter) */
    ch_par_t seen;               /* the parameters the coefficients were worked out for */
    uint8_t fresh;
} ch_rt_t;
static ch_rt_t chr[NCH];

typedef struct {
    uint8_t fx, in, out, lofi, bypass, odd, rsv[2];
    int32_t pres;                /* Q15 */
    int32_t dt;                  /* the delay's time it glides to (samples, Q8) */
    int32_t d;                   /* .. where it is */
    int32_t fb;                  /* feedback (Q15) */
    uint32_t inc;                /* the LFO's step */
    int32_t depth, base;         /* modulation's depth, its centre (samples, Q8) */
    svf_c c1, c2;                /* FORMANT's two */
    int32_t k1, k2;              /* .. their k (Q24), for the band-passes at unity */
    int32_t sh, hold_n, xm, off; /* BITCRUSH: the bits cut, the hold, the XOR's shift, the offset */
    uint32_t glen, greps;        /* GLITCH: the stuck part, how many times */
    int32_t rc[6], rs[6];        /* RESONATOR: each band's rotation, r cos and r sin (Q30) */
    int32_t r1, r2[6];           /* .. 1 - r (Q30); the bands' states (r2: the second) */
    uint8_t xmode, xsrc, rbands, rsv2;   /* XMOD's mode; the other signal (XMOD, DUCK, GATE); RESONATOR's bands sounding */
    int32_t env, gg;             /* DUCK, GATE: the key's envelope, the gain gliding */
    int32_t ferr;                /* the feedback's rounding error, kept for the next sample (DELAY, FLANGER) */
    /* state */
    uint32_t w, lfo, gpos, gleft, gstart, hcnt;
    int32_t last, held;
    svf_t s1, s2;
    int32_t r1s[6];              /* RESONATOR: the bands' first states */
    /* (build 5) */
    uint32_t tw;                 /* samples written, counted on (CYCLES' WAVESET, CUTS: places in the line) */
    int32_t zp, mult;            /* CYCLES: the last input; the half-cycle's gain (Q15) */
    uint32_t cyc, cyp;           /* .. the cycles counted; DISINTEGRATE's chance (of 2^32); WAVELOSS' cycles lost of 8 */
    uint32_t wb[32], wbw, wbr;   /* WAVESET: the cycles' starts, a ring */
    uint32_t cs, ce, rd, rleft, reps;   /* .. the cycle played: its start, end, the place, repeats left; how many */
    int32_t rlen, rz, breath;    /* REED: the bore (samples, Q8); its one-zero filter; the breath (Q15) */
    int32_t olp, otone, ot;      /* OCTAVER: the detector's low-pass; the tone's coefficient and state */
    uint8_t ff1, ff2, omode, cmode;   /* .. its two flip-flops, its mode; CYCLES' mode */
    uint16_t dlen[4], didx[4];   /* DIFFUSER: its four allpasses' lengths and places */
    int32_t dg;                  /* .. their gain (Q15) */
    uint32_t ssum, swin, sshift; /* SAG: the window's sum of |x|, its length, its log2 */
    uint32_t cut_left, cpos, cseg, cstart, cunit;   /* CUTS: the cut's samples left; the place in its repeat; the repeat's
                                                     * length, its start; the unit */
    uint8_t cpat, cpi, crep, rsv5;                  /* .. the pattern, the place in it, repeating */
    rnd_t r;
    slot_par_t seen;
    uint8_t fresh;
} slot_rt_t;
static slot_rt_t slr[NSLOT];

/* the buses, as they were a sample ago, and each channel's outputs */
static int32_t b_mixl, b_mixr, b_s1, b_s2, b_r1, b_r2;
static int32_t ch_pre[NCH], ch_post[NCH];
static int32_t slot_y[NSLOT];

/* a block's work: the output's raw mix (L R) and the channels for USB, then the output stage's */
static int32_t raw_l[CTL], raw_r[CTL], raw_r1[CTL], raw_r2[CTL];
static int32_t usb_ch[NCH][CTL];
static volatile uint8_t ch_meter_q[NCH];          /* (each channel's level for the screen and the lights) */
static int32_t ch_meter[NCH];
static int32_t out_meter;
static volatile uint32_t out_pw, out_pw_n;       /* (the output's power, summed a block at a time: the trim's) */
static volatile uint8_t shed_req;                /* (main.c sets it on an overload: nothing here to shed) */
static volatile uint8_t load_pct;
static volatile uint8_t load_pk;
static int32_t master_q12 = 2048;                /* the MASTER knob (main loop), Q12 */
static uint32_t blk_clock;
static int32_t out_mute_g = Q15;                 /* PLAY's mute, gliding */
static volatile uint32_t mx_ticks, out_ticks;    /* (the CPU page's) */

/* ---------------------------------------------------- the tables of types --- */
static const char *const SRC_NAME[SRC_N] = {"PRE", "POST", "MIX L", "MIX R", "SEND 1", "SEND 2", "RET 1", "RET 2",
                                            "NONE", "CH 1", "CH 2", "CH 3", "CH 4", "CH 5", "CH 6"};
static const char *const NZ_NAME[NZ_N] = {"WHITE", "PINK", "RED", "CRACKLE", "GEIGER", "HUM"};
static const char *const TB_NAME[TB_N] = {"12AX7", "12AT7", "6C16", "EF86", "6L6", "300B"};
static const char *const TM_NAME[TM_N] = {"LINEAR", "SATURATED"};
static const char *const OA_NAME[OA_N] = {"TL072", "NE5532", "4558", "741", "OP07", "CA3140"};
static const char *const MT_NAME[MT_N] = {"PEAK", "NOTCH", "BAND"};
static const char *const FX_NAME[FX_N] = {"DELAY", "FLANGER", "CHORUS", "TREMOLO", "FORMANT", "BITCRUSH", "GLITCH",
                                          "RESONATOR", "XMOD", "DUCK", "GATE", "CYCLES", "REED", "OCTAVER", "DIFFUSER",
                                          "SAG", "CUTS"};
static const char *const CY_NAME[CY_N] = {"WAVELOSS", "SPUTTER", "WAVESET"};
static const char *const OC_NAME[OC_N] = {"ONE DOWN", "TWO DOWN", "BOTH"};
static const char *const CHORD_NAME[NCHORD] = {"OCTAVES", "FIFTHS", "MAJOR", "MINOR", "SUS", "CLUSTER", "PELOG",
                                              "HARMONICS"};
static const char *const XSRC_NAME[NXSRC] = {"CH 1", "CH 2", "CH 3", "CH 4", "CH 5", "CH 6", "MIX L", "MIX R", "SEND 1",
                                            "SEND 2"};
static const char *const XM_NAME[XM_N] = {"RING", "XOR", "FOLD", "COMPARE"};
/* RESONATOR's chords: six bands, cents above the root (PELOG: a gamelan's, approximate; HARMONICS: partials 1 .. 6) */
static const int16_t CHORD_CENTS[NCHORD][6] = {
    {0, 1200, 2400, 3600, 4800, 6000}, {0, 700, 1200, 1900, 2400, 3100}, {0, 400, 700, 1200, 1600, 1900},
    {0, 300, 700, 1200, 1500, 1900},   {0, 500, 700, 1200, 1700, 1900},  {0, 100, 200, 300, 1200, 1300},
    {0, 120, 270, 540, 670, 785},       {0, 1200, 1902, 2400, 2786, 3102}};
/* a choice among the slot's parameters (its count), or 0: a value 0 .. PMAX. The choices are steps -- and out of the
 * modulation's reach */
AINL int32_t fx_pcount(int32_t fx, int which)
{
    switch (fx) {
    case FX_RESON: return which ? NCHORD : 48;   /* ROOT: C2 .. B5, a semitone a step; CHORD */
    case FX_XMOD: return which ? NXSRC : XM_N;   /* MODE; WITH */
    case FX_DUCK: case FX_GATE: return which ? NXSRC : 0;   /* DEPTH; KEY */
    case FX_CYCLES: return which ? 0 : CY_N;     /* MODE; AMOUNT */
    case FX_REED: return which ? 0 : 48;         /* NOTE: C2 .. B5, a semitone a step; BREATH */
    case FX_OCTAVE: return which ? 0 : OC_N;     /* MODE; TONE */
    case FX_DIFFUSE: return which ? 0 : NDIFF;   /* SIZE (the lines' lengths: steps, never glided); DENSITY */
    case FX_SAG: return which ? 0 : NSAGW;       /* TIME (its window: steps); DEPTH */
    default: return 0;
    }
}
static const char *const FI_NAME[FI_N] = {"OFF", "INSERT", "SEND 1", "SEND 2", "PREV"};
static const char *const FO_NAME[FO_N] = {"NONE", "RET 1", "RET 2"};
static const char *const SP_NAME[SP_N] = {"ROOM", "HALL", "PLATE", "CAVE", "ABYSS", "VOID"};
/* each effect's two parameters, by name */
static const char *const FX_P1[FX_N] = {"TIME", "DEPTH", "INTENSITY", "RATE", "VOWEL", "ALGORITHM", "LENGTH", "ROOT",
                                        "MODE", "DEPTH", "DEPTH", "MODE", "NOTE", "MODE", "SIZE", "TIME", "UNIT"};
static const char *const FX_P2[FX_N] = {"FEEDBACK", "RATE", "RATE", "VIBRATO", "WIDTH", "OFFSET", "REPEATS", "CHORD",
                                        "WITH", "KEY", "KEY", "AMOUNT", "BREATH", "TONE", "DENSITY", "DEPTH", "DENSITY"};

/* the tubes: drive when saturated (Q12), bias (Q15: the even harmonics), the halves' gains (Q12: grid current
 * squashes the positive half), as heard: 12AX7 bright and quick to break up, 12AT7 cleaner, 6C16 (a Soviet triode)
 * gritty and lopsided, EF86 a pentode's thick even harmonics, 6L6 a power tube's broad symmetric push, 300B a
 * single-ended triode's soft, asymmetric bloom */
static const struct { int16_t drive, bias, kpos, kneg; } TUBE[TB_N] = {
    {12288, 2600, 4096, 3277}, {9011, 1600, 4096, 3686}, {16384, 4900, 4915, 2867},
    {10240, 3900, 3686, 2458}, {6554, 650, 4096, 4096}, {5325, 6550, 4096, 2048}};
/* the op-amps: gain (Q12), rails + and - (Q15 of full scale), slew a sample (Q15), bandwidth (a one-pole, Q15,
 * 32767 open) -- exaggerated so each is heard in a loop: the 741 and OP07 slow and dark, the 4558 warm, the CA3140
 * lopsided, the TL072 and NE5532 clean and quick */
static const struct { int16_t g; int32_t rp, rn, slew; int16_t lp; } OPAMP[OA_N] = {
    {8192, 52429, 52429, 30000, 32767}, {8192, 58982, 58982, 52000, 32767}, {8192, 42598, 42598, 14000, 24576},
    {8192, 39322, 39322, 3500, 14746}, {8192, 45875, 45875, 2200, 11469}, {8192, 49152, 32768, 20000, 29491}};
/* the spaces: SIZE, DECAY, DAMP (reverb.c's 0 .. 100) */
static const uint8_t SPACE[SP_N][3] = {{30, 30, 55}, {70, 62, 40}, {50, 55, 65}, {85, 80, 28}, {100, 94, 18},
                                       {100, 100, 6}};
/* the vowels, A E I O U: their first two formants (Hz) -- FORMANT's VOWEL glides through them */
static const uint16_t VOWEL[5][2] = {{800, 1150}, {400, 1700}, {300, 2300}, {450, 800}, {325, 700}};

/* ------------------------------------------------------- small helpers --- */
AINL int32_t sat16(int32_t x) { return x > 32767 ? 32767 : x < -32768 ? -32768 : x; }
/* dB in tenths -> a gain, Q15 (up to +48 dB: 8.2e6) */
AINL int32_t db10_q15(int32_t db10)
{
    if (db10 < -1200)
        return 0;
    return (int32_t)(exp2_q16(db10 * 1089) >> 1);   /* log2 = dB / 6.0206: (dB x 10) x 65536 / 60.206 = x 1088.5 */
}
AINL int32_t lvl_q15(int32_t v) { return v <= 0 ? 0 : db10_q15(v - LVL_0DB); }               /* LEVEL */
AINL int32_t snd_q15(int32_t v) { return v <= 0 ? 0 : db10_q15(-600 + v * 660 / PMAX); }   /* SEND, RETURN */
static int32_t lvl_db10(int32_t v) { return v - LVL_0DB; }
static int32_t snd_db10(int32_t v) { return -600 + v * 660 / PMAX; }
AINL int32_t main_db10(int32_t v) { return -300 + v * 600 / PMAX; }        /* the main level: -30 .. +30 dB */
/* a frequency on a log scale: lo * 2^(v / PMAX * oct) */
AINL int32_t hz_log(int32_t v, int32_t lo, int32_t oct_q8)
{
    return (int32_t)(((uint64_t)(uint32_t)lo * exp2_q16(v * oct_q8 / PMAX * 256)) >> 16);
}
AINL int32_t mid_hz(int32_t v) { return hz_log(v, 30, 2319); }            /* 30 Hz .. 16 kHz (9.06 octaves) */
AINL int32_t shelf_db10(int32_t v) { return (v - 500) * 150 / 500; }      /* -15 .. +15 dB */
AINL int32_t hcut_hz(int32_t v) { return hz_log(v, 300, 1552); }          /* 300 Hz .. 20 kHz (6.06 octaves) */
AINL int32_t lcut_hz(int32_t v) { return hz_log(PMAX - v, 20, 1925); }    /* 3 kHz .. 20 Hz (7.2 octaves) */
AINL int32_t peak_db10(int32_t v) { return (v - 500) * 180 / 500; }       /* -18 .. +18 dB */
AINL int32_t bw_q12(int32_t v) { return hz_log(v, 2048, 1280); }          /* Q 0.5 .. 16 (5 octaves), Q12 */
AINL uint32_t lim_len(int32_t v) { return (uint32_t)hz_log(v, 4, 2202); } /* 4 .. ~1600 samples */
AINL int32_t ovl_q15(int32_t v) { return hz_log(v, 16384, 1280); }        /* 0.5 .. 16 x full scale */

/* the zero-delay state-variable filter (dsp.h's), one step giving all three outputs: the high-pass returned,
 * the band-pass (v1) and low-pass (v2) through the pointers */
AINL int32_t svf3(svf_t *s, const svf_c *c, int32_t x, int32_t *bp, int32_t *lp)
{
    int32_t v3 = x - s->ic2;
    int32_t v1 = mulr16(c->a1, s->ic1) + mulr24(c->a2, v3);
    int32_t v2 = s->ic2 + mulr24(c->a2, s->ic1) + mulr24(c->a3, v3);
    s->ic1 = 2 * v1 - s->ic1;
    s->ic2 = 2 * v2 - s->ic2;
    *bp = v1;
    *lp = v2;
    return x - mulr24(c->k, v1) - v2;
}
/* an effect line's sample d (Q8) behind the write index, linearly between two */
AINL int32_t line_rd(const int16_t *b, uint32_t w, int32_t d)
{
    uint32_t i = (w - ((uint32_t)d >> 8)) & SLOT_MASK, f = (uint32_t)d & 255u;
    int32_t a = b[i], c = b[(i - 1u) & SLOT_MASK];
    return a + (((c - a) * (int32_t)f) >> 8);
}

/* ------------------------------------------ the parameters, made usable --- */
static ch_par_t eff_ch[NCH];                     /* (each channel's parameters as the sound has them: modulated) */
static slot_par_t eff_slot[NSLOT];
static FAST void ch_coef(uint32_t c)
{
    ch_rt_t *k = &chr[c];
    ch_par_t now = eff_ch[c];
    if (!k->fresh && fsame(&now, &k->seen, sizeof now))
        return;
    k->fresh = 0;
    k->src = (uint8_t)clampi(now.src, 0, SRC_N - 1);
    k->ntype = (uint8_t)clampi(now.ntype, 0, NZ_N - 1);
    k->tube = (uint8_t)clampi(now.tube, 0, TB_N - 1);
    k->tmode = (uint8_t)clampi(now.tmode, 0, TM_N - 1);
    k->opamp = (uint8_t)clampi(now.opamp, 0, OA_N - 1);
    k->ft = (uint8_t)clampi(now.ftype, 0, FT_N - 1);
    k->mt = (uint8_t)clampi(now.mtype, 0, MT_N - 1);
    {   /* the tube */
        k->drive = TUBE[k->tube].drive;
        k->bias = TUBE[k->tube].bias;
        k->kpos = TUBE[k->tube].kpos;
        k->kneg = TUBE[k->tube].kneg;
        if (k->tmode == TM_LIN) {                /* near linear: half the input, a quarter of the bias, twice out */
            k->drive = 2048;
            k->bias >>= 2;
            k->kpos = 4096 - ((4096 - k->kpos) >> 2);
            k->kneg = 4096 - ((4096 - k->kneg) >> 2);
        }
        k->tb0 = tanh_q15(k->bias);
    }
    k->fgain = now.fold > 0 ? 4096 + clampi(now.fold, 0, PMAX) * 15 * 4096 / PMAX : 0;
    k->opg = OPAMP[k->opamp].g;
    k->rail = OPAMP[k->opamp].rp;
    k->railn = OPAMP[k->opamp].rn;
    k->irail = (int32_t)((1u << 30) / (uint32_t)k->rail);   /* (1 / rail: no division a sample) */
    k->irailn = (int32_t)((1u << 30) / (uint32_t)k->railn);
    k->slew = OPAMP[k->opamp].slew;
    k->oplp = OPAMP[k->opamp].lp;
    k->thr = ovl_q15(clampi(now.over, 0, PMAX));
    k->lahead = lim_len(clampi(now.lim, 0, PMAX));
    if (k->lahead < 4u)
        k->lahead = 4u;
    if (k->lahead > LIM_LEN - 2u)
        k->lahead = LIM_LEN - 2u;
    {   /* the release: 2 ms (fast) .. 400 ms (slow), as the look-ahead lengthens */
        int32_t tau = (int32_t)(88u + (uint32_t)clampi(now.lim, 0, PMAX) * 17552u / PMAX);
        k->rel = 32768 / tau + 1;
    }
    if (k->ntype == NZ_GEIGER)                   /* the clicks' rate: 0.3 .. 300 a second */
        k->namp = now.noise <= 0 ? 0 : (int32_t)(((uint64_t)(uint32_t)hz_log(now.noise, 300, 2550) * 97391u) >> 10);   /* (mHz: 0.3 .. 300 Hz) */
    else
        k->namp = now.noise <= 0 ? 0 : db10_q15(-900 + clampi(now.noise, 0, PMAX) * 720 / PMAX);
    /* the filters */
    if (k->ft == FT_SHELF) {
        svf_coef(&k->fh, 5000, 2896);            /* (Q 0.707) */
        svf_coef(&k->fl, 150, 2896);
        k->ah = db10_q15(shelf_db10(clampi(now.high, 0, PMAX))) - Q15;
        k->al = db10_q15(shelf_db10(clampi(now.low, 0, PMAX))) - Q15;
    } else {
        svf_coef(&k->fh, hcut_hz(clampi(now.high, 0, PMAX)), 2896);
        svf_coef(&k->fl, lcut_hz(clampi(now.low, 0, PMAX)), 2896);
        k->ah = k->al = 0;
    }
    if (k->mt == MT_PEAK) {
        svf_coef(&k->fm, mid_hz(clampi(now.mfreq, 0, PMAX)), 5734);   /* (Q 1.4) */
        k->am = db10_q15(peak_db10(clampi(now.mgain, 0, PMAX))) - Q15;
    } else {
        svf_coef(&k->fm, mid_hz(clampi(now.mfreq, 0, PMAX)), bw_q12(clampi(now.mgain, 0, PMAX)));
        k->am = 0;
    }
    k->seen = now;
}

static FAST void slot_coef(uint32_t s)
{
    slot_rt_t *k = &slr[s];
    slot_par_t now = eff_slot[s];
    int32_t p1, p2;
    if (!k->fresh && fsame(&now, &k->seen, sizeof now))
        return;
    if (!k->fresh && k->fx != (uint8_t)clampi(now.fx, 0, FX_N - 1)) {   /* another effect: its line from silence */
        uint32_t i;
        for (i = 0; i < SLOT_LEN; i++)
            slot_mem[s][i] = 0;
        memset(&k->s1, 0, sizeof k->s1);
        memset(&k->s2, 0, sizeof k->s2);
        memset(k->r1s, 0, sizeof k->r1s);
        memset(k->r2, 0, sizeof k->r2);
        k->env = 0;
        k->gg = Q15;
        k->wbw = k->wbr = 0;
        k->rleft = 0;
        k->cs = k->ce = k->rd = k->tw;
        k->ssum = 0;
        k->swin = 0;
        k->cut_left = 0;
        k->rz = k->olp = k->ot = 0;
        memset(k->didx, 0, sizeof k->didx);
        k->gleft = 0;
        k->d = 0;
    }
    k->fresh = 0;
    k->fx = (uint8_t)clampi(now.fx, 0, FX_N - 1);
    k->in = (uint8_t)clampi(now.in, 0, FI_N - 1);
    k->out = (uint8_t)clampi(now.out, 0, FO_N - 1);
    k->lofi = now.lofi != 0;
    k->bypass = now.bypass != 0;
    k->pres = clampi(now.pres, 0, PMAX) * Q15 / PMAX;
    p1 = clampi(now.p1, 0, PMAX);
    p2 = clampi(now.p2, 0, PMAX);
    switch (k->fx) {
    case FX_DELAY:                               /* 1 ms .. the line's length; feedback 0 .. 0.98 */
        k->dt = hz_log(p1, 44, 2183) << 8;       /* (8.53 octaves: 44 .. 16300 samples) */
        if (k->dt > (int32_t)(SLOT_LEN - 8u) << 8)
            k->dt = (int32_t)(SLOT_LEN - 8u) << 8;
        k->fb = p2 * 32112 / PMAX;
        if (!k->d)
            k->d = k->dt;
        break;
    case FX_FLANGER:                             /* 0.2 .. 6 ms swept; 0.02 .. 5 Hz */
        k->base = 9 << 8;
        k->depth = (9 + p1 * 255 / PMAX) << 8;
        k->inc = hz8_inc(hz_log(p2, 5, 2039));   /* (Hz Q8: 0.02 .. 5 Hz, 7.96 octaves) */
        k->fb = 21299;                           /* 0.65 */
        break;
    case FX_CHORUS:                              /* 12 ms, swept up to +-8 ms; 0.05 .. 3 Hz */
        k->base = 529 << 8;
        k->depth = (p1 * 352 / PMAX) << 8;
        k->inc = hz8_inc(hz_log(p2, 13, 1512));
        break;
    case FX_TREMOLO:                             /* 0.1 .. 20 Hz; vibrato up to 3 ms */
        k->inc = hz8_inc(hz_log(p1, 26, 1957));
        k->depth = (p2 * 132 / PMAX) << 8;
        break;
    case FX_FORMANT: {                           /* A E I O U, glided between; width: Q 1.5 .. 24 */
        int32_t pos = p1 * 4, i = pos / PMAX, f = pos - i * PMAX, q;
        int32_t f1, f2;
        if (i >= 4) {
            i = 3;
            f = PMAX;
        }
        f1 = VOWEL[i][0] + (VOWEL[i + 1][0] - VOWEL[i][0]) * f / PMAX;
        f2 = VOWEL[i][1] + (VOWEL[i + 1][1] - VOWEL[i][1]) * f / PMAX;
        q = hz_log(PMAX - p2, 6144, 1024);       /* (Q 24 .. 1.5 as WIDTH rises: Q12, 4 octaves) */
        svf_coef(&k->c1, f1, q);
        svf_coef(&k->c2, f2, q);
        k->k1 = k->c1.k;
        k->k2 = k->c2.k;
        break;
    }
    case FX_BITCRUSH:
        if (p1 < 333) {                          /* fewer bits: 16 .. 2 */
            k->sh = p1 * 14 / 333;
            k->hold_n = 1;
            k->xm = 0;
        } else if (p1 < 666) {                   /* 6 bits, and the rate held down: 1 .. 32 samples */
            k->sh = 10;
            k->hold_n = 1 + (p1 - 333) * 31 / 333;
            k->xm = 0;
        } else {                                 /* the stream's bits folded on themselves */
            k->sh = 6;
            k->hold_n = 1 + (p1 - 666) * 7 / 334;
            k->xm = 1 + (p1 - 666) * 9 / 334;
        }
        k->off = (p2 - 500) * 16384 / 500;
        break;
    case FX_RESON: {                             /* six bands at the chord's pitches on the root, ringing ~1.2 s */
        int32_t root = clampi(p1, 0, 47), ch = clampi(p2, 0, NCHORD - 1), b;
        k->r1 = 140000;                          /* 1 - r: 6.9 / (1.2 s x FS), Q30 */
        k->rbands = 0;
        for (b = 0; b < 6; b++) {
            int32_t c100 = (36 + root - 69) * 100 + CHORD_CENTS[ch][b];   /* cents from A4 */
            int32_t hz8 = (int32_t)(((uint64_t)440u * 256u * exp2_q16(c100 * 4096 / 75)) >> 16);   /* (x 65536 / 1200) */
            uint32_t inc = hz8_inc(hz8), sh, chh;
            int64_t cq, sq, r = (1 << 30) - k->r1;
            if (hz8 > 15000 * 256)               /* (too high: the band off) */
                break;
            sh = (uint32_t)sin_ph(inc >> 1);     /* sin and cos of half the angle: the rotation exactly unit */
            chh = (uint32_t)sin_ph((inc >> 1) + 0x40000000u);
            cq = (1LL << 30) - 2LL * (int64_t)(int32_t)sh * (int32_t)sh;   /* cos = 1 - 2 sin^2(w/2), Q30 */
            sq = 2LL * (int64_t)(int32_t)sh * (int32_t)chh;                /* sin = 2 sin cos (w/2), Q30 */
            k->rc[b] = (int32_t)((cq * r) >> 30);
            k->rs[b] = (int32_t)((sq * r) >> 30);
            k->rbands = (uint8_t)(b + 1);
        }
        break;
    }
    case FX_XMOD:
        k->xmode = (uint8_t)clampi(p1, 0, XM_N - 1);
        k->xsrc = (uint8_t)clampi(p2, 0, NXSRC - 1);
        break;
    case FX_DUCK:
    case FX_GATE:
        k->depth = p1 * Q15 / PMAX;
        k->xsrc = (uint8_t)clampi(p2, 0, NXSRC - 1);
        break;
    case FX_CYCLES:
        k->cmode = (uint8_t)clampi(p1, 0, CY_N - 1);
        k->cyp = (uint32_t)p2 * 4294967u;        /* (DISINTEGRATE: p2 / 1000 of the half-cycles) */
        k->reps = 1u + (uint32_t)p2 * 7u / PMAX; /* (WAVESET: 1 .. 8 times each; WAVELOSS: 0 .. 7 of 8 lost) */
        break;
    case FX_REED: {                              /* a clarinet's bore: half a period of delay (STK's Clarinet) */
        int32_t note = clampi(p1, 0, 47);
        int32_t hz8 = (int32_t)(((uint64_t)440u * 256u * exp2_q16((36 + note - 69) * 65536 / 12)) >> 16);
        k->rlen = (int32_t)(((uint32_t)FS * 128u / (uint32_t)(hz8 / 256 > 0 ? hz8 / 256 : 1))) - 384;   /* (FS / 2f
                                                                                       * - 1.5, Q8) */
        if (k->rlen < 512)
            k->rlen = 512;
        k->breath = 9830 + p2 * 22938 / PMAX;    /* (0.3 .. 1.0) */
        break;
    }
    case FX_OCTAVE:
        k->omode = (uint8_t)clampi(p1, 0, OC_N - 1);
        k->otone = clampi(hz_log(p2, 150, 1440) * 467 / 100, 300, 32767);   /* (150 Hz .. 7.7 kHz, a one-pole's) */
        break;
    case FX_DIFFUSE: {                           /* four allpasses, Clouds' lengths at 44.1 kHz, scaled by SIZE */
        static const uint16_t DL[4] = {174, 248, 371, 612};
        static const uint8_t DS[NDIFF] = {32, 48, 64, 80, 96, 112, 128, 160};   /* (x / 128) */
        int32_t sz = clampi(p1, 0, NDIFF - 1), i;
        for (i = 0; i < 4; i++) {
            uint16_t n = (uint16_t)(DL[i] * DS[sz] / 128u);
            if (k->dlen[i] != n) {
                k->dlen[i] = n;
                k->didx[i] = 0;
            }
        }
        k->dg = 9830 + p2 * 16384 / PMAX;        /* (0.3 .. 0.8) */
        break;
    }
    case FX_SAG: {                               /* the average of |x| over a window (Airwindows' PowerSag2) */
        uint32_t sh = 6u + (uint32_t)clampi(p1, 0, NSAGW - 1), i;
        if (k->sshift != sh) {                   /* (a new window: from silence) */
            k->sshift = sh;
            k->swin = 1u << sh;
            k->ssum = 0;
            for (i = 0; i < SLOT_LEN; i++)
                slot_mem[s][i] = 0;
        }
        k->depth = p2 * Q15 / PMAX;
        break;
    }
    case FX_CUTS:
        k->cunit = 441u + (uint32_t)p1 * 1607u / PMAX;   /* (10 .. 46 ms) */
        k->depth = p2;                           /* (DENSITY: the cuts that repeat, of 1000) */
        break;
    default:                                     /* GLITCH: 10 .. 300 ms stuck, 1 .. 16 times */
        k->glen = (uint32_t)hz_log(p1, 441, 1258);   /* (4.9 octaves: 441 .. 13230) */
        if (k->glen > SLOT_LEN / 2u)
            k->glen = SLOT_LEN / 2u;
        k->greps = 1u + (uint32_t)p2 * 15u / PMAX;
        break;
    }
    k->seen = now;
}

/* ------------------------------------------------ the effects, a sample --- */
AINL int32_t mulr30(int32_t a, int32_t b) { return (int32_t)(((int64_t)a * b + (1 << 29)) >> 30); }
AINL int32_t tri_fold(int32_t y)                 /* a triangle fold into full scale (FOLD's) */
{
    uint32_t m = ((uint32_t)(y + 32768)) & 0x1FFFFu;
    return m < 65536u ? (int32_t)m - 32768 : 98304 - (int32_t)m;
}
AINL int32_t diode(int32_t v)                    /* a diode's dead zone, squared beyond it */
{
    int32_t a = absi(v) - 21845, q;
    if (a <= 0)
        return 0;
    q = (int32_t)(((int64_t)a * a) >> 15);
    return v < 0 ? -q : q;
}
AINL int32_t xsig(uint32_t s)                    /* XMOD's, DUCK's, GATE's other signal, as it last was */
{
    switch (s) {
    case 6: return b_mixl;
    case 7: return b_mixr;
    case 8: return b_s1;
    case 9: return b_s2;
    default: return ch_post[s < NCH ? s : 0];
    }
}
/* a feedback product, its rounding error carried to the next sample: tails and repeats die as they should, not in
 * a straight line to silence (after ChoralRoot's FM-1 delay, Quixotic7/ChoralRootFM1 cr_fx.c, GPL-3.0) */
AINL int32_t fb_round(slot_rt_t *k, int32_t v)
{
    int64_t acc = (int64_t)v * k->fb + k->ferr;
    int32_t o = (int32_t)(acc >> 15);
    k->ferr = (int32_t)(acc - ((int64_t)o << 15));
    return o;
}
static FAST int32_t fx_tick(uint32_t s, int32_t x)
{
    slot_rt_t *k = &slr[s];
    int16_t *b = slot_mem[s];
    int32_t wet, xi = sat16(x), bp, lp, y;
    if (k->bypass)
        return x;
    if (k->lofi) {                               /* LO-FI: every other sample, 10 bits */
        k->odd ^= 1u;
        if (k->odd)
            return x + mulq15(k->last - x, k->pres);
    }
    switch (k->fx) {
    case FX_DELAY:
        k->d += (k->dt - k->d) >> 11;            /* (a new time glides: the pitch bends, as a tape's) */
        wet = line_rd(b, k->w, k->d);
        b[k->w] = (int16_t)sat16(xi + fb_round(k, wet));
        break;
    case FX_FLANGER:
        k->lfo += k->inc;
        wet = line_rd(b, k->w, k->base + mulq15(k->depth, (sin_ph(k->lfo) + 32768) >> 1));
        b[k->w] = (int16_t)sat16(xi + fb_round(k, wet));
        break;
    case FX_CHORUS: {
        int32_t sn;
        b[k->w] = (int16_t)xi;
        k->lfo += k->inc;
        sn = sin_ph(k->lfo);
        wet = (line_rd(b, k->w, k->base + mulq15(k->depth, sn)) + line_rd(b, k->w, k->base - mulq15(k->depth, sn))) >> 1;
        break;
    }
    case FX_TREMOLO: {
        int32_t sn;
        b[k->w] = (int16_t)xi;
        k->lfo += k->inc;
        sn = sin_ph(k->lfo);
        wet = line_rd(b, k->w, (1 << 8) + mulq15(k->depth, (sn + 32768) >> 1));
        wet = mulq15(wet, 32768 - mulq15(22938, (sn + 32768) >> 1));   /* (0.7 deep) */
        break;
    }
    case FX_FORMANT:
        svf3(&k->s1, &k->c1, x, &bp, &lp);
        wet = mulr24(k->k1, bp);
        svf3(&k->s2, &k->c2, x, &bp, &lp);
        wet += mulr24(k->k2, bp) * 3 / 4;
        wet <<= 1;
        break;
    case FX_BITCRUSH:
        if (++k->hcnt >= (uint32_t)k->hold_n) {
            int32_t u = sat16(xi + k->off);
            k->hcnt = 0;
            u &= ~((1 << k->sh) - 1);
            if (k->xm)
                u ^= (u << k->xm) & 0x7FFF;
            k->held = (int16_t)u - k->off;
        }
        wet = k->held;
        break;
    case FX_RESON: {                             /* coupled-form resonators: a rotation scaled by r, a band */
        uint32_t i;
        int32_t acc = 0;
        for (i = 0; i < k->rbands; i++) {
            int32_t a = k->r1s[i], c = k->r2[i];
            int32_t na = mulr30(k->rc[i], a) - mulr30(k->rs[i], c) + x;
            int32_t nc = mulr30(k->rs[i], a) + mulr30(k->rc[i], c);
            k->r1s[i] = clampi(na, -0x60000000, 0x60000000);
            k->r2[i] = clampi(nc, -0x60000000, 0x60000000);
            acc += mulr30(k->r1s[i], k->r1) * 2; /* (the in-phase state, x 2 (1 - r): unity and no phase turn at a
                                                  * band's pitch -- a loop through it rings there) */
        }
        wet = acc;                               /* (each band whole at its pitch: a loop through it rings there) */
        break;
    }
    case FX_XMOD: {                              /* two loops, one sound (Mutable Instruments Warps' modulations) */
        int32_t m = xsig(k->xsrc);
        switch (k->xmode) {
        case XM_RING:                            /* a diode ring: dead zones squared, the two halves against each
                                                  * other, softly limited */
            wet = tanh_q15(diode((x + m) * 4) - diode((x - m) * 4));   /* (driven into the diodes, as Warps
                                                                         * drives its inputs) */
            break;
        case XM_XOR:                             /* the two as 16-bit words, their bits exclusive-ored */
            wet = (int16_t)(sat16(x) ^ sat16(m));
            break;
        case XM_FOLD:                            /* their sum and a quarter of their product, folded */
            wet = tri_fold(x + m + (mulq15(x, m) >> 2));
            break;
        default:                                 /* the louder of the two, the other turned over */
            wet = absi(x) >= absi(m) ? x : -m;
            break;
        }
        break;
    }
    case FX_DUCK:
    case FX_GATE: {                              /* the KEY's loudness turns this down (DUCK) or opens it (GATE) */
        int32_t a = absi(xsig(k->xsrc)), e, gt;
        k->env += a > k->env ? (a - k->env) >> 5 : -(k->env >> 12);   /* (attack ~1 ms, release ~90 ms) */
        e = k->env > 16383 ? Q15 : k->env * 2;  /* (a channel at its ceiling: fully) */
        gt = k->fx == FX_DUCK ? Q15 - mulq15(k->depth, e) : Q15 - k->depth + mulq15(k->depth, e);
        k->gg += (gt - k->gg) >> 6;              /* (no clicks: ~1.5 ms) */
        wet = mulq15(x, k->gg);
        break;
    }
    case FX_CYCLES: {                            /* whole cycles, between the zero crossings, lost, broken or repeated */
        int up = k->zp < 0 && x >= 0, cross = (k->zp < 0) != (x < 0);
        if (k->cmode == CY_LOSS) {               /* WAVELOSS: the first n of every 8 cycles silent */
            if (up) {
                k->cyc = (k->cyc + 1u) & 7u;
                k->mult = k->cyc < (k->reps - 1u) ? 0 : Q15;
            }
            wet = mulq15(x, k->mult);
        } else if (k->cmode == CY_DISINT) {      /* SPUTTER (SuperCollider's Disintegrator): a half-cycle now and then turned over, or silent */
            if (cross) {
                uint32_t rn = rnd_next(&k->r);
                k->mult = rn < k->cyp ? ((rn & 1u) ? -Q15 : 0) : Q15;
            }
            wet = mulq15(x, k->mult);
        } else {                                 /* WAVESET: each cycle played REPEATS times (Wishart's), in turn */
            b[k->tw & SLOT_MASK] = (int16_t)xi;
            if ((up || k->tw - (k->wbw ? k->wb[(k->wbw - 1u) & 31u] : 0u) >= 2048u) && k->wbw - k->wbr < 32u)
                k->wb[k->wbw++ & 31u] = k->tw;   /* (a cycle's start; a long one cut at 2048) */
            if (k->rd >= k->ce) {
                if (k->rleft > 1u) {             /* the same cycle again */
                    k->rleft--;
                    k->rd = k->cs;
                } else if (k->wbr != k->wbw && k->wb[k->wbr & 31u] > k->ce) {   /* the next cycle */
                    k->cs = k->ce;
                    k->ce = k->wb[k->wbr++ & 31u];
                    k->rd = k->cs;
                    k->rleft = k->reps;
                } else if (k->wbr != k->wbw)
                    k->wbr++;                    /* (a start already passed) */
            }
            if (k->tw - k->rd > 12000u) {        /* (too far behind: to the newest) */
                k->wbr = k->wbw;
                k->cs = k->ce = k->rd = k->tw;
                k->rleft = 0;
            }
            wet = k->rd < k->ce ? b[k->rd++ & SLOT_MASK] : 0;
            k->tw++;
        }
        k->zp = x;
        break;
    }
    case FX_REED: {                              /* the bore's delay, the reed's table, the breath (the input in it) */
        int32_t d = line_rd(b, k->w, k->rlen), f = (d + k->rz) >> 1, pd, reed, o, br;
        k->rz = d;
        br = k->breath + (x >> 2);
        pd = -mulq15(31130, f) - br;             /* (-0.95 x the bore, less the breath) */
        reed = clampi(22938 - mulq15(9830, pd), -Q15, Q15);   /* (0.7 - 0.3 x the pressure, within +-1) */
        o = br + mulq15(pd, reed);
        b[k->w] = (int16_t)sat16(o);
        wet = d;
        break;
    }
    case FX_OCTAVE: {                            /* flip-flops on the zero crossings: one and two octaves down (OWL's) */
        int32_t pl = k->olp, o1, o2;
        k->olp += mulq15(x - k->olp, 3277);      /* (the detector: ~700 Hz) */
        if (pl < 0 && k->olp >= 0) {
            k->ff1 ^= 1u;
            if (k->ff1)
                k->ff2 ^= 1u;
        }
        o1 = k->ff1 ? x : -x;
        o2 = k->ff2 ? x : -x;
        wet = k->omode == OC_ONE ? o1 : k->omode == OC_TWO ? o2 : (o1 + o2) >> 1;
        k->ot += mulq15(wet - k->ot, k->otone);
        wet = k->ot;
        break;
    }
    case FX_DIFFUSE: {                           /* four allpasses in a row (Clouds' diffuser) */
        uint32_t i;
        int32_t v = x;
        for (i = 0; i < 4u; i++) {
            int16_t *ln = b + i * 1024u;
            int32_t dl = ln[k->didx[i]], wv = v + mulq15(dl, k->dg);
            ln[k->didx[i]] = (int16_t)sat16(wv);
            v = dl - mulq15(wv, k->dg);
            if (++k->didx[i] >= k->dlen[i])
                k->didx[i] = 0;
        }
        wet = v;
        break;
    }
    case FX_SAG: {                               /* loud a while, the supply sags: the gain down, slowly back */
        uint16_t *ab = (uint16_t *)b;
        uint32_t a = (uint32_t)clampi(absi(x), 0, 65535), old = ab[(k->w - k->swin) & SLOT_MASK], av;
        ab[k->w] = (uint16_t)a;
        k->ssum += a - old;
        av = k->ssum >> k->sshift;
        k->gg += (Q15 - mulq15(k->depth, clampi((int32_t)av * 3, 0, Q15)) - k->gg) >> 6;
        wet = mulq15(x, k->gg);
        break;
    }
    case FX_CUTS: {                              /* cut patterns (BBCut's way, Livecut): a phrase of cuts, 3 3 2 and
                                                  * the like; a cut plays on, or repeats the sound just before it --
                                                  * whole, in halves or in quarters (a roll) */
        static const uint8_t PAT[6][6] = {{3, 3, 2}, {2, 2, 2, 2}, {3, 3, 1, 1}, {4, 2, 2}, {1, 1, 2, 2, 2}, {2, 1, 1, 2, 2}};
        b[k->tw & SLOT_MASK] = (int16_t)xi;
        if (!k->cut_left) {
            uint32_t n, rn = rnd_next(&k->r);
            if (k->cpi >= 6u || !PAT[k->cpat % 6u][k->cpi]) {   /* (a phrase done: another) */
                k->cpat = (uint8_t)(rn % 6u);
                k->cpi = 0;
            }
            n = PAT[k->cpat % 6u][k->cpi++];
            k->cut_left = n * k->cunit;
            k->crep = (uint8_t)((rn >> 8) % 1000u < (uint32_t)k->depth);
            if (k->crep) {
                uint32_t div = (rn >> 20) & 3u;  /* (whole, half, quarter: rolls the denser the more) */
                k->cseg = k->cut_left >> (div == 3u ? 2u : div == 2u ? 1u : 0u);
                if (k->cseg < 64u)
                    k->cseg = 64u;
                k->cstart = k->tw - k->cseg;
                k->cpos = 0;
            }
        }
        if (k->crep) {
            uint32_t e = k->cpos < k->cseg - k->cpos ? k->cpos : k->cseg - k->cpos;   /* (a 1.5 ms fade at each end) */
            wet = b[(k->cstart + k->cpos) & SLOT_MASK];
            if (e < 64u)
                wet = wet * (int32_t)e >> 6;
            if (++k->cpos >= k->cseg)
                k->cpos = 0;
        } else
            wet = xi;
        k->cut_left--;
        k->tw++;
        break;
    }
    default:                                     /* GLITCH */
        b[k->w] = (int16_t)xi;
        if (!k->gleft) {
            if (rnd_next(&k->r) < 162000u) {     /* (about every 0.6 s, at random) */
                k->gleft = k->greps;
                k->gpos = 0;
                k->gstart = k->w - k->glen;
            }
            wet = xi;
        } else {
            wet = b[(k->gstart + k->gpos) & SLOT_MASK];
            if (++k->gpos >= k->glen) {
                k->gpos = 0;
                k->gleft--;
            }
        }
        break;
    }
    k->w = (k->w + 1u) & SLOT_MASK;
    if (k->lofi)
        wet &= ~63;
    k->last = wet;
    y = x + mulq15(wet - x, k->pres);
    return y;
}

/* ------------------------------------------------- a channel, a sample --- *
 * in: its input (the selector's pick, the buses a sample ago). Out: the limiter's output after the mute (for the
 * sends) in *lim; returned, the same */
static FAST int32_t ch_tick(uint32_t c, int32_t x)
{
    ch_rt_t *k = &chr[c];
    int32_t u, bp, lp, hp, a;
    if (k->kick) {                               /* (the RHYTHM's KICK: a burst into the loop) */
        x += mulq15(rnd_bi(&k->r), k->kick);
        k->kick -= (k->kick >> 7) + 1;
        if (k->kick < 0)
            k->kick = 0;
    }
    /* the noise, before the gain */
    if (k->namp) {
        int32_t w = rnd_bi(&k->r), n;
        switch (k->ntype) {
        case NZ_PINK:                            /* (Paul Kellet's economy filter) */
            k->pk0 = mulq15(k->pk0, 32691) + mulq15(w, 3245);
            k->pk1 = mulq15(k->pk1, 31556) + mulq15(w, 9716);
            k->pk2 = mulq15(k->pk2, 18678) + mulq15(w, 34494);
            n = (k->pk0 + k->pk1 + k->pk2 + mulq15(w, 6056)) >> 2;
            break;
        case NZ_RED:
            k->red = k->red - (k->red >> 8) + (w >> 4);
            n = k->red >> 1;
            break;
        case NZ_CRACKLE:                         /* dust: a crack now and then on a faint hiss */
            n = rnd_next(&k->r) < 4000000u ? w : w >> 5;
            break;
        case NZ_GEIGER:                          /* clicks at the rate set, each a short burst, -20 dB */
            if (rnd_next(&k->r) < (uint32_t)k->namp)
                k->click = (w < 0 ? -3277 : 3277) * 4;
            n = mulq15(k->click, 16384 + (w >> 2));
            k->click -= k->click >> 5;
            x += n;
            n = 0;
            break;
        case NZ_HUM:                             /* the mains: 50 Hz and its buzz, under a hiss */
            k->hum += 4869970u;
            n = (sin_ph(k->hum) * 5 + sin_ph(k->hum * 2u) * 3 + sin_ph(k->hum * 3u) * 2) / 10 + (w >> 3);
            break;
        default:
            n = w;
            break;
        }
        if (k->ntype != NZ_GEIGER)
            x += mulq15(n, k->namp);
    }
    /* the coupling capacitor: no DC round a loop (10 Hz) */
    k->dcy = k->dcy + ((x - k->dcx) << 8) - (k->dcy >> 11) * 3;
    k->dcx = x;
    u = k->dcy >> 8;
    /* GAIN */
    u = mulq15(u, k->gp);
    k->gp += k->dgp;
    /* the tube */
    {
        int32_t t = mulq12(u, k->drive);
        t = t > 0 ? mulq12(t, k->kpos) : mulq12(t, k->kneg);
        u = tanh_q15(t + k->bias) - k->tb0;
        if (k->tmode == TM_LIN)
            u <<= 1;
    }
    /* FOLD: driven into a triangle fold -- never past full scale, more harmonics the harder it is driven (Vital's
     * linear fold, in integers: the drive's wraparound is the fold) */
    if (k->fgain) {
        int32_t y = mulq12(u, k->fgain);
        uint32_t m = ((uint32_t)(y + 32768)) & 0x1FFFFu;
        u = m < 65536u ? (int32_t)m - 32768 : 98304 - (int32_t)m;
    }
    /* the op-amp, and the overload protection over it */
    u = mulq12(u, k->opg);
    a = u < 0 ? -u : u;
    if (k->trip > 0) {                           /* (open: the signal broken off) */
        k->trip--;
        u = 0;
    } else if (a > k->thr) {
        k->trip = 44 + (int32_t)(rnd_next(&k->r) >> 23);   /* (1 .. 12.6 ms) */
        u = 0;
    } else if (u > 0)                            /* the rails, softly */
        u = mulq15(k->rail, tanh_q15(mulq15(u, k->irail)));
    else
        u = mulq15(k->railn, tanh_q15(mulq15(u, k->irailn)));
    {
        int32_t d = u - k->opy;                  /* slew, then bandwidth */
        d = clampi(d, -k->slew, k->slew);
        u = k->opy + d;
        u = k->opy + mulq15(u - k->opy, k->oplp);
        k->opy = u;
    }
    /* HIGH and LOW: shelves or cuts */
    hp = svf3(&k->sh, &k->fh, u, &bp, &lp);
    if (k->ft == FT_SHELF)
        u += mulq15(hp, k->ah);
    else
        u = lp;
    hp = svf3(&k->sl, &k->fl, u, &bp, &lp);
    if (k->ft == FT_SHELF)
        u += mulq15(lp, k->al);
    else
        u = hp;
    /* the middle: peak, notch, band-pass */
    hp = svf3(&k->sm, &k->fm, u, &bp, &lp);
    bp = mulr24(k->fm.k, bp);                    /* (at unity: the normalised band-pass) */
    if (k->mt == MT_PEAK)
        u += mulq15(bp, k->am);
    else if (k->mt == MT_NOTCH)
        u -= bp;
    else
        u = bp;
    /* the brickwall limiter: the look-ahead line, an envelope that jumps to a peak as it enters and holds it until
     * the peak has left, the gain from it -- the line is the loop's delay */
    {
        int16_t *ln = lim_mem[c];
        uint32_t rd;
        int32_t o;
        a = u < 0 ? -u : u;
        ln[k->lw] = (int16_t)sat16(u >> 1);
        rd = (k->lw - k->lahead) & LIM_MASK;
        k->lw = (k->lw + 1u) & LIM_MASK;
        if (a >= k->env) {
            k->env = a;
            k->hold = k->lahead;
            k->gcnt = 0;
        } else if (k->hold)
            k->hold--;
        else
            k->env -= (int32_t)(((int64_t)k->env * k->rel) >> 15);
        if (k->gcnt-- == 0u) {                   /* (the gain anew every fourth sample, at once on a new peak) */
            k->gcnt = 3;
            k->g = k->env > LIM_T ? (int32_t)((uint32_t)(LIM_T << 15) / (uint32_t)k->env) : Q15;
        }
        o = mulq15((int32_t)ln[rd] << 1, k->g);
        u = clampi(o, -LIM_T - 512, LIM_T + 512);
    }
    return u;
}

/* the whole block, a sample at a time: every channel, the slots on the sends, the buses (CPU0, the ISR) */
static FAST void mx_samples(void)
{
    uint32_t i, c, s;
    for (i = 0; i < CTL; i++) {
        int32_t mxl = 0, mxr = 0, s1 = 0, s2 = 0, r1 = 0, r2 = 0;
        for (c = 0; c < NCH; c++) {
            ch_rt_t *k = &chr[c];
            int32_t x, y, q;
            switch (k->src) {
            case SRC_PRE: x = ch_pre[c]; break;
            case SRC_POST: x = ch_post[c]; break;
            case SRC_MIXL: x = b_mixl; break;
            case SRC_MIXR: x = b_mixr; break;
            case SRC_SND1: x = b_s1; break;
            case SRC_SND2: x = b_s2; break;
            case SRC_RET1: x = b_r1; break;
            case SRC_RET2: x = b_r2; break;
            case SRC_NONE: x = 0; break;
            default: x = ch_post[k->src - SRC_CH1]; break;
            }
            y = ch_tick(c, x);
            s1 += mulq15(y, k->gs1);
            s2 += mulq15(y, k->gs2);
            q = mulq15(y, k->gl);
            k->gs1 += k->dgs1;
            k->gs2 += k->dgs2;
            k->gl += k->dgl;
            ch_pre[c] = q;
            if (slr[c].in == FI_INS) {           /* its slot as insert */
                q = fx_tick(c, q);
                slot_y[c] = q;
            }
            ch_post[c] = q;
            usb_ch[c][i] = q;
            mxl += mulq15(q, k->gpl);
            mxr += mulq15(q, k->gpr);
            k->gpl += k->dgpl;
            k->gpr += k->dgpr;
            q = q < 0 ? -q : q;
            if (q > k->pk)
                k->pk = q;
        }
        for (s = 0; s < NSLOT; s++) {            /* the slots on the sends, and the chains */
            slot_rt_t *k = &slr[s];
            int32_t y;
            if (k->in == FI_NON || (k->in == FI_INS && s == SLOT_OUT))
                y = 0;                           /* (the output's insert: the output stage's) */
            else if (k->in == FI_INS)
                y = slot_y[s];                   /* (done in its channel) */
            else
                y = fx_tick(s, k->in == FI_SEN1 ? s1 : k->in == FI_SEN2 ? s2 : s ? slot_y[s - 1u] : 0);
            slot_y[s] = y;
            if (k->out == FO_RET1)
                r1 += y;
            else if (k->out == FO_RET2)
                r2 += y;
        }
        b_mixl = mxl;
        b_mixr = mxr;
        b_s1 = s1;
        b_s2 = s2;
        b_r1 = r1;
        b_r2 = r2;
        raw_l[i] = mxl;
        raw_r[i] = mxr;

        raw_r1[i] = r1;                          /* (the returns: the output stage adds them) */
        raw_r2[i] = r2;
    }
}
static __typeof__(&mx_samples) volatile mx_samples_p = mx_samples;   /* (the flash's way in to RAM code) */

/* --------------------------------------------------------- the output --- *
 * A block late, on CPU1 if it is there: the mix bus and the returns, the equaliser, slot 7 as insert, the SPACE
 * reverb, 6 dB down (six channels at their ceiling fill the bus twice over) */
typedef struct {
    svf_c fh, fl, fm;
    int32_t ah, al, am;
    uint8_t ft, sp, rsv[2];
    int32_t g1, g2, rp;          /* the returns' gains, the reverb's presence (Q15) */
    int32_t gm, gmt;             /* the main level (Q15): now, and where it glides */
    svf_t sh[2], sl[2], sm[2];
    int32_t dcx[2], dcy[2];      /* (the output's coupling: the tubes' asymmetry leaves DC) */
    out_par_t seen;
    uint8_t fresh;
} out_rt_t;
static out_rt_t outr;
static int32_t ost_l[CTL], ost_r[CTL], ost_r1[CTL], ost_r2[CTL];   /* (the last block's raw mix, for the stage) */
static int32_t oout_l[CTL], oout_r[CTL];                            /* (the stage's output) */
static int32_t rv_in[CTL], rv_l[CTL], rv_r[CTL];

static FAST void out_coef(void)
{
    out_rt_t *k = &outr;
    out_par_t now = par.out;
    if (!k->fresh && fsame(&now, &k->seen, sizeof now))
        return;
    k->ft = (uint8_t)clampi(now.ftype, 0, FT_N - 1);
    if (k->ft == FT_SHELF) {
        svf_coef(&k->fh, 5000, 2896);
        svf_coef(&k->fl, 150, 2896);
        k->ah = db10_q15(shelf_db10(clampi(now.high, 0, PMAX))) - Q15;
        k->al = db10_q15(shelf_db10(clampi(now.low, 0, PMAX))) - Q15;
    } else {
        svf_coef(&k->fh, hcut_hz(clampi(now.high, 0, PMAX)), 2896);
        svf_coef(&k->fl, lcut_hz(clampi(now.low, 0, PMAX)), 2896);
        k->ah = k->al = 0;
    }
    svf_coef(&k->fm, mid_hz(clampi(now.mfreq, 0, PMAX)), now.wide ? 2867 : 12288);   /* (Q 0.7 wide, 3 narrow) */
    k->am = db10_q15(peak_db10(clampi(now.mgain, 0, PMAX))) - Q15;
    k->g1 = snd_q15(now.ret1);
    k->g2 = snd_q15(now.ret2);
    k->rp = clampi(now.spres, 0, PMAX) * Q15 / PMAX;
    k->gmt = db10_q15(main_db10(clampi(now.level, 0, PMAX)));
    if (k->fresh)
        k->gm = k->gmt;
    if (k->fresh || now.space != k->seen.space) {
        uint32_t sp = (uint32_t)clampi(now.space, 0, SP_N - 1);
        rv_param(SPACE[sp][0], SPACE[sp][1], SPACE[sp][2], 50);
    }
    k->fresh = 0;
    k->seen = now;
}

static FAST void out_stage(void)
{
    out_rt_t *k = &outr;
    uint32_t i, j;
    int32_t pk = 0;
    for (i = 0; i < CTL; i++) {
        int32_t x[2], bp, lp, hp;
        x[0] = ost_l[i] + mulq15(ost_r1[i], k->g1) + mulq15(ost_r2[i], k->g2);
        x[1] = ost_r[i] + mulq15(ost_r1[i], k->g1) + mulq15(ost_r2[i], k->g2);
        for (j = 0; j < 2u; j++) {
            int32_t u = x[j] >> 1;
            hp = svf3(&k->sh[j], &k->fh, u, &bp, &lp);
            u = k->ft == FT_SHELF ? u + mulq15(hp, k->ah) : lp;
            hp = svf3(&k->sl[j], &k->fl, u, &bp, &lp);
            u = k->ft == FT_SHELF ? u + mulq15(lp, k->al) : hp;
            svf3(&k->sm[j], &k->fm, u, &bp, &lp);
            u += mulq15(mulr24(k->fm.k, bp), k->am);
            x[j] = u;
        }
        if (slr[SLOT_OUT].in == FI_INS) {        /* slot 7 as the output's insert: on the middle, the sides kept */
            int32_t m = (x[0] + x[1]) >> 1, y = fx_tick(SLOT_OUT, m);
            x[0] += y - m;
            x[1] += y - m;
            slot_y[SLOT_OUT] = y;
        }
        for (j = 0; j < 2u; j++) {               /* (the coupling: the tubes' and the insert's DC kept off) */
            k->dcy[j] = k->dcy[j] + ((x[j] - k->dcx[j]) << 8) - (k->dcy[j] >> 11) * 3;
            k->dcx[j] = x[j];
            x[j] = mulq15(k->dcy[j] >> 8, k->gm);
        }
        k->gm += (k->gmt - k->gm) >> 9;          /* (a new main level glides: ~0.3 s) */
        oout_l[i] = x[0];
        oout_r[i] = x[1];
        rv_in[i] = (x[0] + x[1]) >> 1;
        rv_l[i] = rv_r[i] = 0;
    }
    if (k->rp) {                                 /* SPACE: the reverb crossfaded in (at the top, alone) */
        rv_block(rv_in, rv_l, rv_r, CTL);
        for (i = 0; i < CTL; i++) {
            oout_l[i] += mulq15(2 * rv_l[i] - oout_l[i], k->rp);
            oout_r[i] += mulq15(2 * rv_r[i] - oout_r[i], k->rp);
        }
    }
    {
        uint32_t pw = 0;
        for (i = 0; i < CTL; i++) {
            int32_t a = absi(oout_l[i]) > absi(oout_r[i]) ? absi(oout_l[i]) : absi(oout_r[i]), q = clampi(a >> 6, 0, 1023);
            pk = a > pk ? a : pk;
            pw += (uint32_t)(q * q);
        }
        if (out_pw < 0x40000000u) {
            out_pw += pw >> 5;
            out_pw_n++;
        }
    }
    out_meter = pk > out_meter ? pk : out_meter - (out_meter >> 4);
}
static __typeof__(&out_stage) volatile out_stage_p = out_stage;
static void out_job(void *ctx)
{
    (void)ctx;
    out_stage_p();
}

#include "mod.c"

/* ---------------------------------------------- a block, from the ISR --- */
/* the gains a channel glides to this block, and their steps */
/* -------------------------------------------------------------- the RHYTHM, a block at a time (the ISR) --- *
 * Each lane's clock is a phase: a step a wrap. A step with a hit (its chance, CHAOS's ghosts) triggers it -- late if
 * SWING says so, sometimes rolled (ratchets: 2 .. 4 hits in the step). A hit opens its gate for its length, accented
 * or not, then closes it over a few ms; CHOP gates what is heard, CHOKE the loop, KICK the loop with a burst into it,
 * ACCENT pushes GAIN and FOLD up a moment instead. A lane with mutation flips its steps now and then each round */
static const uint8_t DIV2[NDIV] = {1, 2, 4, 6, 8, 10, 12, 16};   /* steps a beat, x 2 */
static struct {
    uint32_t ph, hits_src, hits, rph;            /* the phase; the steps as the panel has them, as playing (mutated) */
    uint8_t step, pend, ratch, rk;               /* the step; a swung hit waiting; the roll's hits, the next */
    uint8_t pacc, rsv[3];
    int32_t env, lvl;                            /* the gate's opening (Q15); the level it opens to */
    uint32_t hold;                               /* blocks it stays open */
} lrt[NCH];
static int32_t rhy_g[NCH] = {Q15, Q15, Q15, Q15, Q15, Q15};   /* each channel's gate this block (Q15) */
static uint8_t rhy_gmode[NCH];                   /* .. how it is applied (RM_*) */
static int32_t rhy_acc[NCH];                     /* ACCENT's push, decaying (Q15) */
static volatile uint8_t rhy_reset = 1;           /* (the main loop: from the top -- every lane on its first step) */
static volatile uint32_t rhy_beat;               /* the beats counted (the lights) */
static uint32_t rhy_bph;
static rnd_t rhy_r = {0x2468ACE1u};
static FAST void rhy_trigger(uint32_t c, const lane_t *L, int accent, uint32_t inc)
{
    uint32_t sb = inc ? 0xFFFFFFFFu / inc : 1000u, g = L->gate * (uint32_t)(par.rhy.gscale ? par.rhy.gscale : 50) / 50u;
    lrt[c].env = lrt[c].lvl = accent ? Q15 : 21000;
    lrt[c].hold = sb * (g > 100u ? 100u : g) / 100u;   /* (the step's blocks, the gate's share) */
    if (L->mode == RM_CHOKE || L->mode == RM_KICK) {   /* (open at least as long as the loop takes to come round its
                                                        * limiter's look-ahead, and ~20 ms: else the struck loop is
                                                        * heard only after its gate has shut) */
        uint32_t mn = chr[c].lahead / CTL + 28u;
        if (lrt[c].hold < mn)
            lrt[c].hold = mn;
    }
    if (L->mode == RM_KICK)
        chr[c].kick = accent ? 30000 : 20000;
    else if (L->mode == RM_ACCENT)
        rhy_acc[c] = accent ? Q15 : 20000;
}
static FAST void rhythm_block(void)
{
    uint32_t c, bpm = (uint32_t)clampi(par.rhy.bpm, 10, 500);
    if (!par.rhy.on) {
        for (c = 0; c < NCH; c++) {
            rhy_gmode[c] = RM_OFF;
            rhy_g[c] = Q15;
            rhy_acc[c] = 0;
        }
        rhy_reset = 1;
        return;
    }
    if (rhy_reset) {                             /* (from the top: each lane's first step on the next block) */
        rhy_reset = 0;
        for (c = 0; c < NCH; c++) {
            lrt[c].ph = 0xFFFFFFFFu;
            lrt[c].step = 0xFF;
            lrt[c].pend = lrt[c].ratch = 0;
            lrt[c].env = 0;
            lrt[c].hold = 0;
        }
        rhy_bph = 0xFFFFFFFFu;
    }
    {   /* the beat, for the lights */
        uint32_t b0 = rhy_bph;
        rhy_bph += bpm * 2u * 25971u;
        if (rhy_bph < b0)
            rhy_beat++;
    }
    for (c = 0; c < NCH; c++) {
        const lane_t *L = &par.rhy.lane[c];
        uint32_t inc, p0, len = (uint32_t)clampi(L->len, 1, 32);
        int32_t floor;
        rhy_gmode[c] = L->mode < RM_N ? L->mode : RM_OFF;
        if (L->mode == RM_OFF || L->mode >= RM_N) {
            rhy_g[c] = Q15;
            continue;
        }
        if (lrt[c].hits_src != L->hits) {        /* (the panel's steps, anew: played from now) */
            lrt[c].hits_src = L->hits;
            lrt[c].hits = L->hits;
        }
        inc = bpm * DIV2[L->div < NDIV ? L->div : 4] * 25971u;   /* (2^32 x CTL / (120 FS) a BPM a half-step) */
        p0 = lrt[c].ph;
        lrt[c].ph += inc;
        if (lrt[c].ph < p0 || lrt[c].step == 0xFF) {   /* a new step */
            uint32_t bit, rn = rnd_next(&rhy_r);
            int fire;
            if (lrt[c].step == 0xFF)
                lrt[c].ph = 0;
            lrt[c].step = (uint8_t)(lrt[c].step == 0xFF ? 0u : (lrt[c].step + 1u) % len);
            if (!lrt[c].step && L->mut) {        /* (a round done: the steps mutate) */
                uint32_t i;
                for (i = 0; i < len; i++)
                    if ((rnd_next(&rhy_r) >> 25) < (uint32_t)L->mut * 128u / 100u)
                        lrt[c].hits ^= 1u << i;
            }
            bit = lrt[c].hits >> lrt[c].step & 1u;
            fire = bit && (rn >> 25) < (uint32_t)(L->prob ? L->prob : 100) * 128u / 100u * (100u - par.rhy.chaos / 2u) / 100u;
            if (!bit && (rn & 0x7Fu) < (uint32_t)par.rhy.chaos / 4u)
                fire = 2;                        /* (CHAOS: a ghost) */
            if (fire) {
                int acc = fire == 1 && (L->acc >> lrt[c].step & 1u);
                lrt[c].ratch = (uint8_t)(((rn >> 8) & 0x7Fu) < (uint32_t)L->ratchet * 128u / 100u ? 2u + ((rn >> 16) % 3u) : 0u);
                lrt[c].rk = 1;
                if (par.rhy.swing && (lrt[c].step & 1u)) {   /* (an off step: late) */
                    lrt[c].pend = 1;
                    lrt[c].pacc = (uint8_t)acc;
                } else
                    rhy_trigger(c, L, acc, inc);
            }
        }
        if (lrt[c].pend && lrt[c].ph >= (uint32_t)par.rhy.swing * 14316557u) {   /* (up to a third of a step late) */
            lrt[c].pend = 0;
            rhy_trigger(c, L, lrt[c].pacc, inc);
        }
        if (lrt[c].ratch && lrt[c].rk < lrt[c].ratch && lrt[c].ph >= lrt[c].rk * (0xFFFFFFFFu / lrt[c].ratch)) {
            lrt[c].rk++;                         /* (a roll: its next hit, softer) */
            rhy_trigger(c, L, 0, inc * lrt[c].ratch);
        }
        if (lrt[c].hold)
            lrt[c].hold--;
        else
            lrt[c].env -= lrt[c].env >> 2;      /* (closing: ~3 ms) */
        floor = Q15 - L->depth * Q15 / 100;
        rhy_g[c] = floor + mulq15((L->mode == RM_KICK ? 2 * Q15 : Q15) - floor, lrt[c].env);   /* (KICK: a +6 dB punch
                                                  * while open -- the struck loop past its edge, at full strength) */
    }
    for (c = 0; c < NCH; c++)                    /* ACCENT's push dies away (~25 ms) */
        rhy_acc[c] -= rhy_acc[c] >> 5;
}

static FAST void ch_gains(uint32_t c)
{
    ch_rt_t *k = &chr[c];
    const ch_par_t *p = &k->seen;
    int32_t on = !p->mute, tp, tl, tpl, tpr, t1, t2;
    uint32_t ph = (uint32_t)(clampi(p->pan, -PAN_MAX, PAN_MAX) + PAN_MAX) * (0x40000000u / (2u * PAN_MAX));
    tp = db10_q15(clampi(p->gain, 0, PMAX) * 480 / PMAX);
    tl = on ? lvl_q15(p->level) : 0;
    tpl = p->mainmix ? sin_ph(ph + 0x40000000u) : 0;
    tpr = p->mainmix ? sin_ph(ph) : 0;
    if (rhy_gmode[c] == RM_CHOP) {               /* the RHYTHM: what is heard gated, the loop running on .. */
        tpl = mulq15(tpl, rhy_g[c]);
        tpr = mulq15(tpr, rhy_g[c]);
    } else if (rhy_gmode[c] == RM_CHOKE || rhy_gmode[c] == RM_KICK)
        tl = mulq15(tl, rhy_g[c]);               /* .. or the loop itself choked */
    t1 = on ? snd_q15(p->send1) : 0;
    t2 = on ? snd_q15(p->send2) : 0;
#define GLIDE(cur, d, t) do { (d) = ((t) - (cur)) / (int32_t)CTL; } while (0)
    GLIDE(k->gp, k->dgp, tp);
    GLIDE(k->gl, k->dgl, tl);
    GLIDE(k->gpl, k->dgpl, tpl);
    GLIDE(k->gpr, k->dgpr, tpr);
    GLIDE(k->gs1, k->dgs1, t1);
    GLIDE(k->gs2, k->dgs2, t2);
#undef GLIDE
}

#if defined(FELUCCA_UAC) && FELUCCA_UAC
static int32_t usb_blk[UAC_NBLK * CTL];          /* (USB audio: the output before the MASTER knob, then each channel) */
#endif
#ifdef HOST
static uint32_t host_cost_clock;
#define COST_NOW_() host_cost_clock
#else
#define COST_NOW_() fm1_ticks()
#endif
#define BLK_TICKS ((uint32_t)CTL * 1000000u / FS * 24u)   /* a block's time, in TIMER4's ticks (24 a microsecond) */
/* above half scale, a soft knee into a ceiling at full scale */
AINL int32_t ceiling(int32_t x)
{
    int32_t a = absi(x);
    if (a <= Q15 / 2)
        return x;
    a = Q15 / 2 + (tanh_q15((a - Q15 / 2) * 2) >> 1);
    return x < 0 ? -a : a;
}
static int32_t vv[CTL];
static FAST void mix_block(int32_t *out)
{
    uint32_t i, c, t0 = COST_NOW_(), t1;
    int any1 = 0, voice;
    int32_t mt = par.out.mute ? 0 : Q15, dm;
    midi_main_drain();
    for (c = 0; c < NSLOT; c++)
        eff_slot[c] = par.slot[c];
    rhythm_block();
    for (c = 0; c < NCH; c++) {                  /* the modulation, then what it means */
        mod_block(c);
        eff_ch[c] = par.ch[c];
        mod_apply(c, &eff_ch[c], &eff_slot[c]);
        if (rhy_acc[c] > 64) {                   /* (the RHYTHM's ACCENT: GAIN and FOLD pushed, never the pitch) */
            eff_ch[c].gain = (int16_t)clampi(eff_ch[c].gain + mulq15(rhy_acc[c], 250), 0, PMAX);
            eff_ch[c].fold = (int16_t)clampi(eff_ch[c].fold + mulq15(rhy_acc[c], 400), 0, PMAX);
        }
        ch_coef(c);
        ch_gains(c);
    }
    for (c = 0; c < NSLOT; c++)
        slot_coef(c);
    out_coef();
    audio_worker_check();
    if (audio_worker_online)
        any1 = audio_worker_submit(out_job, 0);  /* the last block's output stage, on CPU1 .. */
    mx_samples_p();                              /* .. while this block's channels run here */
    t1 = COST_NOW_();
    if (!any1)
        out_stage_p();
    else if (!audio_worker_join())
        for (i = 0; i < CTL; i++)
            oout_l[i] = oout_r[i] = 0;
    out_ticks = any1 ? 0u : COST_NOW_() - t1;
    fcopy(ost_l, raw_l, sizeof ost_l);          /* (this block's raw mix: the stage's next) */
    fcopy(ost_r, raw_r, sizeof ost_r);
    fcopy(ost_r1, raw_r1, sizeof ost_r1);
    fcopy(ost_r2, raw_r2, sizeof ost_r2);
    for (c = 0; c < NCH; c++) {                  /* the meters */
        ch_rt_t *k = &chr[c];
        ch_meter[c] = k->pk > ch_meter[c] ? k->pk : ch_meter[c] - (ch_meter[c] >> 5);
        k->pk = 0;
    }
    voice = vce.speaking || vce.dk || vo_r != vo_w;
    if (voice)
        vo_block(vv, CTL);
    dm = (mt - out_mute_g) / (int32_t)CTL;
    for (i = 0; i < CTL; i++) {
        int32_t l = oout_l[i], r = oout_r[i];
#if defined(FELUCCA_UAC) && FELUCCA_UAC
        usb_blk[UAC_NBLK * i] = ceiling(l);
        usb_blk[UAC_NBLK * i + 1u] = ceiling(r);
#if UAC_NBLK > 2u
        for (c = 0; c < NCH && c + 2u < UAC_NBLK; c++)
            usb_blk[UAC_NBLK * i + 2u + c] = ceiling(usb_ch[c][i]);
#endif
#endif
        out_mute_g += dm;
        l = mulq15(l, out_mute_g);
        r = mulq15(r, out_mute_g);
        if (voice) {
            l = mulq15(l, 32768 - vce.dk) + vv[i];
            r = mulq15(r, 32768 - vce.dk) + vv[i];
        }
        out[2u * i] = ceiling(mulq12(l, master_q12));
        out[2u * i + 1u] = ceiling(mulq12(r, master_q12));
    }
    out_mute_g = mt;
    mx_ticks = t1 - t0;
    blk_clock++;
}

/* --------------------------------------------------- the main loop's --- */
static int ch_sounding(uint32_t c) { return ch_meter[c] > 120; }   /* (above -49 dB) */
static int mx_quiet(void)                         /* nothing heard: the flash may be blanked a sector */
{
    uint32_t c;
    for (c = 0; c < NCH; c++)
        if (ch_sounding(c))
            return 0;
    return out_meter < 40;
}
static void mx_init(void)
{
    uint32_t c;
    memset(chr, 0, sizeof chr);
    memset(slr, 0, sizeof slr);
    memset(&outr, 0, sizeof outr);
    for (c = 0; c < NCH; c++) {
        chr[c].fresh = 1;
        chr[c].g = Q15;
        rnd_seed(&chr[c].r, 0x5EED1u + c * 977u);
    }
    for (c = 0; c < NSLOT; c++) {
        slr[c].fresh = 1;
        rnd_seed(&slr[c].r, 0xF00Du + c * 131u);
    }
    outr.fresh = 1;
    rv_init();
}
