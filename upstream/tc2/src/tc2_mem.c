/*
 * tc2_mem.c -- the R4650's view of System 23 memory, for the lifted program. Every load and store of the generated code lands here with the
 * CPU's (virtual) address and width. KSEG0 / KSEG1 (and, without a TLB model, every other segment) map to the physical address & 0x1FFFFFFF.
 *   0x00000000-0x00FFFFFF  main RAM (16 MB)
 *   0x08000000-0x09FFFFFF  the data ROMs, first 16 MB (mirrored): tss1mtah.2j / tss1mtal.2h byte-interleaved
 *   0x0A000000-0x0BFFFFFF  the data ROMs, second 16 MB (mirrored): tss1mtbh.2m / tss1mtbl.2f byte-interleaved
 *                          (MAME's "data" region, matched byte for byte against these files)
 *   0x0FC00000-0x0FFFFFFF  the boot ROM (boot.bin: tss3verb.2/.1 byte-interleaved). The board decodes 28 address lines (MAME's map), so the
 *                          CPU's 0x1FC00000 (the reset vector, KSEG1 0xBFC00000) and 0x0FC00000 (KSEG1 0xAFC00000, which the monitor reads) both land here
 *   0x06000000.. 0x06A3FFFF the RAMs only this CPU writes (vram_win below: backup RAM, C422 RAM, C361 text, C404 palette): memory
 *   everything else        I/O (docs/mame_address_maps.txt) -- NOT MODELLED YET: every access is logged (address, width, value, PC) and a read
 *                          returns 0, so the first runs list exactly which devices the program touches.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lift_rt.h"
#include "tc2_host.h"
#include "tc2_game.h"

uint8_t tc2_ram[0x1000000];
uint8_t tc2_boot[0x800000];                   /* the boot ROM, ending at the top of the decoded space: tc2_game.boot_size of it */
uint8_t tc2_data[0x2000000];

/* the data ROMs from the ROM files in extracted/ (tools/setup: the MAME zip unpacked) */
int tc2_load_data(const char *dir)
{
    for (int h = 0; h < 2; h++) {
        uint8_t *chip[2];
        for (int c = 0; c < 2; c++) {
            char p[1024]; snprintf(p, sizeof p, "%s/%s", dir, tc2_game.data_rom[h][c]);
            FILE *f = fopen(p, "rb"); chip[c] = malloc(0x800000);
            if (!f || fread(chip[c], 1, 0x800000, f) != 0x800000) { fprintf(stderr, "[TC2] cannot read %s\n", p); if (f) fclose(f); return 0; }
            fclose(f);
        }
        for (uint32_t k = 0; k < 0x800000; k++) { tc2_data[h * 0x1000000 + 2 * k] = chip[0][k]; tc2_data[h * 0x1000000 + 2 * k + 1] = chip[1][k]; }
        free(chip[0]); free(chip[1]);
    }
    return 1;
}
static FILE *io_log;
static unsigned long io_n;

static uint32_t be_rd(const uint8_t *p, int n) { uint32_t v = 0; for (int i = 0; i < n; i++) v = v << 8 | p[i]; return v; }
static void be_wr(uint8_t *p, int n, uint32_t v) { for (int i = n - 1; i >= 0; i--) { p[i] = (uint8_t)v; v >>= 8; } }

static void io(const char *rw, uint32_t a, int size, uint32_t v)
{
    static int tried; static unsigned from; static unsigned long max = 2000000; static uint32_t skip[8]; static int nskip;
    /* only when asked for ($TC2_IOLOG): a played session writes no log. $TC2_IOLOG_FROM = a frame, $TC2_IOLOG_MAX = lines (default 2M),
     * $TC2_IOLOG_SKIP = up to 8 comma-separated physical addresses, & 0x0FFFFFF0, never logged (a RAM test's port, the beam polls) */
    if (!io_log && !tried) {
        tried = 1; const char *p = getenv("TC2_IOLOG"); if (p) io_log = fopen(p, "w");
        if ((p = getenv("TC2_IOLOG_FROM"))) from = (unsigned)atoi(p);
        if ((p = getenv("TC2_IOLOG_MAX"))) max = strtoul(p, NULL, 0);
        for (p = getenv("TC2_IOLOG_SKIP"); p && *p && nskip < 8; ) { char *e; skip[nskip++] = (uint32_t)strtoul(p, &e, 16) & 0x0FFFFFF0u; p = *e ? e + 1 : e; }
    }
    if (!io_log || rr_frame < from) return;
    for (int i = 0; i < nskip; i++) if ((a & 0x0FFFFFF0u) == skip[i]) return;
    { uint32_t tc2_vpos_dbg(void);
      extern uint64_t tc2_cycles_now(void);
      { void tc2_trace_mark(const char *, uint32_t); tc2_trace_mark("IO", a); tc2_trace_mark("CY", (uint32_t)tc2_cycles_now()); }
      if (io_n++ < max) fprintf(io_log, "%u %s %08X %d %08X pc %08X line %u cyc %llu\n", rr_frame, rw, a, size, v, rr_pc, tc2_vpos_dbg(), (unsigned long long)tc2_cycles_now()); }
}

