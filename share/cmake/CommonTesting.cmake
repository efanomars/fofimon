# share/cmake/CommonTesting.cmake

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


# TestFiles                 Create test for a list of files
# 
# Parameters:
# STMMI_TEST_SOURCES        list of test source files for each of which a test executable
#                           is created.
# STMMI_WITH_SOURCES        list of sources that are compiled with each of the tests in 
#                           STMMI_TEST_SOURCES
# STMMI_TARGET_INCLUDES     list of includes needed at compile time
# STMMI_TARGET_LIBS         list of libraries needed at link time
# STMMI_FAKE_IFACE          bool that tells whether the compiler definition STMF_TESTING_IFACE
#                           is defined for each test. This can be used to conditionally declare 
#                           a class method virtual to create mock or fake subclasses.
#
# Implicit paramters:
# STMMI_HEADERS_DIR         The directory of public headers of the to be tested library
# STMMI_INCLUDE_DIR         The directory containing STMMI_HEADERS_DIR
# STMMI_SOURCES_DIR         The directory of private headers of the to be tested library
# 
function(TestFiles STMMI_TEST_SOURCES  STMMI_WITH_SOURCES  STMMI_TARGET_INCLUDES  STMMI_TARGET_LIBS  STMMI_FAKE_IFACE)

    if (BUILD_TESTING)
        set(STMMI_DO_TESTING TRUE)

        #message(STATUS "CMAKE_CURRENT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}")
        #message(STATUS "PROJECT_SOURCE_DIR        ${PROJECT_SOURCE_DIR}")
        #message(STATUS "STMMI_TEST_SOURCES   ${STMMI_TEST_SOURCES}")
        #message(STATUS "STMMI_WITH_SOURCES   ${STMMI_WITH_SOURCES}")
        #message(STATUS "STMMI_HEADERS_DIR    ${STMMI_HEADERS_DIR}")
        #message(STATUS "STMMI_SOURCES_DIR    ${STMMI_SOURCES_DIR}")
        #message(STATUS "ARGC                 ${ARGC}")
        #message(STATUS "ARGN                 ${ARGN}")
        #message(STATUS "ARGV                 ${ARGV}")
        #message(STATUS "ARGV0                ${ARGV0}")

        # Iterate over all tests found. For each, declare an executable and add it to the tests list.
        foreach (STMMI_TEST_CUR_FILE  ${STMMI_TEST_SOURCES})

#message(STATUS "STMMI_TEST_CUR_FILE     ${STMMI_TEST_CUR_FILE}")
            file(RELATIVE_PATH  STMMI_TEST_CUR_REL_FILE  ${PROJECT_SOURCE_DIR}/test  ${STMMI_TEST_CUR_FILE})

            string(REGEX REPLACE "[./]" "_" STMMI_TEST_CUR_TGT ${STMMI_TEST_CUR_REL_FILE})

            add_executable(${STMMI_TEST_CUR_TGT} ${STMMI_TEST_CUR_FILE}
                           ${STMMI_WITH_SOURCES})

#            target_include_directories(${STMMI_TEST_CUR_TGT} BEFORE PRIVATE ${GTESTINCLUDEDIR})
            # tests can also involve non public part of the library!
            target_include_directories(${STMMI_TEST_CUR_TGT} BEFORE PRIVATE ${STMMI_SOURCES_DIR})
            target_include_directories(${STMMI_TEST_CUR_TGT} BEFORE PRIVATE ${STMMI_HEADERS_DIR})

            target_include_directories(${STMMI_TEST_CUR_TGT} BEFORE PRIVATE ${STMMI_TARGET_INCLUDES})

            if (STMMI_FAKE_IFACE)
                target_compile_definitions(${STMMI_TEST_CUR_TGT} PUBLIC STMF_TESTING_IFACE)
            endif (STMMI_FAKE_IFACE)
            DefineTestTargetPublicCompileOptions(${STMMI_TEST_CUR_TGT})

            #target_link_libraries(${STMMI_TEST_CUR_TGT} gtest gtest_main) # link against gtest libs
            target_link_libraries(${STMMI_TEST_CUR_TGT} ${STMMI_TARGET_LIBS})

            add_test(NAME ${STMMI_TEST_CUR_TGT} COMMAND ${STMMI_TEST_CUR_TGT})       # this is how to add tests to CMake

        endforeach (STMMI_TEST_CUR_FILE  ${STMMI_TEST_SOURCES})
    endif (BUILD_TESTING)

endfunction(TestFiles STMMI_TEST_SOURCES  STMMI_WITH_SOURCES  STMMI_TARGET_INCLUDES  STMMI_TARGET_LIBS)
