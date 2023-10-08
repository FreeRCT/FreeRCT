# This file is part of FreeRCT.
# FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
# FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
#

# Sanity check
IF(NOT FREERCT_DONE_TOP_LEVEL_CMAKE)
	message(FATAL_ERROR "Please run cmake in the top-level directory, not this one.")
ENDIF()

project(freerct)

find_package(Git)

IF(WEBASSEMBLY)
	message(STATUS "Configuring for WebAssembly (experimental)")
	add_definitions("-DWEBASSEMBLY")

	set(CMAKE_INSTALL_PREFIX "wasm_static")
	set(USERDATA_PREFIX "wasm_userdata")

	set(CMAKE_C_COMPILER   "emcc")
	set(CMAKE_CXX_COMPILER "em++")
	set(CMAKE_EXECUTABLE_SUFFIX ".html")

	set(WASM_FLAGS "-s USE_LIBPNG=1 -s USE_ZLIB=1 -s USE_FREETYPE=1")
	add_c_cpp_flags("${WASM_FLAGS}")

	add_custom_target(prepare_wasm_rcd ALL
		COMMAND ${CMAKE_COMMAND} -E copy_directory "${FRCT_BINARY_DIR}/rcd" "${CMAKE_INSTALL_PREFIX}/${PACKAGING_DATA_DIR}/rcd"
		DEPENDS rcd
	)
	add_custom_target(prepare_wasm_data ALL
		COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/data" "${CMAKE_INSTALL_PREFIX}/${PACKAGING_DATA_DIR}/data"
	)

	add_flags(CMAKE_EXE_LINKER_FLAGS "${WASM_FLAGS} \
			-s FORCE_FILESYSTEM=1 -s ALLOW_MEMORY_GROWTH=1 -s EXIT_RUNTIME=1 -s USE_GLFW=3 -s FULL_ES3 -s ASSERTIONS=1 -s WASM=1 -s ASYNCIFY \
			--preload-file ${CMAKE_INSTALL_PREFIX}")

	FUNCTION(find_package)
	ENDFUNCTION()
ENDIF(WEBASSEMBLY)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS NO)

# Source files
# TODO: Use explicit list instead?
file(GLOB freerct_SRCS
     "${CMAKE_SOURCE_DIR}/src/*.cpp"
     "${CMAKE_SOURCE_DIR}/src/*.h"
)

set(STRINGFILE "${CMAKE_SOURCE_DIR}/src/generated/coasters_strings")
add_custom_command(OUTPUT ${STRINGFILE}.cpp ${STRINGFILE}.h
                   COMMAND rcdgen --header ${STRINGFILE}.h --code ${STRINGFILE}.cpp --prefix COASTERS --base STR_GENERIC_COASTER_START
				   DEPENDS rcdgen
                   COMMENT "Generating COASTERS string files"
)
set(STRINGFILE "${CMAKE_SOURCE_DIR}/src/generated/shops_strings")
add_custom_command(OUTPUT ${STRINGFILE}.cpp ${STRINGFILE}.h
                   COMMAND rcdgen --header ${STRINGFILE}.h --code ${STRINGFILE}.cpp --prefix SHOPS --base STR_GENERIC_SHOP_START
				   DEPENDS rcdgen
                   COMMENT "Generating SHOPS string files"
)
set(STRINGFILE "${CMAKE_SOURCE_DIR}/src/generated/gentle_thrill_rides_strings")
add_custom_command(OUTPUT ${STRINGFILE}.cpp ${STRINGFILE}.h
                   COMMAND rcdgen --header ${STRINGFILE}.h --code ${STRINGFILE}.cpp --prefix GENTLE_THRILL_RIDES --base STR_GENERIC_GENTLE_THRILL_RIDES_START
				   DEPENDS rcdgen
                   COMMENT "Generating GENTLE/THRILL RIDES string files"
)
set(STRINGFILE "${CMAKE_SOURCE_DIR}/src/generated/entrance_exit_strings")
add_custom_command(OUTPUT ${STRINGFILE}.cpp ${STRINGFILE}.h
                   COMMAND rcdgen --header ${STRINGFILE}.h --code ${STRINGFILE}.cpp --prefix ENTRANCE_EXIT --base STR_GENERIC_ENTRANCE_EXIT_START
				   DEPENDS rcdgen
                   COMMENT "Generating ENTRANCE/EXITS string files"
)
set(STRINGFILE "${CMAKE_SOURCE_DIR}/src/generated/scenery_strings")
add_custom_command(OUTPUT ${STRINGFILE}.cpp ${STRINGFILE}.h
                   COMMAND rcdgen --header ${STRINGFILE}.h --code ${STRINGFILE}.cpp --prefix SCENERY --base STR_GENERIC_SCENERY_START
				   DEPENDS rcdgen
                   COMMENT "Generating SCENERY string files"
)
set(STRINGFILE "${CMAKE_SOURCE_DIR}/src/generated/gui_strings")
add_custom_command(OUTPUT ${STRINGFILE}.cpp ${STRINGFILE}.h
                   COMMAND rcdgen --header ${STRINGFILE}.h --code ${STRINGFILE}.cpp --prefix GUI --base STR_GUI_START
				   DEPENDS rcdgen
                   COMMENT "Generating GUI string files"
)