/* THE ENVIRONMENT ($TC2_ENV, tools/mame/ioread_from.lua): MAME's device reads for the traced stretch, "IOR addr bits value pc", replayed in order
 * -- the devices are not modelled yet, so for an instruction match the program gets exactly what MAME's CPU got. A read takes the next
 * record at its address (looking a few records ahead, so one extra or missing read does not derail the rest); none = 0, counted. */
typedef struct { uint32_t a, v, pc; int used; } env_rec;
static env_rec *env; static size_t env_n, env_pos;
unsigned long tc2_env_hits, tc2_env_miss;
static void env_load(void)
{
    static int done; if (done) return; done = 1;
    const char *p = getenv("TC2_ENV"); if (!p) return;
    FILE *f = fopen(p, "r"); if (!f) { fprintf(stderr, "[TC2] cannot open %s\n", p); return; }
    char tag[8]; unsigned a, bits, v, pc; size_t cap = 0;
    while (fscanf(f, "%7s %x %x %x %x", tag, &a, &bits, &v, &pc) == 5) {
        { int tc2_chips_owns(uint32_t);
          int tc2_screen_owns(uint32_t);
          if ((a >= 0x0D000002u && a <= 0x0D000007u) || tc2_chips_owns(a) || tc2_screen_owns(a) || (a >= 0x06A08000u && a <= 0x06A087FFu)) continue; }   /* MODELLED (src/tc2_ctl.c, src/tc2_chips.c): their
                                                                    * records are not replayed, or they would sit unconsumed in front of the look-ahead window */
        if (env_n == cap) { cap = cap ? cap * 2 : 4096; env = realloc(env, cap * sizeof *env); }
        env[env_n++] = (env_rec){ a, v, pc, 0 };
    }
    fclose(f);
    fprintf(stderr, "[TC2] %zu device reads to replay from %s\n", env_n, p);
}
static int env_take(uint32_t p28, uint32_t *v)
{
    env_load();
    if (!env_n) return 0;                                  /* no replay loaded (every played session): nothing to count as missing */
    for (size_t k = env_pos; k < env_n && k < env_pos + 16; k++)
        if (!env[k].used && env[k].a == p28) {
            env[k].used = 1; *v = env[k].v;
            while (env_pos < env_n && env[env_pos].used) env_pos++;
            tc2_env_hits++; return 1;
        }
    tc2_env_miss++; return 0;
}

/* THE RAMS only this CPU writes (MAME's mips_map for System 23, namcos23.cpp: .ram()): plain memory here, never replayed. NOT 0x04400000, the
 * RAM shared with the H8 sub-CPU (C416), which it writes too: below. */
static const struct { uint32_t lo, hi; } vram_win[] = {
    { 0x06000000u, 0x0600FFFFu },   /* the battery-backed BACKUP RAM ("nvram": settings, rankings) -- to be kept in a file */
    { 0x06200000u, 0x06203FFFu },   /* C422 RAM */
    { 0x06800000u, 0x0681FFFFu },   /* C361 text layer: character RAM 0x06800000-0x0681DFFF, tile map 0x0681E000-0x0681FFFF */
    { 0x06A10000u, 0x06A3FFFFu },   /* C404 palette RAM */
    { 0x06A08000u, 0x06A087FFu },   /* C404 registers: 0x400 words, every named register (fade, palette base, alpha ...) lands here too */
};
static uint8_t vram_mem[sizeof vram_win / sizeof vram_win[0]][0x30000];
/* a snapshot's video RAMs (tools/mame/snapshot.lua writes SNAP.vram: the windows above in order, each its own size) */
int tc2_vram_load(const char *path)
{
    FILE *f = fopen(path, "rb"); if (!f) return 0;
    size_t got = 0;
    for (size_t i = 0; i < sizeof vram_win / sizeof vram_win[0]; i++) got += fread(vram_mem[i], 1, vram_win[i].hi - vram_win[i].lo + 1, f);
    fclose(f);
    fprintf(stderr, "[TC2] %zu bytes of video RAM from %s\n", got, path);
    return 1;
}
uint8_t *tc2_vram_ptr(uint32_t p)     /* for the renderers (src/tc2_text.c): a byte of one of the windows, NULL outside */
{
    for (size_t i = 0; i < sizeof vram_win / sizeof vram_win[0]; i++)
        if (p >= vram_win[i].lo && p <= vram_win[i].hi) return vram_mem[i] + (p - vram_win[i].lo);
    return NULL;
}
/* THE BACKUP RAM IN A FILE (the board's battery: settings, coin options, rankings). The game keeps them in the first window above
 * (0x06000000-0x0600FFFF); MAME saves it as nvram. Ours: tc2_backup.nv beside tc2.cfg -- "TC2BKUP1", the 64 KB, a BE32 byte sum --
 * loaded at power-on in a WINDOW run only (a headless run starts blank, so every gate stays what it measured, unless $TC2_BACKUP names a
 * file), written through a temp file + rename when it changed (checked every 300 frames) and on exit. A file with a bad size or sum is
 * ignored, never trusted. */
