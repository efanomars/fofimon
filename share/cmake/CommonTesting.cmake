# share/cmake/CommonTesting.cmake

# Modified from Barthélémy von Haller's  github.com/Barthelemy/CppProjectTemplate

# TestFiles                 Create test for a list of files
# 
# Parameters:
# STMMF_TEST_SOURCES        list of test source files for each of which a test executable
#                           is created.
# STMMF_WITH_SOURCES        list of sources that are compiled with each of the tests in 
#                           STMMF_TEST_SOURCES
# STMMF_TARGET_INCLUDES     list of includes needed at compile time
# STMMF_TARGET_LIBS         list of libraries needed at link time
# STMMF_FAKE_IFACE          bool that tells whether the compiler definition STMF_TESTING_IFACE
#                           is defined for each test. This can be used to conditionally declare 
#                           a class method virtual to create mock or fake subclasses.
#
# Implicit paramters:
# STMMF_HEADERS_DIR         The directory of public headers of the to be tested library
# STMMF_INCLUDE_DIR         The directory containing STMMF_HEADERS_DIR
# STMMF_SOURCES_DIR         The directory of private headers of the to be tested library
# 
function(TestFiles STMMF_TEST_SOURCES  STMMF_WITH_SOURCES  STMMF_TARGET_INCLUDES  STMMF_TARGET_LIBS  STMMF_FAKE_IFACE)

    if (BUILD_TESTING)
        set(STMMF_DO_TESTING TRUE)

        #message(STATUS "CMAKE_CURRENT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}")
        #message(STATUS "PROJECT_SOURCE_DIR        ${PROJECT_SOURCE_DIR}")
        #message(STATUS "STMMF_TEST_SOURCES   ${STMMF_TEST_SOURCES}")
        #message(STATUS "STMMF_WITH_SOURCES   ${STMMF_WITH_SOURCES}")
        #message(STATUS "STMMF_HEADERS_DIR    ${STMMF_HEADERS_DIR}")
        #message(STATUS "STMMF_SOURCES_DIR    ${STMMF_SOURCES_DIR}")
        #message(STATUS "ARGC                 ${ARGC}")
        #message(STATUS "ARGN                 ${ARGN}")
        #message(STATUS "ARGV                 ${ARGV}")
        #message(STATUS "ARGV0                ${ARGV0}")

        # Iterate over all tests found. For each, declare an executable and add it to the tests list.
        foreach (STMMF_TEST_CUR_FILE  ${STMMF_TEST_SOURCES})

#message(STATUS "STMMF_TEST_CUR_FILE     ${STMMF_TEST_CUR_FILE}")
            file(RELATIVE_PATH  STMMF_TEST_CUR_REL_FILE  ${PROJECT_SOURCE_DIR}/test  ${STMMF_TEST_CUR_FILE})

            string(REGEX REPLACE "[./]" "_" STMMF_TEST_CUR_TGT ${STMMF_TEST_CUR_REL_FILE})

            add_executable(${STMMF_TEST_CUR_TGT} ${STMMF_TEST_CUR_FILE}
                           ${STMMF_WITH_SOURCES})

#            target_include_directories(${STMMF_TEST_CUR_TGT} BEFORE PRIVATE ${GTESTINCLUDEDIR})
            # tests can also involve non public part of the library!
            target_include_directories(${STMMF_TEST_CUR_TGT} BEFORE PRIVATE ${STMMF_SOURCES_DIR})
            target_include_directories(${STMMF_TEST_CUR_TGT} BEFORE PRIVATE ${STMMF_HEADERS_DIR})

            target_include_directories(${STMMF_TEST_CUR_TGT} BEFORE PRIVATE ${STMMF_TARGET_INCLUDES})

            if (STMMF_FAKE_IFACE)
                target_compile_definitions(${STMMF_TEST_CUR_TGT} PUBLIC STMF_TESTING_IFACE)
            endif (STMMF_FAKE_IFACE)
            DefineTestTargetPublicCompileOptions(${STMMF_TEST_CUR_TGT})

            #target_link_libraries(${STMMF_TEST_CUR_TGT} gtest gtest_main) # link against gtest libs
            target_link_libraries(${STMMF_TEST_CUR_TGT} ${STMMF_TARGET_LIBS})

            add_test(NAME ${STMMF_TEST_CUR_TGT} COMMAND ${STMMF_TEST_CUR_TGT})       # this is how to add tests to CMake

        endforeach (STMMF_TEST_CUR_FILE  ${STMMF_TEST_SOURCES})
    endif (BUILD_TESTING)

endfunction(TestFiles STMMF_TEST_SOURCES  STMMF_WITH_SOURCES  STMMF_TARGET_INCLUDES  STMMF_TARGET_LIBS)
