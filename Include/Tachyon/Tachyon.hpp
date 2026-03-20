#pragma once

#include <Scratch/Procedures.hpp>
#include <Scratch/Common.hpp>
#include <Scratch/Blocks.hpp>
#include <Compiler.hpp>
#include <string_view>
#include <cstdint>

#include <SDL3/SDL_render.h>

/**
 * Configuration option: Script watchdog.
 * Enables a watchdog for all scripts.
 */
#define TACHYON_CFG_WATCHDOG  (1 << 0)

/**
 * Configuration option: Psuedo-block support.
 * When enabled, procedures with a certain names are not treated as normal procedures. They instead execute native machine code, which can significantly boosts performance for emulators.
 */
#define TACHYON_CFG_PBLOCK       (1 << 1)

namespace Tachyon {
    
    using TachyonConfig = uint16_t;

    struct VirtualMachine {
        Scratch::ScratchProject * Project = nullptr;
        SDL_Window * TachyonWindow = nullptr;
        SDL_Renderer * TachyonRenderer = nullptr;
        double Timer = 0;
        TachyonConfig Configuration;
        bool ShouldExit = false;
        bool RendererUpdate = false;
    };

    /**
     * Initializes SDL3.
     */
    int Init(void);

    /**
     * Initializes the scheduler.
     * @param The scratch project to execute.
     */
    void InitializeScheduler(Scratch::ScratchProject & Project);

    /**
     * Gets the VM information.
     * @return VM information.
     */
    VirtualMachine * GetVM(void);

    /**
     * Gets the VM configuration.
     * @return The VM configuration.
     */
    TachyonConfig GetConfigVM(void);

    /**
     * Adds a script to the ready queue of the scheduler.
     * @param The script
     */
    void ScriptAddReadyQueue(Scratch::ScratchScript Script);

    /**
     * Gets the loaded project.
     * @return The loaded project.
     */
    Scratch::ScratchProject * GetLoadedProject(void);

    /**
     * Gets the currently running script.
     * @return The script that is currently running.
     */
    Scratch::ScratchScript * GetCurrentScript(void);

    /**
     * Gets the stage sprite.
     * IMPORTANT: Must be called AFTER Tachyon::InitializeScheduler()
     * @return The stage sprite.
     */
    Scratch::ScratchSprite * GetStage(void);

    /**
     * Starts the main VM loop.
     */
    void __hot MainLoop(void);

    /**
     * Performs executions.
     * @param The scratch project to execute.
     * @return Returns true if the VM should exit, false if otherwise. 
     */
    bool __hot Step(void);

    /**
     * Renders sprites and anything else.
     */
    void __hot Render(void);

    /**
     * Registers an opcode handler.
     * @param Opcode string.
     * @param The function that handles the specific opcode.
     */
    void RegisterOpHandler(std::string_view Opcode, Scratch::OpcodeHandler Handler);

    /**
     * Registers an evaluation (reporter) handler.
     * @param Opcode string.
     * @param The function that handles the specific opcode.
     */
    void RegisterEvaluationHandler(std::string_view Opcode, Scratch::EvaluationHandler Handler);

    /**
     * De-initializes SDL3.
     */
    void Quit(void);

    /**
     * Causes Tachyon to exit.
     */
    void Exit(void);

    namespace Psuedo {
        bool IsPsuedo(std::string ProcCode);
        Scratch::ScratchStatus Execute(std::string ProcCode, Scratch::ScratchBlock & Block);
        void RegisterAll(void);
    };
};
