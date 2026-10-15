/* PORT: Evolution 2 frame boundary. Both eyes share a completed scene. */
#include "tc2_vr.h"
#include "tc2_scene.h"
#include "tc2_host.h"
#include "quest_gun.h"
#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
static int opened;
/* PORT: OpenXR owns presentation cadence; upstream's optional flat smoothing is disabled. */
void (*tc2_present_hook)(void);
void tc2_crash_box(const char *reason) {
    fprintf(stderr,"[CZVR] fatal: %s\n",reason);fflush(stderr);
#ifdef TCVR_PC
    if(opened&&!getenv("TC2_NO_DIALOG"))SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,"Crisis Zone VR",reason,NULL);
#endif
}
int tc2_window_open(int scale,int full) {
    (void)scale;(void)full;void tc2_render_soft(int);tc2_render_soft(0);
    opened=tc2_vr_open();return opened;
}
int tc2_window_active(void) { return opened; }
void tc2_window_frame(void) {
    const char *limit=getenv("TC2_MAX_FRAMES");
    if(!opened) {if(limit&&rr_frame>=strtoul(limit,NULL,10)){tc2_on_exit();exit(0);}return;}
    /* User-selected TC2 blue arcade pistol appearance. CSZO4's automatic
     * fire, ammunition and cover/reload behavior remain game-controlled. */
    qgun_select(QGUN_PISTOL);
    qvr_scene_prepare(tc2_built_latest_wait());
    static uint64_t started;if(!started)started=SDL_GetPerformanceCounter();
    if(rr_frame&&rr_frame%300==0)fprintf(stderr,"[CZVR] frame %u elapsed %.3f s\n",rr_frame,(SDL_GetPerformanceCounter()-started)/(double)SDL_GetPerformanceFrequency());
    if((limit&&rr_frame>=strtoul(limit,NULL,10))||!ss22_host_frame()) {
        fprintf(stderr,"[CZVR] normal shutdown at frame %u\n",rr_frame);tc2_on_exit();ss22_host_close();exit(0);
    }
}
void tc2_out_poll(uint16_t value) {void ss22_out_poll(uint16_t);ss22_out_poll((value&1)?2:0);}
void tc2_out_close(void) {}
