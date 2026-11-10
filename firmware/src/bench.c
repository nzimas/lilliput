/* SPDX-License-Identifier: GPL-3.0-only
 * LILLIPUT: the chip's own costs, measured on the FM-1 and sent over USB MIDI (tools/fm1_bench.py), in the
 * installer's framing (ota.c: F0 pack7(00 59 cmd len24 body ~sum) F7). The output is muted while it runs; the patch
 * as it was comes back after.
 *   0x60            -> 0x60  n (u8), then n u32: TIMER4 ticks (24 a microsecond) of
 *                            BN_N samples of each effect (FX_N), of each channel's strip as the patch has it (NCH),
 *                            of the strip bare and with each part added (BN_VAR), then a block (CTL samples) of the
 *                            channels and slots (mx_samples), of the output stage, of the coefficients and the
 *                            modulation (mix_block less the rest), and of the whole mix_block
 *   0x61 seed (u32) fam (u8, 255: any) secs (u8)
 *                   -> (secs later) 0x61  family (u8), then u32: the load's mean and peak (% of a half, x 10),
 *                            halves late, CPU0's block (ticks, mean and peak), CPU1's (peak)
 * The 0x61 patch is drawn as HOME draws it, its RHYTHM off; it stays until another, or the panel moves on. */

#define BN_N 256u
enum { BV_BARE, BV_NOISE, BV_FOLD, BV_SAT, BV_EQ, BV_PEAK, BV_LIMSLOW, BV_ALL, BV_N };
static int32_t bn_sig[BN_N];
static lil_par_t bn_keep POOL;
/* the RAM code, reached from the flash through pointers (far calls) */
static int32_t (*volatile bn_fx)(uint32_t, int32_t) = fx_tick;
static int32_t (*volatile bn_ch)(uint32_t, int32_t) = ch_tick;
static void (*volatile bn_chc)(uint32_t) = ch_coef;
static void (*volatile bn_slc)(uint32_t) = slot_coef;
static void (*volatile bn_mix)(int32_t *) = mix_block;
/* the ISR's tallies while a 0x61 patch plays (main.c) */
static volatile uint32_t bn_lsum, bn_lpk, bn_ln, bn_late0;
static volatile uint32_t bn_m0sum, bn_m0pk, bn_m1pk;
static struct { uint8_t on, fam; uint32_t until; } bnp;

static void bn_signal(void)
{
    uint32_t i, r = 0x1234567u;
    for (i = 0; i < BN_N; i++) {                 /* a sine and some noise, -10 dB or so */
        r = r * 1664525u + 1013904223u;
        bn_sig[i] = (sin_ph(i * (1u << 25)) >> 2) + ((int32_t)(r >> 16) - 32768) / 16;
    }
}
/* the fewest ticks of three runs of BN_N samples, interrupts off (a run: well under a millisecond) */
static uint32_t bn_run_fx(uint32_t s)
{
    uint32_t best = 0xFFFFFFFFu, k, i;
    int32_t acc = 0;
    for (k = 0; k < 4u; k++) {
        uint32_t f = irq_save(), t0 = fm1_ticks(), d;
        for (i = 0; i < BN_N; i++)
            acc += bn_fx(s, bn_sig[i]);
        d = fm1_ticks() - t0;
        irq_restore(f);
        if (k && d < best)                       /* (the first: a warm-up) */
            best = d;
        fm1_wdt_feed();
    }
    bn_sig[0] ^= acc & 1;                        /* (the results used) */
    return best;
}
static uint32_t bn_run_ch(uint32_t c)
{
    uint32_t best = 0xFFFFFFFFu, k, i;
    int32_t acc = 0;
    for (k = 0; k < 4u; k++) {
        uint32_t f = irq_save(), t0 = fm1_ticks(), d;
        for (i = 0; i < BN_N; i++)
            acc += bn_ch(c, bn_sig[i]);
        d = fm1_ticks() - t0;
        irq_restore(f);
        if (k && d < best)
            best = d;
        fm1_wdt_feed();
    }
    bn_sig[0] ^= acc & 1;
    return best;
}
static uint32_t bn_run_blk(void (*fn)(void))
{
    uint32_t best = 0xFFFFFFFFu, k;
    for (k = 0; k < 5u; k++) {
        uint32_t f = irq_save(), t0 = fm1_ticks(), d;
        fn();
        d = fm1_ticks() - t0;
        irq_restore(f);
        if (k && d < best)
            best = d;
        fm1_wdt_feed();
    }
    return best;
}
static void bn_mx(void) { mx_samples_p(); }
static void bn_out(void) { out_stage_p(); }
static int32_t bn_buf[2u * CTL];
static void bn_mixb(void) { bn_mix(bn_buf); }

