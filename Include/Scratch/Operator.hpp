#pragma once

#include <cstdint>

namespace Scratch {

    namespace Operator {
        enum class MathOperation : uint8_t { MathInvalid, MathFloor };
        
        void RegisterAll(void);
    };
};
