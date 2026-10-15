/* Render the shipped weapon through the production GLES path for visual QA. */
#include "quest_gpu.h"
#include "quest_gun.h"
#include <stdint.h>
static V3 unit(V3 v){return mul(v,1/sqrtf(dot(v,v)));}
static void look_at(float *m,V3 eye,V3 target){
    V3 z=unit(sub(eye,target)),x=unit(cross(v3(0,1,0),z)),y=cross(z,x);
    float a[16]={x.x,y.x,z.x,0,x.y,y.y,z.y,0,x.z,y.z,z.z,0,-dot(x,eye),-dot(y,eye),-dot(z,eye),1};
    for(int i=0;i<16;i++)m[i]=a[i];
}
__declspec(dllexport) int weapon_init(const char *path,int kind){
    if(!qgun_init(path))return 0;
    if(kind!=QGUN_PISTOL&&!qgun_load_weapon((QGunKind)kind,path))return 0;
    qgun_select((QGunKind)kind);
    V3 muzzle=qgun_muzzle(v3(0,0,0),(Q4){0,0,0,1});
    if(fabsf(muzzle.x)>1e-5f||!isfinite(muzzle.y)||muzzle.z>=0)return 0;
    /* Both physical hand positions and arbitrary rotations preserve the axis. */
    for(int hand=-1;hand<=1;hand+=2){
        Q4 q=product((Q4){0,sinf(.4f),0,cosf(.4f)},(Q4){sinf(.1f),0,0,cosf(.1f)});
        V3 p=v3(hand*.2f,-.2f,-.4f),actual=qgun_muzzle(p,q),expected=add(p,rotate(q,muzzle));
        if(dot(sub(actual,expected),sub(actual,expected))>1e-10f)return 0;
    }
    return glGetError()==GL_NO_ERROR;
}
__declspec(dllexport) int weapon_draw(int variant,int w,int h,uint8_t *pixels){
    V3 eyes[]={v3(.33f,.12f,.32f),v3(.48f,-.04f,-.09f),v3(-.33f,.12f,.32f),v3(.028f,.026f,.26f)};
    float view[16],proj[16];look_at(view,eyes[variant%4],variant%4==3?v3(0,0,-.2f):v3(0,-.06f,-.07f));
    projection(proj,-.48f,.48f,-.48f,.48f);glViewport(0,0,w,h);
    glClearColor(.075f,.09f,.11f,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    qgun_draw(view,proj,v3(0,0,0),(Q4){0,0,0,1},variant>=4?1:0,variant>=4?1:0,variant>=4?1:0);
    glReadPixels(0,0,w,h,GL_RGBA,GL_UNSIGNED_BYTE,pixels);return glGetError()==GL_NO_ERROR;
}
