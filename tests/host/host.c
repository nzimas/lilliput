/* SPDX-License-Identifier: GPL-3.0-only
 * LILLIPUT host harness: the firmware's mixer, patches, panel and screen, compiled for the computer.
 *
 *   host render DIR       every factory patch and some drawn ones, 12 s each, a WAV each and a line of figures
 *   host check            200 patches drawn by HOME: each must sound, none run away (DC, a stuck rail, silence)
 *   host screens DIR      the screen in its states, as PPM images
 *   host flows            the panel: channels, pages, keys, HOME, sessions -- nothing stuck, nothing lost
 *   host cores N          the same patch on one core and on two: the same sound (a hash)
 *
 * Build: tests/run_tests.sh. */
#define HOST 1
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "tonitor_tables.h"

#define NELEM(a) (sizeof(a) / sizeof((a)[0]))
#define RING_PUBLISH() __asm__ volatile("" ::: "memory")
#define UAC_NBLK 8u
#define FELUCCA_UAC 1
static uint32_t fm1_ms;
static uint32_t host_ticks;
static uint32_t fm1_ticks(void) { return host_ticks += 24000u; }

static int str_eq(const char *a, const char *b) { return strcmp(a, b) == 0; }
static unsigned str_len(const char *s) { return (unsigned)strlen(s); }
static void str_cpy(char *d, const char *s, unsigned max)
{
    unsigned i = 0;
    while (s[i] && i + 1 < max) {
        d[i] = s[i];
        i++;
    }
    d[i] = 0;
}
static void fmt_fix(char *b, int32_t v, int decimals)   /* (libc.c's) */
{
    char t[12];
    int n = 0, neg = v < 0, i;
    uint32_t u = (uint32_t)(neg ? -v : v);
    do {
        t[n++] = (char)('0' + u % 10u);
        u /= 10u;
        if (n == decimals)
            t[n++] = '.';
    } while (u || n <= decimals);
    if (neg)
        t[n++] = '-';
    for (i = 0; i < n; i++)
        b[i] = t[n - 1 - i];
    b[n] = 0;
}
static void fmt_int(char *b, int32_t v) { fmt_fix(b, v, 0); }

/* the screen: a framebuffer instead of the panel */
static uint16_t fb[240 * 240];
static void lcd_sync(void) {}
static void lcd_blit(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint16_t *px)
{
    uint32_t i, j;
    for (j = 0; j < h; j++)
        for (i = 0; i < w; i++)
            if (x + i < 240u && y + j < 240u) {
                uint16_t c = px[j * w + i];
                fb[(y + j) * 240u + x + i] = (uint16_t)((c >> 8) | (c << 8));
            }
}
static void lcd_fill(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t c)
{
    uint32_t i, j;
    for (j = 0; j < h; j++)
        for (i = 0; i < w; i++)
            if (x + i < 240u && y + j < 240u)
                fb[(y + j) * 240u + x + i] = c;
}

#include "dsp.h"
#include "voice.c"
#include "gfx.c"
#include "logo.c"
#include "reverb.c"
/* CPU1's mailbox as one core sees it: a job posted runs at the join */
static int audio_worker_online;
static void (*host_job)(void *);
static void *host_job_ctx;
static int audio_worker_submit(void (*fn)(void *), void *ctx)
{
    if (!audio_worker_online)
        return 0;
    host_job = fn;
    host_job_ctx = ctx;
    return 1;
}
static int audio_worker_join(void)
{
    if (host_job)
        host_job(host_job_ctx);
    host_job = 0;
    return 1;
}
static void audio_worker_check(void) {}
static struct { uint8_t config, suspended, up; } usb;
#define UAC_NMAX 6u
static uint32_t uac_nch = 2;
static void usb_reconnect(uint32_t n) { uac_nch = n; }
static uint32_t host_mout[256], host_mout_n;
static void midi_out_event(uint32_t pkt)
{
    if (host_mout_n < 256u)
        host_mout[host_mout_n] = pkt;
    host_mout_n++;
}
#define MQ 64u
static uint32_t midi_in_q[MQ];
static volatile uint32_t mi_w, mi_r;
#define NLAYER 6
#include "midi.c"
#undef NLAYER
#include "mixer.c"
#include "patch.c"

/* the panel */
#define FM1_NCOL 11u
static const int8_t FM1_KEYMAP[6][FM1_NCOL] = {
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, {5, 11, 4, 10, 3, 9, 2, 8, -1, -1, -1},
    {34, 35, 36, 37, 38, 40, 39, 13, 7, 6, 12}, {23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33},
    {0, 1, 15, 14, 17, 16, 19, 18, 20, 21, 22}, {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}};
static struct { uint32_t notes, buttons; } fm1_in;
static uint32_t host_press;
static int32_t host_enc[7];
static uint8_t fm1_led_dim[FM1_NCOL];
static uint32_t fm1_input_edges(uint32_t *rel)
{
    uint32_t p = host_press;
    host_press = 0;
    if (rel)
        *rel = 0;
    return p;
}
static int32_t fm1_enc_take(uint32_t e)
{
    int32_t s = host_enc[e];
    host_enc[e] = 0;
    return s;
}
static uint8_t host_led[64];
static void fm1_led_key(uint32_t id, int on) { if (id < 64u) host_led[id] = (uint8_t)!!on; }
#include "lui.c"
#include "lses.c"
#include "setup.c"
static int bk_wire(const uint8_t *p, uint32_t n) { (void)p; (void)n; return 0; }
static void bk_done(uint32_t num) { (void)num; }
#include "backup.c"

/* the flash, emulated: NOR (erased to 0xFF, programming only clears bits) */
static uint8_t host_flash[0x100000];
static int ses_flash_read(uint32_t off, void *p, uint32_t n)
{
    memcpy(p, host_flash + off, n);
    return 0;
}
static int ses_flash_erase(uint32_t off)
{
    if (off < SES_LO || off + 4096u > SES_HI || (off & 4095u))
        return -8;
    memset(host_flash + off, 0xFF, 4096u);
    return 0;
}
static int ses_flash_prog(uint32_t off, const void *p, uint32_t n)
{
    uint32_t i;
    if (off < SES_LO || off + n > SES_HI)
        return -8;
    for (i = 0; i < n; i++)
        host_flash[off + i] &= ((const uint8_t *)p)[i];
    return 0;
}