static const char *backup_path; static uint8_t backup_saved[0x10000]; static int backup_on;
void tc2_backup_load(int window)
{
    const char *e = getenv("TC2_BACKUP");
    static char def[64]; snprintf(def, sizeof def, "%s_backup.nv", tc2_game.tag);
    backup_path = e && *e ? e : window ? def : NULL;
    if (!backup_path) return;
    backup_on = 1;
    uint8_t *m = vram_mem[0];
    FILE *f = fopen(backup_path, "rb");
    if (!f) { fprintf(stderr, "[NV] no %s yet: the backup RAM starts blank (the game sets its defaults)\n", backup_path); memcpy(backup_saved, m, sizeof backup_saved); return; }
    uint8_t hdr[8], sum4[4], buf[0x10000];
    const int ok = fread(hdr, 1, 8, f) == 8 && !memcmp(hdr, "TC2BKUP1", 8) && fread(buf, 1, sizeof buf, f) == sizeof buf && fread(sum4, 1, 4, f) == 4;
    fclose(f);
    uint32_t sum = 0; for (size_t i = 0; i < sizeof buf; i++) sum += buf[i];
    if (!ok || sum != ((uint32_t)sum4[0] << 24 | sum4[1] << 16 | sum4[2] << 8 | sum4[3])) {
        fprintf(stderr, "[NV] %s is damaged or not ours: ignored (the backup RAM starts blank)\n", backup_path);
        memcpy(backup_saved, m, sizeof backup_saved); return;
    }
    memcpy(m, buf, sizeof buf); memcpy(backup_saved, buf, sizeof buf);
    fprintf(stderr, "[NV] backup RAM loaded from %s\n", backup_path);
}
int tc2_crashing;                                          /* set by a fatal stop (src/tc2_cpu.c): never save then */
void tc2_backup_save(int force)
{
    if (!backup_on || tc2_crashing) return;
    const uint8_t *m = vram_mem[0];
    (void)force;                                            /* periodic and at exit alike: only when it changed */
    if (!memcmp(m, backup_saved, sizeof backup_saved)) return;
    char tmp[1100]; snprintf(tmp, sizeof tmp, "%s.tmp", backup_path);
    FILE *f = fopen(tmp, "wb");
    if (!f) { fprintf(stderr, "[NV] cannot write %s\n", tmp); backup_on = 0; return; }
    uint32_t sum = 0; for (size_t i = 0; i < sizeof backup_saved; i++) sum += m[i];
    const uint8_t s4[4] = { (uint8_t)(sum >> 24), (uint8_t)(sum >> 16), (uint8_t)(sum >> 8), (uint8_t)sum };
    const int ok = fwrite("TC2BKUP1", 1, 8, f) == 8 && fwrite(m, 1, sizeof backup_saved, f) == sizeof backup_saved && fwrite(s4, 1, 4, f) == 4;
    if (fclose(f) != 0 || !ok) { remove(tmp); fprintf(stderr, "[NV] writing %s failed\n", tmp); return; }
#ifdef _WIN32
    remove(backup_path);                                   /* Windows' rename does not replace */
#endif
    if (rename(tmp, backup_path) != 0) { fprintf(stderr, "[NV] cannot replace %s\n", backup_path); return; }
    memcpy(backup_saved, m, sizeof backup_saved);
    fprintf(stderr, "[NV] backup RAM saved to %s\n", backup_path);
}
static uint8_t *vram(uint32_t p, int size)
{
    for (size_t i = 0; i < sizeof vram_win / sizeof vram_win[0]; i++)
        if (p >= vram_win[i].lo && p + (uint32_t)size - 1 <= vram_win[i].hi) return vram_mem[i] + (p - vram_win[i].lo);
    return NULL;
}

