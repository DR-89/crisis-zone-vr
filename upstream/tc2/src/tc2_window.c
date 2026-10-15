/*
 * tc2_window.c -- Time Crisis 2 in a WINDOW: the OpenGL context, the menu bar (src/eng/eng_ui.c: File / Display / Audio, Nuklear -- TC2's own
 * copies of the System 22 games' menu), the display settings (src/eng/eng_display.c, saved in tc2.cfg) and THE FRAME PACER
 * (src/eng/eng_vsync.h): the game always runs at the board's 59.906 Hz (25.6 MHz / 814 / 525, the same as System 22); Display > Frame rate
 * chooses how many PICTURES a second -- Auto (the default: the display's rate, 60 on a 60 Hz display), or 24 / 30 / 50 / 60 / 75 / 90 /
 * 120 / 144 / 165 / 240.
 *
 * The lifted program owns the C stack (it never returns), so the window runs from the frame callback: tc2_window_frame() at the start of
 * every vertical blank presents the frame, handles the window's events and, while the menu is open or the game is paused, stays there
 * (the game does not run). Keys: Esc the menu, P pause, F11 fullscreen, F12 a screenshot (screenshots/tc2_NNNN.ppm), F8 the light gun's
 * border. Display > Crosshair turns the aiming cross on / off.
 * THE PICTURE is drawn on the GPU (src/tc2_gl.c) at the internal resolution Display > Resolution chooses (640 x 480 times 1..4); with no
 * GLSL 1.30 (or TC2_GPU=0) the software picture (src/tc2_render.c) is shown instead.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <SDL2/SDL.h>
#include "eng/eng_gl.h"
#include "eng/eng_display.h"
#include "eng/eng_ui.h"
#include "eng/eng_vsync.h"
#include "eng/eng_cfg.h"
#include "eng/eng_crosshair.h"
#include "eng/eng_pace.h"
#include "eng/ss22_input.h"
#include "tc2_build.h"
#include "tc2_game.h"
extern const ss22_input_game tc2_input_game;

int g_eng_hud_edges_on;                       /* eng_display.c's widescreen-HUD switch: no 2D HUD to move on this board yet */
void tc2_text_render(uint32_t *rgba);

static SDL_Window *win;
static SDL_GLContext glc;
static eng_vsync vs;
static eng_pace pace;
static GLuint tex;
static int gpu;                                                /* the GPU renderer is up */
static GLuint gpu_tex;
static uint32_t pic[640 * 480];
static int paused, shots;
static int smooth_active;                                      /* motion smoothing: this game frame shows in-between frames */
static const tc2_built *smooth_pic[3]; static double smooth_at[3]; static int smooth_np;   /* the frame's pictures and where each belongs (0..1] */
static const tc2_built *smooth_shown;
static const char *smooth_tag;                                 /* ENG_SWAPLOG: which picture the next swap shows */
static int smooth_frozen;                                      /* the frame's slot may be rewritten (tc2_render_frame_end ran): no more GPU draws from it */
static SDL_Rect pic_r;                                          /* the GAME's 4:3 area in drawable pixels: the gun aims and the crosshair live here */
/* widescreen (Display > Widescreen, GPU only): the picture takes the window's shape, (640 + 2 * ex) x 480 board pixels, Hor+ */
static int wide_ex(void)
{
    if (!gpu || !g_eng_disp.wide || !win) return 0;
    int dw, dh; SDL_GL_GetDrawableSize(win, &dw, &dh);
    if (dh <= 0) return 0;
    int e = (int)(240.0 * dw / dh + 0.5) - 320;
    return e < 0 ? 0 : e > 400 ? 400 : e;
}
/* the whole picture's rectangle for a picture ex board pixels wider each side, and the 4:3 game area inside it */
static SDL_Rect picture_rect(int ex)
{
    const SDL_Rect r = eng_disp_picture_rect(640 + 2 * ex, 480);
    pic_r.x = r.x + (int)((long)ex * r.w / (640 + 2 * ex)); pic_r.w = (int)(640L * r.w / (640 + 2 * ex)); pic_r.y = r.y; pic_r.h = r.h;
    return r;
}

