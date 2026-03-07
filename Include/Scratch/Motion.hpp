#pragma once

namespace Scratch {
    /**
     * Contains X and Y values for a sprite.
     */
    typedef struct {
        double X;
        double Y;
    } ScratchPosition;

    namespace Motion {
        void RegisterAll(void);
    };
};
