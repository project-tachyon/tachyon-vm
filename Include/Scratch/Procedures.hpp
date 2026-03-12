#pragma once

#include <vector>
#include <string>

namespace Scratch {
    struct ScratchProcedure {
        std::vector<std::string> ParametersKeys;
        std::vector<std::string> ParametersNames;
        std::string ProcCode;
        std::string PrototypeKey;
        std::string DefinitionKey;
        bool UseWarp;
    };

    namespace Procedures {
        void RegisterAll(void);
    }
};
