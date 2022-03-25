# Heavily based on https://github.com/OpenTTD/OpenTTD/blob/8d54f765392654ab634ba3a950c56dd0bf1e7dd9/cmake/PackageBundle.cmake

set(CPACK_BUNDLE_NAME "FreeRCT")
set(CPACK_BUNDLE_ICON "${CMAKE_SOURCE_DIR}/graphics/sprites/logo/logo.png")
set(CPACK_BUNDLE_PLIST "${CMAKE_CURRENT_BINARY_DIR}/Info.plist.in")
set(CPACK_DMG_FORMAT "UDBZ")

configure_file("${CMAKE_SOURCE_DIR}/packaging_data/osx/Info.plist.in" "${CMAKE_CURRENT_BINARY_DIR}/Info.plist.in")
set(CPACK_BUNDLE_PLIST_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/Info.plist.in")