/* THE RAM SHARED WITH THE H8 SUB-CPU (0x04400000-0x0440FFFF): memory here, written by this CPU; the H8 writes it too, and until its program is
 * translated what it wrote reaches us only through the replay -- so a read takes MAME's recorded value when there is one (and keeps it),
 * otherwise what is in the RAM (the POST's write-then-read-back test, and anything the H8 did not touch). */
static uint8_t shared_ram[0x10000];
uint8_t *tc2_shared_ram(void) { return shared_ram; }   /* the sub-CPU's 0x080000 (src/tc2_h8.c) */

static int c404log(void) { static int v = -1; if (v < 0) v = getenv("TC2_C404LOG") != NULL; return v; }
/* SYSTEM 23 EVOLUTION 2 (tc2_game.evo2; MAME crszone_state::mips_map): 29 address lines, and the System 23 chips at +0x10000000 -- folded
 * onto System 23's addresses here, so every chip model below serves both boards. Evolution 2's own registers are answered first, unfolded:
 *   0x10000058 W  ignored            0x10000080 W  vblank interrupt acknowledge
 *   0x100000A8 R  IRQ1 status: bit 0 sub-CPU, bit 1 C422     0x100000B0 R  IRQ3 status: bit 0 C451, bit 1 C361     0x100000B4 R  IRQ4 status: bit 1 C450
 *                 (reading acknowledges the sources it reports)
 *   0x10400000 W  C450 DMA address   0x1040000C W  C450 DMA size: starts it (src/tc2_c435.c)     0x1040001C R  C450 interrupt pending
 *   0x1F000000-3  the debug UART (MC6850; nothing is attached on the cabinet): reads 0, writes ignored */
static uint32_t irq_status(uint32_t p)
{
    static const struct { uint32_t a; int src[2]; } reg[] = {
        { 0x100000A8u, { TC2_SRC_SUB, TC2_SRC_C422 } }, { 0x100000B0u, { TC2_SRC_C451, TC2_SRC_C361 } }, { 0x100000B4u, { -1, TC2_SRC_C450 } } };
    for (size_t i = 0; i < sizeof reg / sizeof reg[0]; i++) {
        if (reg[i].a != p) continue;
        uint32_t v = 0;
        for (int b = 0; b < 2; b++) if (reg[i].src[b] >= 0 && tc2_irq_is(reg[i].src[b])) { v |= 1u << b; tc2_irq_set(reg[i].src[b], 0); }
        return v;
    }
    return 0;
}
static int evo2_read(uint32_t p, int size, uint32_t *v)
{
    (void)size;
    if (p >= 0x100000A8u && p <= 0x100000B7u) { *v = irq_status(p & ~3u); return 1; }
    if (p >= 0x1040001Cu && p <= 0x1040001Fu) { *v = tc2_irq_is(TC2_SRC_C450) ? 1 : 0; return 1; }
    if (p >= 0x1F000000u && p <= 0x1F000003u) { *v = 0; return 1; }
    return 0;
}
static int evo2_write(uint32_t p, int size, uint32_t v)
{
    (void)size;
    int tc2_c450_write(uint32_t, uint32_t);
    if (p >= 0x10000080u && p <= 0x10000083u) { tc2_irq_set(TC2_SRC_VBLANK, 0); return 1; }
    if (p >= 0x10000058u && p <= 0x1000005Bu) return 1;
    if (tc2_c450_write(p, v)) return 1;
    if (p >= 0x1F000000u && p <= 0x1F000003u) return 1;
    return 0;
}
/* the physical address on this board: System 23 decodes 28 lines (the boot ROM at 0x0FC00000), Evolution 2 29 (at 0x1F800000) */
static inline uint32_t phys(uint32_t a) { return a & (tc2_game.evo2 ? 0x1FFFFFFFu : 0x0FFFFFFFu); }
static inline uint32_t boot_base(void) { return (tc2_game.evo2 ? 0x20000000u : 0x10000000u) - tc2_game.boot_size; }

