# fofimon: test/CMakeLists.txt

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


include(CommonTesting)

option(BUILD_TESTING "Build tests" OFF)

if (BUILD_TESTING)
    # Test dirs
    set(STMMI_TEST_SOURCES_DIR  "${PROJECT_SOURCE_DIR}/test")

    set(STMMI_TEST_WITH_SOURCES_SIMPLE
            "${STMMI_TEST_SOURCES_DIR}/testingcommon.h"
            "${PROJECT_SOURCE_DIR}/src/util.h"
            "${PROJECT_SOURCE_DIR}/src/util.cc"
           )
    # Test sources should end with .cxx
    set(STMMI_TEST_SOURCES_SIMPLE
            "${STMMI_TEST_SOURCES_DIR}/testUtil.cxx"
           )

    TestFiles("${STMMI_TEST_SOURCES_SIMPLE}" "${STMMI_TEST_WITH_SOURCES_SIMPLE}" "" "" FALSE)

    set(STMMI_TEST_WITH_SOURCES_GLIBMM
            "${STMMI_TEST_SOURCES_DIR}/forkingfixture.h"
            "${STMMI_TEST_SOURCES_DIR}/forkingfixture.cc"
            "${STMMI_TEST_SOURCES_DIR}/mainloopfixture.h"
            "${STMMI_TEST_SOURCES_DIR}/mainloopfixture.cc"
            "${STMMI_TEST_SOURCES_DIR}/tempfiletreefixture.h"
            "${STMMI_TEST_SOURCES_DIR}/tempfiletreefixture.cc"
            "${STMMI_TEST_SOURCES_DIR}/testingcommon.h"
            "${STMMI_TEST_SOURCES_DIR}/testingutil.h"
            "${STMMI_TEST_SOURCES_DIR}/testingutil.cc"
            "${PROJECT_SOURCE_DIR}/src/util.h"
            "${PROJECT_SOURCE_DIR}/src/util.cc"
            "${PROJECT_SOURCE_DIR}/src/inotifiersource.h"
            "${PROJECT_SOURCE_DIR}/src/inotifiersource.cc"
            "${PROJECT_SOURCE_DIR}/src/fofimodel.h"
            "${PROJECT_SOURCE_DIR}/src/fofimodel.cc"
           )

    set(STMMI_TEST_SOURCES_GLIBMM
            "${STMMI_TEST_SOURCES_DIR}/testFofiModel.cxx"
            "${STMMI_TEST_SOURCES_DIR}/testFofiModel01.cxx"
            "${STMMI_TEST_SOURCES_DIR}/testFofiModel02.cxx"
            "${STMMI_TEST_SOURCES_DIR}/testFofiModel03.cxx"
            "${STMMI_TEST_SOURCES_DIR}/testFofiModel04.cxx"
            "${STMMI_TEST_SOURCES_DIR}/testFofiModel05.cxx"
            "${STMMI_TEST_SOURCES_DIR}/testFofiModel06.cxx"
            "${STMMI_TEST_SOURCES_DIR}/testFofiModel07.cxx"
            "${STMMI_TEST_SOURCES_DIR}/testFofiModel08.cxx"
            "${STMMI_TEST_SOURCES_DIR}/testFofiModel09.cxx"
            "${STMMI_TEST_SOURCES_DIR}/testFofiModel10.cxx"
            "${STMMI_TEST_SOURCES_DIR}/testFofiModel11.cxx"
            "${STMMI_TEST_SOURCES_DIR}/testUtil01.cxx"
           )
    TestFiles("${STMMI_TEST_SOURCES_GLIBMM}" "${STMMI_TEST_WITH_SOURCES_GLIBMM}" "${GLIBMM_INCLUDE_DIRS}" "${GLIBMM_LIBRARIES}" FALSE)

    set(STMMI_TEST_WITH_SOURCES_FAKE
            "${STMMI_TEST_SOURCES_DIR}/fakesource.h"
            "${STMMI_TEST_SOURCES_DIR}/fakesource.cc"
            "${STMMI_TEST_SOURCES_DIR}/testingcommon.h"
            "${STMMI_TEST_SOURCES_DIR}/testingutil.h"
            "${STMMI_TEST_SOURCES_DIR}/testingutil.cc"
            "${PROJECT_SOURCE_DIR}/src/util.h"
            "${PROJECT_SOURCE_DIR}/src/util.cc"
            "${PROJECT_SOURCE_DIR}/src/inotifiersource.h"
            "${PROJECT_SOURCE_DIR}/src/inotifiersource.cc"
            "${PROJECT_SOURCE_DIR}/src/fofimodel.h"
            "${PROJECT_SOURCE_DIR}/src/fofimodel.cc"
           )

    set(STMMI_TEST_SOURCES_FAKE
            "${STMMI_TEST_SOURCES_DIR}/testFofiModelF01.cxx"
           )

    TestFiles("${STMMI_TEST_SOURCES_FAKE}" "${STMMI_TEST_WITH_SOURCES_FAKE}" "${GLIBMM_INCLUDE_DIRS}" "${GLIBMM_LIBRARIES}" TRUE)

    include(CTest)
endif()
