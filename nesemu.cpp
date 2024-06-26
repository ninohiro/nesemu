#include <SDL2/SDL.h>
#include<SDL2/SDL_timer.h>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <iterator>
#include "nes.h"
NES nes{};
/*
uint64_t audio_count=0;
void audio_callback(void *unused, Uint8 *stream, int len){
    char *st=(char*)stream;
    for(int i=0;i<len;i++){
        st[i]=0;
    }
    for(int j=0;j<2;j++){
        int dc;
        switch(nes.io[j*4]>>6){
            case 0:
                dc=1;
                break;
            case 1:
                dc=2;
                break;
            case 2:
                dc=4;
                break;
            case 3:
                dc=6;
                break;
        }
        double vol=(double)(nes.io[j*4]&0xF)/16;
        double freq=111860.8/((int)nes.io[j*4+2]+(int)(nes.io[j*4+3]&7)*256+1);
        for(int i=0;i<len;i++){
            double t=double(i+audio_count)/22000;
            double x=vol*((t*freq-(int)(t*freq))*8<=dc?1:-1);
            st[i]+=(char)(x*127/4);
        }
    }
    if(nes.io[8]&(1<<6)){
        double vol=1;
        double freq=55930.4/((int)nes.io[0xA]+(int)(nes.io[0xB]&7)*256+1);
        for(int i=0;i<len;i++){
            double t=double(i+audio_count)/22000;
            int n=(t*freq-(int)(t*freq))*4;
            double f=t*freq*4-(int)(t*freq*4);
            double x=vol*((n&1)+(n&1?-1:1)*f)*(n&2?-1:1);
            st[i]+=(char)(x*127/4);
        }
    }
    audio_count+=len;
}
*/
int main(int argc,char **argv)
{
    if(argc==1){
        printf("Usage: nesemu <file>\n");
        return 0;
    }
    SDL_Init(SDL_INIT_EVERYTHING);
    /*
    SDL_AudioSpec as;

    as.freq = 22000;
    as.format = AUDIO_S8;
    as.samples = 1024;
    as.callback=audio_callback;
    as.userdata=NULL;
    as.channels = 1;
    SDL_OpenAudio(&as, NULL);
    SDL_PauseAudio(0);
    */

    SDL_Window *window = SDL_CreateWindow("nesemu", 100, 100, 640, 480, SDL_WINDOW_SHOWN);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
    SDL_Event ev;
    Uint32 pixel_format_enum=SDL_PIXELFORMAT_ARGB8888;
    SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(0,256,240,32,pixel_format_enum);
    SDL_PixelFormat *pixel_format=SDL_AllocFormat(pixel_format_enum);
    uint32_t *pixels=(uint32_t*)surface->pixels;
    INES ines=read_rom(argv[1]);
    nes.ines=ines;
    nes.pixels=pixels;
    nes.reset();
    uint64_t cl=0;
    uint64_t ms=SDL_GetTicks64();
    while(true){
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
        SDL_RenderClear(renderer);
        while(SDL_PollEvent(&ev)){
            if (ev.type == SDL_QUIT)
                return 0;
        }
        const Uint8 *keys=SDL_GetKeyboardState(NULL);
        nes.controller[0]=keys[SDL_SCANCODE_Z];
        nes.controller[1]=keys[SDL_SCANCODE_X];
        nes.controller[2]=keys[SDL_SCANCODE_A];
        nes.controller[3]=keys[SDL_SCANCODE_S];
        nes.controller[4]=keys[SDL_SCANCODE_UP];
        nes.controller[5]=keys[SDL_SCANCODE_DOWN];
        nes.controller[6]=keys[SDL_SCANCODE_LEFT];
        nes.controller[7]=keys[SDL_SCANCODE_RIGHT];
        uint64_t d=SDL_GetTicks64()-ms;
        uint64_t n_cl=(uint64_t)(d*(1.789773*1000))-cl;
        if(n_cl>100000){
            n_cl=100000;
        }
        for(int i=0;i<n_cl;i++){
            nes.step_ppu();
            nes.step_ppu();
            nes.step_ppu();
            nes.step_cpu();
        }
        cl+=n_cl;
        SDL_Texture *texture=SDL_CreateTextureFromSurface(renderer, surface);
        SDL_Rect src_rect=(SDL_Rect){0,0,256,240};
        SDL_Rect dest_rect=(SDL_Rect){(640-512)/2,0,512,480};
        SDL_RenderCopy(renderer, texture, &src_rect, &dest_rect);
        SDL_RenderPresent(renderer);
    }
    SDL_DestroyWindow(window);
    SDL_Quit();

    return EXIT_SUCCESS;
}
