/* SPDX-License-Identifier: GPL-3.0-only
 * LILLIPUT: patches -- the whole mixer drawn at once (HOME), nudged (SHIFT + HOME), the factory's.
 *
 * HOME draws a new patch: how the channels are connected (each one's input, the rack's slots on inserts, sends
 * and returns, their effects) and how each channel is set (its tube and op-amp, its noise, its filters, its
 * limiter). A patch is drawn from a recipe -- the shapes no-input players reach for:
 *   SOLO     each channel in a loop of its own (PRE, or POST through its insert): voices side by side
 *   RING     channels feeding each other round a ring (CH n): one loop through several channels
 *   WEB      channels fed from the mix bus: everything heard in one tangle
 *   SEND     channels fed from a send bus, the sends making the loops
 *   RETURN   loops through the rack: the sends into an effect, its return into the channels
 *   HYBRID   a bit of each, channel by channel
 * Its levels are set a little past the point where each loop starts to sing -- worked out from the gains in the
 * loop (preamp, tube, op-amp, a resonant peak) -- so a patch drawn sounds at once, and the six faders are where
 * the playing is. Some channels are drawn with their level down: brought up, they join in.
 * SHIFT + HOME moves the tone of every channel a little (as the app's SHIFT: GAIN, NOISE, the filters, LIMIT,
 * OVERLOAD), the connections and levels kept. */

enum { RC_SOLO, RC_RING, RC_WEB, RC_SEND, RC_RETURN, RC_HYBRID, RC_N };
static const char *const RC_NAME[RC_N] = {"SOLO", "RING", "WEB", "SEND", "RETURN", "HYBRID"};

