#ifndef TACHYON_SCRATCH_COMMON_HPP
#define TACHYON_SCRATCH_COMMON_HPP

#include <Scratch/Blocks.hpp>
#include <Scratch/Data.hpp>
#include <Lib/SIMDJson.h>
#include <string>
#include <string_view>
#include <vector>
#include <iostream>
#include <memory>
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
            /* 
             * TODO: optimize these 
             * There's definitely a way to reduce the code size. I'm thinking that templates could possibly do the trick,
             * but I'd have to test that out first to confirm, and right now I am not willing as I have bigger fish to fry.
             * */
            /* same for variables */
            ondemand::object VariableData = SpriteData["variables"];
            for (ondemand::field VariableField : VariableData) {
                try {
                    std::string VariableKey = std::string(VariableField.unescaped_key()->data());
                    this->Variables.emplace(
                        VariableKey,
                        ScratchVariable(VariableField.value(), this->StageSprite)
                    );
                } catch (simdjson_error &Error) {
                    std::cerr << "failed to load variable: " << Error.what() << std::endl;
                    std::cerr << VariableField.value() << std::endl;
                }
            }
            /* lists */
            ondemand::object ListData = SpriteData["lists"];
            for (ondemand::field ListField : ListData) {
                try {
                    std::string ListKey = std::string(ListField.unescaped_key()->data());
                    this->Lists.emplace(
                        ListKey,
                        ScratchList(ListField.value(), this->StageSprite)
                    );
                } catch (simdjson_error &Error) {
                    std::cerr << "failed to load list: " << Error.what() << std::endl;
                    std::cerr << ListField.value() << std::endl;
                }
            }
            /* broadcasts */
            ondemand::object BroadcastData = SpriteData["broadcasts"];
            for (ondemand::field BroadcastField : BroadcastData) {
                try {
                    std::string BroadcastKey = std::string(BroadcastField.unescaped_key()->data());
                    this->Broadcasts.emplace(
                        BroadcastKey,
                        ScratchBroadcast(BroadcastField.value())
                    );
                } catch (simdjson_error &Error) {
                    std::cerr << "failed to load broadcast: " << Error.what() << std::endl;
                    std::cerr << BroadcastField.value() << std::endl;
                }
            }
            /* blocks */
            ondemand::object BlockData = SpriteData["blocks"];
            for (ondemand::field BlockField : BlockData) {
                try {
                    std::string BlockKey = std::string(BlockField.unescaped_key()->data());
                    this->Blocks.emplace(
                        BlockKey,
                        std::make_unique<ScratchBlock>(*this, BlockField.value())
                    );
                } catch (simdjson_error &Error) {
                    std::cerr << "failed to load block: " << Error.what() << std::endl;
                    std::cerr << BlockField.value() << std::endl;
                }
            }
        }
    private:
        bool StageSprite;
        std::string Name;
        std::unordered_map<std::string, ScratchVariable> Variables;
        std::unordered_map<std::string, ScratchList> Lists;
        std::unordered_map<std::string, ScratchBroadcast> Broadcasts;
        std::unordered_map<std::string, std::unique_ptr<ScratchBlock>> Blocks;
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