/* the copied input module asks where the mouse / an absolute-mouse gun points in the picture; the window's outermost pixel = off-screen */
bool ss22_host_pointer(float *nx, float *ny, bool *inside)
{
    if (!win || pic_r.w <= 0 || pic_r.h <= 0) return false;
    int mx, my, ww, wh, dw, dh;
    SDL_GetMouseState(&mx, &my); SDL_GetWindowSize(win, &ww, &wh); SDL_GL_GetDrawableSize(win, &dw, &dh);
    const float px = (float)mx * dw / (ww ? ww : 1), py = (float)my * dh / (wh ? wh : 1);
    *nx = (px - pic_r.x) / pic_r.w; *ny = (py - pic_r.y) / pic_r.h;
    const bool edge = mx <= 0 || my <= 0 || mx >= ww - 1 || my >= wh - 1;
    *inside = !edge && *nx >= 0 && *nx <= 1 && *ny >= 0 && *ny <= 1 && (SDL_GetWindowFlags(win) & SDL_WINDOW_MOUSE_FOCUS);
    return true;
}

int tc2_window_active(void) { return win != NULL; }
/* a fatal stop in a window: say so instead of just vanishing (the details are in the log / terminal) */
void tc2_crash_box(const char *why)
{
    if (!win) return;
    char m[512];
    snprintf(m, sizeof m, "%s had to stop:\n\n%s\n\nYour settings and scores were not touched. Please report this, with %s.log if there is one.", tc2_game.title, why, tc2_game.tag);
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, tc2_game.title, m, win);
}

int tc2_window_open(int scale, int fullscreen)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER) != 0) { fprintf(stderr, "[HOST] SDL: %s\n", SDL_GetError()); return 0; }
    { void tc2_audio_open(void); tc2_audio_open(); }                  /* the C352 (src/tc2_audio.c) */
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    { static char cfg[64]; snprintf(cfg, sizeof cfg, "%s.cfg", tc2_game.tag); eng_disp_load(cfg, scale, fullscreen != 0); }
    g_eng_disp_light_gun = true;                 /* a light-gun game: the Display page shows the crosshair row */
    SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
    win = SDL_CreateWindow(tc2_game.window_title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                           eng_disp_win_w(g_eng_disp.scale), eng_disp_win_h(g_eng_disp.scale),
                           SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | (g_eng_disp.winmode ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0));
    if (!win) { fprintf(stderr, "[HOST] window: %s\n", SDL_GetError()); return 0; }
    glc = SDL_GL_CreateContext(win);
    if (!glc) { fprintf(stderr, "[HOST] OpenGL: %s\n", SDL_GetError()); return 0; }
    SDL_GL_MakeCurrent(win, glc);
    eng_disp_attach(win);
    eng_vsync_init(&vs, win, getenv("TC2_VSYNC"));
    tc2_smooth_set(eng_cfg_int("motion_smooth", 0));
    eng_pace_reset(&pace);
    fprintf(stderr, "[HOST] OpenGL: %s; display %d Hz, %s\n", (const char *)glGetString(GL_RENDERER), vs.hz, eng_vsync_mode(&vs));
    if (!eng_ui_init(win, "Time Crisis II")) fprintf(stderr, "[HOST] no menu\n");
    ss22_input_init(&tc2_input_game);                    /* the controls (keys, mouse, light gun, pads) and the menu's Controls page */
    eng_ui_add_page(ss22_input_page());
    (void)picture_rect(0);
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 640, 480, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
    gpu = tc2_gl_init();
    { void tc2_render_soft(int); tc2_render_soft(!gpu); }       /* the worker then only builds the frame; the GPU draws it */
    return 1;
}

