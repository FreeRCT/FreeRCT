set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
set(CPACK_NSIS_HELP_LINK "${CPACK_PACKAGE_HOMEPAGE_URL}")
set(CPACK_NSIS_URL_INFO_ABOUT "${CPACK_PACKAGE_HOMEPAGE_URL}")
set(CPACK_NSIS_CONTACT "${CPACK_PACKAGE_CONTACT}")

# Use the icon of the application
set(CPACK_NSIS_INSTALLED_ICON_NAME "freerct.exe")
# Tell NSIS the binary will be in the root
set(CPACK_NSIS_EXECUTABLES_DIRECTORY ".")

set(CPACK_NSIS_DEFINES "
; Version Info
Var AddWinPrePopulate
VIProductVersion \\\"0.0.0.0\\\"
VIAddVersionKey \\\"ProductName\\\" \\\"FreeRCT Installer for Windows\\\"
VIAddVersionKey \\\"Comments\\\" \\\"Installs FreeRCT \\\${VERSION}\\\"
VIAddVersionKey \\\"CompanyName\\\" \\\"FreeRCT Developers\\\"
VIAddVersionKey \\\"FileDescription\\\" \\\"Installs FreeRCT \\\${VERSION}\\\"
VIAddVersionKey \\\"ProductVersion\\\" \\\"\\\${VERSION}\\\"
VIAddVersionKey \\\"InternalName\\\" \\\"InstFreeRCT\\\"
VIAddVersionKey \\\"FileVersion\\\" \\\"0.0.0.0\\\"
VIAddVersionKey \\\"LegalCopyright\\\" \\\" \\\"
"
)
