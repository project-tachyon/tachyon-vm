#pragma once

#include <Tachyon/Debug.hpp>
#include <Scratch/Procedures.hpp>
#include <Scratch/Motion.hpp>
#include <Scratch/Blocks.hpp>
#include <Scratch/Data.hpp>
#include <Lib/SIMDJson.h>
#include <string>
#include <string_view>
#include <vector>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <map>
#include <zip.h>

using namespace simdjson;

/* control flags. any bits that arent here are reserved */
#define SCRIPT_INSIDE_PROCEDURE         (1 << 0)
#define SCRIPT_INVALIDATE_BLOCK         (1 << 1)
#define SCRIPT_SHOULD_STAY              (1 << 2)

namespace Scratch {

    class ScratchSprite;

    /**
     * Stack information for script.
     */
    struct Script_StackFrame {
        std::string ReturnId;
        std::string RepeatId;
        double RepeatsLeft = -1;
        bool InsideProcedure;
    };

    /**
     * Contains the information of a script.
     */
    struct ScratchScript {
        std::string FirstBlockId;
        std::string CurrentBlockId;
        std::vector<Script_StackFrame> ReturnStack;
        ScratchSprite * Sprite;
        ScratchStatus CurrentStatus;
        uint8_t ControlFlags;
    };

    static inline void __hot SetControlFlag(ScratchScript & Script, uint8_t Flag) {
        Script.ControlFlags |= Flag;
    }

    static inline void __hot UnsetControlFlag(ScratchScript & Script, uint8_t Flag) {
        Script.ControlFlags &= ~(Flag);
    }

    static inline bool __hot GetControlFlag(ScratchScript & Script, uint8_t Flag) {
        return (Script.ControlFlags & Flag);
    }

    static inline void __hot ScriptReturn(ScratchScript & Script) {
        if (Script.ReturnStack.empty() == true) {
            DebugError("Return stack is EMPTY!\n");
            DebugInfo("Script control flags: 0x%02x\n", Script.ControlFlags);
            return;
        }
        Script_StackFrame & CurrentStackFrame = Script.ReturnStack.back();
        /* restore procedure control flag */
        Script.ControlFlags &= ~(SCRIPT_INSIDE_PROCEDURE);
        Script.ControlFlags |= CurrentStackFrame.InsideProcedure;

        Script.CurrentBlockId = CurrentStackFrame.ReturnId;
        Script.ReturnStack.pop_back();
    }

    /**
     * Only works for block keys.
     * @param The block key
     * @return A 64-bit ID
     */
    inline uint32_t IdToU64(const std::string_view Key) {
        uint64_t IdU64 = 0;
        memcpy(&IdU64, Key.data(), std::min(Key.size(), sizeof(uint64_t)));
        return IdU64;
    }

    /**
     * Contains a scratch sprite's information.
     */
    class ScratchSprite {
        public:
            /**
             * Gets the Scratch sprite's name.
             * @return The sprite's name.
             */
            inline std::string_view GetName(void) {
                return this->Name;
            }
            /**
             * If true, the sprite is publicly accessible. Otherwise, it is local to the sprite.
             * @return A boolean that tells whether a variable is public or not.
             */
            inline bool IsStage(void) {
                return this->StageSprite;
            }
            /**
             * Gets a block from its ID.
             * @param The block's ID.
             * @return A pointer to the block's data.
             */
            inline ScratchBlock * __hot GetBlockFromId(const std::string & Id) {
                if (unlikely(Id.empty() == true)) {
                    return nullptr;
                }
                uint32_t IdU32 = IdToU64(Id);
                auto Item = this->Blocks.find(IdU32);
                if (unlikely(Item != this->Blocks.end())) {
                    return Item->second.get();
                }
                Item = this->ProcedureDefinitions.find(IdU32);
                if (unlikely(Item != this->ProcedureDefinitions.end())) {
                    return Item->second.get();
                }
                return nullptr;
            }

            /**
             * Contains scratch broadcast information.
             */
            struct ScratchBroadcast {
                std::string Name;
                std::string Key;
            };

