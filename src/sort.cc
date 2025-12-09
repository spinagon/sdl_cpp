#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <iostream>
#include <utility>

#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_main.h>

SDL_FColor hsv2rgb(double h, double s, double v);

void quick_sort(int* a, int start, int end);
int partition(int* a, int start, int end, int (*cmp)(int, int));
int cmp(int a, int b);
// void swap(int* a, int* b);
// void swap_n(int* a, int n, int i, int j);
void update_image();
double sortedness(int* a, int n);

void selection_sort(int* a, int n, int (*cmp)(int, int));
void insertion_sort(int* a, int n, int (*cmp)(int, int));

void use_texture(SDL_Renderer* ren) {
    SDL_Texture* tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, 800, 600);
    unsigned char* pixels;
    int pitch;
    SDL_LockTexture(tex, NULL, (void**)&pixels, &pitch);
    for (int i = 0; i < 100000; i++) {
        int x = rand() % (800 * 4);
        int y = rand() % 600;
        int offset = y * pitch + x;
        pixels[offset] = rand();
    }
    SDL_UnlockTexture(tex);
    SDL_RenderTexture(ren, tex, NULL, NULL);
    SDL_DestroyTexture(tex);
}

void use_surface(SDL_Renderer* ren) {
    SDL_Surface* surf = SDL_CreateSurface(800, 600, SDL_PIXELFORMAT_RGBA32);
    for (int i = 0; i < 100000; i++) {
        int x = rand() % (800 * 4);
        int y = rand() % 600;
        int offset = y * surf->pitch + x;
        ((unsigned char*)surf->pixels)[offset] = 255;
    }
    SDL_Texture* tex = SDL_CreateTextureFromSurface(ren, surf);
    SDL_RenderTexture(ren, tex, NULL, NULL);
    SDL_DestroyTexture(tex);
    SDL_DestroySurface(surf);
}

void use_draw(SDL_Renderer* ren) {
    for (int i = 0; i < 100000; i++) {
        int x = rand() % 800;
        int y = rand() % 600;
        SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
        SDL_RenderPoint(ren, x, y);
    }
}

struct global_data {
    SDL_Renderer* ren;
    int* data;
    int h;
    int w;
} global_data;

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *win = SDL_CreateWindow("", 800, 600, SDL_WINDOW_RESIZABLE);
    SDL_Renderer *ren = SDL_CreateRenderer(win, "direct3d");
    SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
    SDL_RenderClear(ren);
    srand(time(NULL));
    SDL_Event e;
    int quit = 0;
    int steps = 0;
    int t_fps = clock();
    int mode = 0;
    int h = 300;
    int w = 400;
    int* data = new int[h * w];
    global_data.ren = ren;
    global_data.data = data;
    global_data.h = h;
    global_data.w = w;
    for (int i = 0; i < h * w; i++) {
        data[i] = rand();
    }
    quick_sort(data, 0, h * w - 1);
    // selection_sort(data, h * w, cmp);
    // insertion_sort(data, h * w, cmp);
    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) {
                quit = 1;
            }
        }
        update_image();
        SDL_RenderPresent(global_data.ren);
        SDL_Delay(16);
    }
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}

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
    vv = (int)round((v * 255));
    pp = (int)round((p * 255));
    qq = (int)round((q * 255));
    tt = (int)round((t * 255));

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

void selection_sort(int* a, int n, int (*cmp)(int, int)) {
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            if (cmp(a[i], a[j]) > 0) {
                std::swap(a[i], a[j]);
            }
        }
    }
}

void insertion_sort(int* a, int n, int (*cmp)(int, int)) {
    for (int i = 1; i < n; i++) {
        for (int j = i; j > 0; j--) {
            if (cmp(a[j], a[j - 1]) > 0) {
                std::swap(a[j], a[j - 1]);
            } else {
                break;
            }
        }
    }
}

void quick_sort(int* a, int start, int end) {
    if (start >= end) {
        return;
    }
    int i_pivot = partition(a, start, end, &cmp);
    quick_sort(a, start, i_pivot);
    quick_sort(a, i_pivot + 1, end);
}

int partition(int* a, int start, int end, int (*cmp)(int, int)) {
    int pivot = a[start + (end - start) / 2];
    int i_pivot;
    int i = start - 1;
    int j = end + 1;
    while (1) {
        do {
            i++;
        } while (cmp(a[i], pivot) < 0);
        do {
            j--;
        } while (cmp(a[j], pivot) > 0);
        if (i >= j) {
            i_pivot = j;
            break;
        }
        std::swap(a[i], a[j]);
    }
    return i_pivot;
}

int cmp(int a, int b) {
    static int steps = 0;
    static int thr = 10000;
    static int t = 0;
    static double next_sortedness = 0;
    if (steps > thr) {
        steps = 0;
        thr = (int)round(thr * 1.1);
        printf("%d\n", thr);
        t = clock();
        double s = sortedness(global_data.data, global_data.h * global_data.w);
        if (s >= next_sortedness) {
            printf("%f\n", s);
            thr = (int)round(thr / 1.1);
            next_sortedness = s + 0.001;
            update_image();
            SDL_RenderPresent(global_data.ren);
            SDL_Delay(16);
        }
    }
    steps++;
    return (a > b) ? 1 : ((a < b) ? -1 : 0);
}

int cmp2(const void* a, const void* b) {
    return *(int*)a - *(int*)b;
}

double sortedness(int* a, int n) {
    long ordered_pairs = 0;
    for (long i = 0; i < n - 1; i++) {
        if (a[i] <= a[i + 1]) {
            ordered_pairs += 1;
        }
    }
    double ret = ordered_pairs / (double)(n - 1);
    return ret;
}

// void swap(int* a, int* b) {
//     int t = *a;
//     *a = *b;
//     *b = t;
// }

inline void unpack_rgba(uint32_t p, uint8_t &r, uint8_t &g, uint8_t &b) {
    r = (p >> 0) & 0xFF;
    g = (p >> 8) & 0xFF;
    b = (p >> 16) & 0xFF;
}

inline unsigned int pack_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return r | g << 8 | b << 16 | a << 24;
}

void update_image() {
    SDL_Renderer* ren = global_data.ren;
    int* data = global_data.data;
    int h = global_data.h;
    int w = global_data.w;
    SDL_FRect r = {0, 0, (float)w, (float)h};
    unsigned int* pixels = new unsigned int[h * w];
    SDL_Surface* surf = SDL_CreateSurfaceFrom(w, h, SDL_PIXELFORMAT_RGBA32, (void*)pixels, w * sizeof(int));
    for (int i = 0; i < w * h; i++) {
        SDL_FColor c = hsv2rgb(data[i] / (double)RAND_MAX * 360, 0.8, 0.8);
        pixels[i] = pack_rgba(c.r * 255, c.g * 255, c.b * 255, 255);
    }
    SDL_Texture* tex = SDL_CreateTextureFromSurface(ren, surf);
    SDL_DestroySurface(surf);
    delete[] pixels;
    SDL_RenderTexture(ren, tex, NULL, &r);
    SDL_DestroyTexture(tex);
}