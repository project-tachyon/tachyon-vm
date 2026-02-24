#ifndef TACHYON_SCRATCH_COMMON_HPP
#define TACHYON_SCRATCH_COMMON_HPP

#include <Scratch/Blocks.hpp>
#include <Scratch/Data.hpp>
#include <Lib/SIMDJson.h>
#include <string>
#include <string_view>
#include <vector>
#include <iostream>
#include <unordered_map>
#include <zip.h>

using namespace simdjson;

class ScratchSprite {
    public:
        std::string_view GetName(void) {
            return this->Name;
        }
        bool IsStage(void) {
            return this->StageSprite;
        }
        ScratchSprite(ondemand::object SpriteData) {
            /* fetch basic sprite data */
            this->StageSprite = SpriteData["isStage"]->get_bool();
            this->Name = SpriteData["name"]->get_string().value();
            /* Totals */
            VariableCount = SpriteData["variables"]->count_fields();
            ListCount = SpriteData["lists"]->count_fields();
            BroadcastCount = SpriteData["broadcasts"]->count_fields();
            BlockCount = SpriteData["blocks"]->count_fields();
            std::cout << "Total loads: " << (VariableCount + ListCount + BroadcastCount + BlockCount) << std::endl;
            /* 
             * TODO: optimize these 
             * There's definitely a way to reduce the code size. I'm thinking that templates could possibly do the trick,
             * but I'd have to test that out first to confirm, and right now I am not willing as I have bigger fish to fry.
             * */
            /* same for variables */
            ondemand::object VariableData = SpriteData["variables"]->get_object();
            for (ondemand::field VariableField : VariableData) {
                std::string_view VariableKey = VariableField.escaped_key();
                ScratchVariable Variable(SpriteData["variables"][VariableKey], this->StageSprite);
                this->Variables.insert({VariableKey, Variable});
            }
            /* lists */
            ondemand::object ListData = SpriteData["lists"]->get_object();
            for (ondemand::field ListField : ListData) {
                std::string_view ListKey = ListField.escaped_key();
                ScratchList List(SpriteData["lists"][ListKey], this->StageSprite);
                this->Lists.insert({ListKey, List});
            }
            /* broadcasts */
            ondemand::object BroadcastData = SpriteData["broadcasts"]->get_object();
            for (ondemand::field BroadcastField : BroadcastData) {
                std::string_view BroadcastKey = BroadcastField.escaped_key();
                ScratchBroadcast Broadcast(SpriteData["broadcasts"][BroadcastKey].value());
                this->Broadcasts.insert({BroadcastKey, Broadcast});
            }
            /* blocks */
            ondemand::object BlockData = SpriteData["blocks"]->get_object();
            for (ondemand::field BlockField : BlockData) {
                std::string_view BlockKey = BlockField.escaped_key();
                ScratchBlock * Block = new ScratchBlock(*this, SpriteData["blocks"][BlockKey]->get_object());
                this->Blocks.insert({BlockKey, Block});
            }
        }
        size_t ListCount = 0;
        size_t VariableCount = 0;
        size_t BroadcastCount = 0;
        size_t BlockCount = 0;
    private:
        bool StageSprite;
        std::string Name;
        std::unordered_map<std::string_view, ScratchVariable> Variables;
        std::unordered_map<std::string_view, ScratchList> Lists;
        std::unordered_map<std::string_view, ScratchBroadcast> Broadcasts;
        std::unordered_map<std::string_view, ScratchBlock *> Blocks;
        void DescendBlocks(ondemand::object BlocksObjects);
};

class ScratchProject {
    public:
        ScratchProject(const char * ZipPath) {
            this->ProjectZip = zip_open(ZipPath, 0, nullptr);
            if (this->ProjectZip == nullptr) {
                /* file doesn't exist */
                return;
            }
            this->ProjectZip_Path = std::string(ZipPath);
        }
        ~ScratchProject(void) {
            Close();
        }
        /* de-initializes project */
        void Close(void) {
            if (this->ProjectZip != nullptr) {
                if (zip_close(this->ProjectZip) < 0) {
                    zip_discard(this->ProjectZip);
                }
            }
            this->Sprites.clear();
            this->ProjectZip_Path.clear();
        }
        bool IsLoaded(void) {
            /* false if empty, true if otherwise */
            return ProjectZip_Path.empty() ? false : true;
        }
        int ParseContents(void);
        std::vector<ScratchSprite> Sprites;
    private:
        std::string ProjectZip_Path;
        zip_t * ProjectZip = nullptr;
        bool IsDirty = false;
        ondemand::document ProjectJson;
};

#endif
