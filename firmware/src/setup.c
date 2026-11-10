/* SPDX-License-Identifier: GPL-3.0-only
 * LILLIPUT: GLOBAL SETUP (Tonitor's, after BIGORNA builds 46-48): the settings of the machine, not of a session, kept as
 * a record of the sessions' store (number SET_NUM). SHIFT + SAVE held opens it; a channel's button leaves it. Its
 * sections -- MIDI, AUDIO, SETTINGS -- then each one's parts, then their rows: PRESETS through them, SAVE into one; on a
 * row SAVE moves the focus to its value, PRESETS changes it, SAVE sets it; SHIFT + SAVE leaves a value as it was, or goes
 * back a level (from the top, out).
 *
 *   MIDI   IN CHAN    CH 1 .. 6, AUTO (the chosen channel)      OFF, 1 .. 16
 *          OUT CHAN   CH 1 .. 6                                 OFF, 1 .. 16
 *          RECEIVE    CC                                        ON / OFF
 *          SEND       CC                                        ON / OFF
 *          PORTS      USB IN, TRS IN (the 3.5 mm MIDI IN jack)  ON / OFF
 *   AUDIO  USB                                                   STEREO (the output) / CHANNELS (the six, each mono)
 *   SETTINGS  SCREEN READER                                      ON / OFF: the screen's words spoken (voice.c)
 *             SPEECH RATE                                        1 (slowest) .. 5 (fastest)
 *
 * A channel answers to, and sends, these CCs on its channels: 7 LEVEL, 10 PAN, 16 GAIN, 17 FREQ, 18 LIMIT, 19 SEND 1,
 * 74 MIDDLE (peak gain or width), 91 SEND 2, 92 NOISE. */

#define SET_NUM (PRE_NUM + 2u)                   /* (the record's number: after the presets' two) */
#define SET_FORMAT 1u
#define SET_LEN (32u + sizeof(midi_cfg_t))
enum { SK_ONOFF, SK_CHAN, SK_USBA, SK_RATE };   /* (a value: ON / OFF; OFF, 1 .. 16; STEREO / TRACKS; 1 .. 5) */
typedef struct { const char *name; uint8_t off, kind; } set_row_t;
#define SR(n, f, k) {n, (uint8_t)__builtin_offsetof(midi_cfg_t, f), k}
static const set_row_t SET_IN[] = {SR("CH 1", in_ch[0], SK_CHAN), SR("CH 2", in_ch[1], SK_CHAN),
                                   SR("CH 3", in_ch[2], SK_CHAN), SR("CH 4", in_ch[3], SK_CHAN),
                                   SR("CH 5", in_ch[4], SK_CHAN), SR("CH 6", in_ch[5], SK_CHAN),
                                   SR("AUTO", auto_ch, SK_CHAN)};
static const set_row_t SET_OUT[] = {SR("CH 1", out_ch[0], SK_CHAN), SR("CH 2", out_ch[1], SK_CHAN),
                                    SR("CH 3", out_ch[2], SK_CHAN), SR("CH 4", out_ch[3], SK_CHAN),
                                    SR("CH 5", out_ch[4], SK_CHAN), SR("CH 6", out_ch[5], SK_CHAN)};
static const set_row_t SET_RCV[] = {SR("CC", rcv_cc, SK_ONOFF)};
static const set_row_t SET_SND[] = {SR("CC", send_cc, SK_ONOFF)};
static const set_row_t SET_PORT[] = {SR("USB IN", usb_in, SK_ONOFF), SR("TRS IN", trs_in, SK_ONOFF)};
static const set_row_t SET_AUD[] = {SR("USB", usb_aud, SK_USBA)};
static const set_row_t SET_GEN[] = {SR("SCREEN READER", reader, SK_ONOFF), SR("SPEECH RATE", rate, SK_RATE)};
typedef struct { const char *name; const set_row_t *rows; uint8_t n; } set_part_t;
#define SET_NPART 7u             /* (all of them: MIDI's five, then AUDIO's one, SETTINGS' one) */
static const set_part_t SET_PART[SET_NPART] = {
    {"IN CHAN", SET_IN, NELEM(SET_IN)}, {"OUT CHAN", SET_OUT, NELEM(SET_OUT)}, {"RECEIVE", SET_RCV, NELEM(SET_RCV)},
    {"SEND", SET_SND, NELEM(SET_SND)}, {"PORTS", SET_PORT, NELEM(SET_PORT)}, {"AUDIO", SET_AUD, NELEM(SET_AUD)},
    {"SETTINGS", SET_GEN, NELEM(SET_GEN)}};