/* the ISR and the main loop, on the computer */
static int32_t mix_out[2 * CTL];
static uint32_t ms_frac;
static void host_pump(void)
{
    mix_block(mix_out);
    ms_frac += CTL * 1000u;
    fm1_ms += ms_frac / 44100u;
    ms_frac %= 44100u;
}
static void run_ms(uint32_t ms)                   /* the audio and the main loop for ms (a pass each ~15 ms) */
{
    uint32_t t0 = fm1_ms, next = fm1_ms;
    while ((uint32_t)(fm1_ms - t0) < ms) {
        host_pump();
        if ((int32_t)(fm1_ms - next) >= 0) {
            ui_input();
            ui_draw();
            ui_leds();
            next = fm1_ms + 15u;
        }
    }
}
static void press_btn(uint32_t label)
{
    host_press |= 1u << PANEL_BTN[label];
    fm1_in.buttons |= 1u << PANEL_BTN[label];
    run_ms(30);
    fm1_in.buttons &= ~(1u << PANEL_BTN[label]);
    run_ms(30);
}
static void hold_btn(uint32_t label, uint32_t ms)
{
    host_press |= 1u << PANEL_BTN[label];
    fm1_in.buttons |= 1u << PANEL_BTN[label];
    run_ms(ms);
    fm1_in.buttons &= ~(1u << PANEL_BTN[label]);
    run_ms(30);
}
static void key(uint32_t k, int down)
{
    if (down)
        fm1_in.notes |= 1u << k;
    else
        fm1_in.notes &= ~(1u << k);
    run_ms(30);
}
static void tap_key(uint32_t k) { key(k, 1); key(k, 0); }
static uint32_t white_key(int32_t w)              /* white key w's place on the keyboard */
{
    uint32_t p;
    for (p = 0; p < NKEY; p++)
        if (white_ix(p) == w)
            return p;
    return 0;
}
static void turn(uint32_t role, int32_t s)
{
    host_enc[PANEL_ENC[role]] += s;
    run_ms(30);
}
static void ppm(const char *path)
{
    FILE *f = fopen(path, "wb");
    uint32_t i;
    if (!f)
        return;
    fprintf(f, "P6\n240 240\n255\n");
    for (i = 0; i < 240u * 240u; i++) {
        uint16_t c = fb[i];
        uint8_t rgb[3] = {(uint8_t)((c >> 11) << 3), (uint8_t)(((c >> 5) & 63u) << 2), (uint8_t)((c & 31u) << 3)};
        fwrite(rgb, 1, 3, f);
    }
    fclose(f);
}

/* ------------------------------------------------------------- audio --- */
static void wav_write(const char *path, const int16_t *s, uint32_t frames)
{
    FILE *f = fopen(path, "wb");
    uint32_t n = frames * 4u, v;
    if (!f)
        return;
    fwrite("RIFF", 1, 4, f);
    v = 36u + n;
    fwrite(&v, 4, 1, f);
    fwrite("WAVEfmt ", 1, 8, f);
    v = 16;
    fwrite(&v, 4, 1, f);
    {
        uint16_t h[2] = {1, 2};
        uint32_t sr = FS, br = FS * 4u;
        uint16_t ba[2] = {4, 16};
        fwrite(h, 2, 2, f);
        fwrite(&sr, 4, 1, f);
        fwrite(&br, 4, 1, f);
        fwrite(ba, 2, 2, f);
    }
    fwrite("data", 1, 4, f);
    fwrite(&n, 4, 1, f);
    fwrite(s, 2, frames * 2u, f);
    fclose(f);
}

typedef struct { double rms, peak, dc, zc_hz, clip, late_rms, move; } stats_t;
static double db(double x);
/* render secs of the patch now (par), the output at MASTER unity; figures from the last part */
static stats_t render(double secs, int16_t *keep, uint32_t keep_max)
{
    uint32_t blocks = (uint32_t)(secs * FS / CTL), b, i, n = 0, zc = 0, clip = 0, wn = 0, nw = 0;
    double w2 = 0, mv1 = 0, mv2 = 0;
    int32_t out[2 * CTL], prev = 0;
    double s2 = 0, s1 = 0, pk = 0, l2 = 0;
    stats_t st;
    master_q12 = 4096;
    fm1_ms = 0;
    trim_start();
    for (b = 0; b < blocks; b++) {
        fm1_ms = (uint32_t)((uint64_t)b * CTL * 1000u / FS);
        trim_poll();
        mix_block(out);
        if (getenv("DBG") && b % 2756 == 0)
            fprintf(stderr, "  t %5.1f gm %d gmt %d level %d meter %d\n", b * CTL / (double)FS, outr.gm, outr.gmt, par.out.level, out_meter);
        for (i = 0; i < CTL; i++) {
            int32_t l = out[2 * i], r = out[2 * i + 1];
            if (keep && n < keep_max) {
                keep[2 * n] = (int16_t)clampi(l, -32768, 32767);
                keep[2 * n + 1] = (int16_t)clampi(r, -32768, 32767);
            }
            n++;
            if (b >= blocks / 3) {               /* (past the start) */
                double x = l / 32768.0;
                s2 += x * x;
                s1 += x;
                if (fabs(x) > pk)
                    pk = fabs(x);
                if ((l >= 0) != (prev >= 0))
                    zc++;
                if (abs(l) > 32000)
                    clip++;
                if (b >= blocks * 5 / 6)
                    l2 += x * x;
                w2 += x * x;                     /* (50 ms windows: how much the loudness moves) */
                if (++wn == 2205u) {
                    double d = db(sqrt(w2 / wn) + 1e-6);
                    mv1 += d;
                    mv2 += d * d;
                    nw++;
                    wn = 0;
                    w2 = 0;
                }
            }
            prev = l;
        }
    }
    {
        double m = (double)(blocks - blocks / 3) * CTL;
        st.rms = sqrt(s2 / m);
        st.dc = s1 / m;
        st.peak = pk;
        st.zc_hz = zc / 2.0 / (m / FS);
        st.clip = clip / m;
        st.late_rms = sqrt(l2 / ((double)(blocks - blocks * 5 / 6) * CTL));
        st.move = nw > 1 ? sqrt(fmax(0, mv2 / nw - (mv1 / nw) * (mv1 / nw))) : 0;
    }
    return st;
}
static double db(double x) { return x > 1e-9 ? 20 * log10(x) : -180; }
static void fresh_mixer(void)
{
    mx_init();
    b_mixl = b_mixr = b_s1 = b_s2 = b_r1 = b_r2 = 0;
    memset(ch_pre, 0, sizeof ch_pre);
    memset(ch_post, 0, sizeof ch_post);
    memset(lim_mem, 0, sizeof lim_mem);
    memset(slot_mem, 0, sizeof slot_mem);
    memset(ch_meter, 0, sizeof ch_meter);
    out_meter = 0;
}
static void print_patch(void)
{
    uint32_t c;
    for (c = 0; c < NCH; c++) {
        const ch_par_t *p = &par.ch[c];
        printf("    ch%u %-6s lvl %5.1f gain %4.1f %s/%s %s %s noise %d lim %d mid %s %d@%d snd %d/%d%s\n", c + 1,
               SRC_NAME[p->src], p->level ? lvl_db10(p->level) / 10.0 : -99.0, p->gain * 48.0 / PMAX, TB_NAME[p->tube],
               TM_NAME[p->tmode], OA_NAME[p->opamp], NZ_NAME[p->ntype], p->noise, p->lim, MT_NAME[p->mtype], p->mgain,
               mid_hz(p->mfreq), p->send1, p->send2, par.slot[c].in != FI_NON ? " [slot]" : "");
    }
    for (c = 0; c < NSLOT; c++)
        if (par.slot[c].in != FI_NON)
            printf("    slot%u %s in %s out %s p %d %d pres %d%s\n", c + 1, FX_NAME[par.slot[c].fx], FI_NAME[par.slot[c].in],
                   FO_NAME[par.slot[c].out], par.slot[c].p1, par.slot[c].p2, par.slot[c].pres, par.slot[c].lofi ? " lofi" : "");
}

