#include "tc2_build.h"
#include "crisis_zone_layers.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* Compare the production text adapter against the untouched release renderer. */
#define tc2_text_layer original_layer
#define tc2_text_mix original_mix
#define tc2_text_render original_render
#include "../upstream/tc2/src/tc2_text.c"
#undef tc2_text_layer
#undef tc2_text_mix
#undef tc2_text_render
void tc2_frame_rgba(uint32_t *rgba){(void)rgba;}
static uint16_t actual[640*480],expected[640*480],unscrolled[640*480];
static uint8_t ap[640*480],ep[640*480],up[640*480];
/* A 32-pixel H spans both ticker tile rows. Check every sub-tile scroll
 * phase, including vertical offsets, against its pixel shape rather than
 * reproducing the renderer's row cutoff. The old y=448 cutoff splits it. */
static int glyph(unsigned x,unsigned y){
    return y>=4&&y<28&&((x>=3&&x<7)||(x>=23&&x<27)||
           (y>=14&&y<18&&x>=3&&x<27));
}
static void whole_glyph(tc2_built *b){
    memset(b,0,sizeof *b);memset(b->fs.text,0xFF,sizeof b->fs.text);
    b->fs.list.count=1;b->fs.list.e[0].type=TC2_RE_MODEL;b->fs.list.e[0].model=9128;
    for(unsigned y=0;y<32;y++)for(unsigned x=0;x<32;x++){
        unsigned code=(y/16)*2+x/16;
        unsigned byte=code*128+(y%16)*8+(x%16)/2;
        unsigned shift=(x&1)?0:4;
        if(glyph(x,y))b->fs.text[byte]=(uint8_t)((b->fs.text[byte]&~(15u<<shift))|(7u<<shift));
    }
    uint8_t *map=b->fs.text+0x1E000;
    for(unsigned y=0;y<2;y++)for(unsigned x=0;x<64;x++){
        unsigned code=y*2+x%2;
        /* Identical glyphs in a stationary header and the moving ticker. */
        for(unsigned band=0;band<2;band++){
            unsigned tile=((band?27:1)+y)*64+x;
            map[tile*2]=0;map[tile*2+1]=(uint8_t)code;
        }
    }
    const unsigned vertical[]={0,9,1008};
    for(unsigned v=0;v<sizeof vertical/sizeof *vertical;v++)for(unsigned scroll=0;scroll<32;scroll++){
        b->fs.yscroll=(uint16_t)vertical[v];
        for(unsigned y=0;y<1024;y++)b->fs.linexscroll[y]=(uint16_t)scroll;
        memset(actual,0,sizeof actual);memset(ap,0,sizeof ap);
        tc2_text_layer(&b->fs,actual,ap);
        for(unsigned y=0;y<480;y++)for(unsigned x=0;x<640;x++){
            unsigned sy=(y+vertical[v])&1023;
            int ink=sy>=432&&sy<464?glyph((x+scroll)%32,sy-432):
                    sy>=16&&sy<48?glyph(x%32,sy-16):0;
            assert(ap[y*640+x]==(ink?4:0));
            assert(actual[y*640+x]==(ink?7:0));
        }
    }
}
int main(int argc,char **argv){
    tc2_built *b=malloc(sizeof *b);assert(b);unsigned menus=0,other=0,changed=0;
    whole_glyph(b);
    for(int i=1;i<argc;i++){
        FILE *f=fopen(argv[i],"rb");assert(f&&fread(b,1,sizeof *b,f)==sizeof *b);fclose(f);
        memset(actual,0,sizeof actual);memset(expected,0,sizeof expected);memset(unscrolled,0,sizeof unscrolled);
        memset(ap,0,sizeof ap);memset(ep,0,sizeof ep);memset(up,0,sizeof up);
        original_layer(&b->fs,expected,ep);tc2_text_layer(&b->fs,actual,ap);
        if(csz_zone_select(&b->fs)){
            menus++;
            memset(b->fs.linexscroll,0,sizeof b->fs.linexscroll);original_layer(&b->fs,unscrolled,up);
            for(unsigned y=0;y<480;y++){
                unsigned sy=(y+b->fs.yscroll)&1023;
                int ticker=sy>=432&&sy<464;
                assert(!memcmp(actual+640*y,(ticker?expected:unscrolled)+640*y,640*sizeof *actual));
                assert(!memcmp(ap+640*y,(ticker?ep:up)+640*y,640));
            }
            if(memcmp(actual,expected,sizeof actual))changed++;
        }else{other++;assert(!memcmp(actual,expected,sizeof actual));assert(!memcmp(ap,ep,sizeof ap));}
    }
    assert(menus&&other&&changed);free(b);
    printf("PASS: whole ticker glyphs at 32 scroll phases and 3 vertical offsets; %u zone-select frames keep title/timer fixed; %u other frames match original text.\n",menus,other);return 0;
}
