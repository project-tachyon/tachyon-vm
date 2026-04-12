#pragma once

#include <Scratch/Common.hpp>
#include <cstdint>

using namespace Scratch;

namespace Tachyon {
    using EventHandler = void (*)();

    enum class Tachyon_EventType : uint8_t {
        BROADCAST,
        SPRITE_CLICK,
        LOUDNESS_THRESHOLD,
        TIMER_THRESHOLD,
        BACKDROP_UPDATE
    };

    struct Tachyon_EventSpriteClick {
        ScratchSprite & ClickedSprite;
    };

    void BindEvent(Tachyon_EventType Type, EventHandler Handler);
    // void FireEvent(std::variant<>);
}

namespace Scratch {
    namespace Events {
        void RegisterAll(void);
    }
}
