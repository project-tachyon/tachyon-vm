#pragma once

#include <Compiler.hpp>

/* some parts taken from lk and ppsspp (thanks :>) */

#define DebugInfo(...) Debug::Report("INFO: " __VA_ARGS__)
#define DebugWarn(...) Debug::Report("WARNING: " __VA_ARGS__)
#define DebugError(...) Debug::Report("ERROR: " __VA_ARGS__)

#define TachyonAssert(Condition) \
    do { if (unlikely((Condition) == false)) { DebugError("Tachyon assertion failed at (%s:%d): %s\n", __FILE__, __LINE__, #Condition); std::terminate(); } } while(false)

#define TachyonUnimplemented(...) \
    do { DebugError("Tachyon Unimplemented: " __VA_ARGS__); std::terminate(); } while(false)


namespace Debug {
    void Report(const char * Message, ...);
};
