/* SPDX-License-Identifier: GPL-3.0-only
 * LILLIPUT: sessions -- the whole mixer at a moment: every channel's settings and connections, the rack, the output,
 * the channel chosen, the patch's name. Tonitor's store (after SLOOP's projects' area), its records and their safety
 * kept as they were; the record is Lilliput's own (its magic "LIL1": Tonitor's sessions are not read, nor these by
 * Tonitor -- though, sharing the area, each firmware blanks what it does not know).
 *
 * In flash 0x97000 .. 0xE0000, 73 sectors of 4 KiB. A session is one record in one sector:
 *   head (32 B): magic, its number, its length, a sequence number (the newest copy wins), the format, the size of
 *                the mixer's parameters, a CRC of the head and one of the rest
 *   body: the channel chosen, the patch's name, the parameters (lil_par_t); which snapshots are kept, the morph
 *         time, the gesture loop's length and count; each snapshot kept; the loop's start and its changes
 * Saving over a session writes the new copy into a free sector first and only then kills the old one, by
 * programming its magic to zero (no erase): a power cut in between leaves the old or both, never neither.
 * Erasing a sector silences the audio for a moment, so a save takes a sector already blank if it can, and the
 * main loop blanks its own dead sectors one at a time while nothing sounds (ses_idle). Unlike Tonitor's, this store
 * leaves what another firmware keeps here (Tonitor's sessions, SLOOP's projects) alone: such a sector is taken only
 * when a save finds no other room. */

#define SES_LO 0x00097000u
#define SES_HI 0x000E0000u
#define SES_SECT 4096u
#define SES_NSECT ((SES_HI - SES_LO) / SES_SECT)
#define SES_MAGIC 0x314C494Cu    /* "LIL1" */
#define SES_FORMAT 3u           /* 3: eight matrix slots, the snapshots and the gesture loop; 2: LEVEL in tenths of a dB,
                                  * PAN -50 .. 50, each channel's modulation */
#define SES_MAX 73u              /* records in the list (one a sector at most) */
#define SES_BUF (32u + 24u + 12u + (uint32_t)sizeof(lil_par_t) * (2u + NSNAP) + GEV_MAX * 8u)   /* the largest: a
                                  * session with every snapshot and a full gesture loop (~27 KiB, seven sectors) */
#define PRE_NUM 1000u            /* (records 1000, 1001: Tonitor's presets, not used here; 1002: GLOBAL SETUP) */

/* the flash, for the target (main.c) or the host test (host.c); offsets from the flash's start */
static int ses_flash_read(uint32_t off, void *p, uint32_t n);
static int ses_flash_erase(uint32_t off);   /* a 4 KiB sector */
static int ses_flash_prog(uint32_t off, const void *p, uint32_t n);

typedef struct {
    uint32_t magic;
    uint16_t num, format;
    uint32_t len;                /* head and body, bytes */
    uint32_t seq;
    uint16_t par_size, rsv0;     /* the parameters' size (lil_par_t) */
    uint16_t body_crc, rsv1;
    uint16_t rsv2, rsv3;
    uint16_t rsv4, head_crc;     /* CRC of the 30 bytes before it */
} ses_head_t;
typedef char ses_head_is_32[sizeof(ses_head_t) == 32u ? 1 : -1];

static struct {
    uint8_t n;                   /* sessions found */
    uint16_t num[SES_MAX];       /* sorted by number */
    uint8_t sect[SES_MAX];       /* the first sector of each */
    uint8_t nsect[SES_MAX];
    uint32_t seq;                /* the highest sequence number seen */
    uint8_t used[SES_NSECT];     /* a sector holds a live record */
    uint8_t dirty[SES_NSECT];    /* free, but not blank: erase before writing (1 = known dirty, 2 = unknown) */
    uint8_t foreign[SES_NSECT];  /* another firmware's (Tonitor's sessions, SLOOP's projects): kept, taken only when a
                                  * save finds no other room */
    uint16_t current;            /* the session loaded or last saved, 0 = none */
    uint8_t ok;                  /* the flash is ours to use */
} ses;
/* A record goes to and from the flash through one sector's buffer, a sector at a time (the largest session is
 * ~55 KB: no RAM holds it whole): counted first (its length and CRC, for its head), then written; read back
 * through the same buffer. Small records (the presets) are made in it whole */