static void draw_picture(void)
{
    int dw, dh; SDL_GL_GetDrawableSize(win, &dw, &dh);
    glViewport(0, 0, dw, dh);
    glClearColor(0, 0, 0, 1); glClear(GL_COLOR_BUFFER_BIT);
    const SDL_Rect r = picture_rect(gpu ? tc2_gl_wide() : 0);
    if (g_eng_disp.gun_border) {                                /* F8: a white frame round the picture for Sinden-style light guns */
        const int bd = g_eng_disp.gun_border * (dw < dh ? dw : dh) / 100;
        glEnable(GL_SCISSOR_TEST); glScissor(r.x - bd, dh - r.y - r.h - bd, r.w + 2 * bd, r.h + 2 * bd);
        glClearColor(1, 1, 1, 1); glClear(GL_COLOR_BUFFER_BIT); glDisable(GL_SCISSOR_TEST); glClearColor(0, 0, 0, 1);
    }
    glViewport(r.x, dh - r.y - r.h, r.w, r.h);
    glMatrixMode(GL_PROJECTION); glLoadIdentity(); glOrtho(0, 1, 1, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, gpu ? gpu_tex : tex);
    const GLint f = eng_disp_sharp() ? GL_NEAREST : GL_LINEAR;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, f); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, f);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f(0, 0); glTexCoord2f(1, 0); glVertex2f(1, 0);
    glTexCoord2f(1, 1); glVertex2f(1, 1); glTexCoord2f(0, 1); glVertex2f(0, 1);
    glEnd();
    glDisable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);   /* the default again: the menu (Nuklear) colours a white texel by its vertices */
    glViewport(0, 0, dw, dh);
}

/* the OS pointer, shown or hidden. SDL hides it by answering WM_SETCURSOR with no cursor, and on Windows that is not always enough: a
 * light gun that drives the pointer as an absolute mouse (RS3 Reaper, GitHub #55) left it on screen in play, where Linux hid it. Windows
 * also keeps a per-program display COUNTER (Win32 ShowCursor): below zero the pointer is hidden over our window whatever WM_SETCURSOR
 * does, so on Windows the counter is held below zero too while the pointer should be hidden, and put back when it should show. */
static void set_pointer(int show)
{
    if (SDL_ShowCursor(SDL_QUERY) != (show ? SDL_ENABLE : SDL_DISABLE)) SDL_ShowCursor(show ? SDL_ENABLE : SDL_DISABLE);
#ifdef _WIN32
    static int hidden;                                           /* our own decrement of the counter is in force */
    if (!show && !hidden) { int guard = 0; while (ShowCursor(FALSE) >= 0 && ++guard < 64) {} hidden = 1; }
    else if (show && hidden) { int guard = 0; while (ShowCursor(TRUE) < 0 && ++guard < 64) {} hidden = 0; }
#endif
}

/* the aiming CROSSHAIR (Display > Crosshair, ON by default, saved as crosshair=): at the aim point -- the mouse until the gun I/O is in --
 * over the picture; the OS cursor hidden there while it is on, and everywhere in play while it is off */
