#include <SDL2/SDL.h>
#include<SDL2/SDL_timer.h>
#include <iostream>
#include <cstdint>
#include "nes.h"

using namespace std;

int main(int argc,char *argv[])
{
    if(argc==1){
        cerr<<"Usage: nesemu <file>"<<endl;
        return 1;
    }
    SDL_Init(SDL_INIT_EVERYTHING);

    SDL_Window *window = SDL_CreateWindow("nesemu", 100, 100, 640, 480, SDL_WINDOW_SHOWN);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
    SDL_Event ev;
    Uint32 pixel_format_enum=SDL_PIXELFORMAT_ARGB8888;
    SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(0,256,240,32,pixel_format_enum);
    SDL_PixelFormat *pixel_format=SDL_AllocFormat(pixel_format_enum);
    uint32_t *pixels=(uint32_t*)surface->pixels;

    NES nes{};
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
        SDL_DestroyTexture(texture);
    }
    SDL_DestroyWindow(window);
    SDL_Quit();

    return EXIT_SUCCESS;
}