uint32_t rr_read(uint32_t a, int size)
{
    uint32_t p = phys(a);
    if (p + (uint32_t)size <= sizeof tc2_ram) return be_rd(tc2_ram + p, size);
    if (p >= boot_base()) return p - boot_base() + (uint32_t)size <= tc2_game.boot_size ? be_rd(tc2_boot + (p - boot_base()), size) : 0;
    if (tc2_game.evo2 && p >= 0x10000000u) {                  /* Evolution 2: its own registers, else the System 23 chip 0x10000000 below */
        uint32_t v; if (evo2_read(p, size, &v)) { io("Re", a, size, v); return v; }
        if (p < 0x11000000u) { io("Ru", a, size, 0); return 0; }
        p -= 0x10000000u;
    }
    if (p >= 0x04400000u && p + (uint32_t)size <= 0x04410000u) {
        uint8_t *m = shared_ram + (p - 0x04400000u);
        uint32_t v;
        if (env_take(p, &v)) { be_wr(m, size, v); io("R", a, size, v); return v; }
        v = be_rd(m, size); io("Rm", a, size, v); return v;
    }
    { uint8_t *m = vram(p, size); if (m) return be_rd(m, size); }
    if (p >= 0x08000000u && p < 0x0C000000u) {                      /* the data ROMs: 0x08/0x09 = the first 16 MB, 0x0A/0x0B = the second */
        static int romstat = -1; if (romstat < 0) romstat = getenv("TC2_ROMSTAT") != NULL;
        if (romstat) { static uint32_t lo = ~0u, hi; static unsigned long n; static unsigned fr;
            if (p < lo) lo = p; if (p > hi) hi = p; n++;
            if (rr_frame != fr) { if (n) fprintf(stderr, "[ROM] frame %u: %lu reads %08X-%08X\n", fr, n, lo, hi); fr = rr_frame; n = 0; lo = ~0u; hi = 0; } }
        const uint32_t off = (p >= 0x0A000000u ? 0x1000000u : 0) + (p & 0x00FFFFFFu);
        if (off + (uint32_t)size <= sizeof tc2_data) return be_rd(tc2_data + off, size);
    }
    uint32_t v = 0;
    if (p >= 0x0C000000u && p < 0x0C000004u) { io("Ru", a, size, 0); return 0; }   /* unmapped on System 23 (MAME's map): reads 0 */
    { int tc2_ctl_read(uint32_t, int, uint32_t *); if (tc2_ctl_read(p, size, &v)) { io("Rc", a, size, v); return v; } }   /* modelled: src/tc2_ctl.c */
    { int tc2_chips_read(uint32_t, int, uint32_t *); if (tc2_chips_read(p, size, &v)) { io("Rc", a, size, v); return v; } }   /* src/tc2_chips.c */
    { int tc2_screen_read(uint32_t, int, uint32_t *); if (tc2_screen_read(p, size, &v)) { io("Rc", a, size, v); return v; } }  /* src/tc2_screen.c */
    const int hit = env_take(p, &v);                  /* MAME's space is the 28-bit physical address */
    io(hit ? "R" : "R?", a, size, v);
    return v;
}

void rr_write(uint32_t a, int size, uint32_t v)
{
    uint32_t p = phys(a);
    if (p + (uint32_t)size <= sizeof tc2_ram) { be_wr(tc2_ram + p, size, v); return; }
    if (tc2_game.evo2 && p >= 0x10000000u) {
        if (evo2_write(p, size, v)) { io("We", a, size, v); return; }
        if (p < 0x11000000u || p >= boot_base()) { io("Wu", a, size, v); return; }
        p -= 0x10000000u;
    }
    { uint8_t *m = vram(p, size); if (m) { if (p >= 0x06A08000u && p < 0x06A08040u && c404log()) fprintf(stderr, "C404 W %08X %d %08X pc %08X frame %u\n", p, size, v, rr_pc, rr_frame); be_wr(m, size, v); return; } }
    if (p >= 0x04400000u && p + (uint32_t)size <= 0x04410000u) be_wr(shared_ram + (p - 0x04400000u), size, v);   /* and logged below */
    { int tc2_h8_main_write(uint32_t, int, uint32_t); if (tc2_h8_main_write(p, size, v)) { io("W", a, size, v); return; } }   /* the sub-CPU's start / stop / acknowledge (src/tc2_h8.c) */
    { int tc2_c435_write(uint32_t, int, uint32_t); if (tc2_c435_write(p, size, v)) return; }   /* the 3D command processor (src/tc2_c435.c) */
    { int tc2_ctl_write(uint32_t, int, uint32_t); tc2_ctl_write(p, size, v); }
    { int tc2_chips_write(uint32_t, int, uint32_t); tc2_chips_write(p, size, v); }
    { int tc2_screen_write(uint32_t, int, uint32_t); tc2_screen_write(p, size, v); }   /* the vblank acknowledge, the C361 registers */
    io("W", a, size, v);
}

void tc2_io_flush(void) { if (io_log) fflush(io_log); }
