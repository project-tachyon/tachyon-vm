cmake_minimum_required(VERSION 3.20)
project(tachyon LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(TACHYON_SOURCE "${CMAKE_SOURCE_DIR}/Source")

file(GLOB_RECURSE CPP_SRC FOLLOW_SYMLINKS "${TACHYON_SOURCE}/*.cpp")
list(FILTER CPP_SRC EXCLUDE REGEX ".*/Libraries/AsmJit/.*")

add_executable(tachyon ${CPP_SRC})

target_include_directories(tachyon PRIVATE "${TACHYON_SOURCE}/Include")

target_compile_options(tachyon PRIVATE
    -Werror
    -Wextra
    -Wpointer-arith
    -Wcast-align
    -Wwrite-strings
    -Wredundant-decls
    -Wformat
    -Wformat-security
    -Wno-missing-field-initializers
    -fno-rtti
    -fno-exceptions
    -march=native
    -O3
)

# AsmJit
add_subdirectory("${TACHYON_SOURCE}/Libraries/AsmJit")
set(ASMJIT_STATIC ON)
set(ASMJIT_NOT_ASMJIT_INIT ON)

target_link_libraries(tachyon PRIVATE SDL3 zip asmjit)