static int cmd_render(const char *dir, int verbose)
{
    static int16_t buf[2 * FS * 12];
    uint32_t i, bad = 0;
    char path[512];
    for (i = 0; i < NFACTORY + 8u; i++) {
        stats_t st;
        const char *name;
        char nm[40];
        fresh_mixer();
        if (i < NFACTORY) {
            factory_load(&par, i);
            name = FACTORY[i].name;
        } else {
            uint32_t rc = patch_draw(&par, 9000u + i * 37u, -1);
            snprintf(nm, sizeof nm, "DRAWN %u %s", i - NFACTORY, FAM[rc].name);
            name = nm;
        }
        st = render(12.0, buf, FS * 12u);
        {
            char fn[64];
            uint32_t k;
            snprintf(fn, sizeof fn, "%02u_%s.wav", i, name);
            for (k = 0; fn[k]; k++)
                if (fn[k] == ' ')
                    fn[k] = '_';
            snprintf(path, sizeof path, "%s/%s", dir, fn);
            wav_write(path, buf, FS * 12u);
        }
        printf("[main %4d] ", par.out.level);
        printf("%-16s rms %6.1f dB  peak %6.1f dB  late %6.1f dB  dc %+.4f  zc %6.0f Hz  clip %.3f%%\n", name, db(st.rms),
               db(st.peak), db(st.late_rms), st.dc, st.zc_hz, st.clip * 100);
        if (verbose)
            print_patch();
        if (db(st.late_rms) < -50 || st.clip > 0.05)
            bad++;
    }
    return bad ? 1 : 0;
}

/* where one channel's own loop starts to sing: its level swept, the output's rms */
static double ch_rms(void (*setup)(ch_par_t *), int32_t level_db10)
{
    stats_t st;
    fresh_mixer();
    patch_plain(&par);
    par.ch[0].level = (int16_t)clampi((level_db10 + 600) * PMAX / 720, 1, PMAX);
    par.out.spres = 0;
    setup(&par.ch[0]);
    trim.on = 0;
    st = render(3.0, 0, 0);
    trim.on = 0;
    return db(st.late_rms);
}
static void cs_plain(ch_par_t *p) { p->noise = 500; p->ntype = NZ_WHITE; p->mgain = 500; }
static void cs_sat(ch_par_t *p) { cs_plain(p); p->tmode = TM_SAT; }
static void cs_band(ch_par_t *p) { cs_plain(p); p->mtype = MT_BAND; p->mgain = 450; p->mfreq = 300; }
static void cs_notch(ch_par_t *p) { cs_plain(p); p->mtype = MT_NOTCH; p->mgain = 600; }
static void cs_peak(ch_par_t *p) { cs_plain(p); p->mgain = 800; }
static void cs_slow(ch_par_t *p) { cs_plain(p); p->lim = 900; }
static int cmd_calib(void)
{
    static const struct { const char *n; void (*f)(ch_par_t *); } C[] = {{"plain", cs_plain}, {"sat 12AX7", cs_sat},
        {"band 300", cs_band}, {"notch", cs_notch}, {"peak +11", cs_peak}, {"slow lim", cs_slow}};
    uint32_t i;
    int32_t l;
    for (i = 0; i < NELEM(C); i++) {
        printf("%-10s", C[i].n);
        for (l = -300; l <= 60; l += 30)
            printf(" %+3d:%5.0f", l / 10, ch_rms(C[i].f, l));
        printf("\n");
    }
    return 0;
}

static int cmd_ring(void)
{
    int32_t l;
    for (l = -300; l <= 0; l += 30) {
        stats_t st;
        fresh_mixer();
        patch_plain(&par);
        par.out.spres = 0;
        par.ch[0].src = SRC_CH2;
        par.ch[1].src = SRC_CH1;
        par.ch[0].level = par.ch[1].level = (int16_t)((l + 600) * PMAX / 720);
        par.ch[0].noise = par.ch[1].noise = 500;
        st = render(3.0, 0, 0);
        printf(" %+3d:%5.0f (ch %d %d)", l / 10, db(st.late_rms), ch_meter[0], ch_meter[1]);
    }
    printf("\n");
    return 0;
}

