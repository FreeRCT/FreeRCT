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
        FILE(GLOB BUNDLE_FILE_NAME ${CPACK_OUTPUT_FILE_PREFIX}/${CPACK_PACKAGE_FILE_NAME}*)
        fixup_bundle(\"\${BUNDLE_FILE_NAME}\"  \"\" \"\")
    "
    DESTINATION .
    COMPONENT Runtime)
