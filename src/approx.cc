#include <time.h>
#include <math.h>
#include <iostream>
#include <algorithm>
#include <string>
#include <random>
#include <chrono>
#include <cstring>
#include <list>

#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_image.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_surface.h>
#include <SDL3/SDL_render.h>

#define DBG(arg) std::cout << #arg << ": " << arg << std::endl;

using std::cout;
using std::endl;

int argc;
char** argv;

int count = 0;
long total_count = 0;
double step_diff = 0;
double total_diff = 0;
bool last = false;

double sim(unsigned int* pixels, unsigned int* pixels2);
double sim_blur(unsigned int* pixels, unsigned int* pixels2);
std::list<double (*)(unsigned int*, unsigned int*)> sim_functions {sim, sim_blur};

void update_image(int steps);

struct global_data {
    SDL_Renderer* ren;
    SDL_Surface* surf;
    SDL_Surface* surf2;
    SDL_Surface* surf_original;
    SDL_Texture* tex;
} global_data;

inline void unpack_rgba(uint32_t p, uint8_t &r, uint8_t &g, uint8_t &b) {
    r = (p >> 0) & 0xFF;
    g = (p >> 8) & 0xFF;
    b = (p >> 16) & 0xFF;
}

void save_image(int step) {
    char s[100];
    sprintf(s, "frames/%04d.jpg", step);
    IMG_SaveJPG(global_data.surf, s, 90);

}

bool in_args(const std::string& arg) {
    for (int i = 1; i < argc; ++i) {
        if (argv[i] == arg) return true;
    }
    return false;
}

int main(int argc_, char* argv_[]) {
    argc = argc_;
    argv = argv_;
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *win;
    SDL_Renderer *ren;
    SDL_CreateWindowAndRenderer("", 800, 600, SDL_WINDOW_RESIZABLE, &win, &ren);
    for (int i = 0; i < SDL_GetNumRenderDrivers(); ++i) {
        std::cout << SDL_GetRenderDriver(i) << "\n";
    }
    SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
    SDL_RenderClear(ren);
    srand(time(NULL));
    SDL_Event e;
    int quit = 0;
    int steps = 1;
    int t = SDL_GetTicks();
    int mode = 0;
    int h = 300;
    int w = 400;
    global_data.ren = ren;
    std::string source, dest;
    if (argc > 1) {
        source = argv[1];
        dest = argv[2];
        if (in_args("--reverse")) {
            std::swap(source, dest);
        }
    } else {
        source = "C:/china.jpg";
        dest = "C:/!Drv/docs/CSS/interactive-examples.mdn.mozilla.net/media/examples/balloon-small.jpg";
    }
    global_data.surf = SDL_ConvertSurface(IMG_Load(source.c_str()), SDL_PIXELFORMAT_RGBA32);
    global_data.surf2 = SDL_ConvertSurface(IMG_Load(dest.c_str()), SDL_PIXELFORMAT_RGBA32);
    global_data.surf = SDL_ScaleSurface(global_data.surf, global_data.surf2->w, global_data.surf2->h, SDL_SCALEMODE_LINEAR);
    global_data.surf_original = SDL_ScaleSurface(global_data.surf, global_data.surf2->w, global_data.surf2->h, SDL_SCALEMODE_LINEAR);
    SDL_Texture* tex = SDL_CreateTexture(global_data.ren,
        SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING,
        global_data.surf->w, global_data.surf->h);
    global_data.tex = tex;
    int step = 0;
    double high = 200;
    double low = 10;
    double mid = 60;
    save_image(step);
    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) quit = 1;
            if (e.type == SDL_EVENT_KEY_DOWN) {
                if (e.key.key == SDLK_Q) quit = 1;
                if (e.key.key == SDLK_B) {
                    sim_functions.push_back(*sim_functions.front());
                    sim_functions.pop_front();
                }
                if (e.key.key == SDLK_R) {
                    std::swap(global_data.surf2, global_data.surf_original);
                }
            }
        }
        update_image(steps);
        SDL_RenderPresent(global_data.ren);
        int interval = 100;
        double passed;
        if ((passed = SDL_GetTicks() - t) > interval) {
            std::cout << (int)round(count * 1000 / passed) << " steps per second\n";
            printf("%d steps per iteration\n", steps);
            double its = count / (double)steps * 1000 / passed;
            printf("%d iterations per second\n", (int)round(its));
            std::cout << "Diff: " << std::endl << step_diff << std::endl;
            if (total_diff > (global_data.surf->w * global_data.surf->h) * 4.) {
                save_image(step + 1);
                total_diff = 0;
            }
            if (step_diff < (global_data.surf->w * global_data.surf->h) / 500. && !last) {
                // save_image(step);
                last = true;
            }
            step_diff = 0;
            SDL_SetWindowTitle(win, std::to_string((int)(count * 1000 / passed)).c_str());
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

double sim(unsigned int* pixels, unsigned int* pixels2) {
    int h = global_data.surf->h;
    int w = global_data.surf->w;
    double sum = 0;
    for (int i = 0; i < h * w; i += 2) {
        sum += diff(pixels[i], pixels2[i]);
    }
    return sum;
}

double sim_blur(unsigned int* pixels, unsigned int* pixels2) {
    int h = global_data.surf->h;
    int w = global_data.surf->w;
    double sum = 0;
    for (int i = 0; i < h * w; ++i) {
        sum += diff(pixels[i], pixels2[i]);
    }
    return sum;
}

void update_image(int steps) {
    SDL_Renderer* ren = global_data.ren;
    SDL_Surface* surf = global_data.surf;
    int h = global_data.surf->h;
    int w = global_data.surf->w;
    unsigned int* pixels = (unsigned int*)surf->pixels;
    unsigned int* pixels2 = (unsigned int*)global_data.surf2->pixels;
    unsigned int* pixels3 = (unsigned int*)global_data.surf_original->pixels;
    SDL_FRect r = {0, 0, (float)w, (float)h};
    for (int i = 0; i < steps; i++) {
        auto sim_f = *sim_functions.front();
        double orig = sim_f(pixels, pixels2);
        int x = rand() % w;
        int y = rand() % h;
        int x1 = rand() % w;
        int y1 = rand() % h;
        SDL_Renderer* ren2 = SDL_CreateSoftwareRenderer(surf);
        SDL_SetRenderDrawColor(ren2, rand() % 255, rand() % 255, rand() % 255, 255);
        SDL_RenderLine(ren2, x, y, x1, y1);
        SDL_DestroyRenderer(ren2);
        double d1 = sim_f(pixels, pixels2);
        if (d1 < orig) {
            std::copy_n(pixels, h * w, pixels3);
            step_diff += orig - d1;
            total_diff += orig - d1;
        } else {
            std::copy_n(pixels3, h * w, pixels);
        }
        ++count;
        ++total_count;
    }
    SDL_UpdateTexture(global_data.tex, NULL, surf->pixels, surf->pitch);
    SDL_RenderTexture(ren, global_data.tex, NULL, &r);
}