static uint8_t ses_chunk[SES_SECT];
static uint16_t ses_crc(const uint8_t *p, uint32_t n, uint32_t c);
enum { SS_COUNT, SS_WRITE, SS_READ };
static struct {
    uint8_t mode;
    int err;
    uint32_t base, pos;          /* the record's place in the flash; bytes so far */
    uint16_t crc;                /* of its body (after the head) so far */
    uint32_t window;             /* SS_READ: the part of the record in ses_chunk (its offset), ~0: none */
} ss;
static void ss_begin(uint32_t mode, uint32_t base)
{
    ss.mode = (uint8_t)mode;
    ss.err = 0;
    ss.base = base;
    ss.pos = 0;
    ss.crc = 0;
    ss.window = 0xFFFFFFFFu;
}
static void ss_crc(const uint8_t *b, uint32_t n)       /* (the head is not the body's) */
{
    if (ss.pos + n <= sizeof(ses_head_t))
        return;
    if (ss.pos < sizeof(ses_head_t)) {
        b += sizeof(ses_head_t) - ss.pos;
        n -= sizeof(ses_head_t) - ss.pos;
    }
    ss.crc = ses_crc(b, n, ss.crc);
}
/* n bytes out: counted, or into the sector's buffer, a full one written */
static void ses_out(const void *src, uint32_t n)
{
    const uint8_t *b = src;
    while (n) {
        uint32_t at = ss.pos % SES_SECT, k = SES_SECT - at < n ? SES_SECT - at : n;
        ss_crc(b, k);
        if (ss.mode == SS_WRITE) {
            memcpy(ses_chunk + at, b, k);
            if (at + k == SES_SECT && ses_flash_prog(ss.base + ss.pos + k - SES_SECT, ses_chunk, SES_SECT))
                ss.err = 1;
        }
        ss.pos += k;
        b += k;
        n -= k;
    }
}
static void ss_end(void)                               /* (the last part of a sector) */
{
    uint32_t at = ss.pos % SES_SECT;
    if (ss.mode == SS_WRITE && at && ses_flash_prog(ss.base + ss.pos - at, ses_chunk, at))
        ss.err = 1;
}
/* n bytes in (dst 0: passed over), through the sector's buffer */
static void ses_in(void *dst, uint32_t n)
{
    uint8_t *d = dst;
    while (n) {
        uint32_t w = ss.pos / SES_SECT * SES_SECT, at = ss.pos - w, k = SES_SECT - at < n ? SES_SECT - at : n;
        if (ss.window != w) {
            ses_flash_read(ss.base + w, ses_chunk, SES_SECT);
            ss.window = w;
        }
        ss_crc(ses_chunk + at, k);
        if (d) {
            memcpy(d, ses_chunk + at, k);
            d += k;
        }
        ss.pos += k;
        n -= k;
    }
}

static uint16_t ses_crc(const uint8_t *p, uint32_t n, uint32_t c)   /* CRC16, poly 0x1021 */
{
    uint32_t i;
    while (n--) {
        c ^= (uint32_t)*p++ << 8;
        for (i = 0; i < 8u; i++)
            c = c & 0x8000u ? (c << 1) ^ 0x1021u : c << 1;
        c &= 0xFFFFu;
    }
    return (uint16_t)c;
}

static int ses_head_ok(const ses_head_t *h)
{
    return h->magic == SES_MAGIC && h->len >= sizeof(ses_head_t) && h->len <= SES_BUF &&
           h->head_crc == ses_crc((const uint8_t *)h, 30u, 0);
}

static void ses_list_add(uint32_t num, uint32_t sect, uint32_t nsect)
{
    uint32_t i = ses.n, k;
    if (ses.n >= SES_MAX)
        return;
    while (i > 0 && ses.num[i - 1u] > num)
        i--;
    for (k = ses.n; k > i; k--) {
        ses.num[k] = ses.num[k - 1u];
        ses.sect[k] = ses.sect[k - 1u];
        ses.nsect[k] = ses.nsect[k - 1u];
    }
    ses.num[i] = (uint16_t)num;
    ses.sect[i] = (uint8_t)sect;
    ses.nsect[i] = (uint8_t)nsect;
    ses.n++;
}
static int ses_find(uint32_t num)
{
    uint32_t i;
    for (i = 0; i < ses.n; i++)
        if (ses.num[i] == num)
            return (int)i;
    return -1;
}
/* kill a record in place: its magic programmed to zero (bits only go 1 -> 0: no erase) */
static void ses_kill(uint32_t sect, uint32_t nsect)
{
    uint32_t zero = 0, k;                   /* (in RAM: the flash is programmed from RAM only) */
    ses_flash_prog(SES_LO + sect * SES_SECT, &zero, 4u);
    for (k = 0; k < nsect && sect + k < SES_NSECT; k++) {
        ses.used[sect + k] = 0;
        ses.dirty[sect + k] = 1;
    }
}