static void draw_crosshair(void)
{
    float ax, ay; int dw, dh;
    SDL_GL_GetDrawableSize(win, &dw, &dh);
    const int on = ss22_input_aim(&ax, &ay);                     /* the copied input module: mouse, absolute-mouse gun, either stick, the keys */
    /* the OS pointer: shown on the menu; in play hidden over the picture with the crosshair on, and ALWAYS with the crosshair off (a light-gun
     * player turns the crosshair off to see nothing but the game) */
    set_pointer(eng_ui_is_open() ? 1 : (!g_eng_disp.crosshair || on) ? 0 : 1);
    if (!g_eng_disp.crosshair || !on || eng_ui_is_open()) return;
    const SDL_Rect r = pic_r;
    const float x = r.x + ax * r.w, y = r.y + ay * r.h;
    const float k = r.h / 480.0f, a = 14 * k, gap = 4 * k, w = k < 1.5f ? 1.5f : k;
    glViewport(0, 0, dw, dh);
    glMatrixMode(GL_PROJECTION); glLoadIdentity(); glOrtho(0, dw, dh, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    glDisable(GL_TEXTURE_2D);
    if (eng_xhair_draw(x, y, (float)r.h)) { glColor3f(1, 1, 1); return; }   /* Display > Crosshair image (src/eng/eng_crosshair.h) */
    for (int pass = 0; pass < 2; pass++) {                    /* a dark outline, then the bright cross: visible on any background */
        glLineWidth(pass ? w : w + 2);
        if (pass) glColor3f(1.0f, 0.25f, 0.2f); else glColor3f(0, 0, 0);
        glBegin(GL_LINES);
        glVertex2f(x - a, y); glVertex2f(x - gap, y); glVertex2f(x + gap, y); glVertex2f(x + a, y);
        glVertex2f(x, y - a); glVertex2f(x, y - gap); glVertex2f(x, y + gap); glVertex2f(x, y + a);
        glEnd();
    }
    glColor3f(1, 1, 1); glLineWidth(1);
}

static void screenshot(void)
{
    mkdir("screenshots", 0755);
    char p[64]; snprintf(p, sizeof p, "screenshots/tc2_%04d.ppm", shots++);
    FILE *f = fopen(p, "wb"); if (!f) return;
    if (gpu) {                                                   /* the whole GPU picture: its internal resolution, widescreen included */
        const int w = (640 + 2 * tc2_gl_wide()) * tc2_gl_scale(), h = 480 * tc2_gl_scale();
        uint8_t *rgb = malloc((size_t)w * h * 3);
        if (rgb && tc2_gl_read_full(rgb, w * h * 3)) { fprintf(f, "P6\n%d %d\n255\n", w, h); fwrite(rgb, 1, (size_t)w * h * 3, f); }
        free(rgb);
    } else {
        fprintf(f, "P6\n640 480\n255\n");
        for (int i = 0; i < 640 * 480; i++) { const unsigned char c[3] = { (unsigned char)(pic[i] >> 16), (unsigned char)(pic[i] >> 8), (unsigned char)pic[i] }; fwrite(c, 1, 3, f); }
    }
    fclose(f); fprintf(stderr, "[HOST] screenshot %s\n", p);
}

static void draw_crosshair(void);
/* motion smoothing: the picture for a tick at frac (0..1) of the game frame -- the first of the frame's pictures whose place is at or after it
 * (at 120: 1/2 then N; at 165: 1/3, 2/3, N), so each tick moves on and none shows an older state than the tick before */
static void smooth_draw(double frac)
{
    int k = 0;
    while (k + 1 < smooth_np && smooth_at[k] < frac) k++;
    /* after tc2_render_frame_end the old frame's slot is the back slot: its snapshot is overwritten and the worker rebuilds it, so the old
     * frame's last ticks re-show what gpu_tex holds instead of drawing from it (a data race, seen as torn pictures with smoothing on) */
    if (smooth_pic[k] && smooth_pic[k] != smooth_shown && !smooth_frozen) { gpu_tex = tc2_gl_draw(smooth_pic[k]); smooth_shown = smooth_pic[k]; }
    { static char t[32]; snprintf(t, sizeof t, "at%.2f", smooth_at[k]); smooth_tag = t; }
}
static void smooth_extra(void *ctx, double frac) { (void)ctx; smooth_draw(frac); eng_vsync_extra_tag = smooth_tag; draw_picture(); draw_crosshair(); }

/* the new frame's picture(s): the newest built frame drawn on the GPU at the chosen internal resolution (or the software picture), and
 * with MOTION SMOOTHING (Display > Motion smoothing, src/tc2_render.c) and more than one picture a game frame, its in-between frames */
static void prepare_frame(void *ctx)
{
    (void)ctx;
    smooth_active = 0; smooth_np = 0; smooth_shown = NULL; smooth_tag = "frame"; smooth_frozen = 0;
    if (gpu) {
        /* the internal resolution is a whole multiple of the board's 480 lines; "Native (window size)" (res_h 0) takes the window's height
         * through eng_disp_render_size like every other game -- it used to be a fixed 1x (640x480) whatever the window (GitHub #44) */
        { int rw, rh; eng_disp_render_size(&rw, &rh); tc2_gl_set_scale((rh + 240) / 480); }
        tc2_gl_set_wide(wide_ex());
        const tc2_built *b = tc2_built_latest();
        tc2_smooth_rate(vs.lock);
        const int nm = tc2_smooth_on() && vs.lock && vs.tp < ENG_FRAME_NS && b ? tc2_built_mid_count() : 0;
        if (nm) {                                                /* the in-between frames at 1/2 (or 1/3, 2/3), then frame N at 1 */
            smooth_active = 1;
            for (int k = 0; k < nm; k++) { smooth_pic[smooth_np] = tc2_built_mid(k); smooth_at[smooth_np++] = nm == 1 ? 0.5 : (k + 1) / 3.0; }
            smooth_pic[smooth_np] = b; smooth_at[smooth_np++] = 1.0;
            smooth_draw(0.0);                                    /* the first one is ready for the period's first tick */
        } else if (b) gpu_tex = tc2_gl_draw(b);
    } else { tc2_text_render(pic); glBindTexture(GL_TEXTURE_2D, tex); glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 640, 480, GL_BGRA, GL_UNSIGNED_BYTE, pic); }
}
/* polled presentation: a tick's picture into the back buffer */
static void present_draw(void *ctx, double frac)
{
    (void)ctx;
    if (smooth_active) smooth_draw(frac); else smooth_tag = "frame";
    eng_vsync_extra_tag = smooth_tag;
    draw_picture();
    draw_crosshair();
}
/* from the CPU's slice loop (src/tc2_cpu.c rr_tick): serve a picture tick that came due while the game runs; the clock is read every 8th slice */
void (*tc2_present_hook)(void);
static void present_poll(void)
{
    static unsigned n;
    if (++n & 7) return;
    if (vs.live && eng_vsync_ns() >= vs.pnext) eng_vsync_poll(&vs, win);
}