# Add platform specific files
IF(UNIX)
	file(GLOB freerct_platform_SRCS
	     "${CMAKE_SOURCE_DIR}/src/unix/*.cpp"
	     "${CMAKE_SOURCE_DIR}/src/unix/*.h"
	)
ELSEIF(WIN32)
	file(GLOB freerct_platform_SRCS
	     "${CMAKE_SOURCE_DIR}/src/windows/*.cpp"
	     "${CMAKE_SOURCE_DIR}/src/windows/*.h"
	)
ENDIF()
set(freerct_SRCS ${freerct_SRCS} ${freerct_platform_SRCS})

# Add generated files
set(freerct_SRCS ${freerct_SRCS}
    "${CMAKE_SOURCE_DIR}/src/rev.cpp"
    "${CMAKE_SOURCE_DIR}/src/generated/coasters_strings.h"
    "${CMAKE_SOURCE_DIR}/src/generated/gui_strings.h"
    "${CMAKE_SOURCE_DIR}/src/generated/shops_strings.h"
    "${CMAKE_SOURCE_DIR}/src/generated/gentle_thrill_rides_strings.h"
    "${CMAKE_SOURCE_DIR}/src/generated/entrance_exit_strings.h"
    "${CMAKE_SOURCE_DIR}/src/generated/scenery_strings.h"
)
set_source_files_properties("${CMAKE_SOURCE_DIR}/src/rev.cpp" GENERATED)
set_source_files_properties("${CMAKE_SOURCE_DIR}/src/generated/coasters_strings.cpp" GENERATED)
set_source_files_properties("${CMAKE_SOURCE_DIR}/src/generated/coasters_strings.h" GENERATED)
set_source_files_properties("${CMAKE_SOURCE_DIR}/src/generated/gui_strings.cpp" GENERATED)
set_source_files_properties("${CMAKE_SOURCE_DIR}/src/generated/gui_strings.h" GENERATED)
set_source_files_properties("${CMAKE_SOURCE_DIR}/src/generated/shops_strings.cpp" GENERATED)
set_source_files_properties("${CMAKE_SOURCE_DIR}/src/generated/shops_strings.h" GENERATED)
set_source_files_properties("${CMAKE_SOURCE_DIR}/src/generated/gentle_thrill_rides_strings.cpp" GENERATED)
set_source_files_properties("${CMAKE_SOURCE_DIR}/src/generated/gentle_thrill_rides_strings.h" GENERATED)
set_source_files_properties("${CMAKE_SOURCE_DIR}/src/generated/entrance_exit_strings.cpp" GENERATED)
set_source_files_properties("${CMAKE_SOURCE_DIR}/src/generated/entrance_exit_strings.h" GENERATED)
set_source_files_properties("${CMAKE_SOURCE_DIR}/src/generated/scenery_strings.cpp" GENERATED)
set_source_files_properties("${CMAKE_SOURCE_DIR}/src/generated/scenery_strings.h" GENERATED)

# On windows, "WIN32" option need to be passed to
# add_excutable to get a Windows instead of Console
# application.
IF(WIN32)
	add_executable(freerct WIN32 ${freerct_SRCS})
ELSE()
	add_executable(freerct ${freerct_SRCS})
ENDIF()
add_dependencies(freerct rcd)

# Library detection

