/* TIME CRISIS 2'S OWN COPY of namco-2x-systems engine/eng_crosshair.c (copied 2026-10-14). TC2 is independent of the System 22 project: change it here. */
/* eng_crosshair.c -- the custom crosshair image (eng_crosshair.h). The PNG reader is our own (zlib for the inflate): 8-bit, not interlaced. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <dirent.h>
#include <zlib.h>
#include <SDL2/SDL.h>
#include "eng_gl.h"
#include "eng_cfg.h"
#include "eng_crosshair.h"

#define XH_MAX 256                                  /* the largest image, per side */
#define XH_FILES 64
static char files[XH_FILES][700];                   /* [0] unused: the built-in cross; then the images (paths) */
static char names[XH_FILES][64];
static int nfiles = 1, scanned, cur;                /* cur 0 = built-in */
static GLuint tex; static int tex_w, tex_h, tex_for = -1, failed_for = -1;

static int cmp_name(const void *a, const void *b) { return strcmp((const char *)a, (const char *)b); }
static void add(const char *rel, const char *name)
{
    if (nfiles >= XH_FILES) return;
    snprintf(files[nfiles], sizeof files[0], "%s", rel);
    snprintf(names[nfiles], sizeof names[0], "%s", name);
    nfiles++;
}
static void scan_dir(const char *dir)                /* dir: "" (the working directory) or a folder ending in a separator */
{
    char p[700];
    snprintf(p, sizeof p, "%scrosshair.png", dir);
    for (int i = 1; i < nfiles; i++) if (!strcmp(files[i], p)) return;   /* the same folder twice */
    FILE *f = fopen(p, "rb"); if (f) { fclose(f); add(p, "crosshair.png"); }
    snprintf(p, sizeof p, "%scrosshairs", dir);
    DIR *d = opendir(p);
    if (!d) return;
    char found[XH_FILES][64]; int n = 0;
    for (struct dirent *e; (e = readdir(d)) && n < XH_FILES; ) {
        const size_t l = strlen(e->d_name);
        if (l > 4 && l < 60 && !SDL_strcasecmp(e->d_name + l - 4, ".png")) snprintf(found[n++], 64, "%s", e->d_name);
    }
    closedir(d);
    qsort(found, (size_t)n, sizeof found[0], cmp_name);
    for (int i = 0; i < n; i++) {
        int dup = 0; for (int j = 1; j < nfiles; j++) dup |= !strcmp(names[j], found[i]);
        if (dup) continue;                          /* the same file name in both folders: the first one found */
        char rel[700]; snprintf(rel, sizeof rel, "%scrosshairs/%s", dir, found[i]); add(rel, found[i]);
    }
}
/* the working directory (the game's folder: launch.sh, a double-click on Windows) and the program's own folder */
static void scan(void)
{
    if (scanned) return;
    scanned = 1;
    scan_dir("");
    char *bp = SDL_GetBasePath();
    if (bp) { scan_dir(bp); SDL_free(bp); }
    /* the saved choice is the file NAME, so it survives the folder moving */
    const char *want = eng_cfg_get("crosshair_image");
    cur = 0;
    if (want && *want) for (int i = 1; i < nfiles; i++) if (!strcmp(names[i], want)) cur = i;
}
int eng_xhair_count(void) { scan(); return nfiles - 1; }
const char *eng_xhair_name(void) { scan(); return cur ? names[cur] : "built-in"; }
void eng_xhair_step(int dir)
{
    scan();
    cur = ((cur + (dir < 0 ? -1 : 1)) % nfiles + nfiles) % nfiles;
    eng_cfg_set("crosshair_image", cur ? names[cur] : "");
}

