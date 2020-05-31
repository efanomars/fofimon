# File: fofimon/CMakeLists.txt

#  Copyright © 2019  Stefano Marsili, <stemars@gmx.ch>
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public
#  License along with this program; if not, see <http://www.gnu.org/licenses/>

cmake_minimum_required(VERSION 3.0)

option(STMM_INSTALL_MAN_PAGE "Install man page in debian.orig/" ON)

project(fofimon CXX)

set(RUNTIME_OUTPUT_DIRECTORY "build")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/share/cmake")

include(CommonUtil)

CheckBinaryNotSourceTree()
CheckBuildType()
DefineCommonCompileOptions("c++14")

# Source files (and headers only used for building)
set(STMMI_SOURCES_DIR "${PROJECT_SOURCE_DIR}/src")
set(STMMI_FOFIMON_SOURCES
        "${STMMI_SOURCES_DIR}/fofimodel.h"
        "${STMMI_SOURCES_DIR}/fofimodel.cc"
        "${STMMI_SOURCES_DIR}/inotifiersource.h"
        "${STMMI_SOURCES_DIR}/inotifiersource.cc"
        "${STMMI_SOURCES_DIR}/util.h"
        "${STMMI_SOURCES_DIR}/util.cc"
        )

set(STMMI_FOFIMON_CLI_SOURCES
        ${STMMI_FOFIMON_SOURCES}
        "${STMMI_SOURCES_DIR}/cli/config.h"
        "${STMMI_SOURCES_DIR}/cli/evalargs.h"
        "${STMMI_SOURCES_DIR}/cli/evalargs.cc"
        "${STMMI_SOURCES_DIR}/cli/main.cc"
        "${STMMI_SOURCES_DIR}/cli/printout.h"
        "${STMMI_SOURCES_DIR}/cli/printout.cc"
        )

add_executable(fofimon  ${STMMI_FOFIMON_CLI_SOURCES} "${PROJECT_BINARY_DIR}/config.cc")

include("fofimon-defs.cmake")

target_include_directories(fofimon SYSTEM PUBLIC ${FOFIMON_EXTRA_INCLUDE_DIRS})
# This allows config.cc to find the config.h include
target_include_directories(fofimon        PUBLIC "${STMMI_SOURCES_DIR}/cli")
target_include_directories(fofimon        PUBLIC "share/thirdparty")

target_link_libraries(fofimon     ${FOFIMON_EXTRA_LIBRARIES})

DefineTargetPublicCompileOptions(fofimon)

include(GNUInstallDirs)

# Create config file for executable
configure_file("${STMMI_SOURCES_DIR}/cli/config.cc.in"
               "${PROJECT_BINARY_DIR}/config.cc" @ONLY)

if (STMM_INSTALL_MAN_PAGE)
    configure_file("${PROJECT_SOURCE_DIR}/scripts/priv/debian.orig/fofimon.1"
                   "${PROJECT_BINARY_DIR}/fofimon.1" @ONLY)
    CreateManGz(fofimon "fofimon.1")
endif()

if ($ENV{STMM_CMAKE_COMMENTS})
message(STATUS "")
message(STATUS "fofimon(-gtk) was configured with the following options:")
message(STATUS " STMMI_FOFIMON_CLI_SOURCES:     ${STMMI_FOFIMON_CLI_SOURCES}")
message(STATUS " FOFIMON_EXTRA_INCLUDE_DIRS:    ${FOFIMON_EXTRA_INCLUDE_DIRS}")
message(STATUS " FOFIMON_EXTRA_LIBRARIES:       ${FOFIMON_EXTRA_LIBRARIES}")
message(STATUS " CMAKE_BUILD_TYPE:              ${CMAKE_BUILD_TYPE}")
message(STATUS " CMAKE_CXX_COMPILER_ID:         ${CMAKE_CXX_COMPILER_ID}")
message(STATUS " CMAKE_CXX_COMPILER_VERSION:    ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS " install prefix:                ${CMAKE_INSTALL_PREFIX}")
endif()

enable_testing()
add_subdirectory(test)

install(TARGETS fofimon     RUNTIME DESTINATION "bin")

if (STMM_INSTALL_MAN_PAGE)
    install(FILES                   "${PROJECT_BINARY_DIR}/fofimon.1.gz"
            DESTINATION "${CMAKE_INSTALL_DATADIR}/man/man1/")
endif()