/* the CPU's clock: TIMER4's ticks for 64000 dependent adds (one a cycle), interrupts off */
static FAST void bn_adds_ram(uint32_t n) { fm1_cpu_adds(n); }
static void (*volatile bn_adds_ram_p)(uint32_t) = bn_adds_ram;
static uint32_t bn_clock_ram(void)              /* the same, run from the RAM (.fast_text) */
{
    uint32_t best = 0xFFFFFFFFu, k;
    for (k = 0; k < 3u; k++) {
        uint32_t f = irq_save(), t0 = fm1_ticks(), d;
        bn_adds_ram_p(1000u);
        d = fm1_ticks() - t0;
        irq_restore(f);
        if (k && d < best)
            best = d;
    }
    return best;
}
static uint32_t bn_clock(void)
{
    uint32_t best = 0xFFFFFFFFu, k;
    for (k = 0; k < 3u; k++) {
        uint32_t f = irq_save(), t0 = fm1_ticks(), d;
        fm1_cpu_adds(1000u);
        d = fm1_ticks() - t0;
        irq_restore(f);
        if (k && d < best)
            best = d;
        fm1_wdt_feed();
    }
    return best;
}

static void bench_costs(void)
{
    uint32_t n = 0, t, c, v;
    bn_keep = par;
    par.out.mute = 1;
    fm1_delay_ms(50);                            /* (the mute glides in) */
    bn_signal();
    for (t = 0; t < FX_N; t++) {                 /* each effect, in slot 1, on SEND 1 */
        slot_par_t *s = &par.slot[0];
        s->fx = (int16_t)t;
        s->p1 = s->p2 = 500;
        s->pres = 1000;
        s->lofi = s->bypass = 0;
        s->in = FI_SEN1;
        s->out = FO_RET1;
        eff_slot[0] = *s;
        bn_slc(0);
        bk_wr32(bk_msg + 7u + 4u * n++, bn_run_fx(0));
    }
    par = bn_keep;
    par.out.mute = 1;
    for (c = 0; c < NCH; c++) {                  /* each channel as the patch has it */
        eff_ch[c] = par.ch[c];
        bn_chc(c);
        bk_wr32(bk_msg + 7u + 4u * n++, bn_run_ch(c));
    }
    for (v = 0; v < BV_N; v++) {                 /* channel 1's strip, bare, then with one part each, then all */
        ch_par_t *p = &par.ch[0];
        *p = bn_keep.ch[0];
        p->noise = 0;
        p->fold = 0;
        p->tmode = TM_LIN;
        p->ftype = FT_SHELF;
        p->high = p->low = 500;
        p->mtype = MT_PEAK;
        p->mgain = 500;
        p->lim = 0;
        if (v == BV_NOISE || v == BV_ALL) {
            p->noise = 500;
            p->ntype = NZ_PINK;
        }
        if (v == BV_FOLD || v == BV_ALL)
            p->fold = 600;
        if (v == BV_SAT || v == BV_ALL)
            p->tmode = TM_SAT;
        if (v == BV_EQ || v == BV_ALL)
            p->high = p->low = 700;
        if (v == BV_PEAK || v == BV_ALL)
            p->mgain = 800;
        if (v == BV_LIMSLOW || v == BV_ALL)
            p->lim = 1000;
        eff_ch[0] = *p;
        bn_chc(0);
        bk_wr32(bk_msg + 7u + 4u * n++, bn_run_ch(0));
    }
    par = bn_keep;
    par.out.mute = 1;
    ch_fresh_all();
    bn_mixb();                                   /* (the coefficients anew) */
    {
        uint32_t mx = bn_run_blk(bn_mx), out = bn_run_blk(bn_out), all = bn_run_blk(bn_mixb);
        bk_wr32(bk_msg + 7u + 4u * n++, mx);
        bk_wr32(bk_msg + 7u + 4u * n++, out);
        bk_wr32(bk_msg + 7u + 4u * n++, all > mx ? all - mx : 0u);   /* (with CPU1 online the stage is not in it) */
        bk_wr32(bk_msg + 7u + 4u * n++, all);
        for (c = 0; c < 7u; c++)                 /* the last block's parts (mixer.c's mb_ph) */
            bk_wr32(bk_msg + 7u + 4u * n++, mb_ph[c]);
        bk_wr32(bk_msg + 7u + 4u * n++, bn_clock());
        bk_wr32(bk_msg + 7u + 4u * n++, bn_clock_ram());
    }
    bk_msg[6] = (uint8_t)n;
    par = bn_keep;
    ch_fresh_all();
    bk_send(0x60u, 1u + 4u * n);
}