/* ---------------------------------------------------------------- check --- */
/* 200 patches HOME draws: each must sound (above -45 dB), none clip for long, none sit on DC */
static int cmd_check(void)
{
    uint32_t i, bad = 0, quiet = 0, fn[32] = {0};
    double lo = 0, hi = -200, fzc[32] = {0}, fmv[32] = {0}, frms[32] = {0};
    for (i = 0; i < 200u; i++) {
        stats_t st;
        uint32_t rc;
        fresh_mixer();
        rc = patch_draw(&par, 0xC0FFEEu + i * 7919u, -1);
        st = render(8.0, 0, 0);
        fn[rc]++;
        fzc[rc] += log2(st.zc_hz + 1);
        fmv[rc] += st.move;
        frms[rc] += db(st.late_rms);
        if (db(st.late_rms) < lo || i == 0)
            lo = db(st.late_rms);
        if (db(st.late_rms) > hi)
            hi = db(st.late_rms);
        if (db(st.late_rms) < -45) {
            quiet++;
            printf("  quiet: patch %u (%s) %.1f dB\n", i, FAM[rc].name, db(st.late_rms));
        }
        if (st.clip > 0.02 || fabs(st.dc) > 0.02) {
            bad++;
            printf("  bad: patch %u (%s) clip %.2f%% dc %+.3f\n", i, FAM[rc].name, st.clip * 100, st.dc);
        }
    }
    for (i = 0; i < NFAM; i++)
        if (fn[i])
            printf("  %-13s %3u patches  pitch ~%6.0f Hz  movement %4.1f dB  loudness %6.1f dB\n", FAM[i].name, fn[i],
                   exp2(fzc[i] / fn[i]), fmv[i] / fn[i], frms[i] / fn[i]);
    printf("check: 200 patches, loudness %.1f .. %.1f dB, %u quiet, %u bad\n", lo, hi, quiet, bad);
    return bad || quiet > 4u ? 1 : 0;
}

/* ------------------------------------------------------------- screens --- */
static int cmd_screens(const char *dir)
{
    char path[512];
#define SHOT(name) do { snprintf(path, sizeof path, "%s/%s.ppm", dir, name); ppm(path); } while (0)
    memset(host_flash, 0xFF, sizeof host_flash);
    ses_scan(1);
    splash_draw("BUILD 1");
    SHOT("00_splash");
    fresh_mixer();
    ui_init();
    run_ms(1500);
    SHOT("01_main");
    turn(EN_K3, 2);
    run_ms(60);
    SHOT("02_dial_limit");
    run_ms(1200);
    press_btn(B_OCTUP);
    turn(EN_K1, 1);
    run_ms(60);
    SHOT("03_amp");
    press_btn(B_OCTUP);
    run_ms(60);
    SHOT("04_noise");
    press_btn(B_OCTUP);
    run_ms(60);
    SHOT("05_eq");
    press_btn(B_OCTUP);
    run_ms(60);
    SHOT("06_send");
    press_btn(B_OCTUP);
    run_ms(60);
    SHOT("07_fx");
    key(KEY_SHIFT, 1);
    run_ms(60);
    SHOT("07a_fx_shift");
    key(KEY_SHIFT, 0);
    press_btn(B_OCTUP);
    run_ms(300);
    SHOT("07b_lfo1");
    press_btn(B_OCTUP);
    press_btn(B_OCTUP);
    run_ms(300);
    SHOT("07c_mod_1_4");
    press_btn(B_OCTUP);
    run_ms(300);
    SHOT("07d_mod_5_8");
    press_btn(B_SEQ);
    run_ms(60);
    SHOT("08_out");
    press_btn(B_SEQ);
    run_ms(60);
    SHOT("09_out_eq");
    press_btn(B_SEQ);
    run_ms(60);
    SHOT("09b_out_fx");
    press_btn(B_SEQ);
    run_ms(60);
    press_btn(B_ARP);
    run_ms(300);
    SHOT("10_overview");
    press_btn(LAYER_BTN[2]);
    tap_key(KEY_MUTE[2]);
    run_ms(100);
    SHOT("11_muted");
    tap_key(KEY_MUTE[2]);
    turn(EN_PRESET, 1);
    run_ms(60);
    SHOT("12_presets");
    press_btn(LAYER_BTN[0]);
    press_btn(B_HOME);
    run_ms(200);
    SHOT("13_new_patch");
    key(KEY_SHIFT, 1);
    key(KEY_CPU, 1);
    run_ms(300);
    SHOT("14_cpu");
    key(KEY_CPU, 0);
    key(KEY_SHIFT, 0);
    press_btn(B_SAVE);
    key(white_key(0), 1);
    run_ms(900);
    key(white_key(0), 0);
    key(white_key(6), 1);
    run_ms(900);
    key(white_key(6), 0);
    turn(EN_K1, 5);
    run_ms(1700);
    SHOT("16_snapshots");
    press_btn(B_SAVE);
    press_btn(B_REC);
    turn(EN_SELECT, 1);
    run_ms(2200);
    SHOT("17_rec");
    press_btn(B_REC);
    hold_btn(B_SAVE, 900);
    run_ms(60);
    SHOT("15_save_ask");
    printf("screens: %s\n", dir);
    return 0;
#undef SHOT
}