/* boot: the records in the area. Two copies of a number (a save cut short): the newer one stays */
static void ses_scan(int flash_ok)
{
    uint32_t s = 0;
    memset(&ses, 0, sizeof ses);
    ses.ok = (uint8_t)flash_ok;
    if (!flash_ok)
        return;
    while (s < SES_NSECT) {
        ses_head_t h;
        uint32_t ns;
        ses_flash_read(SES_LO + s * SES_SECT, &h, sizeof h);
        if (h.magic == 0xFFFFFFFFu) {                   /* blank, or so its start says: checked when used */
            ses.dirty[s] = 2;
            s++;
            continue;
        }
        if (!ses_head_ok(&h) && !(h.magic == 0 && h.len >= sizeof h && h.len <= SES_BUF)) {
            ses.foreign[s] = 1;                          /* anything else (another firmware's): left as it is */
            ses.dirty[s++] = 1;
            continue;
        }
        ns = (h.len + SES_SECT - 1u) / SES_SECT;
        if (h.magic == SES_MAGIC) {
            int dup = ses_find(h.num);
            if (h.seq > ses.seq)
                ses.seq = h.seq;
            if (dup >= 0) {                              /* keep the newer */
                ses_head_t o;
                ses_flash_read(SES_LO + ses.sect[dup] * SES_SECT, &o, sizeof o);
                if (o.seq > h.seq) {
                    ses_kill(s, ns);
                    s += ns;
                    continue;
                }
                ses_kill(ses.sect[dup], ses.nsect[dup]);
                for (; dup + 1 < ses.n; dup++) {
                    ses.num[dup] = ses.num[dup + 1];
                    ses.sect[dup] = ses.sect[dup + 1];
                    ses.nsect[dup] = ses.nsect[dup + 1];
                }
                ses.n--;
            }
            ses_list_add(h.num, s, ns);
            for (; ns && s < SES_NSECT; ns--, s++)
                ses.used[s] = 1;
        } else {                                         /* a dead record: dirty */
            for (; ns && s < SES_NSECT; ns--, s++)
                ses.dirty[s] = 1;
        }
    }
}

/* a sector is blank (all 0xFF) */
static int ses_blank(uint32_t s)
{
    uint32_t buf[64], o, i;
    for (o = 0; o < SES_SECT; o += sizeof buf) {
        ses_flash_read(SES_LO + s * SES_SECT + o, buf, sizeof buf);
        for (i = 0; i < 64u; i++)
            if (buf[i] != 0xFFFFFFFFu)
                return 0;
    }
    return 1;
}
static int ses_is_clean(uint32_t s)
{
    if (ses.dirty[s] == 2u)
        ses.dirty[s] = (uint8_t)(ses_blank(s) ? 0 : 1);
    return !ses.dirty[s];
}

/* where n sectors fit: a free run, a blank one if there is one (no erase, no gap in the sound) */
static int ses_alloc(uint32_t n)
{
    uint32_t s, k, best = SES_NSECT, best_dirty = 0xFFFFu, pass;
    for (pass = 0; pass < 2u && best == SES_NSECT; pass++)   /* (another firmware's sectors: only if nothing else) */
    for (s = 0; s + n <= SES_NSECT; s++) {
        uint32_t d = 0;
        for (k = 0; k < n && !ses.used[s + k] && (pass || !ses.foreign[s + k]); k++)
            d += !ses_is_clean(s + k);
        if (k < n)
            continue;
        if (d < best_dirty) {
            best = s;
            best_dirty = d;
            if (!d)
                break;
        }
    }
    return best < SES_NSECT ? (int)best : -1;
}

static void ses_head_fill(ses_head_t *h, uint32_t num, uint32_t format, uint32_t len, uint16_t crc)
{
    memset(h, 0, sizeof *h);
    h->magic = SES_MAGIC;
    h->num = (uint16_t)num;
    h->format = (uint16_t)format;
    h->len = len;
    h->seq = ++ses.seq;
    h->par_size = (uint16_t)sizeof(lil_par_t);
    h->body_crc = crc;
    h->head_crc = ses_crc((const uint8_t *)h, 30u, 0);
}