/* the sections: a name, their parts (first, how many) -- one part: opened on its rows */
static const struct { const char *name; uint8_t p0, np; } SET_SECTION[3] = {{"MIDI", 0, 5}, {"AUDIO", 5, 1},
                                                                            {"SETTINGS", 6, 1}};
#define C_SET RGB(150, 200, 255)
static int32_t set_hi(const set_row_t *r) { return r->kind == SK_CHAN ? 16 : r->kind == SK_RATE ? 4 : 1; }

static struct {
    uint8_t lvl;                 /* 0 the sections, 1 a section's parts, 2 a part's rows */
    uint8_t sec, part, row;      /* (part: the part entered; on its list pc, 0 BACK; row: 0 BACK, then the rows) */
    uint8_t pc;
    uint8_t edit;                /* the focus on the row's value .. */
    int16_t val;                 /* .. as it is being set */
    uint8_t dirty;               /* changed since read */
    uint8_t was;                 /* (the page was up) */
    int32_t shown;
} set;

/* ------------------------------------------------------------ store --- */
static void setup_write(void)
{
    ses_head_t *h = (ses_head_t *)ses_chunk;
    if (!ses.ok)
        return;
    memset(ses_chunk, 0, SET_LEN);
    memcpy(ses_chunk + 32u, &mcfg, sizeof mcfg);
    h->magic = SES_MAGIC;
    h->num = (uint16_t)SET_NUM;
    h->format = SET_FORMAT;
    h->len = SET_LEN;
    h->seq = ++ses.seq;
    h->body_crc = ses_crc(ses_chunk + 32u, SET_LEN - 32u, 0);
    h->head_crc = ses_crc(ses_chunk, 30u, 0);
    if (ses_write_chunk(SET_NUM, SET_LEN) == SES_OK)
        set.dirty = 0;
}
static void setup_load(void)                     /* (the start, a restore: the settings as last left, or the defaults) */
{
    static const midi_cfg_t D = MIDI_CFG0;
    midi_cfg_t c;
    ses_head_t h;
    uint32_t i, r;
    int at;
    mcfg = D;
    if (!ses.ok || (at = ses_find(SET_NUM)) < 0)
        return;
    ses_flash_read(SES_LO + ses.sect[at] * SES_SECT, &h, sizeof h);
    if (h.format != SET_FORMAT || (h.len != SET_LEN && h.len != 32u + MIDI_CFG_OLD && h.len != 32u + MIDI_CFG_AUD &&
                                 h.len != 32u + MIDI_CFG_RDR))
        return;
    c = D;                                       /* (a record from before USB AUDIO or SETTINGS: those at their default) */
    ses_flash_read(SES_LO + ses.sect[at] * SES_SECT + 32u, &c, h.len - 32u);
    if (ses_crc((const uint8_t *)&c, h.len - 32u, 0) != h.body_crc)
        return;
    for (i = 0; i < SET_NPART; i++)              /* (each value in its range, or the default) */
        for (r = 0; r < SET_PART[i].n; r++) {
            uint8_t *v = (uint8_t *)&c + SET_PART[i].rows[r].off;
            if (*v > set_hi(&SET_PART[i].rows[r]))
                *v = *((const uint8_t *)&D + SET_PART[i].rows[r].off);
        }
    mcfg = c;
}