/* --------------------------------------------------------------- flows --- */
#define EXPECT(c, what) do { if (!(c)) { printf("FAIL flows: %s\n", what); return 1; } } while (0)
static int cmd_flows(void)
{
    int16_t keep_src;
    lil_par_t before;
    memset(host_flash, 0xFF, sizeof host_flash);
    ses_scan(1);
    fresh_mixer();
    ui_init();
    run_ms(3600);
    EXPECT(str_eq(ui.patch, FACTORY[0].name), "the first factory patch at the start");
    EXPECT(out_meter > 300, "the start sounds");
    /* channels and pages */
    press_btn(LAYER_BTN[3]);
    EXPECT(ui.sel == 3 && ui.page == PG_MAIN, "a channel's button chooses it");
    press_btn(B_OCTUP);
    press_btn(B_OCTUP);
    EXPECT(ui.page == PG_DYN, "OCT+ steps through the pages");
    press_btn(LAYER_BTN[1]);
    EXPECT(ui.sel == 1 && ui.page == PG_DYN, "another channel keeps the page");
    press_btn(LAYER_BTN[1]);
    EXPECT(ui.page == PG_MAIN, "the chosen channel again: its main view");
    /* the input keys */
    tap_key(white_key(SRC_MIXR));
    EXPECT(par.ch[1].src == SRC_MIXR, "a white key sets the input");
    tap_key(white_key(SRC_CH4));
    EXPECT(par.ch[1].src == SRC_CH4, "CH 4's key");
    tap_key(white_key(WHITE_MAINMIX));
    EXPECT(!par.ch[1].mainmix, "the last white key: main mix off");
    tap_key(white_key(WHITE_MAINMIX));
    /* the mutes */
    tap_key(KEY_MUTE[4]);
    EXPECT(par.ch[4].mute, "black key 5 mutes channel 5");
    tap_key(KEY_MUTE[4]);
    EXPECT(!par.ch[4].mute, "and back");
    /* level and pan */
    {   /* LEVEL: 1 dB a detent up to -5 dB, then 0.1 dB; PAN: 1 a detent */
        int16_t pn;
        par.ch[1].level = LVL_0DB - 123;         /* -12.3 dB */
        turn(EN_SELECT, 1);
        EXPECT(par.ch[1].level == LVL_0DB - 120, "SELECT up from -12.3 dB: -12 dB");
        turn(EN_SELECT, 1);
        EXPECT(par.ch[1].level == LVL_0DB - 110, "SELECT: -11 dB, a dB a detent");
        par.ch[1].level = LVL_0DB - 60;
        turn(EN_SELECT, 1);
        EXPECT(par.ch[1].level == LVL_0DB - 50, "SELECT: -6 to -5 dB");
        turn(EN_SELECT, 1);
        EXPECT(par.ch[1].level == LVL_0DB - 49, "SELECT: then -4.9 dB, a tenth a detent");
        turn(EN_SELECT, -1);
        turn(EN_SELECT, -1);
        EXPECT(par.ch[1].level == LVL_0DB - 60, "SELECT down: -4.9, -5, then -6 dB");
        par.ch[1].level = 2;                     /* -59.9 dB */
        turn(EN_SELECT, -1);
        turn(EN_SELECT, -1);
        EXPECT(par.ch[1].level == 0, "SELECT: below -60 dB, off");
        par.ch[1].level = LVL_0DB;
        pn = par.ch[1].pan;
        turn(EN_ALGO, -3);
        EXPECT(par.ch[1].pan == pn - 3, "ALGORITHM: the pan, 1 a detent");
    }
    /* KNOB 1 on the main view: GAIN, its dial */
    {
        int16_t g = par.ch[1].gain;
        turn(EN_K1, 1);
        EXPECT(par.ch[1].gain == g + 2 && ui.dial == P_GAIN, "KNOB 1: GAIN, and its dial");
    }
    /* the FX page's switches */
    press_btn(B_OCTUP); press_btn(B_OCTUP); press_btn(B_OCTUP); press_btn(B_OCTUP); press_btn(B_OCTUP);
    EXPECT(ui.page == PG_FX, "the FX page");
    key(KEY_SHIFT, 1);
    turn(EN_K1, 1);
    key(KEY_SHIFT, 0);
    EXPECT(par.slot[1].in != FI_NON || par.slot[1].in == FI_INS, "SHIFT + KNOB 1: the slot's input");
    /* HOME: a new patch of another family; SHIFT + HOME: of the same; SHIFT + PRESETS: the next family; held: nudged */
    press_btn(LAYER_BTN[0]);
    keep_src = par.ch[0].src;
    before = par;
    {
        uint32_t f0 = ui.fam;
        press_btn(B_HOME);
        EXPECT(memcmp(&before, &par, sizeof par) != 0 && ui.fam != f0, "HOME draws a new patch, of another family");
        EXPECT(str_eq(ui.patch, FAM[ui.fam].name), "the patch named after its family");
    }
    run_ms(3600);
    EXPECT(out_meter > 300, "the new patch sounds");
    {
        uint32_t f0 = ui.fam;
        before = par;
        key(KEY_SHIFT, 1);
        press_btn(B_HOME);
        key(KEY_SHIFT, 0);
        EXPECT(ui.fam == f0 && memcmp(&before, &par, sizeof par) != 0, "SHIFT + HOME: a new patch of the same family");
        key(KEY_SHIFT, 1);
        turn(EN_PRESET, 1);
        key(KEY_SHIFT, 0);
        EXPECT(ui.fam == (f0 + 1u) % NFAM, "SHIFT + PRESETS: the next family");
    }
    before = par;
    hold_btn(B_HOME, 900);
    {
        uint32_t c, same = 1;
        for (c = 0; c < NCH; c++)
            same &= before.ch[c].src == par.ch[c].src && before.ch[c].level == par.ch[c].level;
        EXPECT(same && memcmp(&before, &par, sizeof par) != 0, "HOME held: the tone moved, the connections kept");
    }
    /* the LFO pages, the matrix pages; SHIFT + SEQ throws the channel's modulation anew */
    press_btn(LAYER_BTN[2]);
    if (ui.page != PG_MAIN)
        press_btn(LAYER_BTN[2]);
    {
        uint32_t i;
        for (i = 0; i < 6u; i++)
            press_btn(B_OCTUP);
    }
    EXPECT(ui.page == PG_LFO1, "OCT+ to LFO 1");
    {
        uint8_t r0 = par.ch[2].mod.lfo[0].rate;
        turn(EN_K2, r0 < 100 ? 1 : -1);
        EXPECT(par.ch[2].mod.lfo[0].rate == (r0 < 100 ? r0 + 1 : r0 - 1), "KNOB 2 there: LFO 1's rate, a step a detent");
        press_btn(B_OCTUP);
        EXPECT(ui.page == PG_LFO2, "then LFO 2");
        turn(EN_K1, 1);
        press_btn(B_OCTUP);
        EXPECT(ui.page == PG_MTX1, "then MOD 1-4");
        memset(par.ch[2].mod.s, 0, sizeof par.ch[2].mod.s);
        turn(EN_K3, 25);
        EXPECT(par.ch[2].mod.s[2].amt == 25 && par.ch[2].mod.s[2].src == MSRC_L1, "KNOB 3: slot 3's amount (from LFO 1)");
        key(KEY_SHIFT, 1);
        turn(EN_K3, 2);
        key(KEY_SHIFT, 0);
        EXPECT(par.ch[2].mod.s[2].dst == 1, "SHIFT + KNOB 3: where it goes");
        turn(EN_PRESET, 1);
        EXPECT(par.ch[2].mod.s[2].src == MSRC_L2, "PRESETS: where the slot in hand comes from");
        press_btn(B_OCTUP);
        EXPECT(ui.page == PG_MTX2, "then MOD 5-8");
        turn(EN_K4, -10);
        EXPECT(par.ch[2].mod.s[7].amt == -10, "slot 8");
        before = par;
        key(KEY_SHIFT, 1);
        press_btn(B_SEQ);
        key(KEY_SHIFT, 0);
        EXPECT(memcmp(&before.ch[2].mod, &par.ch[2].mod, sizeof(mod_t)) != 0, "SHIFT + SEQ: the modulation thrown anew");
    }
    /* SHIFT + black key 1: the channel at random, its mute kept */
    before = par;
    par.ch[2].mute = 1;
    key(KEY_SHIFT, 1);
    tap_key(KEY_MUTE[0]);
    key(KEY_SHIFT, 0);
    EXPECT(par.ch[2].mute == 1 && !par.ch[0].mute == !before.ch[0].mute, "SHIFT + black key 1: no mute changed");
    EXPECT(memcmp(&before.ch[2].gain, &par.ch[2].gain, sizeof(ch_par_t) - 4) != 0 && memcmp(&before.ch[1], &par.ch[1], sizeof(ch_par_t)) == 0,
           "the chosen channel drawn anew, the others kept");
    par.ch[2].mute = 0;
    press_btn(LAYER_BTN[0]);
    press_btn(LAYER_BTN[0]);
    (void)keep_src;
    press_btn(B_PLAY);
    EXPECT(!gs.play, "PLAY with no gesture loop: nothing");
    /* the gesture loop: REC arms, the first change starts it, REC ends and plays; each round as recorded */
    press_btn(LAYER_BTN[0]);
    press_btn(LAYER_BTN[0]);
    run_ms(200);
    {
        int16_t l0 = par.ch[0].level, l1;
        press_btn(B_REC);
        EXPECT(gs.st == GS_ARMED, "REC arms");
        run_ms(300);
        EXPECT(gs.st == GS_ARMED, "nothing recorded until something changes");
        turn(EN_SELECT, -3);
        EXPECT(gs.st == GS_REC, "the first change starts the recording");
        run_ms(400);
        tap_key(KEY_MUTE[3]);
        run_ms(400);
        turn(EN_SELECT, -2);
        l1 = par.ch[0].level;
        run_ms(300);
        press_btn(B_REC);
        EXPECT(gs.st == GS_IDLE && gs.play && gs.n >= 3, "REC again: the loop ends and plays");
        run_ms(100);
        EXPECT(par.ch[0].level != l1, "a round begins as the recording began");
        run_ms(gs.len);                          /* (into the second round, then to its end) */
        while ((fm1_ms - gs.t0) % gs.len < gs.len - 80u)
            run_ms(10);
        EXPECT(par.ch[0].level == l1 && par.ch[3].mute, "and plays as recorded");
        press_btn(B_PLAY);
        EXPECT(!gs.play, "PLAY stops it");
        press_btn(B_PLAY);
        EXPECT(gs.play, "PLAY plays it again");
        (void)l0;
        key(KEY_SHIFT, 1);
        press_btn(B_PLAY);
        key(KEY_SHIFT, 0);
        EXPECT(par.out.mute, "SHIFT + PLAY: the output muted");
        key(KEY_SHIFT, 1);
        press_btn(B_PLAY);
        key(KEY_SHIFT, 0);
        EXPECT(!par.out.mute, "and back");
    }
    /* snapshots: SAVE short opens them; a key held stores, let go soon recalls; SHIFT + it empties */
    press_btn(B_PLAY);                            /* (the loop stopped: it would play over the recall) */
    press_btn(B_SAVE);
    EXPECT(ui.page == PG_SNAP, "SAVE short: the SNAPSHOTS page");
    {
        lil_par_t a;
        key(white_key(2), 1);
        run_ms(900);
        key(white_key(2), 0);
        EXPECT(snap_has(2), "a white key held: stored");
        a = par;
        par.ch[1].level = 100;
        par.ch[1].src = SRC_MIXR;
        tap_key(white_key(2));
        EXPECT(par.ch[1].level == a.ch[1].level && par.ch[1].src == a.ch[1].src, "let go soon: recalled");
        turn(EN_K1, 4);
        EXPECT(morph_hs == 4, "KNOB 1: the morph time");
        par.ch[1].level = 100;
        tap_key(white_key(2));
        run_ms(900);
        EXPECT(par.ch[1].level > 100 && par.ch[1].level < a.ch[1].level, "a morph on its way");
        run_ms(1400);
        EXPECT(par.ch[1].level == a.ch[1].level, "and there");
        key(KEY_SHIFT, 1);
        tap_key(white_key(5));
        key(white_key(5), 0);
        key(KEY_SHIFT, 0);
        key(white_key(5), 1);
        run_ms(900);
        key(white_key(5), 0);
        EXPECT(snap_has(5), "another stored");
        key(KEY_SHIFT, 1);
        tap_key(white_key(5));
        key(KEY_SHIFT, 0);
        EXPECT(!snap_has(5), "SHIFT + its key: emptied");
        press_btn(B_SAVE);
        EXPECT(ui.page != PG_SNAP, "SAVE again: back");
    }
    /* sessions: save as a new one (asked), load it back */
    hold_btn(B_SAVE, 900);
    EXPECT(ui.page == PG_ASK && ui.ses_new == 1, "SAVE held: a new session, asked");
    press_btn(B_SAVE);
    run_ms(1600);
    EXPECT(ses_has(1) && ses_current() == 1, "saved as session 1");
    before = par;
    press_btn(B_HOME);
    run_ms(200);
    EXPECT(memcmp(&before, &par, sizeof par) != 0, "another patch");
    key(KEY_SHIFT, 1);
    press_btn(B_SAVE);
    key(KEY_SHIFT, 0);
    EXPECT(ui.page == PG_SES && ui.ses_ix == 1, "SHIFT + SAVE: the SESSIONS page");
    press_btn(B_SAVE);
    run_ms(100);
    {   /* (the trim may move the main level after loading: the rest as saved) */
        lil_par_t a = before, b = par;
        a.out.level = b.out.level = 0;
        EXPECT(memcmp(&a, &b, sizeof a) == 0, "session 1 loaded back as it was");
        EXPECT(snap_has(2) && gs.n >= 3, "with its snapshots and its gesture loop");
    }
    /* again, from the flash as a boot finds it */
    ses_scan(1);
    EXPECT(ses_has(1), "the session found after a restart");
    /* presets */
    turn(EN_PRESET, 2);
    EXPECT(ui.page == PG_PRESET, "PRESETS: the factory's page");
    turn(EN_PRESET, 1);
    press_btn(B_SAVE);
    EXPECT(str_eq(ui.patch, FACTORY[1].name), "SAVE loads the preset shown");
    /* GLOBAL SETUP */
    key(KEY_SHIFT, 1);
    hold_btn(B_SAVE, 900);
    key(KEY_SHIFT, 0);
    EXPECT(ui.set_page, "SHIFT + SAVE held: GLOBAL SETUP");
    press_btn(LAYER_BTN[0]);
    EXPECT(!ui.set_page && ui.page == PG_MAIN, "a channel's button leaves it");
    /* another firmware's data in the area: left alone by the idle blanking and by a save while there is room */
    memset(host_flash, 0xFF, sizeof host_flash);
    memcpy(host_flash + SES_LO + 5u * SES_SECT, "ACS1 TONITOR SESSION", 20);
    ses_scan(1);
    {
        uint32_t i;
        for (i = 0; i < 80u; i++) {
            fm1_ms += 1100u;
            ses_idle(1);
        }
    }
    EXPECT(ses_save(9) == SES_OK, "a save beside another firmware's data");
    EXPECT(!memcmp(host_flash + SES_LO + 5u * SES_SECT, "ACS1 TONITOR SESSION", 20), "another firmware's data kept");
    printf("flows: ok\n");
    return 0;
}

