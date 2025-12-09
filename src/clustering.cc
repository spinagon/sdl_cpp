#include <time.h>
#include <math.h>
#include <iostream>
#include <algorithm>
#include <string>
#include <random>
#include <chrono>
#include <cstring>

#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_image.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_timer.h>

using std::cout;
using std::endl;

int count = 0;
long total_count = 0;

void update_image(int steps);

struct global_data {
    SDL_Renderer* ren;
    SDL_Surface* surf;
    SDL_Texture* tex;
    int h;
    int w;
} global_data;

inline void unpack_rgba(uint32_t p, uint8_t &r, uint8_t &g, uint8_t &b) {
    r = (p >> 0) & 0xFF;
    g = (p >> 8) & 0xFF;
    b = (p >> 16) & 0xFF;
}

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *win = SDL_CreateWindow("", 800, 600, SDL_WINDOW_RESIZABLE);
    for (int i = 0; i < SDL_GetNumRenderDrivers(); ++i) {
        std::cout << SDL_GetRenderDriver(i) << "\n";
    }
    SDL_Renderer *ren = SDL_CreateRenderer(win, "direct3d");
    SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
    SDL_RenderClear(ren);
    srand(time(NULL));
    SDL_Event e;
    int quit = 0;
    int steps = 1000;
    int t = SDL_GetTicks();
    int mode = 0;
    int h = 300;
    int w = 400;
    global_data.ren = ren;
    SDL_Surface* surf;
    if (argc > 1) {
        surf = SDL_ConvertSurface(IMG_Load(argv[1]), SDL_PIXELFORMAT_RGBA32);
    } else {
        surf = SDL_ConvertSurface(IMG_Load("C:/china.jpg"), SDL_PIXELFORMAT_RGBA32);
    }
    global_data.surf = surf;
    global_data.h = surf->h;
    global_data.w = surf->w;
    SDL_Texture* tex = SDL_CreateTexture(global_data.ren,
        SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING,
        global_data.w, global_data.h);
    global_data.tex = tex;
    int step = 0;
    double high = 200;
    double low = 10;
    double mid = 60;
    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) {
                quit = 1;
            }
        }
        update_image(steps);
        SDL_RenderPresent(global_data.ren);
        if (SDL_GetTicks() - t > 1000) {
            std::cout << count << " steps per second\n";
            printf("%d steps per iteration\n", steps);
            double its = count / (double)steps;
            printf("%d iterations per second\n", (int)round(its));
            SDL_SetWindowTitle(win, std::to_string(count).c_str());
            if (step == 0) {
                high = its;
                steps = round(steps * (its / low));
            }
            if (step == 1) {
                low = its;
                mid = std::min((high + low) / 2, 60.0);
                std::cout << low << ", " << mid << ", " << high << "\n";
            }
            if (step > 1) {
                steps = round(steps * (its / mid));
            }
            //steps = std::max(steps, 10000);
            t = SDL_GetTicks();
            count = 0;
            ++step;
        }
        //SDL_Delay(1);
    }
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}

double diff(unsigned int p1, unsigned int p2) {
    unsigned char r1, g1, b1, r2, g2, b2;
    SDL_Surface* surf = global_data.surf;
    unpack_rgba(p1, r1, g1, b1);
    unpack_rgba(p2, r2, g2, b2);
    return abs((int)r1 - (int)r2) + abs((int)g1 - (int)g2) + abs((int)b1 - (int)b2);
}

int clip(int n) {
    int n_pixels = global_data.h * global_data.w;
    return (n + n_pixels) % n_pixels;
}

double sim(unsigned int* pixels, int x, int y, unsigned int c) {
    int h = global_data.h;
    int w = global_data.w;
    int d = 2;
    double d1 = diff(c, pixels[clip(y * w + x + d)]);
    double d2 = diff(c, pixels[clip(y * w + x - d)]);
    double d3 = diff(c, pixels[clip((y + d) * w + x)]);
    double d4 = diff(c, pixels[clip((y - d) * w + x)]);
    double d5 = diff(c, pixels[clip((y + d) * w + x + d)]);
    double d6 = diff(c, pixels[clip((y - d) * w + x - d)]);
    double d7 = diff(c, pixels[clip((y + d) * w + x - d)]);
    double d8 = diff(c, pixels[clip((y - d) * w + x + d)]);
    return d1 + d2 + d3 + d4 + (d5 + d6 + d7 + d8) / 1.41;
}

void update_image(int steps) {
    SDL_Renderer* ren = global_data.ren;
    SDL_Surface* surf = global_data.surf;
    int h = global_data.h;
    int w = global_data.w;
    unsigned int* pixels = (unsigned int*)surf->pixels;
    SDL_FRect r = {0, 0, (float)w, (float)h};
    for (int i = 0; i < steps; i++) {
        int y = total_count % w;
        int x = total_count / h;
        int radius = 1;
        int dx = radius; // total_count % radius + 1;
        int dy = radius; // total_count % radius + 1;
        int x1 = x + dx;
        int x2 = x - dx;
        int x3 = x - dx;
        int x4 = x + dx;
        int y1 = y + dy;
        int y2 = y - dy;
        int y3 = y + dy;
        int y4 = y - dy;
        unsigned int* c = &pixels[clip(y * w + x)];
        unsigned int* c1 = &pixels[clip(y1 * w + x1)];
        unsigned int* c2 = &pixels[clip(y2 * w + x2)];
        unsigned int* c3 = &pixels[clip(y3 * w + x3)];
        unsigned int* c4 = &pixels[clip(y4 * w + x4)];
        double orig = sim(pixels, x, y, *c);
        double d1 = sim(pixels, x1, y1, *c) + sim(pixels, x, y, *c1) - orig - sim(pixels, x1, y1, *c1);
        double d2 = sim(pixels, x2, y2, *c) + sim(pixels, x, y, *c2) - orig - sim(pixels, x2, y2, *c2);
        double d3 = sim(pixels, x3, y3, *c) + sim(pixels, x, y, *c3) - orig - sim(pixels, x3, y3, *c3);
        double d4 = sim(pixels, x4, y4, *c) + sim(pixels, x, y, *c4) - orig - sim(pixels, x4, y4, *c4);
        double d5 = 0;
        // double diffs[] {d1, d2, d3, d4, d5};
        double max_d;
        if (rand() % 1 == 0) {
            max_d = std::max({d5, d1, d2, d3, d4});
        } else {
            max_d = std::min({d5, d1, d2, d3, d4});
        }
        // std::sort(&diffs[0], &diffs[5]);
        // max_d = diffs[total_count % 2];
        if (max_d == d1) {
            std::swap(*c, *c1);
        } else if (max_d == d2) {
            std::swap(*c, *c2);
        } else if (max_d == d3) {
            std::swap(*c, *c3);
        } else if (max_d == d4){
            std::swap(*c, *c4);
        }
        ++count;
        ++total_count;
    }
    SDL_UpdateTexture(global_data.tex, NULL, surf->pixels, surf->pitch);
    SDL_RenderTexture(ren, global_data.tex, NULL, &r);
}