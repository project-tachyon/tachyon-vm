#include <Tachyon/Encoder.hpp>
#include <Tachyon/Debug.hpp>
#include <Scratch/Common.hpp>
#include <Tachyon/Tachyon.hpp>
#include <iostream>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

// for compiler testing
int main(int argc, char * argv[]) {
    Tachyon_AMD64Encoder Encoder;
    std::cout << "Tachyon lives on!" << std::endl;
    return 0;
}

// int main(int argc, char * argv[]) {
//     cout << "Tachyon: the faster replacement for TurboWarp" << endl;
//     /* check for potential file */
//     if (argc < 2) {
//         cout << "Please give a sb3 project to run." << endl;
//         return -1;
//     }
//     /* initialize tachyon */
//     if (Tachyon::Init() < 0) {
//         cout << SDL_GetError() << endl;
//         return -1;
//     }
//     /* verify that the project exists, and parse the project */
//     Scratch::ScratchProject MainProject = Scratch::ScratchProject(argv[1]);
//     if (MainProject.IsLoaded() == false) {
//         cout << "Failed to open " << argv[1] << endl;
//         return -1;
//     }
//     if (MainProject.ParseContents() < 0) {
//         cout << "Failed to parse project contents" << endl;
//         return -1;
//     }
//     Tachyon::InitializeScheduler(MainProject);
//     DebugInfo("Beginning execution..\n");
//     Tachyon::MainLoop();
//     /* die tachyon, die */
//     Tachyon::Quit();
//     return 0;
//     /* global deconstructors do most of the project memory deallocation work for us */
// }