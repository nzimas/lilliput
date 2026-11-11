/* SPDX-License-Identifier: GPL-3.0-only
 * LILLIPUT: the DRUMS' steps, a block at a time (the ISR, CPU0, beside the RHYTHM's): each voice's lane a phase, a
 * step a wrap, on the RHYTHM's tempo and its beat (mixer.c's clk_block): started, or given a new rhythm, they begin
 * on the next beat, and the RHYTHM is never disturbed by them, nor they by it. A hit (its chance) is handed to the voices
 * (drums_render, out_stage) with its accent, its pitch and ALT; SWING makes the off steps late. While BREAK's key is
 * held, drumbrk.c says what each step plays instead. */
static struct { uint32_t ph; uint8_t step, pend, rsv[2]; dhit_t pev; } dlr[NDV];
static uint8_t drm_running;      /* (the lanes playing: a new rhythm waits for the beat over them) */
#include "drumbrk.c"                     /* (BREAK: its key held, a variation of the rhythm playing) */
static FAST void drums_block(void)
{
    uint32_t c, bpm = (uint32_t)clampi(par.rhy.bpm, 10, 500);
    for (c = 0; c < NDV; c++)
        dtrig[c].fire = 0;
    if (!drm.on) {
        drm_reset = 1;
        drm_running = 0;
        brk_clear();
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
            brk_clear();
            drm_reset = 0;
        }
    }
    drm_running = 1;
    for (c = 0; c < NDV; c++) {
        const dlane_t *L = &drm.l[c];
        uint32_t len = (uint32_t)clampi(L->len, 1, (int32_t)DSTEPS), p0 = dlr[c].ph, d2 = DIV2[L->div < NDIV ? L->div : 4];
        uint32_t inc = bpm * d2 * 25971u;
        dlr[c].ph += inc;
        if (dlr[c].ph < p0 || dlr[c].step == 0xFF) {   /* a new step */
            uint32_t s;
            dhit_t e;
            if (dlr[c].step == 0xFF)
                dlr[c].ph = 0;
            s = dlr[c].step = (uint8_t)(dlr[c].step == 0xFF ? 0u : (dlr[c].step + 1u) % len);
            bks[c].rn = 0;                       /* (the step before's ratchet: done) */
            brk_step(c, s, len, d2 >= 2u ? d2 / 2u : 1u, &e);
            if (e.fire) {
                if (drm.swing && (s & 1u)) {     /* (an off step: late) */
                    dlr[c].pend = 1;
                    dlr[c].pev = e;
                } else
                    drm_emit(c, &e);
            }
        } else
            brk_block(c);
        if (dlr[c].pend && dlr[c].ph >= (uint32_t)drm.swing * 14316557u) {   /* (up to a third of a step late) */
            dlr[c].pend = 0;
            drm_emit(c, &dlr[c].pev);
        }
    }
}