/* --------------------------------------------------------------- effects --- *
 * build 4's: FOLD in a channel, and RESONATOR, XMOD, DUCK, GATE in a loop (an insert, POST) and on a send/return --
 * each bounded, heard, without DC; the RESONATOR's tuning out of the modulation's reach */
static stats_t fx_run(int fx, int p1, int p2, int as_send)
{
    fresh_mixer();
    patch_plain(&par);
    par.out.spres = 0;
    par.ch[0].level = LVL_0DB - 80;              /* a loop of its own, singing */
    par.ch[1].level = fx == FX_RESON ? 0 : LVL_0DB - 80;   /* (a second loop: XMOD's, DUCK's, GATE's other signal;
                                                         * the RESONATOR heard alone) */
    par.ch[1].src = SRC_PRE;
    par.ch[1].mfreq = 300;
    par.slot[0].fx = (int16_t)fx;
    par.slot[0].p1 = (int16_t)p1;
    par.slot[0].p2 = (int16_t)p2;
    par.slot[0].pres = PMAX;
    if (as_send) {
        par.slot[0].in = FI_SEN1;
        par.slot[0].out = FO_RET1;
        par.ch[0].src = SRC_RET1;
        par.ch[0].send1 = 700;
    } else {
        par.slot[0].in = FI_INS;
        par.ch[0].src = SRC_POST;
    }
    return render(6.0, 0, 0);
}
static int cmd_effects(void)
{
    static const struct { const char *n; int fx, p1, p2; } T[] = {
        {"RESONATOR C3 MAJOR", FX_RESON, 12, 2}, {"RESONATOR A2 PELOG", FX_RESON, 9, 6},
        {"XMOD RING", FX_XMOD, XM_RING, 1}, {"XMOD XOR", FX_XMOD, XM_XOR, 1}, {"XMOD FOLD", FX_XMOD, XM_FOLD, 1},
        {"XMOD COMPARE", FX_XMOD, XM_COMP, 1}, {"DUCK BY CH 2", FX_DUCK, 900, 1}, {"GATE BY CH 2", FX_GATE, 900, 1}};
    uint32_t i, bad = 0, a;
    for (i = 0; i < NELEM(T); i++)
        for (a = 0; a < 2u; a++) {
            stats_t st = fx_run(T[i].fx, T[i].p1, T[i].p2, (int)a);
            int ok = db(st.late_rms) > -50 && st.clip < 0.02 && fabs(st.dc) < 0.02;
            printf("  %-20s %-6s rms %6.1f dB  zc %6.0f Hz  clip %.2f%%  %s\n", T[i].n, a ? "SEND" : "INSERT",
                   db(st.late_rms), st.zc_hz, st.clip * 100, ok ? "ok" : "BAD");
            bad += !ok;
        }
    {   /* FOLD: harmonics up (the zero crossings), never louder past full scale */
        stats_t s0, s1;
        fresh_mixer();
        patch_plain(&par);
        par.out.spres = 0;
        par.ch[0].level = LVL_0DB - 80;
        s0 = render(4.0, 0, 0);
        fresh_mixer();
        patch_plain(&par);
        par.out.spres = 0;
        par.ch[0].level = LVL_0DB - 80;
        par.ch[0].fold = 800;
        s1 = render(4.0, 0, 0);
        printf("  FOLD 0 -> 800         zc %6.0f -> %6.0f Hz, peak %.2f -> %.2f\n", s0.zc_hz, s1.zc_hz, s0.peak, s1.peak);
        if (!(s1.zc_hz > s0.zc_hz * 1.3) || s1.clip > 0.02) {
            printf("  FOLD: BAD\n");
            bad++;
        }
    }
    {   /* the RESONATOR's root and chord: an LFO aimed at them moves nothing */
        int32_t b;
        fresh_mixer();
        patch_plain(&par);
        par.slot[0].fx = FX_RESON;
        par.slot[0].p1 = 12;
        par.slot[0].p2 = 2;
        par.slot[0].in = FI_INS;
        par.ch[0].level = LVL_0DB - 80;
        par.ch[0].mod.lfo[0].rate = 80;
        par.ch[0].mod.s[0].src = MSRC_L1;
        par.ch[0].mod.s[0].dst = MD_FX1;
        par.ch[0].mod.s[0].amt = 100;
        par.ch[0].mod.s[1].src = MSRC_L1;
        par.ch[0].mod.s[1].dst = MD_FX2;
        par.ch[0].mod.s[1].amt = 100;
        for (b = 0; b < 3000; b++) {
            int32_t out[2 * CTL];
            mix_block(out);
            if (eff_slot[0].p1 != 12 || eff_slot[0].p2 != 2) {
                printf("  RESONATOR: its tuning moved by an LFO -- BAD\n");
                bad++;
                break;
            }
        }
    }
    printf("effects: %s\n", bad ? "FAILED" : "all bounded, heard, tuning fixed");
    return bad ? 1 : 0;
}

