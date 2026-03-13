# The Tachyon VM
Tachyon VM is a reimplementation of the Scratch VM written in C++ and SDL3. It aims to be the killer of any Scratch mod/runtime that currently exists. (I'm looking at you ScratchEverywhere)

# What makes this VM so great?
Tachyon VM utilizes modern SIMD features during JSON parsing, and during runtime to give a smooth experience when running big or small projects. Not only that, the VM makes use of compiler optimizations such as compiling with ``-O3``, using the hot attribute for improved CPU cache locality and more aggresive function optimizations, and ``likely()`` and ``unlikely()`` macros for improved CPU branch predictions.

# What can the VM do as of right now?
The VM can currently load huge Scratch projects in under 1 second (result given when testing Linux On Scratch).

# Compiling Tachyon
To compile Tachyon, you need to install a C++ compiler such as clang or g++, and libzip.\
To install libzip, run the command that is right for your Operating System:
## Linux
Debian/Ubuntu: ``sudo apt install libzip-dev``\
Fedora/RHEL: ``sudo dnf install libzip-devel``\
Arch Linux: ``sudo pacman -S libzip``
## MacOS
Homebrew: ``brew install libzip``

After libzip is installed on your system, simply run the following command: ``make``\
To clean the source tree of object files and such, run the following command: ``make clean``