IF(NOT WEBASSEMBLY)
	find_package(PNG REQUIRED)
	find_package(OpenGL REQUIRED)
	find_package(glfw3 3.3 REQUIRED)
	find_package(GLEW REQUIRED)
	find_package(Freetype REQUIRED)
	include_directories(freerct ${GLEW_INCLUDE_DIR} ${FREETYPE_INCLUDE_DIRS})
	target_link_libraries(freerct PNG::PNG glfw OpenGL::GL GLEW::GLEW ${FREETYPE_LIBRARIES})
ENDIF(NOT WEBASSEMBLY)

# Determine version string
IF(NOT DEFINED VERSION_STRING)
	IF(GIT_FOUND AND IS_DIRECTORY "${CMAKE_SOURCE_DIR}/.git")
		execute_process(COMMAND ${GIT_EXECUTABLE} rev-list --count HEAD
			            OUTPUT_VARIABLE VERSION_STRING_1
			            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
		)
		execute_process(COMMAND ${GIT_EXECUTABLE} describe --tags --always
			            OUTPUT_VARIABLE VERSION_STRING_2
			            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
		)
		# Need to strip newlines from the strings:
		string(STRIP "${VERSION_STRING_1}" VERSION_STRING_1)
		string(STRIP "${VERSION_STRING_2}" VERSION_STRING_2)
		string(STRIP "0.2~git${VERSION_STRING_1}-${VERSION_STRING_2}" VERSION_STRING)
	ELSE()
		message(WARNING "WARNING: No VCS checkout detected and no custom version specified")
		set(VERSION_STRING "0.0.0-revdetect-broken")
	ENDIF()
ENDIF()

set(VERSION_STRING "${VERSION_STRING}" PARENT_SCOPE)
message(STATUS "Version string: ${VERSION_STRING}")
message(STATUS "Generating src/rev.cpp")
configure_file("${CMAKE_SOURCE_DIR}/src/rev.cpp.in" "${CMAKE_SOURCE_DIR}/src/rev.cpp")

IF(ASAN)
	if(RELEASE)
		message(FATAL_ERROR "ASan should be used only in debug builds.")
	ENDIF()
	add_c_cpp_flags("-fsanitize=address -fno-optimize-sibling-calls -fsanitize-address-use-after-scope -fno-omit-frame-pointer -g")
ENDIF()

# Compiler flags
# TODO: Extend for debug/release versions
IF(MSVC)
	set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")

	# "If /Zc:rvalueCast is specified, the compiler follows section 5.4 of the
	# C++11 standard". We need C++11 for the way we use threads.
	add_c_cpp_flags(/Zc:rvalueCast)

	IF(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
		# Enable multi-threaded compilation.
		add_c_cpp_flags(/MP)
	ENDIF()

	add_c_cpp_flags(/W3)

	# Enable static linking.
	add_definitions(-DWIN32_LEAN_AND_MEAN -D__STDC_FORMAT_MACROS -DNOMINMAX)
	target_link_libraries(freerct
		version ole32 imm32 winmm gdi32 user32 oleaut32 setupapi shell32 advapi32 dinput8 uuid
	)
	IF(RELEASE)
		string(REPLACE "/MD" "" CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})
		add_c_cpp_flags("/MT /EHsc")
	ELSE()
		string(REPLACE "/MDd" "" CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
		add_c_cpp_flags("/MTd /EHsc")
	ENDIF()
ELSE()
	IF(RELEASE)
		# O3 is prone to generating broken code on some compilers, O2 is safer.
		add_definitions("-DNDEBUG")
		add_c_cpp_flags("-O2 -s")
	ELSE()
		add_c_cpp_flags("-g")
	ENDIF()
	add_c_cpp_flags("-Wall -Wextra -Wpedantic")
ENDIF()

# Add platform define
IF(UNIX)
	add_definitions("-DLINUX")
ELSEIF(WIN32)
	add_definitions("-DWINDOWS")
ELSE()
	message(FATAL_ERROR "Unsupported platform")
ENDIF()

IF(WEBASSEMBLY)
	add_dependencies(freerct prepare_wasm_rcd prepare_wasm_data)
ENDIF()

include(GNUInstallDirs)
IF(WIN32)
	install(PROGRAMS ${CMAKE_BINARY_DIR}/bin/freerct.exe DESTINATION "${CMAKE_INSTALL_BINDIR}")
ELSE()
	install(PROGRAMS ${CMAKE_BINARY_DIR}/bin/freerct DESTINATION "${CMAKE_INSTALL_BINDIR}")
ENDIF()
