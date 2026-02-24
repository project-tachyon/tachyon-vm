#ifndef TACHYON_SCRATCH_BLOCKS_HPP
#define TACHYON_SCRATCH_BLOCKS_HPP

#include <Scratch/Data.hpp>
#include <string>
#include <Lib/SIMDJson.h>

using namespace simdjson;

class ScratchSprite;

class ScratchBroadcast {
    public:
        ScratchBroadcast(ondemand::value BroadcastName) {
            this->Name = BroadcastName.get_string();
        }
    private:
        std::string_view Name;
};

class ScratchBlock {
    public:
        ScratchBlock (ScratchSprite &OwnerSprite, ondemand::object BlockData) {
            this->Opcode = std::string(BlockData["opcode"]->get_string()->data());
            this->TopLevel = BlockData["topLevel"]->get_bool();
            this->Sprite = &OwnerSprite;
            /* basic info */
            if (BlockData["next"].is_null() == false) {
                this->NextBlock_Key = std::string(BlockData["next"]->get_string()->data());
            }
            if (BlockData["parent"].is_null() == false) {
                this->ParentBlock_Key = std::string(BlockData["parent"]->get_string()->data());
            }
        }
        ~ScratchBlock(void) {
            this->Inputs.clear();
        }
    private:
        void DescendInputs(ondemand::object InputObjects);
        ScratchSprite * Sprite;
        std::string Opcode;
        std::string NextBlock_Key;
        std::string ParentBlock_Key;
        std::vector<ScratchData> Inputs;
        bool TopLevel;
};

#endif
