#pragma once

#include <Scratch/Common.hpp>
#include <Scratch/Blocks.hpp>
#include <Compiler.hpp>
#include <string_view>

namespace Tachyon {
    /**
     * Initializes SDL3.
     */
    int Init(void);

    /**
     * Performs executions.
     */
    void __hot Step(Scratch::ScratchProject & Project);

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