static void bench_patch(uint32_t seed, int fam, uint32_t secs)
{
    static lil_par_t q POOL;
    uint32_t f = patch_draw(&q, seed, fam);
    q.rhy.on = 0;
    q.out.mute = 1;                              /* (measured, not heard) */
    ui.fam = ui.recipe = (uint8_t)f;
    patch_set(&q, FAM[f].name);
    bnp.fam = (uint8_t)f;
    bnp.until = fm1_ms + 1000u * (secs ? secs : 3u);
    bn_lsum = bn_lpk = bn_ln = 0;
    bn_m0sum = bn_m0pk = bn_m1pk = 0;
    bn_late0 = dbg.late;
    audio_worker.max_job_ticks = 0;
    bnp.on = 1;
}
/* main loop: a 0x61 patch's time up, its tallies sent */
static void bench_poll(void)
{
    uint32_t n = bn_ln ? bn_ln : 1u;
    if (!bnp.on || (int32_t)(fm1_ms - bnp.until) < 0)
        return;
    bnp.on = 0;
    bk_msg[6] = bnp.fam;
    bk_wr32(bk_msg + 7, bn_lsum * 10u / n);
    bk_wr32(bk_msg + 11, bn_lpk * 10u);
    bk_wr32(bk_msg + 15, dbg.late - bn_late0);
    bk_wr32(bk_msg + 19, bn_m0sum / n);
    bk_wr32(bk_msg + 23, bn_m0pk);
    bk_wr32(bk_msg + 27, bn_m1pk);
    bk_send(0x61u, 25u);
}
/* the ISR, each half: its load (%), CPU0's last block, CPU1's longest job */
static void bench_tally(uint32_t pc)
{
    if (!bnp.on)
        return;
    bn_lsum += pc;
    bn_ln++;
    if (pc > bn_lpk)
        bn_lpk = pc;
    bn_m0sum += mx_ticks;
    if (mx_ticks > bn_m0pk)
        bn_m0pk = mx_ticks;
    if (audio_worker.max_job_ticks > bn_m1pk)
        bn_m1pk = audio_worker.max_job_ticks;
}

static void bench_handle(const uint8_t *m, uint32_t d)
{
    const uint8_t *b = m + 6;
    if (m[2] == 0x60u && d == 7u)
        bench_costs();
    else if (m[2] == 0x61u && d == 7u + 6u) {
        bench_patch(bk_rd32(b), b[4] == 255u ? -1 : (int)b[4], b[5] & 0x7Fu);
        if (b[5] & 0x80u) {                      /* (+ the DRUMS at their densest: every voice on every sixteenth, long) */
            uint32_t c;
            drm_draw_rhythm(bk_rd32(b), DS_PISTON);
            for (c = 0; c < NDV; c++) {
                drm.l[c].hits = 0xFFFFu;
                drm.l[c].len = 16;
                drm.v[c].decay = 800;
                drm.v[c].verb = 500;
                drm.v[c].metal = 4;
            }
            drm_coef_all();
            drm.on = 1;
            drm_reset = 1;
        } else
            drm.on = 0;
    }
}
