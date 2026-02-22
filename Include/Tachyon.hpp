#ifndef __TACHYON_H
#define __TACHYON_H

#include <SDL3/SDL_main.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>

class Tachyon {
    public:
        int Init(void) {
            if (TachyonInitialized == true) {
                return 0;
            }
            if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
                return -1;
            }
            this->TachyonWindow = SDL_CreateWindow("Tachyon", 480, 360, 0);
            if (this->TachyonWindow == nullptr) {
                SDL_Quit();
                return -1;
            }
            this->TachyonRenderer = SDL_CreateRenderer(this->TachyonWindow, nullptr);
            if (this->TachyonRenderer == nullptr) {
                SDL_DestroyWindow(this->TachyonWindow);
                SDL_Quit();
                return -1;
            }
            TachyonInitialized = true;
            return 0;
        }
        inline int Quit(void) {
            if (this->TachyonWindow)
                SDL_DestroyWindow(this->TachyonWindow);
            if (this->TachyonRenderer)
                SDL_DestroyRenderer(this->TachyonRenderer);
            SDL_Quit();
            return 0;
        }

        void Render(void) {
            SDL_RenderClear(this->TachyonRenderer);
            SDL_SetRenderDrawColor(this->TachyonRenderer, 255, 255, 255, 0);
            SDL_RenderPresent(this->TachyonRenderer);
            return;
        }
        void Update(void) {
            return;
        }
    private:
        SDL_Window * TachyonWindow = nullptr;
        SDL_Renderer * TachyonRenderer = nullptr;
        bool TachyonInitialized = false;
};

#endif
