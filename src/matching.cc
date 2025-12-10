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
#include <SDL3/SDL_surface.h>

using std::cout;
using std::endl;

int count = 0;
long total_count = 0;
double total_diff = 0;
int radius = 1;

void update_image(int steps);

struct global_data {
    SDL_Renderer* ren;
    SDL_Surface* surf;
    SDL_Surface* surf2;
    SDL_Texture* tex;
} global_data;

inline void unpack_rgba(uint32_t p, uint8_t &r, uint8_t &g, uint8_t &b) {
    r = (p >> 0) & 0xFF;
    g = (p >> 8) & 0xFF;
    b = (p >> 16) & 0xFF;
}

void save_image(int step) {
    char s[100];
    sprintf(s, "frames/%04d.png", step);
    IMG_SavePNG(global_data.surf, s);

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
    std::string source, dest;
    if (argc > 1) {
        source = argv[1];
        dest = argv[2];
        if (argc > 3 && std::string(argv[3]) == "--reverse") {
            std::swap(source, dest);
        }
    } else {
        source = "C:/china.jpg";
        dest = "C:/!Drv/docs/CSS/interactive-examples.mdn.mozilla.net/media/examples/balloon-small.jpg";
    }
    global_data.surf = SDL_ConvertSurface(IMG_Load(source.c_str()), SDL_PIXELFORMAT_RGBA32);
    global_data.surf2 = SDL_ConvertSurface(IMG_Load(dest.c_str()), SDL_PIXELFORMAT_RGBA32);
    // if (argc > 1) {
    //     global_data.surf = SDL_ConvertSurface(IMG_Load(argv[1]), SDL_PIXELFORMAT_RGBA32);
    //     global_data.surf2 = SDL_ConvertSurface(IMG_Load(argv[2]), SDL_PIXELFORMAT_RGBA32);
    // } else {
    //     global_data.surf = SDL_ConvertSurface(IMG_Load("C:/china.jpg"), SDL_PIXELFORMAT_RGBA32);
    //     global_data.surf2 = SDL_ConvertSurface(IMG_Load("C:/!Drv/docs/CSS/interactive-examples.mdn.mozilla.net/media/examples/balloon-small.jpg"), SDL_PIXELFORMAT_RGBA32);
    // }
    global_data.surf = SDL_ScaleSurface(global_data.surf, global_data.surf2->w, global_data.surf2->h, SDL_SCALEMODE_LINEAR);
    SDL_Texture* tex = SDL_CreateTexture(global_data.ren,
        SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING,
        global_data.surf->w, global_data.surf->h);
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
            std::cout << "Diff: " << std::endl << total_diff << std::endl;
            std::cout << "Radius: " << std::endl << radius << std::endl;
            if (total_diff < global_data.surf->w * global_data.surf->h) {
                radius += std::max(1, radius / 5);
                save_image(step);
            }
            int shortest = std::min(global_data.surf->h, global_data.surf->w);
            if (radius >= shortest) {
                radius = (radius % shortest) + 1;
            }
            total_diff = 0;
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
    unpack_rgba(p1, r1, g1, b1);
    unpack_rgba(p2, r2, g2, b2);

    double y1 = 0.299*r1 + 0.587*g1 + 0.114*b1;
    double cb1 = 0.564*(b1 - y1);
    double cr1 = 0.713*(r1 - y1);
    double y2 = 0.299*r2 + 0.587*g2 + 0.114*b2;
    double cb2 = 0.564*(b2 - y2);
    double cr2 = 0.713*(r2 - y2);

    double dy = y1 - y2;
    double dcb = cb1 - cb2;
    double dcr = cr1 - cr2;

    return sqrt(dy*dy + 0.5*(dcb*dcb + dcr*dcr));
}


int clip(int n) {
    int n_pixels = global_data.surf->h * global_data.surf->w;
    return (n + n_pixels) % n_pixels;
}

double sim(unsigned int* pixels, int x, int y, unsigned int c) {
    int h = global_data.surf->h;
    int w = global_data.surf->w;
    double d = diff(c, pixels[clip(y * w + x)]);
    return d;
}

void update_image(int steps) {
    SDL_Renderer* ren = global_data.ren;
    SDL_Surface* surf = global_data.surf;
    int h = global_data.surf->h;
    int w = global_data.surf->w;
    unsigned int* pixels = (unsigned int*)surf->pixels;
    unsigned int* pixels2 = (unsigned int*)global_data.surf2->pixels;
    SDL_FRect r = {0, 0, (float)w, (float)h};
    for (int i = 0; i < steps; i++) {
        // int y = rand() % h;
        // int x = rand() % w;
        int x = ((int)total_count / h) % w;
        int y = (int)total_count % h;
        int dx = rand() % radius + 1;
        // dx = radius;
        int dy = rand() % radius + 1;
        // dy = radius;
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
        double orig = sim(pixels2, x, y, *c);
        double d1 = (orig + sim(pixels2, x1, y1, *c1)) - (sim(pixels2, x1, y1, *c) + sim(pixels2, x, y, *c1));
        double d2 = (orig + sim(pixels2, x2, y2, *c2)) - (sim(pixels2, x2, y2, *c) + sim(pixels2, x, y, *c2));
        double d3 = (orig + sim(pixels2, x3, y3, *c3)) - (sim(pixels2, x3, y3, *c) + sim(pixels2, x, y, *c3));
        double d4 = (orig + sim(pixels2, x4, y4, *c4)) - (sim(pixels2, x4, y4, *c) + sim(pixels2, x, y, *c4));
        double d0 = 0;
        double max_d;
        max_d = std::max({d0, d1, d2, d3, d4});
        // double diffs[] {d1, d2, d3, d4, d5};
        // std::sort(&diffs[0], &diffs[5]);
        // max_d = diffs[2];
        if (max_d == d0) {
            // pass;
        } else if (max_d == d1) {
            std::swap(*c, *c1);
        } else if (max_d == d2) {
            std::swap(*c, *c2);
        } else if (max_d == d3) {
            std::swap(*c, *c3);
        } else if (max_d == d4){
            std::swap(*c, *c4);
        }
        total_diff += max_d;
        ++count;
        ++total_count;
    }
    SDL_UpdateTexture(global_data.tex, NULL, surf->pixels, surf->pitch);
    SDL_RenderTexture(ren, global_data.tex, NULL, &r);
}