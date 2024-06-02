#include <SDL2/SDL.h>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <iterator>
#include "ines.h"
int main()
{
    SDL_Init(SDL_INIT_EVERYTHING);

    SDL_Window *window = SDL_CreateWindow("Hello World!", 100, 100, 640, 480, SDL_WINDOW_SHOWN);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
    SDL_Event ev;
    Uint32 pixel_format_enum=SDL_PIXELFORMAT_RGBA8888;
    SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(0,256,240,32,pixel_format_enum);
    SDL_PixelFormat *pixel_format=SDL_AllocFormat(pixel_format_enum);
    uint32_t *pixels=(uint32_t*)surface->pixels;
    for(int i=0;i<240;i++){
        for(int j=0;j<256;j++){
            pixels[i*256+j]=SDL_MapRGBA(pixel_format,j,i,0,0);
        }
    }
    while(true){
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderClear(renderer);
        while(SDL_PollEvent(&ev)){
            if (ev.type == SDL_QUIT)
                return 0;
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
