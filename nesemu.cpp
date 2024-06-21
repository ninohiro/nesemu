#include <SDL2/SDL.h>
#include<SDL2/SDL_timer.h>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <iterator>
#include "nes.h"
int main()
{
    SDL_Init(SDL_INIT_EVERYTHING);

    SDL_Window *window = SDL_CreateWindow("Hello World!", 100, 100, 640, 480, SDL_WINDOW_SHOWN);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
    SDL_Event ev;
    Uint32 pixel_format_enum=SDL_PIXELFORMAT_ARGB8888;
    SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(0,256,240,32,pixel_format_enum);
    SDL_PixelFormat *pixel_format=SDL_AllocFormat(pixel_format_enum);
    uint32_t *pixels=(uint32_t*)surface->pixels;
    INES ines=read_rom("../roms/nes-test-roms/instr_test-v5/build/02-implied.nes");
    NES nes(ines,pixels);
    uint64_t cl=0;
    uint64_t ms=SDL_GetTicks64();
    uint64_t f=0;
    while(true){
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
        SDL_RenderClear(renderer);
        while(SDL_PollEvent(&ev)){
            if (ev.type == SDL_QUIT)
                return 0;
        }
        uint64_t d=SDL_GetTicks64()-ms;
        uint64_t n_cl=(uint64_t)(d*(1.789773*1000))-cl;
        for(int i=0;i<n_cl;i++){
            nes.step_ppu();
            nes.step_ppu();
            nes.step_ppu();
            nes.step_cpu();
        }
        cl+=n_cl;
        uint64_t n_f=d*60/1000-f;
        if(n_f>0){
            nes.render_fb();
            nes.copy_fb();
            f+=n_f;
        }
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
