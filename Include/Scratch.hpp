#ifndef __PROJECT_HPP
#define __PROJECT_HPP

#include <Json.hpp>
#include <string>
#include <vector>
#include <iostream>
#include <unordered_map>
#include <variant>
#include <cstdint>
#include <zip.h>

using json = nlohmann::ordered_json;

class ScratchVariable_Base {
    public:
        std::string Name;
        bool IsPublic;
};

class ScratchVariable : ScratchVariable_Base {
    public:
        ScratchVariable(json VariableData, bool Public) {
            /* VariableData[0] = list name, VariableData[1] = actual data */
            this->Name = VariableData[0];
            this->IsPublic = Public;
        }
    private:
        std::variant<uint64_t, std::string, double> Data;
};

class ScratchList : ScratchVariable_Base {
    public:
        ScratchList(json ListData, bool Public) {
            /* ListData[0] = list name, ListData[1] = actual data */
            this->Name = ListData[0];
            this->IsPublic = Public;
            this->TotalItems = ListData[1].size();
        }
        size_t TotalItems;
    private:
        std::vector<std::variant<uint64_t, std::string>> Elements;
};

class ScratchSprite {
    public:
        ScratchSprite(json SpriteData) {
            /* fetch basic sprite data */
            this->IsStage = SpriteData["isStage"];
            this->Name = SpriteData["name"];
            size_t ListCount = SpriteData["lists"].size();
            size_t VariableCount = SpriteData["variables"].size();
            /* add lists only if there's any in the sprite */
            if (ListCount > 0) {
                /* range-based for loops are very interesting */
                for (const auto& [ListKey, ListData]: SpriteData["lists"].items()) {
                    ScratchList List(ListData, this->IsStage);
                    this->Lists.insert({ListKey, List});
                }
            }
            /* same for variables */
            if (VariableCount > 0) {
                for (const auto& [VariableKey, VariableData]: SpriteData["variables"].items()) {
                    ScratchVariable Variable(VariableData, this->IsStage);
                    this->Variables.insert({VariableKey, Variable});
                }
            }
            Dump();
        }
        void Dump(void) {
            std::cout << "------ SPRITE DUMP ------" << std::endl;
            std::cout << "Is stage: " << (this->IsStage ? "true" : "false") << std::endl;
            std::cout << "Sprite name: " << this->Name << std::endl;
            std::cout << "Total variables: " << this->Variables.size() << std::endl;
            std::cout << "Total lists: " << this->Lists.size() << std::endl;
        }
        bool IsStage;
        std::string Name;
    private:
        std::unordered_map<std::string, ScratchList> Lists;
        std::unordered_map<std::string, ScratchVariable> Variables;
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
        inline void Close(void) {
            if (ProjectZip != nullptr) {
                if (zip_close(ProjectZip) < 0) {
                    zip_discard(ProjectZip);
                }
            }
            Sprites.clear();
        }
        int ParseContents(void);
        std::string ProjectZip_Path;
        std::vector<ScratchSprite> Sprites;
    private:
        bool IsDirty = false;
        zip_t * ProjectZip = nullptr;
        json ProjectJson;
};

#endif
