#include <SDL2/SDL.h>
#include <android/log.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
/* Crisis Zone has one cabinet/player route. */
int tc_game_main(int,char**);
int SDL_main(int argc,char **argv) {
    (void)argc;(void)argv;
    const char *dir=SDL_AndroidGetInternalStoragePath();
    if(!dir||chdir(dir))return 2;
    remove("crisiszone-vr-previous.log");rename("crisiszone-vr.log","crisiszone-vr-previous.log");
    freopen("crisiszone-vr.log","w",stderr);setvbuf(stderr,NULL,_IOLBF,0);
    setenv("SDL_ACCELEROMETER_AS_JOYSTICK","0",1);
    char *args[]={"CrisisZoneVR","--window",NULL};

    int result=tc_game_main(2,args);
    __android_log_print(ANDROID_LOG_ERROR,"TCVR","Game ended: %d; see files/crisiszone-vr.log",result);
    if(result) SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,"Crisis Zone VR","Startup failed. Log: files/crisiszone-vr.log (adb run-as).",NULL);
    return result;
}
