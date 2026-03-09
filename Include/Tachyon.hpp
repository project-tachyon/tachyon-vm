#pragma once

#include <Scratch/Common.hpp>
#include <Scratch/Blocks.hpp>
#include <Compiler.hpp>
#include <string_view>
#include <cstdint>

#define CONFIG_OPT_WATCHDOG (1 << 0)
#define CONFIG_OPT_PSUEDO   (1 << 1)

using TachyonConfig = uint16_t;

namespace Tachyon {
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
     * Performs executions.
     * @param The scratch project to execute.
     * @returns Returns true if the VM should exit, false if otherwise. 
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
};