/* room for a record of len bytes: free sectors, blank (erased now if they must be), marked dirty until it is
 * written and read back; the first, or -SES_FULL / -SES_FAIL */
static int ses_place(uint32_t len, uint32_t *nsect)
{
    uint32_t ns = (len + SES_SECT - 1u) / SES_SECT, k;
    int s = ses_alloc(ns);
    *nsect = ns;
    if (s < 0)
        return -SES_FULL;
    for (k = 0; k < ns; k++)
        if (!ses_is_clean((uint32_t)s + k)) {
            if (ses_flash_erase(SES_LO + ((uint32_t)s + k) * SES_SECT))
                return -SES_FAIL;
            ses.dirty[(uint32_t)s + k] = 0;
            ses.foreign[(uint32_t)s + k] = 0;
        }
    for (k = 0; k < ns; k++)                            /* (any write marks them dirty if it fails) */
        ses.dirty[(uint32_t)s + k] = 1;
    return s;
}
/* the record written at sector s is number num's now: its head read back, and only then the old copy of num
 * killed (a power cut leaves the old, or both: never neither) */
static int ses_commit(uint32_t num, int s, uint32_t ns, const ses_head_t *head)
{
    uint32_t k;
    int old;
    ses_head_t h;
    ses_flash_read(SES_LO + (uint32_t)s * SES_SECT, &h, sizeof h);
    if (memcmp(&h, head, sizeof h))
        return SES_FAIL;
    for (k = 0; k < ns; k++) {
        ses.used[(uint32_t)s + k] = 1;
        ses.dirty[(uint32_t)s + k] = 0;
    }
    old = ses_find(num);
    if (old >= 0) {                                     /* the old copy goes now, the new one is safe */
        ses_kill(ses.sect[old], ses.nsect[old]);
        ses.sect[old] = (uint8_t)s;
        ses.nsect[old] = (uint8_t)ns;
    } else
        ses_list_add(num, (uint32_t)s, ns);
    return SES_OK;
}
/* a record made whole in ses_chunk (its head filled; at most a sector) written as number num */
static int ses_write_chunk(uint32_t num, uint32_t len)
{
    ses_head_t h;
    uint32_t ns;
    int s = ses_place(len, &ns);
    if (s < 0)
        return -s;
    memcpy(&h, ses_chunk, sizeof h);
    if (ses_flash_prog(SES_LO + (uint32_t)s * SES_SECT, ses_chunk, len))
        return SES_FAIL;
    return ses_commit(num, s, ns, &h);
}