/* ------------------------------------------------------------ page --- */
static uint8_t *set_ptr(const set_row_t *r) { return (uint8_t *)&mcfg + r->off; }
static void set_text(const set_row_t *r, int32_t v, char *b)
{
    if (r->kind == SK_USBA)
        str_cpy(b, v ? "CHANNELS" : "STEREO", 10);
    else if (r->kind == SK_ONOFF)
        str_cpy(b, v ? "ON" : "OFF", 4);
    else if (r->kind == SK_RATE)
        fmt_int(b, v + 1);
    else if (!v)
        str_cpy(b, "OFF", 4);
    else
        fmt_int(b, v);
}
static void setup_open(void)
{
    ui.set_page = 1;
    set.lvl = 0;
    set.edit = 0;
    ui.force = 1;
}
/* SAVE (SHIFT: back) on the page */
static void setup_press(int sh)
{
    ui.force = 1;
    if (sh) {
        if (set.edit)
            set.edit = 0;                        /* (the value as it was) */
        else if (set.lvl == 2u && SET_SECTION[set.sec].np == 1u)
            set.lvl = 0;
        else if (set.lvl)
            set.lvl--;
        else
            ui.set_page = 0;
        return;
    }
    if (set.lvl == 0u && SET_SECTION[set.sec].np == 1u) {   /* (a section of one part: its rows at once) */
        set.lvl = 2;
        set.part = SET_SECTION[set.sec].p0;
        set.row = 1;
        set.edit = 0;
    } else if (set.lvl == 0u) {
        set.lvl = 1;
        set.pc = 1;
    } else if (set.lvl == 1u && !set.pc)         /* (BACK) */
        set.lvl = 0;
    else if (set.lvl == 1u) {
        set.lvl = 2;
        set.part = (uint8_t)(SET_SECTION[set.sec].p0 + set.pc - 1u);
        set.row = 1;
        set.edit = 0;
    } else if (!set.edit && !set.row)            /* (BACK) */
        set.lvl = SET_SECTION[set.sec].np == 1u ? 0u : 1u;
    else if (!set.edit) {
        set.edit = 1;
        set.val = *set_ptr(&SET_PART[set.part].rows[set.row - 1u]);
    } else {
        uint8_t *p = set_ptr(&SET_PART[set.part].rows[set.row - 1u]);
        set.edit = 0;
        if (*p != (uint8_t)set.val) {
            *p = (uint8_t)set.val;
            set.dirty = 1;
        }
    }
}
static void setup_preset(int32_t s)
{
    if (set.lvl == 0u)
        set.sec = (uint8_t)clampi((int32_t)set.sec + s, 0, (int32_t)NELEM(SET_SECTION) - 1);
    else if (set.lvl == 1u)
        set.pc = (uint8_t)clampi((int32_t)set.pc + s, 0, (int32_t)SET_SECTION[set.sec].np);
    else if (!set.edit)
        set.row = (uint8_t)clampi((int32_t)set.row + s, 0, (int32_t)SET_PART[set.part].n);
    else
        set.val = (int16_t)clampi(set.val + s, 0, set_hi(&SET_PART[set.part].rows[set.row - 1u]));
}
/* each pass: the page left (a track's button, SHIFT + SAVE), what changed written */
static void setup_poll(void)
{
#if defined(FELUCCA_UAC) && FELUCCA_UAC
    if (usb.up && (mcfg.usb_aud ? UAC_NMAX : 2u) != uac_nch)
        usb_reconnect(mcfg.usb_aud ? UAC_NMAX : 2u);   /* (USB AUDIO changed, or restored: the new device, at once) */
#endif
    if (ui.set_page)
        set.was = 1;
    else if (set.was) {
        set.was = 0;
        set.edit = 0;
        if (set.dirty)
            setup_write();
    }
}
/* each pass, after the screen: the SCREEN READER on (the whole screen drawn again, for it to read), off, or what
 * changed spoken (voice.c) */
