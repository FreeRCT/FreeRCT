# FreeRCT build instructions #

Currently only Linux is officially supported, but Windows support should be possible with a few minor modifications. See [BuildProgramVisualStudio](BuildProgramVisualStudio.md) for instructions on building FreeRCT on Windows using Visual Studio.

## Checkout ##

The source code is hosted on Codeberg, so it is recommended you make a git clone of the repo. Alternatively, you can download a zip of the current state of the repo.

A command to make a git clone is `git clone https://codeberg.org/FreeRCT/FreeRCT`.

## Building the program ##

Almost everything is written in C++, which means you need **g++** or **clang++** to compile it. FreeRCT uses C++17 features, so g++ 7+ or clang 5+ is recommended.
In addition, you need:

  * **libpng** - Making the RCD data files that contain the graphics and other data read by the program.
  * **GLFW3**, **GLEW**, **Freetype** - Displaying graphics of the program.
  * **CMake>=2.8** & **make/ninja** - Building the program.
  * `[optional]` **lex/flex** - Scanner generator for generating RCD input files.
  * `[optional]` **yacc/bison** - Parser generator for generating RCD input files.

The existence of these programs/libraries is checked by `cmake`.

See the [README](../README.rst) for instructions how to build and run.