/* one window pass: events, the picture (+ the menu), the swap, the pacing. Returns 0 to quit. */
static int pass(int new_frame)
{
    bool quit = false;
    SDL_Event e;
    if (new_frame) smooth_frozen = 1;                            /* tc2_render_frame_end has just run (see smooth_draw) */
    eng_ui_input_begin();
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) quit = true;
        if (eng_ui_event(&e)) continue;
        if (e.type == SDL_KEYDOWN && !e.key.repeat) {
            switch (e.key.keysym.scancode) {
            case SDL_SCANCODE_ESCAPE: eng_ui_set_open(true); ss22_input_neutral(); break;
            case SDL_SCANCODE_P: paused = !paused; fprintf(stderr, "[HOST] %s\n", paused ? "paused" : "running"); break;
            case SDL_SCANCODE_F11: eng_disp_toggle_fullscreen(); break;
            case SDL_SCANCODE_F12: screenshot(); break;
            case SDL_SCANCODE_F6: { void ss22_input_flash_step(int); ss22_input_flash_step(1); { extern int ss22_input_gun_flash(void); char b[48]; const int f = ss22_input_gun_flash(); void eng_ui_set_hint(const char *, int); if (f) snprintf(b, sizeof b, "Gun shot flash: %d%%", f); else snprintf(b, sizeof b, "Gun shot flash: OFF"); eng_ui_set_hint(b, 120); } } break;   /* the shot flash's strength: 100, 75, 50, 25, OFF */
            case SDL_SCANCODE_F7: { void ss22_input_mouse_step(int); ss22_input_mouse_step(1); } break;   /* the mouse speed: Direct, 125, 150, 200, 300 % */
            case SDL_SCANCODE_F8: eng_disp_cycle_gun_border(); break;     /* the light gun's white border round the picture (Sinden-style guns) */
            default: break;
            }
        }
        ss22_input_event(&e);                                    /* pads hot-plugged */
    }
    eng_ui_input_end();
    { /* tests: TC2_MENU_AT=frame:page:row opens the menu there (TC2_MENU_KEYS=<u d l r o b n p> then drives it) */
      static long at = -2, n; static int mp, mr;
      if (at == -2) { at = -1; const char *e = getenv("TC2_MENU_AT"); if (e) sscanf(e, "%ld:%d:%d", &at, &mp, &mr); }
      if (new_frame && at > 0 && ++n == at) { eng_ui_goto(mp, mr); eng_ui_set_open(true); ss22_input_neutral();
          for (const char *k = getenv("TC2_MENU_KEYS"); k && *k; k++) eng_ui_nav(*k); }
    }
    if (eng_ui_is_open() || paused) ss22_input_neutral(); else if (new_frame) ss22_input_update();
    eng_vsync_want(&vs, g_eng_disp.fps);
    /* A FIXED PICTURE RATE (Display > Frame rate other than 60, Auto on a 60+ Hz display), the game running: POLLED PRESENTATION -- the
     * frame on screen keeps its ticks to the end of its period, then this one's pictures are prepared and served on their ticks, also
     * while the CPUs run the next frame (tc2_present_poll from the slice loop): before, every tick inside the ~7 ms the emulation takes
     * waited for it (measured: 6.5% of pictures late at 120, 10% at 165, then a short gap -- an uneven rate) */
    if (new_frame && vs.lock && !vs.vsync && !vs.swap_blocks && !eng_ui_is_open() && !paused) {   /* not with vsync: the swap blocks, the timer cannot place the ticks */
        vs.draw = present_draw; vs.extra = NULL;
        eng_vsync_next_frame(&vs, win, prepare_frame, NULL);
        tc2_present_hook = present_poll;
        eng_pace_after(&pace, tc2_game.tag);
        return !(quit || eng_ui_quit_requested());
    }
    tc2_present_hook = NULL;
    if (vs.live) eng_vsync_resync(&vs);                          /* leaving polled presentation (menu, pause, a blocking swap): clocks from now */
    if (new_frame) prepare_frame(NULL);
    if (!new_frame) smooth_active = 0;                            /* the menu / pause: the kept picture (menu included) is re-shown */
    vs.extra = smooth_active ? smooth_extra : NULL;
    if (eng_vsync_show(&vs)) {
        draw_picture();
        draw_crosshair();
        if (eng_ui_is_open()) eng_ui_draw(&quit);
        { /* tests: TC2_WINSHOT=path:pass reads the whole window back (menu included) to a PPM */
          static long at = -2; static char path[512]; static long n;
          if (at == -2) { at = -1; const char *e = getenv("TC2_WINSHOT"); if (e) { snprintf(path, sizeof path, "%s", e); char *c = strrchr(path, ':'); if (c) { *c = 0; at = atol(c + 1); } } }
          n++;                                                  /* window passes: the menu holds the game, not the window */
          if (at > 0 && n == at) {
              int dw, dh; SDL_GL_GetDrawableSize(win, &dw, &dh);
              unsigned char *buf = malloc((size_t)dw * dh * 3);
              glReadBuffer(GL_BACK); glPixelStorei(GL_PACK_ALIGNMENT, 1); glReadPixels(0, 0, dw, dh, GL_RGB, GL_UNSIGNED_BYTE, buf);
              FILE *f = fopen(path, "wb");
              if (f) { fprintf(f, "P6\n%d %d\n255\n", dw, dh); for (int y = dh - 1; y >= 0; y--) fwrite(buf + (size_t)y * dw * 3, 1, (size_t)dw * 3, f); fclose(f); }
              free(buf); fprintf(stderr, "[HOST] window -> %s\n", path); at = -1;
          }
        }
        eng_pace_before_swap(&pace);
        eng_vsync_capture(&vs, win);
        SDL_GL_SwapWindow(win);
        eng_vsync_note_swap(smooth_active ? smooth_tag : "frame");
    }
    eng_vsync_after_frame(&vs, win);
    eng_pace_after(&pace, tc2_game.tag);
    return !(quit || eng_ui_quit_requested());
}

/* the frame callback (the start of vertical blank): present it; hold here while the menu is open or the game is paused */
void tc2_window_frame(void)
{
    if (!win) return;
    int ok = pass(1);
    while (ok && (eng_ui_is_open() || paused)) { ok = pass(0); if (!eng_ui_is_open() && !paused) eng_vsync_resync(&vs); }
    if (!ok) { fprintf(stderr, "[HOST] quit\n"); { void tc2_backup_save(int); tc2_backup_save(1); } { void tc2_out_close(void); tc2_out_close(); } ss22_input_close(); eng_ui_shutdown(); SDL_Quit(); exit(0); }
}