/* ---------------------------------------------------------------- fits --- *
 * no words over other words: every page's title, every row's name and every value any parameter can take, every
 * patch's name -- each must fit where it is drawn, whole, without being squeezed */
static int cmd_fits(void)
{
    uint32_t pg, j, bad = 0, f;
    char t[24];
#define FIT(cond, what, txt) do { if (!(cond)) { printf("  does not fit: %s '%s'\n", what, txt); bad++; } } while (0)
    fresh_mixer();
    ui_init();
    for (pg = 0; pg <= PG_OFX; pg++)
        for (j = 0; j < 8u; j++) {
            uint32_t id = j < 4u ? PG_ROW[pg][j] : PG_SHROW[pg][j - 4u];
            int32_t v;
            if (id == P_NONE)
                continue;
            ui.page = (uint8_t)pg;
            FIT(text_w(&FONT_XS, p_name(id)) <= 150, "a row's name", p_name(id));
            for (v = p_lo(id); v <= p_hi(id); v++) {
                uint32_t ft, fx, mt;
                for (ft = 0; ft < FT_N; ft++)
                    for (mt = 0; mt < MT_N; mt++)
                        for (fx = 0; fx < FX_N; fx += (id == P_P1 || id == P_P2) ? 1 : FX_N) {
                            par.ch[0].ftype = par.out.ftype = (int16_t)ft;
                            par.ch[0].mtype = (int16_t)mt;
                            par.slot[0].fx = par.slot[SLOT_OUT].fx = (int16_t)fx;
                            ui.sel = 0;
                            p_text(id, v, t);
                            FIT(text_w(&FONT_S, t) <= 228, p_name(id), t);
                            if (!(text_w(&FONT_S, t) <= 228))
                                goto next_id;
                        }
            }
        next_id:;
        }
    for (f = 0; f < NFAM; f++)
        FIT(text_w(&FONT_XS, FAM[f].name) <= 236, "a family's name", FAM[f].name);
    for (f = 0; f < NFACTORY; f++)
        FIT(text_w(&FONT_XS, FACTORY[f].name) <= 236, "a preset's name", FACTORY[f].name);
    for (f = 0; f < MD_N; f++)
        FIT(text_w(&FONT_S, MD_NAME[f]) <= 228, "a destination", MD_NAME[f]);
    printf("fits: %s\n", bad ? "FAILED" : "every word in its place");
    return bad ? 1 : 0;
#undef FIT
}