/* ---- the PNG reader ---- */
static uint32_t be32(const uint8_t *p) { return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3]; }
static int paeth(int a, int b, int c) { const int p = a + b - c, pa = abs(p - a), pb = abs(p - b), pc = abs(p - c); return pa <= pb && pa <= pc ? a : pb <= pc ? b : c; }
/* the file into RGBA (w * h * 4); NULL + a reason */
static uint8_t *png_load(const char *path, int *ow, int *oh, const char **why)
{
    *why = "cannot open the file";
    FILE *f = fopen(path, "rb"); if (!f) return NULL;
    fseek(f, 0, SEEK_END); const long n = ftell(f); fseek(f, 0, SEEK_SET);
    *why = "not a PNG";
    if (n < 33 || n > (32 << 20)) { fclose(f); return NULL; }
    uint8_t *d = malloc((size_t)n); if (!d) { fclose(f); return NULL; }
    if (fread(d, 1, (size_t)n, f) != (size_t)n) { fclose(f); free(d); return NULL; }
    fclose(f);
    static const uint8_t sig[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
    if (memcmp(d, sig, 8)) { free(d); return NULL; }
    uint32_t w = 0, h = 0; int depth = 0, ctype = 0, interlace = 0, have_ihdr = 0;
    uint8_t pal[256][4]; int npal = 0; uint8_t trns_g[2] = { 0 }, trns_rgb[6] = { 0 }; int have_trns = 0;
    for (int i = 0; i < 256; i++) pal[i][0] = pal[i][1] = pal[i][2] = 0, pal[i][3] = 255;
    uint8_t *idat = NULL; size_t nidat = 0;
    for (long o = 8; o + 12 <= n; ) {
        const uint32_t len = be32(d + o); const uint8_t *t = d + o + 4, *c = d + o + 8;
        if (len > (uint32_t)(n - o - 12)) break;
        if (!memcmp(t, "IHDR", 4) && len >= 13) { w = be32(c); h = be32(c + 4); depth = c[8]; ctype = c[9]; interlace = c[12]; have_ihdr = 1; }
        else if (!memcmp(t, "PLTE", 4)) { npal = (int)(len / 3) > 256 ? 256 : (int)(len / 3); for (int i = 0; i < npal; i++) memcpy(pal[i], c + 3 * i, 3); }
        else if (!memcmp(t, "tRNS", 4)) {
            have_trns = 1;
            if (ctype == 3) for (uint32_t i = 0; i < len && i < 256; i++) pal[i][3] = c[i];
            else if (ctype == 0 && len >= 2) memcpy(trns_g, c, 2);
            else if (ctype == 2 && len >= 6) memcpy(trns_rgb, c, 6);
        } else if (!memcmp(t, "IDAT", 4)) {
            uint8_t *ni = realloc(idat, nidat + len); if (!ni) break;
            idat = ni; memcpy(idat + nidat, c, len); nidat += len;
        } else if (!memcmp(t, "IEND", 4)) break;
        o += 12 + (long)len;
    }
    free(d);
    const int ch = ctype == 0 ? 1 : ctype == 2 ? 3 : ctype == 3 ? 1 : ctype == 4 ? 2 : ctype == 6 ? 4 : 0;
    *why = "unsupported PNG (use 8 bits per channel, not interlaced)";
    if (!have_ihdr || !idat || depth != 8 || !ch || interlace) { free(idat); return NULL; }
    *why = "too large (at most 256 x 256)";
    if (!w || !h || w > XH_MAX || h > XH_MAX) { free(idat); return NULL; }
    const size_t stride = (size_t)w * ch, raw_n = (stride + 1) * h;
    uint8_t *raw = malloc(raw_n), *rgba = malloc((size_t)w * h * 4);
    uLongf got = (uLongf)raw_n;
    *why = "damaged image data";
    if (!raw || !rgba || uncompress(raw, &got, idat, (uLong)nidat) != Z_OK || got != raw_n) { free(idat); free(raw); free(rgba); return NULL; }
    free(idat);
    uint8_t *prev = NULL;
    for (uint32_t y = 0; y < h; y++) {               /* undo the per-line filters */
        uint8_t *line = raw + y * (stride + 1), *px = line + 1; const int ft = line[0];
        for (size_t i = 0; i < stride; i++) {
            const int a = i >= (size_t)ch ? px[i - ch] : 0, b = prev ? prev[i] : 0, c = prev && i >= (size_t)ch ? prev[i - ch] : 0;
            px[i] = (uint8_t)(px[i] + (ft == 1 ? a : ft == 2 ? b : ft == 3 ? (a + b) / 2 : ft == 4 ? paeth(a, b, c) : 0));
        }
        prev = px;
        for (uint32_t x = 0; x < w; x++) {
            const uint8_t *s = px + x * ch; uint8_t *o = rgba + ((size_t)y * w + x) * 4;
            switch (ctype) {
            case 0: o[0] = o[1] = o[2] = s[0]; o[3] = have_trns && s[0] == trns_g[1] ? 0 : 255; break;
            case 2: o[0] = s[0]; o[1] = s[1]; o[2] = s[2]; o[3] = have_trns && s[0] == trns_rgb[1] && s[1] == trns_rgb[3] && s[2] == trns_rgb[5] ? 0 : 255; break;
            case 3: memcpy(o, pal[s[0]], 4); break;
            case 4: o[0] = o[1] = o[2] = s[0]; o[3] = s[1]; break;
            default: memcpy(o, s, 4); break;
            }
        }
    }
    free(raw);
    *ow = (int)w; *oh = (int)h;
    return rgba;
}

int eng_xhair_draw(float x, float y, float pic_h)
{
    scan();
    if (!cur || cur == failed_for) return 0;
    GLint keep = 0; glGetIntegerv(GL_TEXTURE_BINDING_2D, &keep);   /* the caller's texture is put back */
    if (tex_for != cur) {
        const char *p = files[cur];
        int w, h; const char *why;
        uint8_t *rgba = png_load(p, &w, &h, &why);
        if (!rgba) { fprintf(stderr, "[HOST] crosshair image %s: %s -- the built-in cross is used\n", p, why); failed_for = cur; glBindTexture(GL_TEXTURE_2D, (GLuint)keep); return 0; }
        if (!tex) glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
        free(rgba);
        tex_w = w; tex_h = h; tex_for = cur;
        fprintf(stderr, "[HOST] crosshair image %s (%d x %d)\n", p, w, h);
    }
    const float k = pic_h / 1080.0f, hw = tex_w * k / 2, hh = tex_h * k / 2;
    glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, tex);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glColor4f(1, 1, 1, 1);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f(x - hw, y - hh);
    glTexCoord2f(1, 0); glVertex2f(x + hw, y - hh);
    glTexCoord2f(1, 1); glVertex2f(x + hw, y + hh);
    glTexCoord2f(0, 1); glVertex2f(x - hw, y + hh);
    glEnd();
    glDisable(GL_BLEND); glDisable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, (GLuint)keep);
    return 1;
}
