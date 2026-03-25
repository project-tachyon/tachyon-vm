#pragma once

#include <utility>

namespace Scratch {
    /**
     * Contains X and Y values for a sprite.
     */
    using ScratchPosition = std::pair<double, double>;

    namespace Motion {
        void RegisterAll(void);
    };
};
