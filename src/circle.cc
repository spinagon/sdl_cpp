#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <cmath>
#include <iostream>

#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_image.h>
#include <SDL3/SDL_main.h>

#define DEBUG std::cout << __LINE__ << "\n"

constexpr int M_PI=3.14159;
constexpr int W = 800;
constexpr int H = 600;

SDL_Texture* tex;

SDL_FColor hsv2rgb(double h, double s, double v) {
    double hh, p, q, t, ff;
    int vv, pp, qq, tt;
    long i;
    SDL_FColor out = {0, 0, 0, 1};

    if(s <= 0.0) {       // < is bogus, just shuts up warnings
        out.r = v;
        out.g = v;
        out.b = v;
        return out;
    }
    hh = h;
    while(hh >= 360.0) hh -= 360;
    hh /= 60.0;
    i = (long)hh;
    ff = hh - i;
    p = v * (1.0 - s);
    q = v * (1.0 - (s * ff));
    t = v * (1.0 - (s * (1.0 - ff)));

    switch(i) {
        case 0:
        out.r = v;
        out.g = t;
        out.b = p;
        break;
        case 1:
        out.r = q;
        out.g = v;
        out.b = p;
        break;
        case 2:
        out.r = p;
        out.g = v;
        out.b = t;
        break;
        case 3:
        out.r = p;
        out.g = q;
        out.b = v;
        break;
        case 4:
        out.r = t;
        out.g = p;
        out.b = v;
        break;
        case 5:
        default:
        out.r = v;
        out.g = p;
        out.b = q;
        break;
    }
    return out;
}

double clip(double n, double min, double max) {
    if (n > max) return max;
    if (n < min) return min;
    return n;
}

void draw_circle_lines(SDL_Renderer *ren, double x, double y, double r, int sides) {
    SDL_FPoint points[sides + 1];
    for (int i = 0; i < sides; i++) {
        points[i].x = x + r * cos(2 * M_PI * (double)i / sides);
        points[i].y = y + r * sin(2 * M_PI * (double)i / sides);
    }
    points[sides] = points[0];
    SDL_RenderLines(ren, points, sides + 1);
}

void draw_circle_geo(SDL_Renderer *ren, double x, double y, double r, int sides, SDL_FColor c) {
    SDL_Vertex points[sides]{};
    for (int i = 0; i < sides; i++) {
        points[i].position.x = x + r * cos(2 * M_PI * (double)i / sides);
        points[i].position.y = y + r * sin(2 * M_PI * (double)i / sides);
        points[i].color = c;
    }
    int indices[3 * (sides - 2)];
    for (int i = 0; i < sides - 2; i++) {
        indices[i * 3] = 0;
        indices[i * 3 + 1] = i + 1;
        indices[i * 3 + 2] = i + 2;
    }
    SDL_RenderGeometry(ren, NULL, points, sides, indices, 3 * (sides - 2));
}

