#include <vector>
#include <cmath>
#include <iostream>
#include <algorithm>
#include <string>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_image.h>
#include "hsluv.h"

// Brush settings
int BRUSH_RADIUS = 15;

class Image {
public:
    int w, h;
    std::vector<double> r, g, b;
    std::vector<bool> mask;      // true = masked (missing), false = fixed

    Image() : w(0), h(0) {}

    virtual void init(int width, int height) {
        w = width;
        h = height;
        r.assign(w * h, 0.0);
        g.assign(w * h, 0.0);
        b.assign(w * h, 0.0);
        mask.assign(w * h, false);
    }

    virtual void fromSurface(SDL_Surface* surf) {};
    virtual void toSurface(SDL_Surface* surf) {
        this->toPixels(surf->pixels, surf->pitch);
    }
    virtual void toTexture(SDL_Texture* tex) {
        void* pixels = nullptr;
        int pitch;
        if (SDL_LockTexture(tex, NULL, &pixels, &pitch)) {
            this->toPixels(pixels, pitch);
            SDL_UnlockTexture(tex);
        }
    }
    virtual void toPixels(void* pixels, int pitch) {};
};

class FloatImage : public Image {
public:
    FloatImage() : Image() {}

    virtual void fromSurface(SDL_Surface* surf) {
        this->init(surf->w, surf->h);
        uint32_t* pixels = (uint32_t*)surf->pixels;
        
        for (int i = 0; i < this->w * this->h; ++i) {
            uint32_t p = pixels[i];
            
            uint8_t r = (p >> 16) & 0xFF;
            uint8_t g = (p >> 8) & 0xFF;
            uint8_t b = (p >> 0)  & 0xFF;

            this->r[i] = (double)r;
            this->g[i] = (double)g;
            this->b[i] = (double)b;
            this->mask[i] = false;
        }
    }

    virtual void toPixels(void* pixels, int pitch) {
        uint8_t* bytePixels = (uint8_t*)pixels;
        for (int y = 0; y < this->h; ++y) {
            uint32_t* row = (uint32_t*)(bytePixels + y * pitch);
            for (int x = 0; x < this->w; ++x) {
                int idx = y * this->w + x;
                
                uint8_t r = (uint8_t)std::clamp(this->r[idx], 0.0, 255.0);
                uint8_t g = (uint8_t)std::clamp(this->g[idx], 0.0, 255.0);
                uint8_t b = (uint8_t)std::clamp(this->b[idx], 0.0, 255.0);
                
                row[x] = (255 << 24) | (r << 16) | (g << 8) | b;
            }
        }
    }
};

class HSLuvImage : public Image {
public:
    HSLuvImage() : Image() {}

    virtual void fromSurface(SDL_Surface* surf) {
        this->init(surf->w, surf->h);

        uint32_t* pixels = (uint32_t*)surf->pixels;
        
        for (int i = 0; i < this->w * this->h; ++i) {
            uint32_t p = pixels[i];
            
            uint8_t r = (p >> 16) & 0xFF;
            uint8_t g = (p >> 8) & 0xFF;
            uint8_t b = (p >> 0)  & 0xFF;

            rgb2hsluv(r / 255.0, g / 255.0, b / 255.0, &this->r[i], &this->g[i], &this->b[i]);

            this->mask[i] = false;
        }
    }

    virtual void toPixels(void* pixels, int pitch) {
        uint8_t* bytePixels = (uint8_t*)pixels;
        for (int y = 0; y < this->h; ++y) {
            uint32_t* row = (uint32_t*)(bytePixels + y * pitch);
            for (int x = 0; x < this->w; ++x) {
                int idx = y * this->w + x;
                
                uint8_t r = (uint8_t)std::clamp(this->r[idx], 0.0, 255.0);
                uint8_t g = (uint8_t)std::clamp(this->g[idx], 0.0, 255.0);
                uint8_t b = (uint8_t)std::clamp(this->b[idx], 0.0, 255.0);
                
                row[x] = (255 << 24) | (r << 16) | (g << 8) | b;
            }
        }
    }
};

