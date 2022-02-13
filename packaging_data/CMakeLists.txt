# This file is part of FreeRCT.
# FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
# FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
#

# Sanity check
IF(NOT FREERCT_DONE_TOP_LEVEL_CMAKE)
	message(FATAL_ERROR "Please run cmake in the top-level directory, not this one.")
ENDIF()

include(GNUInstallDirs)
configure_file("freerct.metainfo.xml.in" "${CMAKE_CURRENT_BINARY_DIR}/freerct.metainfo.xml" @ONLY)
configure_file("freerct.desktop.in" "${CMAKE_CURRENT_BINARY_DIR}/freerct.desktop" @ONLY)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/freerct.metainfo.xml" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/metainfo" RENAME "${FREERCT_BASENAME}.metainfo.xml")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/freerct.desktop" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/applications" RENAME "${FREERCT_BASENAME}.desktop")
install(FILES "../graphics/sprites/logo/logo_256.png" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/256x256/apps" RENAME "${FREERCT_BASENAME}.png")
