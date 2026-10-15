/* tc2_game.c -- TIME CRISIS II's table for the System 23 host (include/tc2_game.h): MAME's timecrs2 set (US, TSS3 Ver. B), System 23. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tc2_game.h"

static const eng_rom_t chips[] = {
    { "tss3verb.1", 0x200000, NULL, NULL, 0x6E3F232B }, { "tss3verb.2", 0x200000, NULL, NULL, 0xC7BE691F },
    { "tss1vera.3", 0x80000, NULL, NULL, 0x41E41994 },  { "tssioprog.ic3", 0x40000, NULL, NULL, 0xEDAD4538 },
    { "tss1ccrh.7e", 0x200000, NULL, NULL, 0xF998DE1A }, { "tss1ccrl.7f", 0x400000, NULL, NULL, 0x3A325FE7 },
    { "tss1cgll.4m", 0x800000, NULL, NULL, 0x18433AAA }, { "tss1cglm.4k", 0x800000, NULL, NULL, 0x669974C2 },
    { "tss1cgum.4j", 0x800000, NULL, NULL, 0xC22739E1 }, { "tss1cguu.4f", 0x800000, NULL, NULL, 0x76924E04 },
    { "tss1mtah.2j", 0x800000, NULL, NULL, 0x697C26ED }, { "tss1mtal.2h", 0x800000, NULL, NULL, 0xBFC79190 },
    { "tss1mtbh.2m", 0x800000, NULL, NULL, 0x82582776 }, { "tss1mtbl.2f", 0x800000, NULL, NULL, 0xE648BEA4 },
    { "tss1pt0h.7a", 0x400000, NULL, NULL, 0xCDBE0BA8 }, { "tss1pt0l.7c", 0x400000, NULL, NULL, 0x896F0FB4 },
    { "tss1pt1h.5a", 0x400000, NULL, NULL, 0x63647596 }, { "tss1pt1l.5c", 0x400000, NULL, NULL, 0x5A09921F },
    { "tss1pt2h.4a", 0x400000, NULL, NULL, 0x9B06E22D }, { "tss1pt2l.4c", 0x400000, NULL, NULL, 0x4B230D79 },
    { "tss1waveh.2a", 0x800000, NULL, NULL, 0x5C8758B4 }, { "tss1wavel.2c", 0x800000, NULL, NULL, 0xDEAEAD26 },
};
static const char *const zips[] = { "timecrs2.zip", "namco_tssio.zip" };   /* a split MAME set keeps the I/O board's program in its device zip */

int tc2_rom_read(const char *dir, const char *name, uint8_t *dst, size_t n);   /* src/tc2_rom.c */

/* the boot image (4 MB at 0x1FC00000): tss3verb.2 and .1 byte-interleaved, .2 first */
static int rom_boot(const char *dir, uint8_t *boot)
{
    uint8_t *a = malloc(0x200000), *b = malloc(0x200000);
    const int ok = a && b && tc2_rom_read(dir, "tss3verb.1", a, 0x200000) && tc2_rom_read(dir, "tss3verb.2", b, 0x200000);
    if (ok) for (uint32_t k = 0; k < 0x200000; k++) { boot[2 * k] = b[k]; boot[2 * k + 1] = a[k]; }
    free(a); free(b);
    return ok;
}
/* the sub-CPU's (H8/3002) program: tss1vera.3 is stored word-swapped (MAME ROM_LOAD16_WORD_SWAP) */
static int rom_sub(const char *dir, uint8_t *dst, size_t n)
{
    if (n < 0x80000 || !tc2_rom_read(dir, "tss1vera.3", dst, 0x80000)) return 0;
    for (uint32_t k = 0; k < 0x80000; k += 2) { const uint8_t t = dst[k]; dst[k] = dst[k + 1]; dst[k + 1] = t; }
    return 1;
}
/* the I/O board's (H8/3334) program, as it is: its first n bytes (the H8/3334 maps 16 KB of it) */
static int rom_io(const char *dir, uint8_t *dst, size_t n)
{
    uint8_t *all = malloc(0x40000);
    const int ok = all && n <= 0x40000 && tc2_rom_read(dir, "tssioprog.ic3", all, 0x40000);
    if (ok) memcpy(dst, all, n);
    free(all);
    return ok;
}

const tc2_game_t tc2_game = {
    .title = "Time Crisis 2", .tag = "tc2", .window_title = "Time Crisis II", .mame_set = "timecrs2",
    .speech_voices = 0x80000000u,                 /* voice 31: every call-out (banks 0x94-0xB8, full volume on all four outputs) */
    .zip_help = "MAME's timecrs2.zip (Time Crisis II, US TSS3 Ver. B)",
    .zips = zips, .nzips = 2, .chips = chips, .nchips = (int)(sizeof chips / sizeof chips[0]),
    .evo2 = 0,
    .clock = 169344000u,                          /* BUSCLOCK * 5 */
    .px_num = 200, .px_den = 1323,                /* 25.6 MHz / 169.344 MHz */
    .sub_div = 10,                                /* 16.9344 MHz */
    .io_num = 64, .io_den = 735,                  /* 14.7456 MHz */
    .c352_div = 1920,                             /* 88200 Hz */
    .ip = { [TC2_SRC_VBLANK] = TC2_IP(0), [TC2_SRC_C361] = TC2_IP(1), [TC2_SRC_SUB] = TC2_IP(1), [TC2_SRC_C435] = TC2_IP(2),
            [TC2_SRC_C422] = TC2_IP(3), [TC2_SRC_TIMER] = TC2_IP(5) },
    .boot_size = 0x400000, .rom_boot = rom_boot, .rom_sub = rom_sub, .rom_io = rom_io, .io_rom_size = 0x4000, .c417_status = 0x008E, .depth_default = 1, .gun_x0 = 91, .gun_xw = 733, .gun_y0 = 38, .gun_yh = 247,
    .data_rom = { { "tss1mtah.2j", "tss1mtal.2h" }, { "tss1mtbh.2m", "tss1mtbl.2f" } },
    .npt = 3, .pt_rom = { { "tss1pt0h.7a", "tss1pt0l.7c" }, { "tss1pt1h.5a", "tss1pt1l.5c" }, { "tss1pt2h.4a", "tss1pt2l.4c" } },
    .ccrl = "tss1ccrl.7f", .ccrh = "tss1ccrh.7e", .cg = { "tss1cgll.4m", "tss1cglm.4k", "tss1cgum.4j", "tss1cguu.4f" },
    .wave = { "tss1wavel.2c", "tss1waveh.2a" },
};
