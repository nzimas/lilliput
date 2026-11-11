/* SPDX-License-Identifier: GPL-3.0-only
 * LILLIPUT: the splash screen -- the logo (build 18): the name drawn on one grid, "lilliput" from bare strokes,
 * squares and two arcs, every stroke one gap from the next, so the word reads as a pulse train (tools/gen_logo.py
 * makes the SVGs in assets/logo from the same numbers). Drawn here one unit to a pixel: the strokes 6 px, on whole
 * pixels; only the two bowls are worked out from their distance (a pixel's colour its coverage, half a pixel either
 * way), over black. The build under it. */

static int32_t lg_cov(int32_t d16)               /* coverage (0 .. 256) of a pixel by a shape d16 / 16 px away */
{
    return clampi(128 - d16 * 16, 0, 256);
}
/* the logotype's strokes and squares (tools/gen_logo.py's units: x, y, w, h), then its two bowls */
static const int16_t LG_RECT[][4] = {
    {0, 8, 6, 72}, {15, 44, 6, 36}, {15, 8, 6, 6}, {30, 8, 6, 72}, {45, 8, 6, 72},   /* l i l l */
    {60, 44, 6, 36}, {60, 8, 6, 6},                                                 /* i */
    {75, 44, 6, 60},                                                                /* p's stem */
    {120, 44, 6, 18}, {150, 44, 6, 36},                                             /* u's stems */
    {165, 20, 6, 60}, {158, 44, 20, 6}};                                            /* t */
#define LG_W 178                 /* the logotype's width; its height 8 .. 104 */
#define LG_SW 6                  /* the stroke */
#define LG_RC 15                 /* the bowls' radius, on the stroke's middle */
#define LG_P_CX 93               /* p's bowl: a ring */
#define LG_U_CX 138              /* u's: the lower half of one, under its stems */
#define LG_CY 62
static void splash_draw(const char *build)
{
    const int32_t ox = (240 - LG_W) / 2, oy = 34;   /* (the word a little above the middle) */
    uint32_t k;
    CV_AREA(0, 0, 240, 176, C_BLACK) {
        int32_t x, y;
        for (y = LG_CY - LG_RC - LG_SW; y <= LG_CY + LG_RC + LG_SW; y++)
            for (x = LG_P_CX - LG_RC - LG_SW; x <= LG_U_CX + LG_RC + LG_SW; x++) {
                int32_t cov = 0, c, cx;
                for (cx = LG_P_CX; cx <= LG_U_CX; cx += LG_U_CX - LG_P_CX) {
                    /* (distance from the pixel's centre, x 16: the bowl's stroke is r +- 3 px) */
                    int32_t dx = x * 2 + 1 - cx * 2, dy = y * 2 + 1 - LG_CY * 2, d, v;
                    if (cx == LG_U_CX && y < LG_CY)
                        continue;                /* (u's bowl: the lower half only) */
                    d = (int32_t)isqrt32((uint32_t)((dx * dx + dy * dy) * 64));   /* (x 16, from half-pixels) */
                    v = lg_cov((d < LG_RC * 16 ? LG_RC * 16 - d : d - LG_RC * 16) - LG_SW / 2 * 16);
                    cov = v > cov ? v : cov;
                }
                if (cov) {
                    c = cov * 255 / 256;
                    cv_rect(ox + x, oy + y, 1, 1, (uint16_t)RGB(c, c, c));
                }
            }
        for (k = 0; k < sizeof LG_RECT / sizeof LG_RECT[0]; k++)   /* (the strokes over the bowls' soft edges) */
            cv_rect(ox + LG_RECT[k][0], oy + LG_RECT[k][1], LG_RECT[k][2], LG_RECT[k][3], C_WHITE);
    }
    CV_AREA(0, 176, 240, 64, C_BLACK) {
        cv_text_c(16, &FONT_XS, build, RGB(150, 150, 150));
    }
}
