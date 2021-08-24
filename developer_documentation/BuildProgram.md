# FreeRCT build instructions #

Currently only Linux is officially supported, but Windows support should be possible with a few minor modifications. See [BuildProgramVisualStudio](BuildProgramVisualStudio.md) for instructions on building FreeRCT on Windows using Visual Studio.

## Checkout ##

The source code is hosted on GitHub, so it is recommended you make a git clone of the repo. Alternatively, you can download a zip of the current state of the repo.

A command to make a git clone is `git clone https://github.com/FreeRCT/FreeRCT`.

## Building the program ##

Almost everything is written in C++, which means you need **g++** or **clang++** to compile it. FreeRCT uses C++11 features, so g++ 4.8+ or clang 3.3+ is recommended.
In addition, you need:

  * **libpng** - Making the RCD data files that contain the graphics and other data read by the program.
  * **SDL2** & **SDL2\_ttf** - Displaying graphics of the program. Note that SDL2 versions of the libraries are required.
  * **CMake>=2.8** & **make** - Building the program.
  * `[optional]` **lex/flex** - Scanner generator for generating RCD input files.
  * `[optional]` **yacc/bison** - Parser generator for generating RCD input files.

The existence of these programs/libraries is checked by `cmake`.

See the [README](../README.rst) for instructions how to build and run.
