# Introduction #

This is a first step in documenting what steps are needed to compile FreeRCT on Windows using Visual Studio. We hope to reduce the length of this list later on.

# Details #

We here assume F:\trunk is the path to a checkout/clone of trunk, and that you have a folder F:\libraries. You can replace these with actual paths on your system.

This have been tested to build win32/x86 version. It may also work for x64, but this have not been tested.

## Prepare: Install software ##
  * Install CMake
  * Install Visual Studio 2013 (It may work with earlier versions, but that is untested)
  * Install an SVN client. Eg. TortoiseSVN

## Prepare: Download files ##
  1. Clone/check out FreeRCT source code from SVN to F:\trunk. There should not be a trunk folder inside F:\trunk. Instead you should have F:\trunk\src etc.
  1. Download SDL2 development packet and unpack to F:\libraries\SDL2 https://www.libsdl.org/download-2.0.php
  1. Download SDL2\_ttf development packet and unpack to F:\libraries\SDL2\_ttf https://www.libsdl.org/projects/SDL_ttf/
  1. Download OpenTTD useful 5.1 ("Headers/libraries to compile with MSVC for windows") and unpack to F:\libraries\OpenTTD\_essentials\ http://www.openttd.org/en/download-openttd-useful
  1. Download SDL2 and SDL2\_ttf runtime binaries from the links above and put the DLLs in F:\trunk\bin (you may need to create the bin folder). If you plan to distribute the bin directory, mind to include any license/copyright files from SDL and SDL\_ttf that you are required to include.

## Prepare: Font config ##
Get a font. Here we use Bitstream Vera that is an open font.
  1. Go to http://www.dafont.com/bitstream-vera-sans.font and click on Download-button. Extract vera.ttf and put in F:\trunk\bin
  1. Create F:\trunk\bin\freerct.cfg in your favoruite text editor:
```
[font]
medium-size = 12
medium-path = ./vera.ttf
```

**Note:** if you plan to distribute your bin directory, mind the license of the fort. You are likely required to include its copyright/license files.

## CMake ##
  1. Start the CMake GUI application
  1. Click on "Browse Source" and select F:\trunk
  1. Click on "Browse Build" and select F:\trunk
  1. Check the checkmark "Advanced"
  1. Click on "Configure". You will be presented with an error in red text at the bottom that say what you need to configure. Here follows a list of what paths you will set when asked for:
```
PNG_LIBRARY_DEBUG = F:\libraries\OpenTTD_essentials\win32\library\libpng.lib
PNG_LIBRARY_RELEASE = F:\libraries\OpenTTD_essentials\win32\library\libpng.lib
PNG_PNG_INCLUDE_DIR = F:\libraries\OpenTTD_essentials\shared\include\
SDL2MAIN_LIBRARY = F:\libraries\SDL2\lib\x86\SDL2main.lib
SDL2TTFMAIN_LIBRARY = 
SDL2TTF_INCLUDE_DIR = F:\libraries\SDL2_ttf\include\
SDL2TTF_LIBRARY = F:\libraries\SDL2_ttf\lib\x86\SDL2_ttf.lib
SDL2_INCLUDE_DIR = F:\libraries\SDL2\include\
SDL2_LIBRARY = F:\libraries\SDL2\lib\x86\SDL2.lib
ZLIB_INCLUDE_DIR = F:\libraries\OpenTTD_essentials\shared\include\
ZLIB_LIBRARY = F:\libraries\OpenTTD_essentials\win32\library\zlibstat.lib
```
  1. When there are no more errors, click on "Generate"

## Visual Studio ##
You have now generated the Visual Studio project files. Start Visual Studio and open solution F:\trunk\FreeRCT.sln.

  1. In Solution Explorer, right click on "Solution 'FreeRCT' (5 projects)" and select Properties.
    1. Go to Common Properties => Startup Project. In the Single startup project dropdown, select freerct.
    1. Go to Common Properties => Debugging. Change "Working Directory" to `"$(ProjectDir)..\bin"`
    1. Click OK.
  1. Press F7 on your keyboard to compile
  1. When compilation is done, you can run using F5.

The changes made for startup project and working directory are stored separate from the project files and will thus be kept even if the project files are re-generated later.