static void reader_tick(void)
{
    static uint8_t was;
    if (ui.rd_flip) {                            /* SHIFT + black key 8: on or off, kept, and said */
        ui.rd_flip = 0;
        mcfg.reader = (uint8_t)!mcfg.reader;
        setup_write();
        sp.intro = mcfg.reader ? 1u : 2u;
    }
    if (mcfg.reader && !was)
        ui.force = 1;
    was = mcfg.reader;
    vo_rate(ui.set_page && set.lvl == 2u && set.edit && set.row &&
            SET_PART[set.part].rows[set.row - 1u].kind == SK_RATE ? (uint32_t)set.val : mcfg.rate);   /* (heard as set) */
    sp_tick(mcfg.reader, fm1_ms);
    if (sp.intro == 2u) {                        /* (off: silent, but for that) */
        sp.intro = 0;
        vo_begin(1);
        vo_text("SCREEN READER OFF");
        vo_end();
        sp_log("SCREEN READER OFF\n");
    }
}
static void set_title(const char *t)
{
    cv_begin(240, 55, C_BLACK);
    cv_text_fit((50 - (int32_t)FONT_S.h) / 2, &FONT_S, t, C_SET);
    cv_rect(0, 52, 240, 3, C_SET);
    cv_blit(0, 0);
}
/* row r of five: a name, and a value at the right (the value being set in a white field) */
static void set_line(uint32_t r, const char *name, const char *val, int chosen, int focus)
{
    int32_t y = (37 - (int32_t)FONT_S.h) / 2;
    cv_begin(240, 37, chosen && !focus ? C_WHITE : C_BLACK);
    if (focus)                                   /* (the field as wide as its value: STEREO, TRACKS; or a long name) */
        cv_rect(val && (text_w(&FONT_S, val) > 64 || (name && text_w(&FONT_S, name) > 148)) ? 224 - text_w(&FONT_S, val)
                                                                                        : 160, 1, 240, 35, C_WHITE);
    if (name) {                                  /* (a name too long beside its value, SCREEN READER: smaller,
                                                  * its letters closer) */
        int32_t room = val ? 220 - text_w(&FONT_S, "OFF") - 8 : 224, n = 0, sq = 0, x = 8, i;
        const font_t *f = text_w(&FONT_S, name) > room ? &FONT_XS : &FONT_S;
        uint16_t fg = chosen && !focus ? C_BLACK : C_WHITE;
        for (i = 0; name[i] && name[i + 1]; i++)  /* (the gaps between letters: a word's space kept) */
            n += name[i] != ' ' && name[i + 1] != ' ';
        if (text_w(f, name) > room && n)
            sq = clampi((text_w(f, name) - room + n - 1) / n, 0, 6);   /* (as cv_text_fit at most) */
        cv_cap((37 - (int32_t)f->h) / 2, f, name, fg);
        for (; *name; name++) {
            char one[2] = {*name, 0};
            x = cv_text_px(x, (37 - (int32_t)f->h) / 2, f, one, fg) - (*name != ' ' && name[1] && name[1] != ' ' ? sq : 0);
        }
    }
    if (val && val[0])
        cv_text(232 - text_w(&FONT_S, val), y, &FONT_S, val, chosen || focus ? C_BLACK : C_SET);
    cv_rect(0, 36, 240, 1, RGB(50, 50, 50));
    cv_blit(0, 55 + 37 * r);
}
static void setup_draw(void)
{
    uint32_t n, sel, top, r;
    int32_t key = set.lvl * 1000000 + set.sec * 100000 + set.part * 10000 + set.pc * 1000 + set.row * 100 +
                  set.edit * 50 + set.val;
    char t[20];
    if (!ui.force && key == set.shown)
        return;
    set.shown = key;
    if (set.lvl == 0u) {
        set_title("GLOBAL SETUP");
        n = NELEM(SET_SECTION);
        sel = set.sec;
    } else if (set.lvl == 1u) {
        set_title(SET_SECTION[set.sec].name);
        n = SET_SECTION[set.sec].np + 1u;        /* (BACK, then the parts) */
        sel = set.pc;
    } else if (SET_SECTION[set.sec].np == 1u) {  /* (a section of one part: its name) */
        set_title(SET_SECTION[set.sec].name);
        n = SET_PART[set.part].n + 1u;
        sel = set.row;
    } else {
        str_cpy(t, SET_SECTION[set.sec].name, sizeof t);
        str_cpy(t + str_len(t), " ", 2);
        str_cpy(t + str_len(t), SET_PART[set.part].name, sizeof t - str_len(t));
        set_title(t);
        n = SET_PART[set.part].n + 1u;           /* (BACK, then the rows) */
        sel = set.row;
    }
    top = n <= 5u || sel < 2u ? 0u : sel > n - 3u ? n - 5u : sel - 2u;
    for (r = 0; r < 5u; r++) {
        uint32_t i = top + r;
        char v[12];
        if (i >= n) {
            set_line(r, 0, 0, 0, 0);
            continue;
        }
        if (set.lvl == 0u)
            set_line(r, SET_SECTION[i].name, 0, i == sel, 0);
        else if (!i)
            set_line(r, "BACK", 0, i == sel, 0);
        else if (set.lvl == 1u)
            set_line(r, SET_PART[SET_SECTION[set.sec].p0 + i - 1u].name, 0, i == sel, 0);
        else {
            const set_row_t *w = &SET_PART[set.part].rows[i - 1u];
            set_text(w, set.edit && i == sel ? set.val : *set_ptr(w), v);
            set_line(r, w->name, v, i == sel, set.edit && i == sel);
        }
    }
}

