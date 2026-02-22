#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keycode.h>
#include <Tachyon.hpp>
#include <Scratch.hpp>
#include <iostream>

using namespace std;

int main(int argc, char * argv[]) {
    cout << "Tachyon: the faster replacement for TurboWarp" << endl;
    /* check for potential file */
    if (argc < 2) {
        cout << "Please give a sb3 project to run." << endl;
        return -1;
    }
    /* initialize tachyon */
    Tachyon TachyonVM;
    if (TachyonVM.Init() < 0) {
        cout << SDL_GetError() << endl;
        return -1;
    }
    /* verify that the project exists, and parse the project */
    ScratchProject MainProject = ScratchProject(argv[1]);
    if (MainProject.ProjectZip_Path.empty() == true) {
        cout << "Failed to open " << argv[1] << endl;
        return -1;
    }
    if (MainProject.ParseContents() < 0) {
        cout << "Failed to parse project contents" << endl;
        return -1;
    }
    /* prepare for main loop */
    bool shouldExit = false;
    while(shouldExit == false) {
        SDL_Event event;
        /* update vars */
        TachyonVM.Update();
        while(SDL_PollEvent(&event) == true) {
            switch (event.type) {
                case SDL_EVENT_KEY_DOWN:
                    if (event.key.key == SDLK_ESCAPE) {
                        shouldExit = true;
                    }
                    break;
                case SDL_EVENT_QUIT:
                    shouldExit = true;
                    break;
                case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                    shouldExit = true;
                    break;
                default:
                    break;
            }
        }
        /* only render if anything has been rendered */
        TachyonVM.Render();
    }
    /* die tachyon, die */
    TachyonVM.Quit();
    return 0;
}
