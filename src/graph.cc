#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <numeric>
#include <iostream>

#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_rect.h>

void polar_to_xy(double phi, double r, double* x, double* y) {
    *x = cos(phi) * r + *x;
    *y = sin(phi) * r + *y;
}

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *win = SDL_CreateWindow("", 800, 600, SDL_WINDOW_RESIZABLE);
    SDL_Renderer *ren = SDL_CreateRenderer(win, "direct3d");
    SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
    SDL_RenderClear(ren);
    srand(time(NULL));
    SDL_Event e;
    int quit = 0;
    int w = 600;
    int h = 400;
    double steps1 = 0;
    double steps2 = 0;
    double p1 = 1;
    double p2 = -101;
    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) {
                quit = 1;
            }
        }
        SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
        SDL_RenderClear(ren);
        int old_x = 0;
        int old_y = 0;
        p1 += (rand() % 100  == 0);
        p2 += (rand() % 100  == 0);
        for (double phi = 0; phi < 3.14 * 2; phi += 0.01) {
            double x = w / 2;
            double y = h / 2;
            double r = pow(sin(phi * p1 + steps1), 1) * 50 + pow(cos(phi * p2 + steps2), 1) * 50;
            polar_to_xy(phi, r, &x, &y);
            SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
            if (phi > 0) {
                SDL_RenderLine(ren, old_x, old_y, x, y);
            }
            old_x = x;
            old_y = y;
        }
        SDL_RenderPresent(ren);
        SDL_Delay(16);
        steps1 += 0.1;
        steps2 += 0.1;
    }
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}