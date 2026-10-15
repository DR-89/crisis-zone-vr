#include "tc2_scene.h"
#include "quest_clock.h"
#include "quest_refresh.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
static float length(V3 a){return sqrtf(dot(a,a));}
static V3 normalize(V3 a){return mul(a,1/length(a));}
/* Capture support is compiled on Windows but remains disabled for this fixture. */
void tc2_render_assets(const uint8_t **r,const uint32_t **m,const uint8_t **a){*r=NULL;*m=NULL;*a=NULL;}
int main(void){
    tc2_built *b=calloc(1,sizeof *b);assert(b);
    b->fs.list.count=1;tc2_render_entry *e=&b->fs.list.e[0];e->type=TC2_RE_MODEL;e->vp_fov=772.53125f;
    tc2_poly *p=&b->polys[0];p->entry=0;p->n=3;p->vp_size_x=320;p->vp_size_y=240;
    p->pv[0].x=320;p->pv[0].y=240;p->pv[0].p[0]=1/.47f;
    const int hud[]={9010,9011,9012,9017,9094,9098,9107,9108};
    qvr_world_scale=1;qvr_view_distance=0;
    for(unsigned i=0;i<sizeof hud/sizeof *hud;i++){
        p->model=hud[i];assert(qvr_tc2_layer(b,p)==QTC2_OVERLAY);
        V3 a=qvr_tc2_vertex(b,p,0);assert(fabsf(a.z+2.5f)<1e-6f);
        qvr_world_scale=4;qvr_view_distance=1;V3 c=qvr_tc2_vertex(b,p,0);
        assert(a.x==c.x&&a.y==c.y&&a.z==c.z);qvr_world_scale=1;qvr_view_distance=0;
    }
    p->model=9039;assert(qvr_tc2_layer(b,p)==QTC2_WORLD);
    V3 a=qvr_tc2_vertex(b,p,0);qvr_world_scale=2;qvr_view_distance=.4f;V3 c=qvr_tc2_vertex(b,p,0);
    assert(fabsf(c.z-(a.z*2-.4f))<1e-6f);
    e->type=TC2_RE_DIRECT;assert(qvr_tc2_layer(b,p)==QTC2_OVERLAY);
    /* A cursor/hit flash at the game's synthetic 772/16384 layout depth
     * must meet the world at the same place as a tracked controller ray. */
    memset(b,0,sizeof *b);b->fs.list.count=3;b->npoly=3;
    qvr_units_per_meter=1;qvr_flat_view=0;
    for(int i=0;i<3;i++){
        tc2_render_entry *r=&b->fs.list.e[i];r->type=TC2_RE_MODEL;r->vp_fov=772.53125f;
        tc2_poly *q=&b->polys[i];q->entry=i;q->n=4;q->vp_size_x=320;q->vp_size_y=240;
    }
    tc2_poly *wall=&b->polys[0];wall->model=1;
    const float corner[4][2]={{0,0},{640,0},{640,480},{0,480}};
    for(int k=0;k<4;k++){wall->pv[k].x=corner[k][0];wall->pv[k].y=corner[k][1];wall->pv[k].p[0]=1.f/6;}
    for(int i=1;i<3;i++){
        tc2_render_entry *r=&b->fs.list.e[i];r->model=i==1?9124:9683;
        r->v[0]=64;r->v[1]=-32;r->v[2]=772;r->m[0]=r->m[4]=16384;r->m[8]=-16384;
        tc2_poly *q=&b->polys[i];q->model=r->model;
        for(int k=0;k<4;k++){q->pv[k].x=320+64*r->vp_fov/772;q->pv[k].y=240+32*r->vp_fov/772;q->pv[k].p[0]=16384.f/772;}
    }
    const float sizes[]={.25f,1,4},distances[]={0,.4f,1};
    for(int s=0;s<3;s++)for(int d=0;d<3;d++){
        qvr_world_scale=sizes[s];qvr_view_distance=distances[d];qvr_scene_prepare(b);
        V3 target=v3(6.f*64/772*sizes[s],-6.f*32/772*sizes[s],-6*sizes[s]-distances[d]);
        for(int i=1;i<3;i++){
            assert(qvr_tc2_layer(b,&b->polys[i])==QTC2_PROJECTED);
            V3 vertex=qvr_tc2_vertex(b,&b->polys[i],0);assert(length(sub(vertex,target))<1e-4f);
        }
        for(int hand=-1;hand<=1;hand+=2){
            V3 origin=v3(hand*.3f,-.15f,-.3f),hit;float nx,ny;
            assert(qvr_aim(origin,normalize(sub(target,origin)),&nx,&ny,&hit));
            assert(length(sub(hit,target))<1e-4f);
            assert(fabsf(nx-(320+64*772.53125f/772)/640)<1e-5f);
            assert(fabsf(ny-(240+32*772.53125f/772)/480)<1e-5f);
        }
    }
    /* A zone-select menu has one physical panel regardless of layout z. */
    b->fs.list.e[0].model=9128;b->fs.list.e[0].vp_fov=686.21875f;
    qvr_scene_prepare(b);
    for(int i=0;i<3;i++){
        assert(qvr_tc2_layer(b,&b->polys[i])==QTC2_OVERLAY);
        assert(fabsf(qvr_tc2_vertex(b,&b->polys[i],0).z+2.5f)<1e-6f);
    }
    const int rates[]={30,45,60,72,90,120};
    for(unsigned i=0;i<sizeof rates/sizeof *rates;i++){
        int64_t deadline=0;unsigned ticks=0;
        for(int f=0;f<rates[i]*10;f++)while(qclock_take(&deadline,1000000000LL+(int64_t)f*1000000000/rates[i]))ticks++;
        assert(ticks>=598&&ticks<=600);assert(qclock_take(&deadline,30000000000LL));assert(!qclock_take(&deadline,30000000000LL));
    }
    float offered[]={72,90,120};assert(qrefresh_choose(offered,3,120)==120);
    free(b);puts("PASS: Crisis Zone HUD/menu stability, cursor/hit-flash surface alignment in both hands at all scales/distances, world effects preserved, arcade timing at 30..120 Hz.");return 0;
}
