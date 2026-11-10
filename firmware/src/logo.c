/* SPDX-License-Identifier: GPL-3.0-only
 * LILLIPUT: the splash screen -- six rings in the channels' colours, each one feeding the next round a circle (the
 * no-input mixer's loop), the name under them, the build. Drawn from shapes, no image kept: each pixel's colour is
 * each shape's coverage of it (its distance, half a pixel either way), over black. */

static int32_t lg_cov(int32_t d16)               /* coverage (0 .. 256) of a pixel by a shape d16 / 16 px away */
{
    return clampi(128 - d16 * 16, 0, 256);
}
static void splash_draw(const char *build)
{
    static const uint32_t COL[6] = {0xFFD600u, 0x00D6FFu, 0xFF5CD2u, 0x60FF60u, 0xFF8C00u, 0xBEA0FFu};
    int32_t cx[6], cy[6], k;
    for (k = 0; k < 6; k++) {                    /* the rings' centres: round a circle of 52 px, from the top */
        uint32_t a = (uint32_t)k * 715827883u;
        cx[k] = 120 + mulq15(52, sin_ph(a));
        cy[k] = 92 - mulq15(52, sin_ph(a + 0x40000000u));
    }
    CV_AREA(0, 0, 240, 168, C_BLACK) {
        int32_t x, y;
        for (y = 0; y < (int32_t)cv_h; y++)
            for (x = 0; x < 240; x++) {
                int32_t py = y + cv_oy, rgb[3] = {0, 0, 0}, d, cov, c;
                /* the loop: a thin grey circle through the rings */
                d = (int32_t)isqrt32((uint32_t)(((x - 120) * (x - 120) + (py - 92) * (py - 92)) * 256)) - 52 * 16;
                cov = lg_cov((d < 0 ? -d : d) - 16);
                rgb[0] = rgb[1] = rgb[2] = 90 * cov;
                for (k = 0; k < 6; k++) {        /* each ring, over it */
                    int32_t dx = x - cx[k], dy = py - cy[k];
                    d = (int32_t)isqrt32((uint32_t)((dx * dx + dy * dy) * 256));
                    cov = lg_cov((d - 15 * 16 < 0 ? 15 * 16 - d : d - 15 * 16) - 4 * 16);
                    if (cov) {
                        rgb[0] = (rgb[0] * (256 - cov) + (int32_t)(COL[k] >> 16) * cov * 256 / 256) ;
                        rgb[1] = (rgb[1] * (256 - cov) + (int32_t)(COL[k] >> 8 & 255u) * cov * 256 / 256);
                        rgb[2] = (rgb[2] * (256 - cov) + (int32_t)(COL[k] & 255u) * cov * 256 / 256);
                        rgb[0] >>= 8;
                        rgb[1] >>= 8;
                        rgb[2] >>= 8;
                        rgb[0] <<= 8;
                        rgb[1] <<= 8;
                        rgb[2] <<= 8;
                    }
                }
                c = RGB(clampi(rgb[0] >> 8, 0, 255), clampi(rgb[1] >> 8, 0, 255), clampi(rgb[2] >> 8, 0, 255));
                if (c)
                    cv_rect(x, py, 1, 1, (uint16_t)c);
            }
    }
    CV_AREA(0, 168, 240, 72, C_BLACK) {
        cv_text_fit(0, &FONT_M, "LILLIPUT", C_WHITE);
        cv_text_c(48, &FONT_XS, build, RGB(150, 150, 150));
    }
}