/* ------------------------------------------------------------ the body --- */
typedef struct {
    uint8_t sel, rsv[3];
    char patch[16];
    uint32_t rsv2;
} ses_glob_t;
typedef struct {
    uint16_t snap_used;
    uint8_t morph_hs, rsv;
    uint16_t gn, rsv2;           /* the gesture loop's changes (0: none) */
    uint32_t glen;               /* .. its length, ms */
} ses_perf_t;
static void ses_emit(void)
{
    ses_glob_t g;
    memset(&g, 0, sizeof g);
    g.sel = ui.sel;
    g.rsv[0] = ui.fam;                           /* (the family: SHIFT + HOME draws again from it) */
    str_cpy(g.patch, ui.patch, sizeof g.patch);
    ses_out(&g, sizeof g);
    ses_out(&par, sizeof par);
    {
        ses_perf_t pf;
        uint32_t i;
        memset(&pf, 0, sizeof pf);
        pf.snap_used = snap_used;
        pf.morph_hs = morph_hs;
        pf.gn = gs.st == GS_REC ? 0u : gs.n;     /* (a loop being recorded is not kept) */
        pf.glen = gs.len;
        ses_out(&pf, sizeof pf);
        for (i = 0; i < NSNAP; i++)
            if (snap_has(i))
                ses_out(&snap[i], sizeof snap[i]);
        if (pf.gn) {
            ses_out(&g_s0, sizeof g_s0);
            ses_out(gev, pf.gn * (uint32_t)sizeof gev[0]);
        }
    }
}
static int ses_save(uint32_t num)
{
    ses_head_t h;
    uint32_t len, ns;
    uint16_t crc;
    int s;
    if (!ses.ok)
        return SES_NOFLASH;
    if (num < 1u || num > SES_SLOTS)
        return SES_FAIL;
    ss_begin(SS_COUNT, 0);
    ss.pos = sizeof h;
    ses_emit();
    len = ss.pos;
    crc = ss.crc;
    ses_head_fill(&h, num, SES_FORMAT, len, crc);
    s = ses_place(len, &ns);
    if (s < 0)
        return -s;
    ss_begin(SS_WRITE, SES_LO + (uint32_t)s * SES_SECT);
    ses_out(&h, sizeof h);
    ses_emit();
    ss_end();
    if (ss.err)
        return SES_FAIL;
    {
        int rc = ses_commit(num, s, ns, &h);
        if (rc == SES_OK)
            ses.current = (uint16_t)num;
        return rc;
    }
}
/* session num into the mixer: read whole, checked (its CRC, its parameters' size), only then used */
static int ses_load(uint32_t num)
{
    static lil_par_t q;
    ses_head_t h;
    ses_glob_t g;
    ses_perf_t pf;
    uint32_t i, ns = 0, pass, base;
    int at = ses_find(num);
    if (!ses.ok)
        return SES_NOFLASH;
    if (at < 0)
        return SES_FAIL;
    base = SES_LO + ses.sect[at] * SES_SECT;
    for (pass = 0; pass < 2u; pass++) {          /* the whole record checked first (its CRC), only then taken */
        ss_begin(SS_READ, base);
        ses_in(&h, sizeof h);
        if (!ses_head_ok(&h) || h.format != SES_FORMAT || h.par_size != sizeof(lil_par_t))
            return SES_FAIL;
        ses_in(&g, sizeof g);
        ses_in(&q, sizeof q);
        ses_in(&pf, sizeof pf);
        if (pf.gn > GEV_MAX)
            return SES_FAIL;
        for (i = 0, ns = 0; i < NSNAP; i++)
            if (pf.snap_used >> i & 1u) {
                ses_in(pass ? &snap[i] : 0, sizeof snap[i]);
                ns++;
            }
        if (pf.gn) {
            ses_in(pass ? &g_s0 : 0, sizeof g_s0);
            ses_in(pass ? gev : 0, pf.gn * (uint32_t)sizeof gev[0]);
        }
        if (ss.pos != h.len || ss.crc != h.body_crc)
            return SES_FAIL;
    }
    g.patch[sizeof g.patch - 1u] = 0;
    ui.sel = (uint8_t)(g.sel < NCH ? g.sel : 0u);
    ui.fam = (uint8_t)(g.rsv[0] < NFAM ? g.rsv[0] : 0u);
    for (i = 0; i < NCH; i++)
        mod_check(&q.ch[i].mod);
    for (i = 0; i < NSNAP; i++)
        if (pf.snap_used >> i & 1u) {
            uint32_t c;
            for (c = 0; c < NCH; c++)
                mod_check(&snap[i].ch[c].mod);
        }
    snap_used = pf.snap_used;
    snap_cur = -1;
    morph_hs = (uint8_t)(pf.morph_hs <= 60u ? pf.morph_hs : 0u);
    patch_set(&q, g.patch);
    gs.st = GS_IDLE;                             /* the session's gesture loop, stopped (PLAY plays it) */
    gs.play = 0;
    gs.n = pf.gn;
    gs.len = pf.glen ? pf.glen : 200u;
    memset(g_touched, 0, sizeof g_touched);
    for (i = 0; i < gs.n; i++)
        if (gev[i].w < NWORD)
            g_touched[gev[i].w >> 3] |= (uint8_t)(1u << (gev[i].w & 7u));
        else
            gs.n = 0;                            /* (a change beyond the mixer: the loop not trusted) */
    (void)ns;
    ses.current = (uint16_t)num;
    return SES_OK;
}
static uint32_t ses_next_number(void)
{
    uint32_t n;
    for (n = 1; n <= SES_SLOTS; n++)
        if (ses_find(n) < 0)
            return n;
    return 0;
}
static int ses_has(uint32_t num) { return ses_find(num) >= 0; }
static int ses_current(void) { return ses.current; }

/* the main loop, now and then: one free dirty sector blanked, while nothing sounds (the erase silences the
 * audio for some tens of ms) */
static uint32_t ses_idle_at;
static void ses_idle(int quiet)
{
    uint32_t s;
    if (!ses.ok || !quiet || (uint32_t)(fm1_ms - ses_idle_at) < 1000u)
        return;
    ses_idle_at = fm1_ms;
    for (s = 0; s < SES_NSECT; s++)
        if (!ses.used[s] && !ses.foreign[s] && ses.dirty[s] == 1u) {
            if (!ses_flash_erase(SES_LO + s * SES_SECT))
                ses.dirty[s] = 0;
            return;
        }
}
