# Heavily based on https://github.com/OpenTTD/OpenTTD/blob/8d54f765392654ab634ba3a950c56dd0bf1e7dd9/cmake/PackageBundle.cmake

set(CPACK_BUNDLE_NAME "FreeRCT")
set(CPACK_BUNDLE_ICON "${CMAKE_SOURCE_DIR}/graphics/sprites/logo/logo.png")
set(CPACK_BUNDLE_PLIST "${CMAKE_CURRENT_BINARY_DIR}/Info.plist.in")
# https://cmake.org/cmake/help/latest/cpack_gen/bundle.html#variable:CPACK_BUNDLE_STARTUP_COMMAND
set(CPACK_BUNDLE_STARTUP_COMMAND "${CMAKE_CURRENT_BINARY_DIR}/bin/freerct")
set(CPACK_DMG_FORMAT "UDBZ")

configure_file("${CMAKE_SOURCE_DIR}/packaging_data/osx/Info.plist.in" "${CMAKE_CURRENT_BINARY_DIR}/Info.plist.in")
set(CPACK_BUNDLE_PLIST_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/Info.plist.in")

# Make standalone / https://cmake.org/cmake/help/latest/module/BundleUtilities.html
# This won't work unless the executable is in /MacOS/ and not /Resources/

#install(CODE
#	"
#		include(BundleUtilities)
#		set(BU_CHMOD_BUNDLE_ITEMS TRUE)
#		fixup_bundle(\"\${CMAKE_INSTALL_PREFIX}/../MacOS/FreeRCT\"  \"\" \"\")
#	"
#)
