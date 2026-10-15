/* TIME CRISIS 2'S OWN COPY of namco-2x-systems engine/eng_crosshair.h (copied 2026-10-14). TC2 is independent of the System 22 project: change it here. */
/* eng_crosshair.h -- A CUSTOM CROSSHAIR IMAGE for the light-gun games (Display > Crosshair image).
 *   Any PNG in the crosshairs/ folder beside the program (and crosshair.png beside it) can replace the built-in cross. The menu row
 *   cycles "built-in" and every image found; the choice is saved as crosshair_image=<file name> in the game's cfg.
 *   The image: 8-bit PNG (grey, grey+alpha, RGB, RGBA or palette, tRNS honoured), not interlaced, at most 256 x 256. It is drawn centred
 *   on the aim point at its own pixel size on a 1080-line picture and scaled with the picture (a 64 x 64 image is about the built-in
 *   cross's size). Transparent pixels show the game. A file that cannot be read is logged once and the built-in cross is drawn.
 *   Shared by Time Crisis (engine/ss22_host.c) and, as a copy in tc2/src/eng/, the System 23 host (Time Crisis 2, Crisis Zone). */
#ifndef ENG_CROSSHAIR_H
#define ENG_CROSSHAIR_H

void        eng_xhair_step(int dir);      /* the menu row: the next (+1) / previous (-1) choice; saved to the cfg */
const char *eng_xhair_name(void);         /* the current choice, for the menu: "built-in" or the file name */
int         eng_xhair_count(void);        /* the images found (0: only the built-in cross) */
/* draw the chosen image centred at (x, y) in drawable pixels with an ortho (0, dw, dh, 0) projection already set; pic_h = the game
 * picture's height in drawable pixels. Returns 1 if it drew, 0 = the caller draws the built-in cross. Needs the GL context. */
int         eng_xhair_draw(float x, float y, float pic_h);
#endif
