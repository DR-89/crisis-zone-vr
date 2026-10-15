/*
 * tc2_rom.c -- THE PLAYER'S ROMS, for whichever game this binary is (the chip list and image rules: include/tc2_game.h; Time Crisis II's
 * in src/tc2_game.c, Crisis Zone's in ../crszone/src/csz_game.c) and the program images made from them at start-up.
 *   - extracted/ is filled from the player's own MAME zip on the first run (src/eng/romzip.c: by name, size and the zip's own CRC),
 *     found in the current folder, roms/, or beside the executable.
 *   - the boot image, the sub-CPU's program and the I/O board's are made in memory by the game's own rules -- what tools/make_prog.py
 *     writes as boot.bin / sub.bin / io.bin for the translators.
 * The ROM supplies data only: the programs on it run as translated C (gen/), nothing here executes ROM code.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "eng/romzip.h"

#include "tc2_game.h"
#define chips  (tc2_game.chips)
#define NCHIPS (tc2_game.nchips)

char tc2_rom_error[1024];   /* the player's message when it fails (a window shows it: src/tc2_main.c) */
/* the ROM folder complete, from the player's zip if need be; 0 = not, with a message for the player */
int tc2_rom_setup(const char *dir, const char *exe_dir)
{
    const char *miss = eng_romzip_missing(dir, chips, NCHIPS);
    if (!miss) return 1;
    char err[512] = "";
    fprintf(stderr, "[ROM] %s/%s missing: looking for %s (here, roms/, beside the program)\n", dir, miss, tc2_game.zips[0]);
    if (eng_romzip_autosetup(dir, exe_dir, tc2_game.zips, tc2_game.nzips, chips, NCHIPS, err, sizeof err)) { fprintf(stderr, "[ROM] unpacked %s into %s/\n", tc2_game.zips[0], dir); return 1; }
    fprintf(stderr, "[ROM] %s\n[ROM] Put %s in the roms/ folder beside the game.\n", err, tc2_game.zip_help);
    snprintf(tc2_rom_error, sizeof tc2_rom_error, "%s needs its ROM file (no game data is included).\n\nPut %s in the \"roms\" folder next to the game, then start it again.\n\n(%s)",
             tc2_game.title, tc2_game.zip_help, err);
    return 0;
}

/* one ROM file, exactly n bytes (the games' image builders: src/tc2_game.c, ../crszone/src/csz_game.c) */
int tc2_rom_read(const char *dir, const char *name, uint8_t *dst, size_t n)
{
    char p[1024]; snprintf(p, sizeof p, "%s/%s", dir, name);
    FILE *f = fopen(p, "rb"); if (!f) { fprintf(stderr, "[ROM] cannot read %s\n", p); return 0; }
    const size_t got = fread(dst, 1, n, f); fclose(f);
    if (got != n) { fprintf(stderr, "[ROM] %s: %zu of %zu bytes\n", p, got, n); return 0; }
    return 1;
}
/* the program images, by the game's own rules */
int tc2_rom_boot(const char *dir, uint8_t *boot) { return tc2_game.rom_boot(dir, boot); }
int tc2_rom_sub(const char *dir, uint8_t *dst, size_t n) { return tc2_game.rom_sub(dir, dst, n); }
int tc2_rom_io(const char *dir, uint8_t *dst, size_t n) { return tc2_game.rom_io(dir, dst, n); }