/* a channel as it starts: a quiet, plain voice, its level down */
static void patch_ch_plain(ch_par_t *p, uint32_t c)
{
    static const int16_t PAN0[NCH] = {-300, 300, -150, 150, -450, 450};
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

/* ------------------------------------------------------------ drawing --- */
static int32_t pr_in(rnd_t *r, int32_t a, int32_t b) { return a + rnd_below(r, b - a + 1); }
static int16_t lvl_of_db10(int32_t db10) { return (int16_t)clampi((db10 + 600) * PMAX / 720, 40, PMAX); }
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
/* a channel's voice, at random: everything but its connections and levels */
static void draw_voice(rnd_t *r, ch_par_t *p)
{
    static const uint8_t NZW[NZ_N] = {3, 4, 1, 2, 1, 1};
    static const uint8_t OAW[OA_N] = {3, 3, 2, 2, 1, 2};
    p->gain = (int16_t)pr_in(r, 0, 300);        /* (0 .. 14 dB: the levels near 0 dB, where a desk's faders sit) */
    p->tube = (int16_t)rnd_below(r, TB_N);
    p->tmode = (int16_t)(rnd_chance(r, 40) ? TM_SAT : TM_LIN);
    p->opamp = (int16_t)rnd_weighted(r, OAW, OA_N);
    p->ntype = (int16_t)rnd_weighted(r, NZW, NZ_N);
    p->noise = (int16_t)(p->ntype == NZ_GEIGER ? pr_in(r, 250, 650) : pr_in(r, 330, 680));
    p->over = (int16_t)pr_in(r, 250, 950);
    p->lim = (int16_t)pr_in(r, 0, PMAX);
    if (rnd_chance(r, 70)) {
        p->ftype = FT_SHELF;
        p->high = (int16_t)pr_in(r, 280, 720);
        p->low = (int16_t)pr_in(r, 280, 720);
    } else {
        p->ftype = FT_CUT;
        p->high = (int16_t)pr_in(r, 450, PMAX);
        p->low = (int16_t)pr_in(r, 450, PMAX);
    }
    {
        int32_t m = rnd_below(r, 100);
        p->mtype = (int16_t)(m < 60 ? MT_PEAK : m < 75 ? MT_NOTCH : MT_BAND);
    }
    p->mgain = (int16_t)(p->mtype == MT_PEAK ? pr_in(r, 520, 960) : pr_in(r, 250, 900));
    p->mfreq = (int16_t)pr_in(r, 120, 920);
    p->pan = (int16_t)pr_in(r, -420, 420);
    p->mute = 0;
    p->mainmix = 1;
    p->send1 = p->send2 = 0;
}
static void draw_slot(rnd_t *r, slot_par_t *s, uint32_t in, uint32_t out)
{
    static const uint8_t FXW[FX_N] = {4, 2, 2, 2, 3, 2, 2};
    s->fx = (int16_t)rnd_weighted(r, FXW, FX_N);
    s->p1 = (int16_t)pr_in(r, 100, 900);
    s->p2 = (int16_t)pr_in(r, 100, 850);
    s->pres = (int16_t)(in == FI_INS ? pr_in(r, 300, 850) : PMAX);
    s->lofi = (uint8_t)rnd_chance(r, 25);
    s->bypass = 0;
    s->in = (uint8_t)in;
    s->out = (uint8_t)out;
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
    q->out.level = (int16_t)clampi((-160 - est + 300) * PMAX / 600, 100, PMAX);   /* (to about -16 dB) */
}

/* a new patch into q, from seed (recipe -1: one at random) */
static uint32_t patch_draw(lil_par_t *q, uint32_t seed, int recipe)
{
    static const uint8_t RCW[RC_N] = {3, 2, 2, 2, 2, 3};
    rnd_t r;
    uint32_t c, n, i, act[NCH];
    int32_t margin;
    rnd_seed(&r, seed);
    patch_plain(q);
    if (recipe < 0 || recipe >= RC_N)
        recipe = (int)rnd_weighted(&r, RCW, RC_N);
    for (c = 0; c < NCH; c++)
        draw_voice(&r, &q->ch[c]);
    n = (uint32_t)pr_in(&r, 2, recipe == RC_RING ? 4 : 5);   /* the channels heard at once; the rest drawn too, down */
    for (c = 0; c < NCH; c++)
        act[c] = c;
    for (c = NCH - 1u; c > 0u; c--) {            /* (which: shuffled) */
        uint32_t j = (uint32_t)rnd_below(&r, (int32_t)c + 1), t = act[c];
        act[c] = act[j];
        act[j] = t;
    }
    margin = pr_in(&r, 25, 55);
    for (i = 0; i < NCH; i++) {
        uint32_t k = act[i], heard = i < n;
        ch_par_t *p = &q->ch[k];
        int rc = recipe;
        if (rc == RC_HYBRID) {
            static const uint8_t HW[RC_N] = {4, 2, 2, 2, 0, 0};
            rc = (int)rnd_weighted(&r, HW, RC_N);
        }
        switch (rc) {
        case RC_RING:                            /* fed by the channel before it in the ring */
            p->src = (int16_t)(SRC_CH1 + act[(i + n - 1u) % n]);
            if (!heard)
                p->src = (int16_t)(SRC_CH1 + act[rnd_below(&r, (int32_t)n)]);
            p->level = sing_at(p, -pr_in(&r, 0, 140), margin, 15);
            break;
        case RC_WEB:                             /* the mix bus's side it is panned to */
            p->src = (int16_t)(p->pan < 0 ? SRC_MIXL : SRC_MIXR);
            p->level = sing_at(p, -pr_in(&r, 0, 140), margin, 30 - (int32_t)n * 10);
            break;
        case RC_SEND: {                          /* a send bus: its own send makes the loop */
            int b = rnd_chance(&r, 50);
            p->src = (int16_t)(b ? SRC_SND2 : SRC_SND1);
            if (b)
                p->send2 = sing_send(p, -pr_in(&r, 20, 120), margin, 0);
            else
                p->send1 = sing_send(p, -pr_in(&r, 20, 120), margin, 0);
            p->level = lvl_of_db10(-pr_in(&r, 0, 120));
            break;
        }
        default:                                 /* SOLO: its own loop, through its insert sometimes */
            if (rnd_chance(&r, 35)) {
                draw_slot(&r, &q->slot[k], FI_INS, FO_NON);
                p->src = (int16_t)(rnd_chance(&r, 70) ? SRC_POST : SRC_PRE);
            } else
                p->src = SRC_PRE;
            p->level = sing_at(p, -pr_in(&r, 0, 140), margin, 0);
            break;
        }
        if (!heard)
            p->level = 0;
    }
    if (recipe == RC_RETURN || (recipe == RC_HYBRID && rnd_chance(&r, 50))) {
        /* loops through the rack: a free slot on send 1 into return 1, sometimes a second on send 2, or chained */
        uint32_t s1 = SLOT_OUT, s2 = SLOT_OUT;
        for (i = NCH; i-- > 0u;)
            if (q->slot[act[i]].in == FI_NON) {
                s1 = act[i];
                break;
            }
        if (s1 == SLOT_OUT)
            s1 = act[NCH - 1u];
        draw_slot(&r, &q->slot[s1], FI_SEN1, FO_RET1);
        if (q->slot[s1].fx == FX_BITCRUSH)       /* (into a loop: something that moves the pitch, or rings) */
            q->slot[s1].fx = FX_DELAY;
        if (s1 + 1u < NCH && q->slot[s1 + 1u].in == FI_NON && rnd_chance(&r, 40)) {
            s2 = s1 + 1u;
            draw_slot(&r, &q->slot[s2], FI_PREV, FO_RET2);
        } else if (rnd_chance(&r, 40)) {
            for (i = 0; i < NCH; i++)
                if (act[i] != s1 && q->slot[act[i]].in == FI_NON) {
                    s2 = act[i];
                    draw_slot(&r, &q->slot[s2], FI_SEN2, FO_RET2);
                    break;
                }
        }
        for (i = 0; i < NCH; i++) {
            uint32_t k = act[i];
            ch_par_t *p = &q->ch[k];
            if (recipe != RC_RETURN && rnd_chance(&r, 50))
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
    /* the output: a space, a touch of equalising */
    q->out.space = (int16_t)rnd_below(&r, SP_N);
    q->out.spres = (int16_t)pr_in(&r, 80, 420);
    q->out.mgain = (int16_t)pr_in(&r, 420, 580);
    q->out.mfreq = (int16_t)pr_in(&r, 250, 800);
    if (rnd_chance(&r, 20)) {                    /* now and then the output's own insert */
        draw_slot(&r, &q->slot[SLOT_OUT], FI_INS, FO_NON);
        q->slot[SLOT_OUT].pres = (int16_t)pr_in(&r, 200, 600);
    }
    patch_balance(q);
    return (uint32_t)recipe;
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
typedef struct { const char *name; uint8_t recipe; uint32_t seed; } factory_t;
static const factory_t FACTORY[] = {
    {"FIRST LIGHT", RC_SOLO, 101}, {"LONELY RADIO", RC_SOLO, 7342}, {"CIRCLE", RC_RING, 23},
    {"WEATHERSTATION", RC_HYBRID, 911}, {"TANGLE", RC_WEB, 52}, {"UNDERCURRENT", RC_SEND, 3},
    {"ECHO CHAMBER", RC_RETURN, 77}, {"RELAY", RC_RING, 4410}, {"STATIC FIELD", RC_HYBRID, 1312},
    {"SWARM", RC_WEB, 808}, {"DEEP WIRE", RC_SEND, 2718}, {"MIRROR HALL", RC_RETURN, 31415},
    {"SIGNAL LOSS", RC_HYBRID, 66}, {"SIX STRINGS", RC_SOLO, 2026}, {"ORBIT", RC_RING, 999},
    {"FAULTY MACHINE", RC_RETURN, 4096}};
#define NFACTORY NELEM(FACTORY)
static void factory_load(lil_par_t *q, uint32_t i)
{
    patch_draw(q, FACTORY[i % NFACTORY].seed, FACTORY[i % NFACTORY].recipe);
}

/* the trim: a patch just drawn or loaded is played into for a moment, as a player would before letting it run.
 * For the first two seconds, a channel with its fader up that is still not sounding has its preamp turned up
 * 3 dB at a time (every 150 ms) until it sings -- a loop's pitch may fall between what its filters let through,
 * and no estimate knows that; then the output is listened to for a second, and its main level moved once, so
 * that it comes out near -16 dB. The faders are never touched. Nothing more after that */
static struct { uint8_t on; uint32_t t0, woke_at; } trim;
static void trim_start(void)
{
    trim.on = 1;
    trim.t0 = fm1_ms;
    trim.woke_at = fm1_ms;
}
static void trim_poll(void)
{
    uint32_t el = fm1_ms - trim.t0, c;
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
        return;
    }
    if (el < 3400u || !out_pw_n)
        return;
    trim.on = 0;
    {
        uint32_t mean = out_pw / out_pw_n;       /* (mean of (|x| / 64)^2 over a block / 32: x 32 back) */
        int32_t db10;
        if (mean < 1u)
            return;                              /* (silent: nothing to go by) */
        /* dB of the rms: 10 log10(mean) + 20 log10(64 / 32768), and 20 log10(1 / 512) is 18 octaves of power */
        db10 = (int32_t)(((int64_t)(log2_q16(mean) - (18 << 16)) * 1972830) >> 32);
        par.out.level = (int16_t)clampi(par.out.level + (-160 - db10) * PMAX / 600, 40, PMAX);
    }
}