            /**
             * ScratchSprite constructor.
             * @param The sprite's JSON data.
             */
            ScratchSprite(ondemand::object SpriteData) {
                /* fetch basic sprite data */
                this->StageSprite = SpriteData["isStage"]->get_bool().value();
                this->Name = SpriteData["name"]->get_string().value();
                if (this->StageSprite == false) {
                    double SpriteX = SpriteData["x"]->get_number()->as_double();
                    double SpriteY = SpriteData["y"]->get_number()->as_double();
                    this->Position = { .X = SpriteX, .Y = SpriteY };
                }
                /* 
                 * TODO: optimize these 
                 * There's definitely a way to reduce the code size. I'm thinking that templates could possibly do the trick,
                 * but I'd have to test that out first to confirm, and right now I am not willing as I have bigger fish to fry.
                 * */
                /* same for variables */
                ondemand::object VariableData = SpriteData["variables"]->get_object().value();
                for (ondemand::field VariableField : VariableData) {
                    try {
                        std::string VariableKey(VariableField.unescaped_key().value());
                        this->Variables.emplace(
                            VariableKey,
                            ScratchVariable(VariableField.value(), this->StageSprite)
                        );
                    } catch (simdjson_error &Error) {
                        std::cerr << "failed to load variable: " << Error.what() << std::endl;
                    }
                }
                /* lists */
                ondemand::object ListData = SpriteData["lists"]->get_object().value();
                for (ondemand::field ListField : ListData) {
                    try {
                        std::string ListKey(ListField.unescaped_key().value());
                        this->Lists.emplace(
                            ListKey,
                            ScratchList(ListField.value(), this->StageSprite)
                        );
                    } catch (simdjson_error &Error) {
                        std::cerr << "failed to load list: " << Error.what() << std::endl;
                    }
                }
                /* blocks */
                ondemand::object BlockData = SpriteData["blocks"]->get_object().value();
                for (ondemand::field BlockField : BlockData) {
                    try {
                        std::string BlockKey(BlockField.unescaped_key().value());
                        /* lightning fast when looking for blocks */
                        uint64_t BlockIdU64 = IdToU64(BlockKey);
                        std::unique_ptr<ScratchBlock> Block = std::make_unique<ScratchBlock>(BlockKey, BlockField.value(), *this);
                        if (Block->GetOpcode() == "event_whenflagclicked") {
                            this->GreenFlags.insert({ BlockIdU64, std::move(Block) });
                        } else if (Block->IsProcedureDef() == true) {
                            this->ProcedureDefinitions.insert({ BlockIdU64, std::move(Block) });
                        } else if (Block->GetOpcode() == "event_whenbroadcastreceived") {
                            this->BroadcastReceivers.insert({ BlockIdU64, std::move(Block) });
                        } else {
                            this->Blocks.insert({ BlockIdU64, std::move(Block) });
                        }
                    } catch (simdjson_error &Error) {
                        std::cerr << "failed to load block: " << Error.what() << std::endl;
                    }
                }
                /* all blocks loaded. no missing dependencies to worry about */
                if (this->ProcedureDefinitions.empty() == false) {
                    this->ResolveProcedureDefinitions();
                }
                this->CreateScripts();
            }
            void CreateScript(ScratchBlock & Block);
            ScratchVariable * __hot GetVariable(std::string VarKey);
            ScratchList * __hot GetList(std::string ListKey);           

            std::map<uint32_t, std::unique_ptr<ScratchBlock>> GreenFlags;
            std::unordered_map<uint32_t, std::unique_ptr<ScratchBlock>> BroadcastReceivers;
            std::unordered_map<uint32_t, std::unique_ptr<ScratchBlock>> Blocks;
            std::unordered_map<uint32_t, std::unique_ptr<ScratchBlock>> ProcedureDefinitions;
            std::unordered_map<std::string, ScratchVariable> Variables;
            std::unordered_map<std::string, ScratchList> Lists;
            std::vector<ScratchScript> Scripts;
            std::vector<ScratchProcedure> Procedures;
            ScratchPosition Position;
        private:
            bool StageSprite;
            std::string Name;
            void CreateScripts(void);
            void DescendBlocks(ondemand::object BlocksObjects);
            void ResolveProcedureDefinitions(void);
    };

    /**
     * Contains a scratch project's information.
     */
    class ScratchProject {
        public:
            /**
             * ScratchProject constructor.
             * @param The scratch project's SB3 file path.
             */
            explicit ScratchProject(std::string ZipPath) {
                this->ProjectZip = zip_open(ZipPath.c_str(), 0, nullptr);
                if (this->ProjectZip == nullptr) {
                    /* file doesn't exist */
                    return;
                }
                this->ProjectZip_Path = ZipPath;
            }

            ~ScratchProject(void) {
                Close();
            }

            /**
             * De-initializes and closes the project and it's file.
             */
            void Close(void) {
                if (this->ProjectZip != nullptr) {
                    if (zip_close(this->ProjectZip) < 0) {
                        zip_discard(this->ProjectZip);
                    }
                }
                this->Sprites.clear();
                this->ProjectZip_Path.clear();
            }

            //ScratchBlock * SearchBroadcast(std::string_view Name);

            /**
             * Checks whether the project has been loaded.
             * @return Returns true if it has been loaded, otherwise false.
             */
            bool IsLoaded(void) {
                return ProjectZip_Path.empty() == false;
            }

            /**
             * Parses the Scratch project into objects and prepares essential data.
             */
            int ParseContents(void);
            std::vector<std::unique_ptr<ScratchSprite>> Sprites;

        private:
            std::string ProjectZip_Path;
            zip_t * ProjectZip = nullptr;
            /**
             * If true, the project has been modified. Otherwise, it is false.
             */
            bool IsDirty = false;
    };
};
