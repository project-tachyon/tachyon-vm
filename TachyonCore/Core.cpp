#include <Scratch/Operator.hpp>
#include <Scratch/Motion.hpp>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>
#include <Scratch/Common.hpp>
#include <Tachyon.hpp>

static SDL_Window * TachyonWindow = nullptr;
static SDL_Renderer * TachyonRenderer = nullptr;
static bool TachyonInitialized = false;

int Tachyon::Init(void) {
    if (TachyonInitialized == true) {
        return 0;
    }
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        return -1;
    }
    TachyonWindow = SDL_CreateWindow("Tachyon", 480, 360, 0);
    if (TachyonWindow == nullptr) {
        SDL_Quit();
        return -1;
    }
    TachyonRenderer = SDL_CreateRenderer(TachyonWindow, nullptr);
    if (TachyonRenderer == nullptr) {
        SDL_DestroyWindow(TachyonWindow);
        SDL_Quit();
        return -1;
    }
    Scratch::Motion::RegisterAll();
    Scratch::Operator::RegisterAll();
    TachyonInitialized = true;
    return 0;
}

void __hot Tachyon::Render(void) {
    SDL_RenderClear(TachyonRenderer);
    SDL_SetRenderDrawColor(TachyonRenderer, 255, 255, 255, 0);
    SDL_RenderPresent(TachyonRenderer);
    return;
}

void Tachyon::Quit(void) {
    if (TachyonWindow)
        SDL_DestroyWindow(TachyonWindow);
    if (TachyonRenderer)
        SDL_DestroyRenderer(TachyonRenderer);
    SDL_Quit();
}
