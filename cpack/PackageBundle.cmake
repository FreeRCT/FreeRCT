string(TIMESTAMP CURRENT_YEAR "%Y")

set(CPACK_BUNDLE_NAME "FreeRCT")
set(CPACK_DMG_FORMAT "UDBZ")

# Delay fixup_bundle() till the install step; this makes sure all executables
# exists and it can do its job.
install(
    CODE
    "
        include(BundleUtilities)
        set(BU_CHMOD_BUNDLE_ITEMS TRUE)
        fixup_bundle(\"\${CMAKE_INSTALL_PREFIX}/../MacOS/freerct\"  \"\" \"\")
    "
    DESTINATION .
    COMPONENT Runtime)
