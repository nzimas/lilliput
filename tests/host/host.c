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
#include "rhythm.c"
#include "drumkit.c"

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
    for (i = 0; i < NFAM + 8u; i++) {           /* (a patch of each recipe, then eight from any) */
        stats_t st;
        const char *name;
        char nm[40];
        fresh_mixer();
        if (i < NFAM) {
            patch_draw(&par, 101u + i * 7919u, (int)i);
            name = FAM[i].name;
        } else {
            uint32_t rc = patch_draw(&par, 9000u + i * 37u, -1);
            snprintf(nm, sizeof nm, "DRAWN %u %s", i - NFAM, FAM[rc].name);
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
/* the DRUMS (build 16): each sound recipe heard alone -- loud enough, not clipped, its transient and its length as
 * its kind is; each rhythm recipe's hits counted against its lanes; started under the RHYTHM, on its beat; the reverb
 * send's tail with SPACE off */
static void drm_quiet_mixer(void)
{
    fresh_mixer();
    patch_plain(&par);                           /* (every channel down: the drums alone) */
    par.out.spres = 0;
    drm_reset = 1;
    clk_run = 0;                                 /* (nothing playing: the beat starts with what starts) */
    memset(dvs, 0, sizeof dvs);
}
static void drm_one_hit(uint32_t c)              /* (one hit of voice c, at the start; the others silent) */
{
    uint32_t v;
    for (v = 0; v < NDV; v++)
        memset(&drm.l[v], 0, sizeof drm.l[v]), drm.l[v].len = 32, drm.l[v].div = DV_2, drm.l[v].prob = 100;
    drm.l[c].hits = 1u;
    drm.swing = 0;
    drm.on = 1;
    par.rhy.bpm = 60;
}
static int16_t drm_buf[2 * 44100 * 3];
static int16_t cmp_buf[2 * 44100 * 8];
static int cmd_drums(void)
{
    uint32_t k, i, c, bad = 0;
    printf("  sound          peak    rise   length   pitch\n");
    for (k = 0; k < NDKIT; k++) {
        double pk = 0, worst_rise = 0, longest = 0, hz = 0;
        uint32_t clips = 0, seed;
        for (seed = 0; seed < 4u; seed++) {
            uint32_t n = 44100u * 3u, ipk = 0, irise = 0, iend = 0, zc = 0;
            double p = 0;
            drm_quiet_mixer();
            drm_init(77u);
            c = DKIT[k].dv;
            drm_draw_voice(c, 1000u + k * 37u + seed * 7919u, (int)k);
            drm_one_hit(c);
            drm_coef_all();
            memset(drm_buf, 0, sizeof drm_buf);
            render(3.0, drm_buf, n);
            for (i = 0; i < n; i++)
                if (abs(drm_buf[2 * i]) > p) {
                    p = abs(drm_buf[2 * i]);
                    ipk = i;
                }
            for (i = 0; i < n; i++)
                if (abs(drm_buf[2 * i]) >= p / 2) {
                    irise = i;
                    break;
                }
            for (i = 0; i < n; i++) {
                if (abs(drm_buf[2 * i]) > p / 100)
                    iend = i;
                if (abs(drm_buf[2 * i]) >= 32767)
                    clips++;
            }
            for (i = ipk + 441u; i < ipk + 3528u && i + 1u < n; i++)   /* (10 .. 80 ms after the peak) */
                if ((drm_buf[2 * i] >= 0) != (drm_buf[2 * i + 2] >= 0))
                    zc++;
            (void)ipk;
            if (p > pk)
                pk = p;
            /* (the hit lands within the first block or two: the rise from the start) */
            if (irise / 44.1 > worst_rise)
                worst_rise = irise / 44.1;
            if (iend / 44.1 > longest)
                longest = iend / 44.1;
            hz += zc / 2.0 / 0.07 / 4.0;
        }
        printf("  %-5s %-7s %5.1f dB %5.1f ms %6.0f ms %6.0f Hz\n", DV_NAME[DKIT[k].dv], DKIT[k].name, db(pk / 32768.0),
               worst_rise, longest, hz);
        if (db(pk / 32768.0) < -24 || clips > 4410u || longest > 2900 || longest < 20) {
            printf("FAIL drums: %s %s out of bounds (clipped samples %u)\n", DV_NAME[DKIT[k].dv], DKIT[k].name, clips);
            bad++;
        }
        if (DKIT[k].dv == DV_KICK && (str_eq(DKIT[k].name, "SOFT") ? worst_rise < 3.0 : worst_rise > 3.0)) {
            printf("FAIL drums: %s %s: rise %.1f ms (hard kicks under 3 ms, SOFT over)\n", DV_NAME[DKIT[k].dv], DKIT[k].name, worst_rise);
            bad++;
        }
    }
    /* each rhythm recipe: its hits over four rounds of its longest lane, against its lanes (chance 100 % ones) */
    for (k = 0; k < DS_N; k++) {
        uint32_t got[NDV] = {0}, want[NDV] = {0}, b, blocks, maxlen = 1;
        int32_t out[2 * CTL];
        drm_quiet_mixer();
        drm_init(5u);
        drm_draw_rhythm(900u + k, (int)k);
        drm.on = 1;
        drm.vary = 0;                            /* (VARIATION off: the hits against the lanes) */
        drm.swing = 0;
        par.rhy.bpm = 120;
        for (c = 0; c < NDV; c++)
            if (drm.l[c].len > maxlen)
                maxlen = drm.l[c].len;
        blocks = (uint32_t)((uint64_t)maxlen * 4u * FS / CTL / 8u);   /* (a sixteenth at 120: 1/8 s) */
        for (b = 0; b < blocks; b++) {
            mix_block(out);
            for (c = 0; c < NDV; c++)
                got[c] += dtrig[c].fire;
        }
        for (c = 0; c < NDV; c++) {
            const dlane_t *L = &drm.l[c];
            double steps = (double)blocks * CTL / FS * 120.0 / 60.0 * DIV2[L->div] / 2.0;
            want[c] = (uint32_t)(steps / L->len) * (uint32_t)__builtin_popcount(L->hits & ((L->len >= 32u ? 0u : 1u << L->len) - 1u));
            if (L->prob >= 100u && (got[c] + 1u < want[c] || got[c] > want[c] + __builtin_popcount(L->hits))) {
                printf("FAIL drums: %s %s: %u hits, %u expected\n", DS_NAME[k], DV_NAME[c], got[c], want[c]);
                bad++;
            }
        }
        printf("  %-10s kick %3u snare %3u hats %3u\n", DS_NAME[k], got[0], got[1], got[2]);
    }
    {   /* (build 22) no pitch moves: no style, no seed gives a step a pitch of its own */
        uint32_t sd, pitched = 0, i3;
        for (k = 0; k < DS_N; k++)
            for (sd = 0; sd < 40u; sd++) {
                drm_draw_rhythm(sd * 7919u + k, (int)k);
                for (c = 0; c < NDV; c++)
                    for (i3 = 0; i3 < DSTEPS; i3++)
                        pitched += drm.l[c].pitch[i3] != 0;
            }
        if (pitched) {
            printf("FAIL drums: %u steps drawn with a pitch of their own\n", pitched);
            bad++;
        }
    }
    /* VARIATION (build 21): at 100 %, rounds played otherwise now and then and home again -- home never lost, the
     * kick never resting a round; at 0, every round as written */
    {
        uint32_t vv;
        for (vv = 0; vv < 2u; vv++) {
            uint32_t b, rounds = 0, varied = 0, home = 0, kick_rest = 0, h0;
            uint8_t last = 0xFF;
            int32_t out[2 * CTL];
            drm_quiet_mixer();
            drm_init(5u);
            drm_draw_rhythm(31u, DS_BROKEN);
            drm.vary = (int16_t)(vv ? 100 : 0);
            for (c = 0; c < NDV; c++)
                drm.l[c].var = 100;
            h0 = drm.l[DV_HAT].hits;
            drm.on = 1;
            par.rhy.bpm = 160;
            for (b = 0; b < 60000u; b++) {
                mix_block(out);
                if (dlr[DV_HAT].step == 1u && last == 0u) {   /* (a round begun: what it plays) */
                    rounds++;
                    varied += drm.l[DV_HAT].hits != h0 || drm.l[DV_HAT].acc != dvr[DV_HAT].acc;
                    home += drm.l[DV_HAT].hits == h0;
                    kick_rest += !drm.l[DV_KICK].hits;
                }
                last = dlr[DV_HAT].step;
            }
            printf("  VARIATION %3u %%: %u rounds, %u otherwise, %u as written, home %s, kick resting %u\n", vv ? 100u : 0u,
                   rounds, varied, home, dvr[DV_HAT].hits == h0 ? "kept" : "LOST", kick_rest);
            if (vv ? (varied < rounds / 2u || home < rounds / 10u || dvr[DV_HAT].hits != h0 || kick_rest)
                   : (varied != 0u || rounds < 10u)) {
                printf("FAIL drums: VARIATION %u %%\n", vv ? 100u : 0u);
                bad++;
            }
        }
    }
    /* under the RHYTHM: switched on mid-beat, the drums' first hit on its next beat */
    {
        uint32_t b, beat0, first = 0, beat_at = 0;
        int32_t out[2 * CTL];
        drm_quiet_mixer();
        drm_init(9u);
        rhythm_draw(&par, 4242u, 0);
        par.rhy.on = 1;
        par.rhy.bpm = 120;
        rhy_reset = 1;
        drm_draw_rhythm(1u, DS_DBREAK);
        drm.vary = 0;                        /* (build 21: VARIATION off -- hits counted against the lanes) */
        drm.on = 0;
        for (b = 0; b < 300u; b++)
            mix_block(out);
        beat0 = rhy_beat;
        drm.on = 1;
        drm_reset = 1;
        for (b = 0; b < 3000u && !first; b++) {
            mix_block(out);
            if (!beat_at && rhy_beat != beat0)
                beat_at = b + 1u;
            if (dtrig[DV_KICK].fire)
                first = b + 1u;
        }
        printf("  under the RHYTHM: its next beat at block %u, the drums' first kick at %u\n", beat_at, first);
        if (!first || first != beat_at) {
            printf("FAIL drums: not started on the RHYTHM's beat\n");
            bad++;
        }
    }
    /* the reverb send: its tail with SPACE off, after the dry hit has gone */
    {
        uint32_t n = 44100u * 3u;
        double tail = 0, dry = 0;
        drm_quiet_mixer();
        drm_init(3u);
        drm_draw_voice(DV_SNARE, 5u, -1);
        drm.v[DV_SNARE].decay = 250;
        drm.v[DV_SNARE].verb = 900;
        drm_one_hit(DV_SNARE);
        drm_coef_all();
        memset(drm_buf, 0, sizeof drm_buf);
        render(3.0, drm_buf, n);
        for (i = 0; i < 4410u; i++)
            dry += abs(drm_buf[2 * i]);
        for (i = 44100u; i < 66150u; i++)        /* (1 .. 1.5 s on) */
            tail += abs(drm_buf[2 * i]);
        printf("  reverb send, SPACE off: the tail at 1 s %.1f dB under the hit\n", db((tail / 22050) / (dry / 4410) + 1e-9));
        if (tail / 22050 < dry / 4410 * 0.003) {
            printf("FAIL drums: no reverb tail from the send\n");
            bad++;
        }
    }
    /* a new RHYTHM for the mixer, drawn mid-beat while the drums play: the drums never restarted (build 17) */
    {
        uint32_t b, nk = 0, kb[16], i2, uneven = 0;
        int32_t out[2 * CTL];
        drm_quiet_mixer();
        drm_init(9u);
        drm_draw_rhythm(1u, DS_DBREAK);
        drm.vary = 0;                        /* (build 21: VARIATION off -- hits counted against the lanes) */
        dl_set(DV_KICK, "X...X...X...X...", 100);
        drm.on = 1;
        par.rhy.bpm = 120;
        rhythm_draw(&par, 4242u, 0);
        par.rhy.on = 1;
        rhy_reset = 1;
        for (b = 0; b < 4000u; b++) {
            if (b == 1000u) {                    /* (mid-beat: as SHIFT + the RHYTHM's key does) */
                rhythm_draw(&par, 77u, 3);
                rhy_reset = 1;
            }
            mix_block(out);
            if (dtrig[DV_KICK].fire && nk < 16u)
                kb[nk++] = b;
        }
        for (i2 = 1; i2 < nk; i2++)
            if (kb[i2] - kb[i2 - 1u] < 688u || kb[i2] - kb[i2 - 1u] > 690u)
                uneven++;
        printf("  a new RHYTHM mid-beat: %u kicks, %u intervals uneven\n", nk, uneven);
        if (nk < 5u || uneven) {
            printf("FAIL drums: a new RHYTHM disturbed the drums\n");
            bad++;
        }
    }
    /* BREAK (build 19): each way, held from mid-beat for 4 bars at 120 BPM over a written rhythm: as written until
     * its quantum (the next beat, or half bar), something else after it, as written again from the step after it is
     * let go; never a voice firing more than 4 times a step */
    {
        enum { NB2 = 12000 };
        static uint8_t base[NB2], brk[NB2];
        uint32_t w, b, pass;
        int32_t out[2 * CTL];
        for (w = 0; w < BK_N; w++) {
            uint32_t press = 1000u, rel = 1000u + 5512u, q, firstdiff = 0, diffs = 0, after = 0, dense = 0, i3;
            for (pass = 0; pass < 2u; pass++) {
                uint8_t *rec = pass ? brk : base;
                drm_quiet_mixer();
                drm_init(9u);
                drm_draw_rhythm(1u, DS_DBREAK);
        drm.vary = 0;                        /* (build 21: VARIATION off -- hits counted against the lanes) */
                dl_set(DV_KICK, "X...X...X..XX...", 100);
                dl_set(DV_SNARE, "....X.......X.X.", 100);
                dl_set(DV_HAT, "..X...X...X...XX", 100);
                dl_set(DV_PERC, "...X.....X...X..", 100);
                drm.l[DV_SNARE].acc = 1u << 12;
                drm.on = 1;
                drm.swing = 0;
                par.rhy.bpm = 120;
                brk_hold = 0;
                brk_plan.way = (uint8_t)w;
                brk_plan.cell = 5;
                brk_plan.q = (uint8_t)(w == BK_HALF || w == BK_MIRROR ? 8u : 4u);
                for (i3 = 0; i3 < 8u; i3++)
                    brk_plan.ord[i3] = (uint8_t)((i3 * 5u) % 8u);
                for (b = 0; b < NB2; b++) {
                    if (pass && b == press) {
                        brk_fill();
                        brk_hold = 1;
                    }
                    if (pass && b == rel)
                        brk_hold = 0;
                    mix_block(out);
                    rec[b] = 0;
                    for (i3 = 0; i3 < NDV; i3++)
                        rec[b] |= (uint8_t)(dtrig[i3].fire << i3);
                }
            }
            q = brk_plan.q == 8u ? 1378u : 689u;    /* (blocks: a beat, half a bar at 120) */
            q = (press / q + 1u) * q;
            for (b = 0; b < NB2; b++)
                if (base[b] != brk[b]) {
                    if (!firstdiff)
                        firstdiff = b;
                    diffs++;
                    if (b > rel + 400u)
                        after++;
                }
            for (b = press; b + 172u < rel; b += 172u) {   /* (a step: 172 blocks) */
                uint32_t n4[NDV] = {0}, v;
                for (i3 = 0; i3 < 172u; i3++)
                    for (v = 0; v < NDV; v++)
                        n4[v] += brk[b + i3] >> v & 1u;
                for (v = 0; v < NDV; v++)
                    dense |= n4[v] > 4u;
            }
            printf("  BREAK %-10s from block %u (quantum %u): first change %u, %u blocks differ, %u after letting go\n",
                   BK_NAME[w], press, q, firstdiff, diffs, after);
            if (!diffs || firstdiff < q - 1u || firstdiff > q + 1400u || after || dense) {
                printf("FAIL drums: BREAK %s: %s\n", BK_NAME[w], !diffs ? "nothing changed" : firstdiff < q - 1u ?
                       "before its quantum" : firstdiff > q + 1400u ? "late" : after ? "not as written after" : "too dense");
                bad++;
            }
        }
        brk_hold = 0;
    }
    /* the COMPRESSOR: AMOUNT brings the mix forward and ducks it under the kick, more with more */
    {
        static const uint8_t AM[3] = {0, 35, 90};
        double rms[3], duck[3], fore[3];
        uint32_t a2;
        for (a2 = 0; a2 < 3u; a2++) {
            uint32_t n = 44100u * 8u, kk;
            double s2 = 0, da = 0, db2 = 0;
            fresh_mixer();
            patch_draw(&par, 0xC0FFEEu + 5u * 7919u, 0);   /* (build 20: the generator draws more; a patch sitting as build 19's did) */
            par.rhy.on = 0;
            drm_init(11u);
            for (kk = 0; kk < NDV; kk++)
                dl_set(kk, "................", 100);
            dl_set(DV_KICK, "X...X...X...X...", 100);
            drm.v[DV_KICK].level = 0;            /* (the kick unheard: it only keys the ducking) */
            drm.v[DV_KICK].verb = 0;
            drm_coef_all();
            drm.on = 1;
            drm_reset = 1;
            clk_run = 0;
            par.rhy.bpm = 120;
            cmp.amount = AM[a2];
            cmp.sc = 1;
            cmp.thresh = 625;
            cmp.ratio = 2;
            cmp.attack = 300;
            cmp.release = 450;
            cmp_coef();
            cmpr.g = 1 << 16;
            cmpr.env = 0;
            cmpr.duck = 0;
            render(8.0, cmp_buf, n);
            for (i = 44100u * 4u; i < n; i++)
                s2 += (double)cmp_buf[2 * i] * cmp_buf[2 * i];
            for (kk = 8; kk < 16u; kk++) {       /* (a kick every 0.5 s from the start: just after it, against just
                                                  * before the next) */
                uint32_t t0 = kk * 22050u, j;
                for (j = t0 + 220u; j < t0 + 2646u; j++)
                    da += abs(cmp_buf[2 * j]);
                for (j = t0 + 17640u; j < t0 + 20066u; j++)
                    db2 += abs(cmp_buf[2 * j]);
            }
            rms[a2] = db(sqrt(s2 / (n - 44100u * 4u)) / 32768.0 + 1e-9);
            duck[a2] = db((db2 + 1) / (da + 1));
            fore[a2] = db(db2 / (8.0 * 2426.0) / 32768.0 + 1e-9);   /* (the mix between the kicks: brought forward) */
            printf("  COMPRESSOR, AMOUNT %3u %%: the mix between kicks %6.1f dB (overall %6.1f), ducked under the kick %5.1f dB\n",
                   AM[a2], fore[a2], rms[a2], duck[a2]);
        }
        cmp.amount = 0;
        cmp_coef();
        if (fabs(duck[0]) > 2.0 || duck[2] < 6.0 || duck[1] >= duck[2] || fore[1] < fore[0] + 3.0 || fore[2] < fore[1] - 0.5) {   /* (forward, then held at the output's headroom;
                                                                       * the pumping deeper still) */
            printf("FAIL drums: the COMPRESSOR does not bring the mix forward and pump as AMOUNT grows\n");
            bad++;
        }
    }
    /* a build 16 session's drums (three voices) read as ours */
    {
        static drm_par_t d;
        drm16_t o;
        memset(&o, 0, sizeof o);
        o.v[1].tune = 412;
        o.l[2].hits = 0x5555u;
        o.l[2].len = 16;
        o.kit[0] = 3;
        o.on = 1;
        o.swing = 22;
        o.level = 707;
        memset(&d, 0x5A, sizeof d);
        memcpy(&d, &o, sizeof o);
        drm_from16(&d);
        if (d.v[1].tune != 412 || d.l[2].hits != 0x5555u || d.kit[0] != 3 || !d.on || d.swing != 22 || d.mute ||
            d.l[DV_PERC].hits || d.l[DV_PERC].len != 16 || DKIT[d.kit[DV_PERC]].dv != DV_PERC) {
            printf("FAIL drums: a build 16 session's drums not read right\n");
            bad++;
        } else
            printf("  a build 16 session's drums: read, PERC resting\n");
    }
    printf("%sdrums: %u sounds, %u rhythms\n", bad ? "FAIL " : "", (unsigned)NDKIT, (unsigned)DS_N);
    return bad ? 1 : 0;
}
/* the website's demos (tools/make_demos.sh): three pieces rendered from the machine, 48 s each */
static int fam_of(const char *name)
{
    uint32_t i;
    for (i = 0; i < NFAM; i++)
        if (str_eq(FAM[i].name, name))
            return (int)i;
    return -1;
}
static int cmd_demo(const char *dir)
{
    static int16_t buf[2 * FS * 48];
    static const struct { const char *file, *fam; uint32_t seed; int drums, dstyle, rstyle; uint32_t bpm, comp; } D[3] = {
        {"beating", "BEATING", 31u, 0, 0, -1, 120, 0},
        {"under-the-kick", "ENGINE", 7u, 1, DS_AKSAK, -1, 136, 85},
        {"ritual", "CAVERN", 12u, 1, DS_TRIBAL, RS_TRIBAL, 118, 45}};
    uint32_t i;
    char path[512];
    for (i = 0; i < 3u; i++) {
        stats_t st;
        fresh_mixer();
        patch_draw(&par, D[i].seed * 7919u + 11u, fam_of(D[i].fam));
        par.rhy.bpm = (uint16_t)D[i].bpm;
        if (D[i].rstyle >= 0) {
            rhythm_draw(&par, D[i].seed + 4242u, D[i].rstyle);
            par.rhy.on = 1;
            rhy_reset = 1;
        }
        drm_init(D[i].seed + 99u);
        drm.on = 0;
        if (D[i].drums) {
            drm_draw_rhythm(D[i].seed + 5u, D[i].dstyle);
            drm.on = 1;
        }
        drm_coef_all();
        drm_reset = 1;
        clk_run = 0;
        memset(dvs, 0, sizeof dvs);
        cmp.amount = (uint8_t)D[i].comp;
        cmp.sc = 1;
        cmp.thresh = 600;
        cmp.ratio = 2;
        cmp.attack = 300;
        cmp.release = 420;
        cmp_coef();
        cmpr.g = 1 << 16;
        cmpr.env = cmpr.duck = 0;
        st = render(48.0, buf, FS * 48u);
        snprintf(path, sizeof path, "%s/demo-%s.wav", dir, D[i].file);
        wav_write(path, buf, FS * 48u);
        printf("demo %-16s %-8s rms %6.1f dB  peak %6.1f dB  movement %4.1f dB\n", D[i].file, D[i].fam, db(st.rms),
               db(st.peak), st.move);
    }
    cmp.amount = 0;
    cmp_coef();
    drm.on = 0;
    return 0;
}
/* FREE: anything goes, but never silence. 200 drawn, each heard 8 s: none quiet, none clipped */
static int cmd_free(void)
{
    uint32_t i, quiet = 0, bad = 0, ffree = NFAM - 1u;
    double lo = 0, hi = -200, mv = 0;
    if (!str_eq(FAM[ffree].name, "FREE")) {
        printf("FAIL free: FREE is not the last recipe\n");
        return 1;
    }
    for (i = 0; i < 200u; i++) {
        stats_t st;
        fresh_mixer();
        patch_draw(&par, 0xF4EEu + i * 104729u, (int)ffree);
        st = render(8.0, 0, 0);
        mv += st.move;
        if (db(st.late_rms) < lo || i == 0)
            lo = db(st.late_rms);
        if (db(st.late_rms) > hi)
            hi = db(st.late_rms);
        if (db(st.late_rms) < -45) {
            quiet++;
            printf("  quiet: FREE %u %.1f dB\n", i, db(st.late_rms));
        }
        if (st.clip > 0.02 || fabs(st.dc) > 0.02) {
            bad++;
            printf("  bad: FREE %u clip %.2f%% dc %+.3f\n", i, st.clip * 100, st.dc);
        }
    }
    printf("%sfree: 200 FREE patches, loudness %.1f .. %.1f dB, movement %.1f dB, %u quiet, %u bad\n",
           quiet || bad ? "FAIL " : "", lo, hi, mv / 200, quiet, bad);
    return quiet || bad ? 1 : 0;
}
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
    press_btn(B_SEQ);
    run_ms(60);
    SHOT("07e_compressor");
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
    run_ms(300);
    SHOT("09c_lfo1");
    press_btn(B_SEQ);
    press_btn(B_SEQ);
    run_ms(300);
    SHOT("09d_mod_1_4");
    press_btn(B_SEQ);
    run_ms(300);
    SHOT("09e_mod_5_8");
    press_btn(B_SEQ);
    run_ms(60);
    press_btn(B_ARP);
    run_ms(300);
    SHOT("10_overview");
    press_btn(B_ARP);
    tap_key(KEY_DRM);
    run_ms(100);
    SHOT("10b_drums_kick");
    key(KEY_SHIFT, 1);
    run_ms(60);
    SHOT("10c_drums_kick_shift");
    key(KEY_SHIFT, 0);
    press_btn(B_OCTUP);
    run_ms(60);
    SHOT("10d_drums_kick_2");
    press_btn(B_OCTUP);
    key(KEY_SHIFT, 1);
    tap_key(KEY_DRM);
    key(KEY_SHIFT, 0);
    run_ms(1300);
    SHOT("10e_drums_rhythm");
    goto_page(PG_DRUM);
    tap_key(white_key(3));
    key(white_key(3), 1);
    run_ms(800);
    key(white_key(3), 0);
    run_ms(60);
    SHOT("10f_drums_perc_muted");
    key(white_key(3), 1);
    run_ms(800);
    key(white_key(3), 0);
    ui.brk_last = BK_AVALANCHE;                  /* (BREAK held: its way on the title -- any but AVALANCHE) */
    key(white_key(15), 1);
    run_ms(400);
    SHOT("10g_drums_break");
    key(white_key(15), 0);
    goto_page(PG_DRUM3);
    key(KEY_DRM, 1);
    run_ms(900);
    key(KEY_DRM, 0);
    tap_key(KEY_DRM);
    run_ms(60);
    press_btn(LAYER_BTN[2]);
    tap_key(KEY_MUTE[2]);
    run_ms(100);
    SHOT("11_muted");
    tap_key(KEY_MUTE[2]);
    turn(EN_PRESET, 1);
    run_ms(60);
    SHOT("12_recipe");
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
    press_btn(B_PLAY);
    key(KEY_RHY, 1);
    run_ms(900);
    key(KEY_RHY, 0);
    tap_key(KEY_RHY);
    run_ms(100);
    SHOT("18_tempo");
    press_btn(B_OCTUP);
    run_ms(100);
    SHOT("19_rhythm_mix");
    tap_key(KEY_RHY);
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
    EXPECT(ui.fam < NFAM && str_eq(ui.patch, FAM[ui.fam].name) && ui.recipe == ui.fam,
           "the start: a patch drawn from any recipe, named after it");
    EXPECT(par.rhy.bpm > 90 && par.rhy.bpm < 300, "the start's tempo: drawn, 91 .. 299 BPM");
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
    /* as on Tonitor: HOME a new patch from any recipe; SHIFT + HOME from the recipe named in the main view; PRESETS
     * names another, in place (nothing else changes); HOME held: nudged */
    press_btn(LAYER_BTN[0]);
    keep_src = par.ch[0].src;
    {
        uint32_t i, seen = 0, pg = ui.page, f0, fp;
        for (i = 0; i < 12u; i++) {
            press_btn(B_HOME);
            seen |= 1u << ui.fam;
        }
        EXPECT(__builtin_popcount(seen) > 3 && ui.recipe == ui.fam && str_eq(ui.patch, FAM[ui.fam].name),
               "HOME: a new patch from any recipe, each time; named after its recipe");
        run_ms(4000);                            /* (NEW PATCH said and gone; the trim done: it may raise a quiet
                                                  * channel and set the main level for 3.6 s) */
        f0 = ui.fam;
        before = par;
        turn(EN_PRESET, 1);
        run_ms(60);
        fp = (f0 + 1u) % NFAM;
        EXPECT(ui.recipe == fp && !memcmp(&before, &par, sizeof par) && ui.page == PG_MAIN,
               "PRESETS: the next recipe named, the patch left as it was, the main view");
        EXPECT(!ui.flash_on && str_eq(ui.shown_text, FAM[fp].name), "PRESETS: its name in the main view, in place, no flash");
        turn(EN_PRESET, -1);
        run_ms(60);
        EXPECT(ui.recipe == f0 && str_eq(ui.shown_text, ui.patch), "PRESETS back to the patch's own: its name again");
        turn(EN_PRESET, -1);
        run_ms(60);
        fp = (f0 + NFAM - 1u) % NFAM;
        EXPECT(ui.recipe == fp && str_eq(ui.shown_text, FAM[fp].name), "PRESETS back: the one before, named");
        for (i = 0; i < 4u; i++) {
            before = par;
            key(KEY_SHIFT, 1);
            press_btn(B_HOME);
            key(KEY_SHIFT, 0);
            EXPECT(ui.fam == fp && ui.recipe == fp && memcmp(&before, &par, sizeof par) != 0,
                   "SHIFT + HOME: a new patch from the recipe named, each time");
        }
        EXPECT(str_eq(ui.patch, FAM[fp].name), "the patch named after its recipe");
        for (i = 0, seen = 0; i < 12u; i++) {
            press_btn(B_HOME);
            seen |= 1u << ui.fam;
        }
        EXPECT(__builtin_popcount(seen) > 3, "HOME after: any recipe again");
        goto_page(pg);                           /* (the page the tests below expect) */
    }
    run_ms(3600);
    EXPECT(out_meter > 300, "the new patch sounds");
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
    EXPECT(ui.page == PG_FX, "OCT+ stops at FX: the channel's pages");
    press_btn(B_SEQ);
    EXPECT(ui.page == PG_COMP, "SEQ: the COMPRESSOR first");
    press_btn(B_SEQ);
    EXPECT(ui.page == PG_OUT, "then OUT");
    press_btn(B_SEQ);
    press_btn(B_SEQ);
    EXPECT(ui.page == PG_OFX, "OUT EQ, OUT FX");
    press_btn(B_SEQ);
    EXPECT(ui.page == PG_LFO1 && ui.sel == 2, "SEQ on: the chosen channel's LFO 1");
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
        EXPECT(ui.page == PG_MTX2, "(SHIFT + SEQ: the page kept)");
        press_btn(B_OCTDN);
        EXPECT(ui.page == PG_MTX1, "OCT- there: back through SEQ's pages");
        press_btn(LAYER_BTN[3]);
        EXPECT(ui.page == PG_MTX1 && ui.sel == 3, "another channel's button: its MOD 1-4");
        press_btn(LAYER_BTN[2]);
        press_btn(B_SEQ);
        press_btn(B_SEQ);
        EXPECT(ui.page == PG_FX, "SEQ after MOD 5-8: back to the channel's page");
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
    EXPECT(!gl[0].play, "PLAY with no gesture loop: nothing");
    /* the gesture loops, one a channel: REC arms the chosen channel's, its first change starts it, REC ends and
     * plays; each round as recorded; another channel's changes are not in it */
    press_btn(LAYER_BTN[0]);
    press_btn(LAYER_BTN[0]);
    run_ms(200);
    {
        int16_t l0 = par.ch[0].level, l1;
        uint8_t m3 = par.ch[3].mute, m0;
        int dir = l0 > 10 ? -1 : 1;              /* (the way the level can move) */
        press_btn(B_REC);
        EXPECT(gl_state(0) == GS_ARMED, "REC arms the channel's loop");
        run_ms(300);
        par.ch[3].level = (int16_t)(par.ch[3].level ^ 1);   /* (another channel's change: not its) */
        run_ms(100);
        EXPECT(gl_state(0) == GS_ARMED, "nothing recorded until the channel changes");
        turn(EN_SELECT, 3 * dir);
        EXPECT(gl_state(0) == GS_REC, "the channel's first change starts the recording");
        run_ms(400);
        tap_key(KEY_MUTE[0]);
        tap_key(KEY_MUTE[3]);                    /* (channel 4's mute: not in channel 1's loop) */
        m0 = par.ch[0].mute;
        run_ms(400);
        turn(EN_SELECT, 2 * dir);
        l1 = par.ch[0].level;
        run_ms(300);
        press_btn(B_REC);
        EXPECT(gl_state(0) == GS_IDLE && gl[0].play && gl[0].n >= 3, "REC again: the loop ends and plays");
        {
            uint32_t i, li, mine = 1;
            for (i = 0; i < gl[0].n; i++)
                mine &= g_owner(gev[gl[0].off + i].w, &li) == 0;
            EXPECT(mine, "only the channel's own changes in its loop");
        }
        tap_key(KEY_MUTE[3]);                    /* (channel 4 as it was) */
        EXPECT(par.ch[3].mute == m3, "channel 4 as it was");
        run_ms(100);
        EXPECT(par.ch[0].level != l1, "a round begins as the recording began");
        run_ms(gl[0].len);                       /* (into the second round, then to its end) */
        while ((fm1_ms - gl[0].t0) % gl[0].len < gl[0].len - 80u)
            run_ms(10);
        EXPECT(par.ch[0].level == l1 && par.ch[0].mute == m0 && par.ch[3].mute == m3, "and plays as recorded");
        press_btn(B_PLAY);
        EXPECT(!gl[0].play, "PLAY stops it");
        press_btn(B_PLAY);
        EXPECT(gl[0].play, "PLAY plays it again");
        {   /* (build 24) the loops are the hands', not the patch's: a new patch, and the loop plays on over it */
            uint16_t n0 = gl[0].n;
            uint32_t ui_page = ui.page;
            press_btn(B_HOME);
            run_ms(gl[0].len + 200u);
            EXPECT(gl[0].play && gl[0].n == n0, "HOME: the loop plays on, over the new patch, as it was");
            while ((fm1_ms - gl[0].t0) % gl[0].len < gl[0].len - 80u)
                run_ms(10);
            EXPECT(par.ch[0].level == l1 && par.ch[0].mute == m0, "and plays its moves there");
            (void)ui_page;
        }
        (void)l0;
        /* channel 2's own loop, beside channel 1's */
        press_btn(LAYER_BTN[1]);
        if (ui.page != PG_MAIN)
            press_btn(LAYER_BTN[1]);
        EXPECT(ui.sel == 1 && !gl[1].play, "channel 2: no loop of its own yet");
        press_btn(B_REC);
        EXPECT(gl_state(1) == GS_ARMED && gl[0].play, "REC on channel 2 arms its loop; channel 1's plays on");
        turn(EN_ALGO, 5);
        run_ms(300);
        turn(EN_ALGO, -9);
        run_ms(300);
        press_btn(B_REC);
        EXPECT(gl[1].play && gl[0].play && gl[1].n >= 2 && gl[1].len != gl[0].len,
               "REC again: channel 2's loop plays too, its own length");
        press_btn(B_PLAY);
        EXPECT(!gl[1].play && gl[0].play, "PLAY on channel 2 stops its loop only");
        press_btn(B_REC);                        /* (REC on channel 2 again, then on channel 1: 2's ends, 1 armed) */
        turn(EN_ALGO, 3);
        run_ms(200);
        press_btn(LAYER_BTN[0]);
        press_btn(B_REC);
        EXPECT(gl_state(1) == GS_IDLE && gl[1].play && gl_state(0) == GS_ARMED,
               "REC on another channel: the recording ends (it plays), that channel armed");
        press_btn(B_REC);
        EXPECT(gl_state(0) == GS_IDLE && gl[0].play && gl[0].n >= 3, "armed and let go: channel 1's loop as it was");
        press_btn(LAYER_BTN[1]);
        hold_btn(B_REC, 1100);
        EXPECT(!gl[1].n && !gl[1].play && gl[0].n >= 3 && gl[0].play, "REC held: channel 2's loop emptied, 1's kept");
        press_btn(LAYER_BTN[0]);
        if (ui.page != PG_MAIN)
            press_btn(LAYER_BTN[0]);
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
        par.ch[1].level = 500;                   /* (above the 100 set below, whatever the patch drew) */
        run_ms(30);
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
    /* the RHYTHM (build 19, as the DRUMS' key): held, on and off; SHIFT + it a new one; short, its pages */
    press_btn(LAYER_BTN[0]);
    if (ui.page != PG_MAIN)
        press_btn(LAYER_BTN[0]);
    key(KEY_RHY, 1);
    run_ms(900);
    key(KEY_RHY, 0);
    EXPECT(par.rhy.on && ui.page == PG_MAIN, "the RHYTHM's key held: on (a rhythm drawn), the page kept");
    {
        uint8_t st = par.rhy.style;
        key(KEY_SHIFT, 1);
        tap_key(KEY_RHY);
        key(KEY_SHIFT, 0);
        EXPECT(par.rhy.on && par.rhy.style != st, "SHIFT + it: a new rhythm, of another style");
    }
    key(KEY_RHY, 1);
    run_ms(900);
    key(KEY_RHY, 0);
    EXPECT(!par.rhy.on, "held again: off");
    tap_key(KEY_RHY);
    EXPECT(ui.page == PG_TEMPO && !par.rhy.on, "short: the TEMPO page (the rhythm left as it was)");
    {
        uint16_t b0 = par.rhy.bpm;
        turn(EN_K1, 1);
        EXPECT(par.rhy.bpm == b0 + 1, "KNOB 1 there: a BPM a detent");
        par.rhy.bpm = 12;
        turn(EN_K1, -5);
        EXPECT(par.rhy.bpm == 10, "10 BPM at the least");
        par.rhy.bpm = 498;
        turn(EN_K1, 5);
        EXPECT(par.rhy.bpm == 500, "500 BPM at the most");
        par.rhy.bpm = 120;
        turn(EN_PRESET, 1);
        EXPECT(par.rhy.on, "PRESETS there: a rhythm of the next style, playing");
        press_btn(B_OCTUP);
        EXPECT(ui.page == PG_RHY2, "OCT+ there: the RHYTHM's second page");
        EXPECT(p_get(P_RMIX) == 100, "its MIX: all gate, as before");
        turn(EN_K1, -40);
        EXPECT(p_get(P_RMIX) == 60 && par.rhy.dry == 40, "KNOB 1 there: the MIX");
        turn(EN_K1, 40);
        press_btn(B_OCTDN);
        EXPECT(ui.page == PG_TEMPO, "OCT- : back to TEMPO");
    }
    tap_key(KEY_RHY);
    EXPECT(ui.page != PG_TEMPO && !rhy_page(), "short again: back");
    {
        rhy_par_t keep = par.rhy;
        press_btn(B_HOME);
        EXPECT(!memcmp(&keep, &par.rhy, sizeof keep), "a new patch: the rhythm plays on");
    }
    /* sessions: save as a new one (asked), load it back */
    hold_btn(B_SAVE, 900);
    if (ui.page != PG_ASK)                       /* (SAVE held: over the session loaded, if there is one) */
        printf("  (note: saved over a session)\n");
    EXPECT(ui.page == PG_ASK && ui.ses_new == 1, "SAVE held: a new session, asked");
    trim.on = 0;                                 /* (the patch settled: nothing moves it between the save and now) */
    press_btn(B_SAVE);
    before = par;
    run_ms(1600);
    EXPECT(ses_has(1) && ses_current() == 1, "saved as session 1");
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
        if (memcmp(&a, &b, sizeof a)) {
            uint32_t i;
            for (i = 0; i < sizeof a; i++)
                if (((uint8_t *)&a)[i] != ((uint8_t *)&b)[i])
                    printf("  differs at byte %u (ch size %u, rhy at %u): %u vs %u\n", i, (unsigned)sizeof(ch_par_t),
                           (unsigned)__builtin_offsetof(lil_par_t, rhy), ((uint8_t *)&a)[i], ((uint8_t *)&b)[i]);
        }
        EXPECT(memcmp(&a, &b, sizeof a) == 0, "session 1 loaded back as it was");
        EXPECT(snap_has(2) && gl[0].n >= 3 && !gl[1].n, "with its snapshots and its gesture loops");
    }
    /* again, from the flash as a boot finds it */
    ses_scan(1);
    EXPECT(ses_has(1), "the session found after a restart");
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
    /* the DRUMS: their key short opens their pages (again: back); white keys 1 .. 3 the voices, SHIFT + one a new
     * sound; the knobs their values; OCT through the pages; PRESETS their rhythms; held, on and off; SHIFT + it, a new
     * rhythm; kept with a session */
    press_btn(LAYER_BTN[0]);
    if (ui.page != PG_MAIN)
        press_btn(LAYER_BTN[0]);
    {
        uint16_t src0 = (uint16_t)par.ch[0].src;
        uint32_t st0, inc0;
        drm_par_t keep;
        tap_key(KEY_DRM);
        EXPECT(ui.page == PG_DRUM && !drm.on, "the DRUMS' key, short: their page, nothing playing yet");
        tap_key(white_key(1));
        EXPECT(ui.dsel == DV_SNARE && par.ch[0].src == src0, "white key 2 there: the SNARE, no input changed");
        keep = drm;
        key(KEY_SHIFT, 1);
        tap_key(white_key(1));
        key(KEY_SHIFT, 0);
        EXPECT(memcmp(&keep.v[DV_SNARE], &drm.v[DV_SNARE], sizeof(dvoice_t)) != 0 &&
                   !memcmp(&keep.v[DV_KICK], &drm.v[DV_KICK], sizeof(dvoice_t)) && DKIT[drm.kit[DV_SNARE]].dv == DV_SNARE,
               "SHIFT + it: a new snare sound, the kick kept");
        inc0 = drc[DV_SNARE].inc0;
        turn(EN_K1, 3);
        EXPECT(drm.v[DV_SNARE].tune != keep.v[DV_SNARE].tune && drc[DV_SNARE].inc0 != inc0, "KNOB 1: its TUNE, heard at once");
        press_btn(B_OCTUP);
        EXPECT(ui.page == PG_DRUM2, "OCT+: the voice's second page");
        press_btn(B_OCTUP);
        EXPECT(ui.page == PG_DRUM3, "OCT+: the drums' own page");
        {
            uint32_t sw0 = drm.swing;            /* (build 21: a style may draw some swing) */
            turn(EN_K1, 10);
            EXPECT(drm.swing == (sw0 + 10u > 100u ? 100u : sw0 + 10u), "KNOB 1 there: SWING");
        }
        {
            int32_t v0 = drm.vary;
            turn(EN_K2, -5);
            EXPECT(drm.vary == (v0 > 5 ? v0 - 5 : 0), "KNOB 2 there: VARIATION");
        }
        st0 = drm.style;
        turn(EN_PRESET, 1);
        EXPECT(drm.style == (st0 + 1u) % DS_N && drm.on, "PRESETS there: the next drum rhythm, playing");
        run_ms(600);
        key(KEY_DRM, 1);
        run_ms(900);
        key(KEY_DRM, 0);
        run_ms(30);
        EXPECT(!drm.on && ui.page == PG_DRUM3, "held: the drums off, the page kept");
        st0 = drm.style;
        key(KEY_SHIFT, 1);
        tap_key(KEY_DRM);
        key(KEY_SHIFT, 0);
        EXPECT(drm.on && drm.style != st0, "SHIFT + the key: a new drum rhythm, another style, playing");
        run_ms(1000);
        EXPECT(drm_live, "the drums sound");
        /* a voice's white key held: muted (its steps unheard, MUTED on its page, its key blinking), held again: back */
        goto_page(PG_DRUM);
        key(white_key(0), 1);
        run_ms(800);
        key(white_key(0), 0);
        EXPECT(ui.dsel == DV_KICK && (drm.mute & 1u), "KICK's key held: muted");
        run_ms(1500);
        EXPECT(!dvs[DV_KICK].live && (drm.mute & 1u), "muted: unheard");
        tap_key(white_key(1));
        EXPECT(ui.dsel == DV_SNARE && (drm.mute & 1u), "another voice chosen: KICK stays muted");
        key(white_key(0), 1);
        run_ms(800);
        key(white_key(0), 0);
        EXPECT(!(drm.mute & 1u), "held again: back");
        /* BREAK (build 19): the last white key held, a way drawn and played; let go, the rhythm as written */
        {
            uint32_t w0, k2, same = 0;
            uint16_t mm = drm.mute;
            uint8_t ds = ui.dsel;
            drm.mute = 0;
            key(white_key(15), 1);
            run_ms(300);
            EXPECT(ui.brk_on && brk_hold && ui.dsel == ds, "the last white key held: BREAK, the voice chosen kept");
            EXPECT(host_led[14u + KEY_BRK], "its key lit");
            w0 = brk_plan.way;
            key(white_key(15), 0);
            run_ms(30);
            EXPECT(!ui.brk_on && !brk_hold, "let go: off");
            for (k2 = 0; k2 < 12u; k2++) {
                key(white_key(15), 1);
                run_ms(40);
                same += brk_plan.way == w0;
                w0 = brk_plan.way;
                key(white_key(15), 0);
                run_ms(20);
            }
            EXPECT(!same, "each press another way");
            drm.mute = mm;
        }
        drm.mute = 2;                            /* (the SNARE muted, to be saved) */
        tap_key(KEY_DRM);
        EXPECT(ui.page == PG_MAIN, "the key again: back to the channel");
        tap_key(white_key(1));
        EXPECT(par.ch[0].src == 1, "white key 2 off the DRUMS' pages: the channel's input again");
        par.ch[0].src = (int16_t)src0;
        /* kept with a session */
        keep = drm;
        cmp.amount = 60;
        cmp.ratio = 4;
        cmp_coef();
        EXPECT(ses_save(14) == SES_OK, "a session saved with the drums");
        cmp.amount = 0;
        drm.v[DV_KICK].tune = (int16_t)(drm.v[DV_KICK].tune ^ 77);
        drm.style = (uint8_t)((drm.style + 1u) % DS_N);
        EXPECT(ses_load(14) == SES_OK && !memcmp(&keep, &drm, sizeof drm), "and loaded back: the drums as saved");
        EXPECT(cmp.amount == 60 && cmp.ratio == 4 && cmpk.on && (drm.mute & 2u), "the COMPRESSOR and the mutes too");
        cmp.amount = 0;
        cmp_coef();
        drm.mute = 0;
        drm.on = 0;
    }
    /* a channel not in the patch: its screens grey */
    press_btn(LAYER_BTN[2]);
    if (ui.page != PG_MAIN)
        press_btn(LAYER_BTN[2]);
    par.ch[2].mute = 0;
    par.ch[2].level = 500;
    run_ms(200);
    EXPECT(!chr[2].dead && !ui.shown_dead && ch_col(2) == LAYER_COL[2], "a channel in the patch: its colour");
    tap_key(KEY_MUTE[2]);
    run_ms(200);
    EXPECT(ui.shown_dead && ch_col(2) == C_MONO, "muted, out of the patch: grey");
    tap_key(KEY_MUTE[2]);
    par.ch[2].level = 0;
    par.ch[2].send1 = par.ch[2].send2 = 0;
    par.rhy.on = 0;
    run_ms(200);
    EXPECT(chr[2].dead && ui.shown_dead, "its LEVEL and SENDs off: grey");
    par.ch[2].level = 500;
    run_ms(200);
    EXPECT(!chr[2].dead && !ui.shown_dead, "and back: its colour");
    /* an older session's one loop: each channel's changes its own loop, the rest left out */
    {
        uint32_t w3 = CH_W0 + 3u * CHW + 5u, w0 = CH_W0 + 1u, ws = SL_W0 + 3u * SLW + 1u, wo = NWORD - 1u, li;
        gev_t old[5] = {{10, (uint16_t)w3, 7}, {20, (uint16_t)wo, 9}, {30, (uint16_t)w0, 8}, {40, (uint16_t)ws, 6},
                        {50, (uint16_t)w3, 5}};
        memcpy(gev, old, sizeof old);
        g_prev = par;
        ((uint16_t *)&g_prev)[w3] = 77;
        ses_gl_split(5, 1000);
        EXPECT(gl[0].n == 1 && gl[3].n == 3 && gl[1].n == 0 && g_used == 4, "the old loop split by channel, the rest out");
        EXPECT(gev[gl[3].off].t == 10 && gev[gl[3].off + 1u].t == 40 && gev[gl[3].off + 2u].t == 50 &&
                   gl[3].len == 1000 && gl[1].len == 0, "each in time, as long as the old loop");
        (void)g_owner(w3, &li);
        EXPECT(g_s0[3][li] == 77 && (g_touched[3][li >> 3] >> (li & 7u) & 1u), "each from the old loop's start");
    }
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
    par.ch[1].level = fx == FX_XMOD || fx == FX_DUCK || fx == FX_GATE || fx == FX_LPG ? LVL_0DB - 80 : 0;   /* (a second loop: XMOD's, DUCK's, GATE's other signal;
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
        {"XMOD COMPARE", FX_XMOD, XM_COMP, 1}, {"DUCK BY CH 2", FX_DUCK, 900, 1}, {"GATE BY CH 2", FX_GATE, 900, 1},
        {"CYCLES WAVELOSS", FX_CYCLES, CY_LOSS, 600}, {"CYCLES SPUTTER", FX_CYCLES, CY_DISINT, 500},
        {"CYCLES WAVESET", FX_CYCLES, CY_WAVESET, 600}, {"REED A2", FX_REED, 9, 700}, {"REED E4", FX_REED, 28, 600},
        {"OCTAVER ONE DOWN", FX_OCTAVE, OC_ONE, 600}, {"OCTAVER BOTH", FX_OCTAVE, OC_BOTH, 400},
        {"DIFFUSER SIZE 5", FX_DIFFUSE, 4, 700}, {"SAG 23 MS", FX_SAG, 4, 800}, {"SAG 371 MS", FX_SAG, 8, 900},
        {"CUTS", FX_CUTS, 500, 600},
        {"AUTO-LEVEL -12 DB", FX_AUTOLVL, 600, 500}, {"AUTO-LEVEL -3 DB", FX_AUTOLVL, 900, 900},
        {"LP GATE BY CH 2", FX_LPG, 900, 1}, {"CROSSOVER DEAD", FX_CROSS, CR_DEAD, 400},
        {"CROSSOVER INSIDE", FX_CROSS, CR_INOUT, 500}, {"FRICTION 110 HZ", FX_FRICT, 2, 400},
        {"FRICTION 440 HZ", FX_FRICT, 6, 800}, {"GATELOPE", FX_GLOPE, 200, 500}, {"FREEZE 100 MS", FX_FREEZE, 2, 300},
        {"FREEZE 371 MS", FX_FREEZE, 7, 700}};
    uint32_t i, bad = 0, a;
    for (i = 0; i < NELEM(T); i++)
        for (a = 0; a < 2u; a++) {
            stats_t st = fx_run(T[i].fx, T[i].p1, T[i].p2, (int)a);
            int ok = db(st.late_rms) > -50 && st.clip < 0.02 && fabs(st.dc) < 0.02;
            printf("  %-20s %-6s rms %6.1f dB  zc %6.0f Hz  clip %.2f%%  %s\n", T[i].n, a ? "SEND" : "INSERT",
                   db(st.late_rms), st.zc_hz, st.clip * 100, ok ? "ok" : "BAD");
            bad += !ok;
        }
    {   /* the SHAPERS: each tube MODE, a loop through it singing, bounded -- and each its own colour (the
         * brightness: the first difference's level against the signal's) */
        static int16_t kb[2 * 44100];
        double br[TM_N] = {0};
        int32_t m, j;
        for (m = TM_SAT; m < TM_N; m++) {
            stats_t st;
            double e = 0, d = 0;
            fresh_mixer();
            patch_plain(&par);
            par.out.spres = 0;
            par.ch[0].level = LVL_0DB - 80;
            par.ch[0].tmode = (int16_t)m;
            par.ch[0].gain = 400;                /* (driven: the curves apart) */
            st = render(4.0, kb, NELEM(kb) / 2u);
            for (j = 2; j < (int32_t)NELEM(kb); j += 2) {
                e += (double)kb[j] * kb[j];
                d += (double)(kb[j] - kb[j - 2]) * (kb[j] - kb[j - 2]);
            }
            br[m] = e > 0 ? sqrt(d / e) : 0;
            int ok = db(st.late_rms) > -50 && st.clip < 0.02 && fabs(st.dc) < 0.02;
            if (m > TM_SAT)                      /* (each a colour of its own beside the tube's) */
                ok &= fabs(br[TM_SAT] - br[m]) > 0.01 * br[TM_SAT];
            printf("  TUBE %-15s rms %6.1f dB  zc %6.0f Hz  bright %.4f  %s\n", TM_NAME[m], db(st.late_rms), st.zc_hz,
                   br[m], ok ? "ok" : "BAD");
            bad += !ok;
        }
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

/* ---------------------------------------------------------------- rhythm --- *
 * every style over patches of several families: the output moves (in 50 ms windows) more with the rhythm than
 * without, and is neither silent nor clipped */
/* the trim's curve: a patch's loudness over its first 6 s, half a second at a time, against where it settles (5 .. 6 s)
 * -- build 24: no longer ~6 dB down for the first 3.6 s */
static int cmd_trimcurve(int with_cmp)
{
    enum { NW = 12, NP = 200 };
    static double acc[NW];
    uint32_t p, w, n = 0, bad = 0, clip = 0, clipped = 0;
    double settled = 0;
    int32_t out[2 * CTL];
    for (p = 0; p < NP; p++) {
        double e[NW] = {0}, ref;
        uint32_t b, i, blocks = 44100u * 6u / CTL, per = blocks / NW;
        fresh_mixer();
        patch_draw(&par, 9001u + p * 7u, -1);
        cmp.amount = with_cmp ? 85 : 0;          /* (with the COMPRESSOR as it starts, build 24: 85 %, RELEASE from 120
                                                  * BPM) */
        cmp_rel_tempo(120);
        master_q12 = 4096;
        fm1_ms = 0;
        trim_start();
        for (b = 0; b < blocks; b++) {
            fm1_ms = (uint32_t)((uint64_t)b * CTL * 1000u / FS);
            trim_poll();
            mix_block(out);
            for (i = 0; i < CTL; i++) {
                double x = out[2 * i] / 32768.0;
                if (b / per < NW)
                    e[b / per] += x * x;
                if (abs(out[2 * i]) > 32000)
                    clip++;
            }
        }
        if (clip > 44100u * 6u / 50u)            /* (over 2 % of its 6 s at the ceiling) */
            clipped++;
        clip = 0;
        ref = db(sqrt((e[NW - 2] + e[NW - 1]) / (2.0 * per * CTL)) + 1e-9);
        settled += ref;
        if (ref < -50)
            continue;                            /* (silent: nothing to go by) */
        for (w = 0; w < NW; w++)
            acc[w] += db(sqrt(e[w] / (per * CTL)) + 1e-9) - ref;
        n++;
    }
    printf("  %s: settled at %.1f dB on average, %u of %u patches clipped; loudness against the settled:\n ",
           with_cmp ? "COMPRESSOR 85 %" : "no COMPRESSOR", settled / n, clipped, NP);
    if (clipped > NP / 100u)
        bad++;
    for (w = 0; w < NW; w++)
        printf(" %4.2fs %+5.1f", (w + 0.5) * 0.5, acc[w] / n);
    printf("\n");
    for (w = 2; w < NW - 2; w++)                 /* (from 1 s on: within 2 dB of where it settles, on average) */
        if (acc[w] / n < -2.0 || acc[w] / n > 2.0)
            bad++;
    printf("trimcurve: %s\n", bad ? "FAILED" : "a new patch at its loudness from 1 s on");
    return bad ? 1 : 0;
}
static int cmd_rhythm(void)
{
    uint32_t st, f, bad = 0;
    for (st = 0; st < RS_N; st++) {
        double m0 = 0, m1 = 0, lo = 0;
        uint32_t n = 0;
        /* (every recipe: no recipe x style may fall silent -- build 14: rings no longer choked, sparse lanes over a
         * bed, the trim's measure fine enough for a chopped quiet output) */
        for (f = 0; f < NFAM; f++) {
            stats_t a, b;
            fresh_mixer();
            patch_draw(&par, 777u + f * 31u + st, (int)f);
            a = render(6.0, 0, 0);
            fresh_mixer();
            patch_draw(&par, 777u + f * 31u + st, (int)f);
            rhythm_draw(&par, 4242u + st * 17u + f, (int)st);
            par.rhy.on = 1;
            par.rhy.bpm = 132;
            rhy_reset = 1;
            b = render(6.0, 0, 0);
            m0 += a.move;
            m1 += b.move;
            if (db(a.rms) > -40 && (n == 0 || db(b.rms) < lo))   /* (over the run: a sparse style may rest a whole
                                                  * second. Build 20: of the patches heard without it -- a patch
                                                  * quiet by itself, rare (1 in ~600), is not the rhythm's doing) */
                lo = db(b.rms);
            if (b.clip > 0.02) {
                printf("  %s x %s: %.1f %% clipped (output: %s)\n", RS_NAME[st], FAM[f].name, b.clip * 100,
                       par.slot[SLOT_OUT].in == FI_INS ? FX_NAME[par.slot[SLOT_OUT].fx] : "none");
                bad++;
            }
            n++;
        }
        printf("  %-12s movement %4.1f -> %4.1f dB   quietest %6.1f dB\n", RS_NAME[st], m0 / n, m1 / n, lo);
        if (m1 / n < m0 / n + 0.2 || lo < -40)   /* (build 24: was 1.5 dB -- the trim's old jump at 3.6 s, inside
                                                  * the window, made up most of it; now the rhythm's own) */
            bad++;
    }
    {   /* the clock: beats in 10 s at 10, 120, 500 BPM */
        static const uint16_t BP[3] = {10, 120, 500};
        for (st = 0; st < 3u; st++) {
            uint32_t b0, want = BP[st] * 10u / 60u, got;
            fresh_mixer();
            patch_draw(&par, 5u, 0);
            rhythm_draw(&par, 9u, RS_INDUSTRIAL);
            par.rhy.on = 1;
            par.rhy.bpm = BP[st];
            rhy_reset = 1;
            render(0.1, 0, 0);
            b0 = rhy_beat;
            render(10.0, 0, 0);
            got = rhy_beat - b0;
            printf("  clock %3u BPM: %u beats in 10 s (%u expected)\n", BP[st], got, want);
            if (got + 1u < want || got > want + 1u)
                bad++;
        }
    }
    printf("rhythm: %s\n", bad ? "FAILED" : "every style moves the sound, none silent or clipped; the clock in time");
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
    for (pg = 0; pg <= PG_COMP; pg++)
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
    for (f = 0; f < NFAM; f++)
        FIT(text_w(&FONT_XS, FAM[f].name) <= 236, "a recipe's name", FAM[f].name);
    for (pg = PG_TEMPO; pg <= PG_RHY2; pg++)    /* (the RHYTHM's pages) */
        for (j = 0; j < 4u; j++)
            if (PG_ROW[pg][j] != P_NONE)
                FIT(text_w(&FONT_XS, p_name(PG_ROW[pg][j])) <= 150, "a RHYTHM row's name", p_name(PG_ROW[pg][j]));
    for (f = 0; f < MD_N; f++)
        FIT(text_w(&FONT_S, MD_NAME[f]) <= 228, "a destination", MD_NAME[f]);
    for (pg = PG_DRUM; pg <= PG_DRUM3; pg++)     /* (the DRUMS' pages: every row, every value) */
        for (j = 0; j < 8u; j++) {
            uint32_t id = j < 4u ? PG_ROW[pg][j] : PG_SHROW[pg][j - 4u];
            int32_t v;
            if (id == P_NONE)
                continue;
            FIT(text_w(&FONT_XS, p_name(id)) <= 150, "a DRUMS row's name", p_name(id));
            for (v = p_lo(id); v <= p_hi(id); v++) {
                p_text(id, v, t);
                if (text_w(&FONT_S, t) > 228) {
                    FIT(0, p_name(id), t);
                    break;
                }
            }
        }
    for (f = 0; f < NDKIT; f++) {                /* (a voice page's title: the voice and its sound) */
        str_cpy(t, DV_NAME[DKIT[f].dv], 32);
        str_cpy(t + str_len(t), "  ", 3);
        str_cpy(t + str_len(t), DKIT[f].name, 16);
        FIT(text_w(fit_font(t, 236, &FONT_M), t) <= 236, "a DRUMS title (unsqueezed, at the size it falls to)", t);
    }
    for (f = 0; f < DS_N; f++)
        FIT(text_w(fit_font(DS_NAME[f], 236, &FONT_M), DS_NAME[f]) <= 236, "a drum rhythm (unsqueezed)", DS_NAME[f]);
    for (f = 0; f < NDV; f++) {                  /* (SHIFT held: the voice and SHIFT) */
        str_cpy(t, DV_NAME[f], 16);
        str_cpy(t + str_len(t), "  SHIFT", 8);
        FIT(text_w(fit_font(t, 236, &FONT_M), t) <= 236, "a DRUMS title with SHIFT", t);
    }
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
    patch_draw(&par, 77u, 9);                    /* (the patch build 9 measured: ECHO CHAMBER, then a factory one) */
    for (b = 0; b < 44100u * 4u / CTL; b++) {
        mix_block(out);
        for (i = 0; i < 2 * CTL; i++)
            h = (h ^ (uint32_t)out[i]) * 16777619u;
    }
    printf("cores %d: hash %08x\n", n, h);
    return 0;
}

/* host breaks: each BREAK way over 2 bars of a written rhythm, as a grid of steps (x a hit, X an accent, o a ghost,
 * a digit: its hits in the step) -- to look at */
static int cmd_breaks(void)
{
    uint32_t w, b, v;
    int32_t out[2 * CTL];
    static const char *const RH[NDV] = {"X...X...X..XX...", "....X.......X.X.", "..X...X...X...XX", "...X.....X...X.."};
    for (w = 0; w <= BK_N; w++) {
        char g[NDV][33];
        drm_quiet_mixer();
        drm_init(9u);
        drm_draw_rhythm(1u, DS_DBREAK);
        drm.vary = 0;                        /* (build 21: VARIATION off -- hits counted against the lanes) */
        for (v = 0; v < NDV; v++)
            dl_set(v, RH[v], 100);
        drm.l[DV_SNARE].acc = 1u << 12;
        drm.on = 1;
        drm.swing = 0;
        par.rhy.bpm = 120;
        brk_hold = 0;
        if (w < BK_N) {
            brk_plan.way = (uint8_t)w;
            brk_plan.cell = 5;
            brk_plan.q = 4;
            for (v = 0; v < 8u; v++)
                brk_plan.ord[v] = (uint8_t)((v * 5u) % 8u);
            brk_fill();
            brk_hold = 1;
        }
        memset(g, '.', sizeof g);
        for (b = 0; b < 32u * 172u + 86u; b++) {
            mix_block(out);
            for (v = 0; v < NDV; v++)
                if (dtrig[v].fire) {
                    uint32_t st = (b + 40u) / 172u;
                    char *c = &g[v][st < 32u ? st : 31u];
                    if (*c == '.')
                        *c = dtrig[v].acc == 1u ? 'X' : dtrig[v].acc == 2u ? 'o' : 'x';
                    else
                        *c = *c >= '2' && *c < '9' ? (char)(*c + 1) : '2';
                }
        }
        printf("%s\n", w < BK_N ? BK_NAME[w] : "(as written)");
        for (v = 0; v < NDV; v++) {
            g[v][32] = 0;
            printf("  %-6s %.16s %.16s\n", DV_NAME[v], g[v], g[v] + 16);
        }
    }
    brk_hold = 0;
    return 0;
}
/* each drum style's lanes, as drawn from a few seeds: x hit, X accent, o short, O both; each lane its length and
 * its step (a look at what the generator makes) */
static int cmd_rhythms(uint32_t seeds)
{
    static const char *const DVN[NDIV] = {"1/2", "1/4", "1/8", "1/8T", "1/16", "1/16Q", "1/16T", "1/32"};
    uint32_t k, sd, c, i;
    for (k = 0; k < DS_N; k++)
        for (sd = 0; sd < seeds; sd++) {
            drm_draw_rhythm(1000u + sd * 7919u + k, (int)k);
            printf("%-12s seed %u  VARIATION %d  SWING %u\n", DS_NAME[k], sd, drm.vary, drm.swing);
            for (c = 0; c < NDV; c++) {
                const dlane_t *L = &drm.l[c];
                char g[33];
                for (i = 0; i < L->len; i++) {
                    uint32_t h = L->hits >> i & 1u, a = L->acc >> i & 1u, l = L->alt >> i & 1u;
                    g[i] = !h ? '.' : a ? (l ? 'O' : 'X') : (l ? 'o' : 'x');
                }
                g[L->len] = 0;
                printf("  %-5s %2u x %-5s p%3u v%2u  %s\n", DV_NAME[c], L->len, DVN[L->div], L->prob, L->var, g);
            }
        }
    return 0;
}
int main(int argc, char **argv)
{
    sine_init();                                 /* (as the firmware's boot: SINE from SINE_ROM) */
    if (argc >= 3 && str_eq(argv[1], "demo"))
        return cmd_demo(argv[2]);
    if (argc >= 2 && str_eq(argv[1], "drums"))
        return cmd_drums();
    if (argc >= 2 && str_eq(argv[1], "rhythms"))
        return cmd_rhythms(argc >= 3 ? (uint32_t)atoi(argv[2]) : 3u);
    if (argc >= 2 && str_eq(argv[1], "breaks"))
        return cmd_breaks();
    if (argc >= 2 && str_eq(argv[1], "free"))
        return cmd_free();
    if (argc >= 2 && str_eq(argv[1], "check"))
        return cmd_check();
    if (argc >= 3 && str_eq(argv[1], "screens"))
        return cmd_screens(argv[2]);
    if (argc >= 3 && str_eq(argv[1], "one")) {   /* one drawn patch (its seed as check draws it: 0xC0FFEE + i x 7919) */
        stats_t st;
        fresh_mixer();
        printf("family %s\n", FAM[patch_draw(&par, 0xC0FFEEu + (uint32_t)atoi(argv[2]) * 7919u, -1)].name);
        print_patch();
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
    if (argc >= 3 && str_eq(argv[1], "checkone")) {   /* (one of check's patches, traced on stderr) */
        uint32_t i = (uint32_t)atoi(argv[2]);
        stats_t st;
        fresh_mixer();
        patch_draw(&par, 0xC0FFEEu + i * 7919u, -1);
        st = render(8.0, 0, 0);
        printf("patch %u: clip %.2f%% late %.1f dB\n", i, st.clip * 100, db(st.late_rms));
        return 0;
    }
    if (argc >= 2 && str_eq(argv[1], "trimcurve"))
        return cmd_trimcurve(0) | cmd_trimcurve(1);
    if (argc >= 2 && str_eq(argv[1], "rhythm"))
        return cmd_rhythm();
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
