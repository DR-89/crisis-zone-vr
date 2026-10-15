#pragma once
#include "tc2_frame.h"
#include <math.h>

/* CSZO4 Ver. B's zone-selection grid. It identifies the menu without
 * confusing its building models with the same buildings during gameplay. */
static inline int csz_zone_select(const tc2_frame_state *fs){
    for(int i=0;i<fs->list.count;i++)
        if(fs->list.e[i].type==TC2_RE_MODEL&&fs->list.e[i].model==9128)return 1;
    return 0;
}

/* The game draws its gun cursor and hit flashes at one unit per screen pixel:
 * translation.z = truncated focal length, with a camera-facing XY matrix.
 * 772/16384 is a layout depth, not a surface 7 cm in front of the player.
 * This also covers the rotating hit-flash models (e.g. 9680/9683/9686).
 * Score popups 9112/9121 use a second fixed layout plane. */
static inline int csz_screen_effect(const tc2_render_entry *e){
    if(e->type!=TC2_RE_MODEL)return 0;
    if(e->model==9112||e->model==9121)return 1;
    return isfinite(e->vp_fov)&&e->vp_fov>1&&e->vp_fov<32768&&
        e->v[2]==(int)e->vp_fov&&e->m[2]==0&&e->m[5]==0&&
        e->m[6]==0&&e->m[7]==0&&e->m[8]==-16384;
}
