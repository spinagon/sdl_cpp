#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <complex>

#include "SDL.h"
#include "SDL_events.h"

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("SDL_Init Error: %s\n", SDL_GetError());
    }
    SDL_Window *win = SDL_CreateWindow("Hello World!", 100, 100, 1000, 1000, SDL_WINDOW_SHOWN);
    if (win == NULL) {
        printf("SDL_CreateWindow Error: %s\n", SDL_GetError());
        return 1;
    }
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_Event e;
    int quit = 0;
    int i = 0;
    SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
    SDL_RenderClear(ren);
    srand(time(NULL));
    int t = clock();
    double x0 = 0;
    double y0 = 0;
    double scale = 1;
    int n = 64;
    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = 1;
            }
            if (e.type == SDL_MOUSEBUTTONDOWN) {
                x0 = (e.button.x / 400. - 1) * scale + x0;
                y0 = (e.button.y / 400. - 1) * scale + y0;
                scale /= 2;
                i = 0;
            }
        }
        double x = rand() % 800;
        double y = rand() % 800;
        if (y < 1000 + i / 800.) {
            x = i % 800;
            y = (i / 800) % 800;
            i++;
        }
        double cx = ((x / 400. - 1) * scale) + x0;
        double cy = ((y / 400. - 1) * scale) + y0;
        std::complex<double> z = 0;
        int j = 0;
        for (j = 0; j < n; j++) {
            z = z * z + std::complex<double>(cx, cy);
            if (std::abs(z) > 4) {
                break;
            }
        }
        j = j * 255 / n;
        int r = (j % 64) * 4;
        int g = j;
        int b = (j % 32) * 8;
        SDL_SetRenderDrawColor(ren, r % 255, g % 255, b % 255, 255);
        SDL_RenderDrawPointF(ren, x, y);
        if (clock() - t > 10) {
            SDL_RenderPresent(ren);
            t = clock();
        }
    }
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