// Helper: Load image from disk and convert to FloatImage
bool loadImage(const std::string& path, Image& img) {
    SDL_Surface* loadedSurface = IMG_Load(path.c_str());
    if (!loadedSurface) {
        std::cerr << "IMG_Load failed: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_Surface* formattedSurf = SDL_ConvertSurface(loadedSurface, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(loadedSurface);

    if (!formattedSurf) return false;

    img.fromSurface(formattedSurf);

    SDL_DestroySurface(formattedSurf);
    return true;
}

void saveImage(Image& img, const std::string& path) {
    SDL_Surface* surf = SDL_CreateSurface(img.w, img.h, SDL_PIXELFORMAT_RGBA32);
    img.toSurface(surf);
    IMG_SavePNG(surf, path.c_str());
    SDL_DestroySurface(surf);
}

// Draw the mask at mouse position
void applyBrush(Image& img, int mx, int my, bool clear=false) {
    int r2 = BRUSH_RADIUS * BRUSH_RADIUS;

    for (int y = my - BRUSH_RADIUS; y <= my + BRUSH_RADIUS; ++y) {
        for (int x = mx - BRUSH_RADIUS; x <= mx + BRUSH_RADIUS; ++x) {
            if (x >= 0 && x < img.w && y >= 0 && y < img.h) {
                int dx = x - mx;
                int dy = y - my;
                if (dx*dx + dy*dy <= r2) {
                    int idx = y * img.w + x;
                    img.mask[idx] = true;
                    if (clear) {
                        img.r[idx] = 0.0;
                        img.g[idx] = 0.0;
                        img.b[idx] = 0.0;
                    }
                }
            }
        }
    }
}

void loadMask(FloatImage& img, FloatImage& mask) {
    for (int i = 0; i < img.h * img.w; ++i) {
        if (mask.r[i] == 0 && mask.g[i] == 0 && mask.b[i] == 0) {
            img.mask[i] = true;
        }
    }
}

// 1. LAPLACE SOLVER (Membrane)
// Minimizes 1st Derivative (Gradient). Creates a tight, smooth transition.
// Stencil: 4 neighbors.
void solveLaplaceStep(Image& img) {
    int w = img.w;
    int h = img.h;

    for (int y = 1; y < h - 1; ++y) {
        for (int x = 1; x < w - 1; ++x) {
            int idx = y * w + x;
            if (img.mask[idx]) {
                int n_up    = (y - 1) * w + x;
                int n_down  = (y + 1) * w + x;
                int n_left  = y * w + (x - 1);
                int n_right = y * w + (x + 1);

                img.r[idx] = (img.r[n_up] + img.r[n_down] + img.r[n_left] + img.r[n_right]) * 0.25f;
                img.g[idx] = (img.g[n_up] + img.g[n_down] + img.g[n_left] + img.g[n_right]) * 0.25f;
                img.b[idx] = (img.b[n_up] + img.b[n_down] + img.b[n_left] + img.b[n_right]) * 0.25f;
            }
        }
    }
}

// 2. BIHARMONIC SOLVER (Thin Plate)
// Minimizes 2nd Derivative (Curvature). Preserves slopes/gradients entering the hole.
// Stencil: 13 points (Center, 4 Neighbors, 4 Diagonals, 4 Far Neighbors).
void solveBiharmonicStep(Image& img) {
    int w = img.w;
    int h = img.h;

    // We must start from 2 and end at h-2 because the stencil looks 2 pixels out
    for (int y = 2; y < h - 2; ++y) {
        for (int x = 2; x < w - 2; ++x) {
            int idx = y * w + x;

            if (img.mask[idx]) {
                // Immediate Neighbors (Weight 8)
                int n_up    = (y - 1) * w + x;
                int n_down  = (y + 1) * w + x;
                int n_left  = y * w + (x - 1);
                int n_right = y * w + (x + 1);

                // Diagonals (Weight -2)
                int n_ul = (y - 1) * w + (x - 1);
                int n_ur = (y - 1) * w + (x + 1);
                int n_dl = (y + 1) * w + (x - 1);
                int n_dr = (y + 1) * w + (x + 1);

                // Far Neighbors (Weight -1)
                int n_up2    = (y - 2) * w + x;
                int n_down2  = (y + 2) * w + x;
                int n_left2  = y * w + (x - 2);
                int n_right2 = y * w + (x + 2);

                // Formula: 20*I = 8*Sum(N) - 2*Sum(D) - 1*Sum(F)
                // Therefore I = ( ... ) / 20.0
                
                auto computeChannel = [&](const std::vector<double>& c) -> double {
                    double sum_n = c[n_up] + c[n_down] + c[n_left] + c[n_right];
                    double sum_d = c[n_ul] + c[n_ur] + c[n_dl] + c[n_dr];
                    double sum_f = c[n_up2] + c[n_down2] + c[n_left2] + c[n_right2];
                    return (8.0 * sum_n - 2.0 * sum_d - 1.0 * sum_f) / 20.0;
                };

                img.r[idx] = computeChannel(img.r);
                img.g[idx] = computeChannel(img.g);
                img.b[idx] = computeChannel(img.b);
            }
        }
    }
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL init failed: " << SDL_GetError() << std::endl;
        return 1;
    }

    FloatImage image;
    std::string imagePath = "c:/china.jpg"; // Default
    if (argc > 1) imagePath = argv[1];
    
    if (!loadImage(imagePath, image)) {
        std::cout << "Could not load '" << imagePath << "'. Generating synthetic gradient." << std::endl;
        image.init(800, 600);
        for(int y=0; y<600; y++) 
            for(int x=0; x<800; x++) {
                int idx = y*800+x;
                image.r[idx] = (double)x * 255.0/800.0;
                image.g[idx] = (double)y * 255.0/600.0;
                image.b[idx] = 128.0;
            }
    }

    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--mask") {
            if (i + 1 == argc) {
                loadMask(image, image);
            } else {
                FloatImage mask;
                loadImage(argv[i + 1], mask);
                loadMask(image, mask);
            }
        }
    }

    SDL_Window* window = SDL_CreateWindow("SDL3 Inpainting - Mode: Laplace", image.w, image.h, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, 
                                             SDL_TEXTUREACCESS_STREAMING, image.w, image.h);
    if (!texture) {
        std::cerr << "Failed to create texture: " << SDL_GetError() << std::endl;
        return 1;
    }

    bool quit = false;
    bool isLeftMouseDown = false;
    bool isSolving = false;
    bool useBiharmonic = false; // Toggle state
    bool textureNeedsUpdate = true;

    SDL_Event e;

    std::cout << "Controls:\n" 
              << "  [Left Mouse] Draw Mask\n" 
              << "  [Space]      Toggle Solving\n" 
              << "  [B]          Toggle Algorithm (Laplace vs Biharmonic)\n" 
              << "  [[/]]        Brush Size\n" 
              << "  [R]          Reload Image\n" << std::endl;

    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) {
                quit = true;
            }
            else if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                if (e.button.button == SDL_BUTTON_LEFT) {
                    isLeftMouseDown = true;
                    applyBrush(image, (int)e.button.x, (int)e.button.y, !isSolving);
                    textureNeedsUpdate = true;
                }
            }
            else if (e.type == SDL_EVENT_MOUSE_BUTTON_UP) {
                if (e.button.button == SDL_BUTTON_LEFT) {
                    isLeftMouseDown = false;
                }
            }
            else if (e.type == SDL_EVENT_MOUSE_MOTION) {
                if (isLeftMouseDown) {
                    applyBrush(image, (int)e.motion.x, (int)e.motion.y, !isSolving);
                    textureNeedsUpdate = true;
                }
            }
            else if (e.type == SDL_EVENT_KEY_DOWN) {
                switch (e.key.key) {
                    case SDLK_SPACE:
                        isSolving = !isSolving;
                        break;
                    case SDLK_R:
                        loadImage(imagePath, image);
                        textureNeedsUpdate = true;
                        isSolving = false;
                        std::cout << "Image Reloaded" << std::endl;
                        break;
                    case SDLK_S:
                        saveImage(image, "saved.png");
                        std::cout << "Image Saved" << std::endl;
                        break;
                    case SDLK_Q:
                        exit(0);
                        break;
                    case SDLK_RIGHTBRACKET:
                        BRUSH_RADIUS = std::min(100, BRUSH_RADIUS + 2);
                        std::cout << "Brush Radius: " << BRUSH_RADIUS << std::endl;
                        break;
                    case SDLK_LEFTBRACKET:
                        BRUSH_RADIUS = std::max(2, BRUSH_RADIUS - 2);
                        std::cout << "Brush Radius: " << BRUSH_RADIUS << std::endl;
                        break;
                    case SDLK_B:
                        useBiharmonic = !useBiharmonic;
                        SDL_SetWindowTitle(window, useBiharmonic ? 
                            "SDL3 Inpainting - Mode: Biharmonic (Curvature)" : 
                            "SDL3 Inpainting - Mode: Laplace (Gradient)");
                        std::cout << "Algorithm: " << (useBiharmonic ? "Biharmonic" : "Laplace") << std::endl;
                        break;
                }
            }
        }

        // Run Solver Logic
        if (isSolving) {
            // Biharmonic is heavier, so we might do fewer iterations to keep FPS up
            // or just do the same amount if the CPU is fast enough.
            int iterations = 40; 
            for (int k = 0; k < iterations; ++k) {
                if (useBiharmonic) {
                    solveBiharmonicStep(image);
                } else {
                    solveLaplaceStep(image);
                }
            }
            textureNeedsUpdate = true;
        }

        if (textureNeedsUpdate) {
            void* pixels = nullptr;
            int pitch = 0;
            
            image.toTexture(texture);
    
            textureNeedsUpdate = false;
        }

        SDL_RenderClear(renderer);
        SDL_RenderTexture(renderer, texture, NULL, NULL);
        
        // Brush Cursor
        {
            float mx, my;
            SDL_GetMouseState(&mx, &my);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 128);
            SDL_FRect rect = {mx - BRUSH_RADIUS, my - BRUSH_RADIUS, (float)BRUSH_RADIUS*2, (float)BRUSH_RADIUS*2};
            SDL_RenderRect(renderer, &rect);
        }

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}