# Heavily based on https://github.com/OpenTTD/OpenTTD/blob/8d54f765392654ab634ba3a950c56dd0bf1e7dd9/cmake/PackageBundle.cmake

set(CPACK_BUNDLE_NAME "FreeRCT")
set(CPACK_BUNDLE_ICON "${CMAKE_SOURCE_DIR}/graphics/sprites/logo/logo.png")
set(CPACK_BUNDLE_PLIST "${CMAKE_CURRENT_BINARY_DIR}/Info.plist.in")
# https://cmake.org/cmake/help/latest/cpack_gen/bundle.html#variable:CPACK_BUNDLE_STARTUP_COMMAND
set(CPACK_BUNDLE_STARTUP_COMMAND "${CMAKE_CURRENT_BINARY_DIR}/bin/freerct")
set(CPACK_DMG_FORMAT "UDBZ")

configure_file("${CMAKE_SOURCE_DIR}/packaging_data/osx/Info.plist.in" "${CMAKE_CURRENT_BINARY_DIR}/Info.plist.in")
set(CPACK_BUNDLE_PLIST_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/Info.plist.in")

# Link libs, the wrong way (libpng sdl2 sdl2_ttf)
# This only works when the libraries are installed by Homebrew and you're using an Intel Mac. It
# also has versions hardcoded so it will break, and soon.
target_link_libraries(/usr/local/Cellar/libpng/1.6.37/lib/libpng.a)
target_link_libraries(/usr/local/Cellar/sdl2/2.0.20/lib/libSDL2.a)
target_link_libraries(/usr/local/Cellar/sdl2_ttf/2.0.18_1/lib/libSDL2_ttf.a)