/* --------------------------------------------------------------- cores --- */
static int cmd_cores(int n)
{
    uint32_t b, i, h = 2166136261u;
    int32_t out[2 * CTL];
    fresh_mixer();
    audio_worker_online = n > 1;
    factory_load(&par, 6);
    for (b = 0; b < 44100u * 4u / CTL; b++) {
        mix_block(out);
        for (i = 0; i < 2 * CTL; i++)
            h = (h ^ (uint32_t)out[i]) * 16777619u;
    }
    printf("cores %d: hash %08x\n", n, h);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc >= 2 && str_eq(argv[1], "check"))
        return cmd_check();
    if (argc >= 3 && str_eq(argv[1], "screens"))
        return cmd_screens(argv[2]);
    if (argc >= 3 && str_eq(argv[1], "one")) {   /* one drawn patch (its seed as check draws it: 0xC0FFEE + i x 7919) */
        stats_t st;
        fresh_mixer();
        printf("family %s\n", FAM[patch_draw(&par, 0xC0FFEEu + (uint32_t)atoi(argv[2]) * 7919u, -1)].name);
        print_patch();
        setenv("DBG", "1", 1);
        st = render(12.0, 0, 0);
        printf("rms %.1f late %.1f clip %.2f%%\n", db(st.rms), db(st.late_rms), st.clip * 100);
        return 0;
    }
    if (argc >= 3 && str_eq(argv[1], "fam")) {   /* a patch of family F (by number), its channels traced */
        uint32_t b, c;
        int32_t out[2 * CTL];
        fresh_mixer();
        patch_draw(&par, 1234u, atoi(argv[2]));
        print_patch();
        for (c = 0; c < NCH; c++)
            if (par.ch[c].level)
                printf("    ch%u mod: lfo0 shape %u rate %u  slot0 %s -> %s %d | lfo1 rate %u slot1 %s %d\n", c + 1,
                       par.ch[c].mod.lfo[0].shape, par.ch[c].mod.lfo[0].rate, MSRC_NAME[par.ch[c].mod.s[0].src],
                       MD_NAME[par.ch[c].mod.s[0].dst], par.ch[c].mod.s[0].amt, par.ch[c].mod.lfo[1].rate,
                       MD_NAME[par.ch[c].mod.s[1].dst], par.ch[c].mod.s[1].amt);
        fm1_ms = 0;
        trim_start();
        for (b = 0; b < 44100u * 8u / CTL; b++) {
            fm1_ms = (uint32_t)((uint64_t)b * CTL * 1000u / FS);
            trim_poll();
            mix_block(out);
            if (b > 44100u * 4u / CTL && b % 100u == 0) {
                printf("  %5.2f s out %6d |", b * CTL / 44100.0, out_meter);
                for (c = 0; c < NCH; c++)
                    if (par.ch[c].level)
                        printf(" ch%u m%6d lv%+5d g%3d", c + 1, ch_meter[c], mst[c].off[MD_LEVEL], par.ch[c].gain);
                printf("\n");
            }
        }
        return 0;
    }
    if (argc >= 2 && str_eq(argv[1], "effects"))
        return cmd_effects();
    if (argc >= 2 && str_eq(argv[1], "fits"))
        return cmd_fits();
    if (argc >= 2 && str_eq(argv[1], "flows"))
        return cmd_flows();
    if (argc >= 3 && str_eq(argv[1], "cores"))
        return cmd_cores(atoi(argv[2]));
    if (argc >= 2 && str_eq(argv[1], "ring"))
        return cmd_ring();
    if (argc >= 2 && str_eq(argv[1], "calib"))
        return cmd_calib();
    if (argc >= 3 && str_eq(argv[1], "render"))
        return cmd_render(argv[2], argc > 3);
    fprintf(stderr, "host render DIR | check | screens DIR | flows | cores N\n");
    return 2;
}
