#include <Scratch/Blocks.hpp>
#include <Scratch/Common.hpp>
#include <Tachyon.hpp>
#include <string_view>
#include <vector>

using namespace Scratch;

/**
 * True if all green flags in the project have been ran, false if otherwise.
 */
static bool RanGreenFlags = false;

/**
 * Map containing all opcode handlers.
 */
static std::unordered_map<std::string_view, OpcodeHandler> OpcodeHandlers;

/**
 * Map containing all reporter handlers.
 */
static std::unordered_map<std::string_view, EvaluationHandler> ReporterHandlers;

void ScratchBlock::LinkHandlers(void) {
    auto OpItem = OpcodeHandlers.find(this->Opcode);
    if (OpItem != OpcodeHandlers.end()) {
        this->Handler = OpItem->second;
    }
    auto EvalItem = ReporterHandlers.find(this->Opcode);
    if (EvalItem != ReporterHandlers.end()) {
        this->ReporterHandler = EvalItem->second;
    }
}

void Tachyon::RegisterOpHandler(std::string_view Opcode, OpcodeHandler Handler) {
    if (OpcodeHandlers.find(Opcode) != OpcodeHandlers.end()) {
        std::cerr << "WARNING: Conflicting opcode handlers for " << Opcode << std::endl;
        return;
    }
    OpcodeHandlers.emplace(Opcode, Handler);
}

void Tachyon::RegisterEvaluationHandler(std::string_view Opcode, EvaluationHandler Handler) {
    if (ReporterHandlers.find(Opcode) != ReporterHandlers.end()) {
        std::cerr << "WARNING: Conflicting reporter handlers for " << Opcode << std::endl;
        return;
    }
    ReporterHandlers.emplace(Opcode, Handler);
}

static inline ScratchStatus __hot ExecuteBlockFromID(ScratchSprite & Sprite, const std::string & Id) {   
    ScratchBlock * Block = Sprite.GetBlockFromId(Id);
    if (Block == nullptr) {
        std::cerr << "WARNING: Invalid block ID: " << Id << std::endl;
        return ScratchStatus::SCRATCH_END;
    }
    /* execute block */
    ScratchStatus Status = Block->Execute();
    return Status;
}

void __hot Tachyon::Step(ScratchProject & Project) {
    /* cpu branch predictor will love this (i hope) */
    if (unlikely(RanGreenFlags == false)) {
        /* fire up all green flags */
        for(size_t i = 0; i < Project.Sprites.size(); i++) {
            ScratchSprite * Sprite = Project.Sprites[i].get();
            for (size_t j = 0; j < Sprite->GreenFlags.size(); j++) {
                std::string NextKey = Sprite->GreenFlags[j]->GetNextKey();
                ExecuteBlockFromID(*Sprite, NextKey);
            }
        }
        RanGreenFlags = true;
    }
}