/* ------------------------------------------------------------ MIDI --- */
#define MCC_N 9u
static const uint8_t MIDI_CC[MCC_N] = {7, 10, 16, 17, 18, 19, 74, 91, 92};
static const uint8_t MCC_P[MCC_N] = {P_LEVEL, P_PAN, P_GAIN, P_MFREQ, P_LIM, P_SEND1, P_MGAIN, P_SEND2, P_NOISE};
static uint8_t mcc_last[NLAYER][MCC_N];          /* (each one's value as last sent or received: no echo) */
static uint32_t mcc_val(uint32_t t, uint32_t i)  /* channel t's parameter i as a CC value, 0 .. 127 */
{
    uint32_t keep = ui.sel, v;
    ui.sel = (uint8_t)t;
    v = MCC_P[i] == P_PAN ? (uint32_t)((p_get(P_PAN) + 500) * 127 / 1000) : (uint32_t)(p_get(MCC_P[i]) * 127 / PMAX);
    ui.sel = (uint8_t)keep;
    return v > 127u ? 127u : v;
}
static void mcc_set(uint32_t t, uint32_t cc, uint32_t v)
{
    uint32_t i, keep = ui.sel;
    for (i = 0; i < MCC_N && MIDI_CC[i] != cc; i++)
        ;
    if (i == MCC_N)
        return;
    ui.sel = (uint8_t)t;
    p_set(MCC_P[i], MCC_P[i] == P_PAN ? (int32_t)v * 1000 / 127 - 500 : (int32_t)v * PMAX / 127);
    ui.sel = (uint8_t)keep;
    mcc_last[t][i] = (uint8_t)mcc_val(t, i);      /* (as it now reads: not sent back) */
    if (t == ui.sel)
        ui.force = 1;
}
/* the MIDI in: by port, then by channel to the channels (each its IN channel; AUTO's to the chosen one). Only CCs:
 * Lilliput plays no notes */
static void midi_take(void)
{
    while (mi_r != mi_w) {
        uint32_t pkt = midi_in_q[mi_r % MQ], cin = pkt & 15u, trs = (pkt >> 4) & 15u;
        uint32_t ch = ((pkt >> 8) & 15u) + 1u, d1 = (pkt >> 16) & 0x7Fu, d2 = (pkt >> 24) & 0x7Fu, m = 0, t;
        RING_PUBLISH();
        mi_r++;
        if (trs ? !mcfg.trs_in : !mcfg.usb_in)
            continue;
        if (mcfg.auto_ch == ch)
            m |= 1u << ui.sel;
        for (t = 0; t < NLAYER; t++)
            if (mcfg.in_ch[t] == ch)
                m |= 1u << t;
        if (cin == 0xBu && mcfg.rcv_cc)
            for (t = 0; t < NLAYER; t++)
                if (m >> t & 1u)
                    mcc_set(t, d1, d2);
    }
}
/* each pass: what changed on the tracks (from the knobs, a session, MIDI) sent as CCs on their OUT channels, a few
 * at a time */
static void midi_poll(void)
{
    uint32_t t, i, n = 0;
    if (!mcfg.send_cc)
        return;
    for (t = 0; t < NLAYER; t++)
        for (i = 0; i < MCC_N; i++) {
            uint32_t v = mcc_val(t, i), ch = mcfg.out_ch[t];
            if (v == mcc_last[t][i])
                continue;
            if (!ch || n >= 8u)
                continue;
            mcc_last[t][i] = (uint8_t)v;
            midi_main_send(0x0Bu | (0xB0u | (ch - 1u)) << 8 | (uint32_t)MIDI_CC[i] << 16 | v << 24);
            n++;
        }
}
