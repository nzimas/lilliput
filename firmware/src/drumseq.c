/* SPDX-License-Identifier: GPL-3.0-only
 * LILLIPUT: the DRUMS' steps, a block at a time (the ISR, CPU0, beside the RHYTHM's): each voice's lane a phase, a
 * step a wrap, on the RHYTHM's tempo and its beat (mixer.c's clk_block): started, or given a new rhythm, they begin
 * on the next beat, and the RHYTHM is never disturbed by them, nor they by it. A hit (its chance) is handed to the voices
 * (drums_render, out_stage) with its accent, its pitch and ALT; SWING makes the off steps late. */
static struct { uint32_t ph; uint8_t step, pend, pacc, palt; int8_t ppitch, rsv[3]; } dlr[NDV];
static uint8_t drm_running;      /* (the lanes playing: a new rhythm waits for the beat over them) */
static FAST void drm_hit(uint32_t c, uint32_t s)
{
    const dlane_t *L = &drm.l[c];
    if (drm.mute >> c & 1u)                      /* (muted: its steps run on, unheard) */
        return;
    dtrig[c].acc = (uint8_t)(L->acc >> s & 1u);
    dtrig[c].alt = (uint8_t)(L->alt >> s & 1u);
    dtrig[c].pitch = L->pitch[s];
    dtrig[c].fire = 1;
}
static FAST void drums_block(void)
{
    uint32_t c, bpm = (uint32_t)clampi(par.rhy.bpm, 10, 500);
    for (c = 0; c < NDV; c++)
        dtrig[c].fire = 0;
    if (!drm.on) {
        drm_reset = 1;
        drm_running = 0;
        return;
    }
    if (drm_reset) {                             /* (from the top on the beat; until then a rhythm playing plays on) */
        if (!clk_edge && !drm_running)
            return;
        if (clk_edge) {
            for (c = 0; c < NDV; c++) {
                dlr[c].ph = 0xFFFFFFFFu;
                dlr[c].step = 0xFF;
                dlr[c].pend = 0;
            }
            drm_reset = 0;
        }
    }
    drm_running = 1;
    for (c = 0; c < NDV; c++) {
        const dlane_t *L = &drm.l[c];
        uint32_t len = (uint32_t)clampi(L->len, 1, (int32_t)DSTEPS), p0 = dlr[c].ph;
        uint32_t inc = bpm * DIV2[L->div < NDIV ? L->div : 4] * 25971u;
        dlr[c].ph += inc;
        if (dlr[c].ph < p0 || dlr[c].step == 0xFF) {   /* a new step */
            uint32_t s;
            if (dlr[c].step == 0xFF)
                dlr[c].ph = 0;
            s = dlr[c].step = (uint8_t)(dlr[c].step == 0xFF ? 0u : (dlr[c].step + 1u) % len);
            if ((L->hits >> s & 1u) && (rnd_next(&rhy_r) >> 25) < (uint32_t)(L->prob ? L->prob : 100) * 128u / 100u) {
                if (drm.swing && (s & 1u)) {     /* (an off step: late) */
                    dlr[c].pend = 1;
                    dlr[c].pacc = s;
                } else
                    drm_hit(c, s);
            }
        }
        if (dlr[c].pend && dlr[c].ph >= (uint32_t)drm.swing * 14316557u) {   /* (up to a third of a step late) */
            dlr[c].pend = 0;
            drm_hit(c, dlr[c].pacc);
        }
    }
}