int main(int argc, char* argv[]) {
    int res;
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        printf("SDL_Init Error: %s\n", SDL_GetError());
    }
    SDL_Window *win = SDL_CreateWindow("Hello World!", W, H, SDL_WINDOW_RESIZABLE);
    if (win == NULL) {
        printf("SDL_CreateWindow Error: %s\n", SDL_GetError());
        return 1;
    }
    SDL_Renderer *ren = SDL_CreateRenderer(win, "direct3d");
    tex = SDL_CreateTexture(ren,
        SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, W, H);
    SDL_SetRenderTarget(ren, tex);
    int quit = 0;
    SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
    SDL_RenderClear(ren);
    srand(time(NULL));
    int max_n = 1000;
    double x[max_n];
    double y[max_n];
    double xs[max_n];
    double ys[max_n];
    int n = 0;
    int r = rand() % 256;
    int g = rand() % 256;
    int b = rand() % 256;
    int w = W;
    int h = H;
    int t = clock();
    int t_fps = clock();
    int steps = 0;
    int total_steps = 0;
    double attr_strength = 0;
    double attr_x = 0;
    double attr_y = 0;
    if (argc > 1) {
        if (argv[1] == std::string("--bench")) {
            for (int i = 0; i < 100; i++) {
                x[n] = 300 + (i % 10) * 27;
                y[n] = 150 + (i / 10) * 27;
                xs[n] = 0.0;
                ys[n] = 0.0;
                n++;
            }
        }
    }
    while (!quit) {
        ++steps;
        ++total_steps;
        SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
        SDL_RenderClear(ren);
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) {
                quit = 1;
            }
            if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                if (e.button.button == 1) {
                    x[n] = e.button.x;
                    y[n] = e.button.y;
                    xs[n] = 0.0;
                    ys[n] = 0.0;
                    n++;
                } else {
                    attr_strength = 1;
                    attr_x = e.button.x;
                    attr_y = e.button.y;
                }
            }
            if (e.type == SDL_EVENT_MOUSE_BUTTON_UP) {
                attr_strength = 0;
            }
            if (e.type == SDL_EVENT_MOUSE_MOTION) {
                attr_x = e.motion.x;
                attr_y = e.motion.y;
            }
        }
        SDL_SetRenderDrawColor(ren, r % 256, g % 256, b % 256, 255);
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                double dx = x[i] - x[j];
                double dy = y[i] - y[j];
                if (fabs(dx) > 1 || fabs(dy) > 1) {
                    double angle = atan2(dy, dx);
                    double mag = 1 / hypot(dx, dy);
                    xs[i] -= pow(mag, 2) * cos(angle) * 1e1;
                    ys[i] -= pow(mag, 2) * sin(angle) * 1e1;
                    xs[i] += pow(mag * 10, 5) * cos(angle) * 1e1;
                    ys[i] += pow(mag * 10, 5) * sin(angle) * 1e1;
                }

                dx = x[i] - attr_x;
                dy = y[i] - attr_y;
                if (fabs(dx) > 10 || fabs(dy) > 10){
                    double angle = atan2(dy, dx);
                    double mag = 1 / hypot(dy, dx);
                    xs[i] -= mag * attr_strength * cos(angle) * 1e-1;
                    ys[i] -= mag * attr_strength * sin(angle) * 1e-1;
                }
            }
        }
        for (int i = 0; i < n; i++) {
            SDL_FColor c = hsv2rgb(cos(xs[i])*1000 + sin(ys[i])*1000, 0.8, 1);
            // std::cout << "";
            draw_circle_geo(ren, x[i], y[i], 5, 9, c);
            x[i] += xs[i];
            y[i] += ys[i];
            xs[i] *= 0.98;
            ys[i] *= 0.98;
            xs[i] = clip(xs[i], -1e0, 1e0);
            ys[i] = clip(ys[i], -1e0, 1e0);
            if (x[i] < 0) {
                xs[i] = fabs(xs[i]);
                x[i] = 0;
            }
            if (x[i] > w) {
                xs[i] = -fabs(xs[i]);
                x[i] = w;
            }
            if (y[i] < 0) {
                ys[i] = fabs(ys[i]);
                y[i] = 0;
            }
            if (y[i] > h) {
                ys[i] = -fabs(ys[i]);
                y[i] = h;
            }
        }
        if (clock() - t > 10) {
            SDL_RenderPresent(ren);
            SDL_GetCurrentRenderOutputSize(ren, &w, &h);
            t = clock();
        }
        if (clock() - t_fps > 1000) {
            double fps = steps * 1000. / (clock() - t_fps);
            char s[100];
            sprintf(s, "%.1lf", fps);
            SDL_SetWindowTitle(win, s);
            t_fps = clock();
            steps = 0;
        }
        SDL_Delay(1);
        // SDL_Surface* surf = SDL_RenderReadPixels(ren, NULL);
        // char s[100];
        // sprintf(s, "frames/%04d.png", total_steps);
        // IMG_SavePNG(surf, s);
        // SDL_DestroySurface(surf);
    }
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
