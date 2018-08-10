# fofimon: test/CMakeLists.txt

# See   github.com/Barthelemy/CppProjectTemplate

include(CommonTesting)

option(BUILD_TESTING "Build tests" OFF)

if (BUILD_TESTING)
    # Test dirs
    set(STMMF_TEST_SOURCES_DIR  "${PROJECT_SOURCE_DIR}/test")

    set(STMMF_TEST_WITH_SOURCES_SIMPLE
            ${STMMF_TEST_SOURCES_DIR}/testingcommon.h
            ${PROJECT_SOURCE_DIR}/src/util.h
            ${PROJECT_SOURCE_DIR}/src/util.cc
           )
    # Test sources should end with .cxx
    set(STMMF_TEST_SOURCES_SIMPLE
            ${STMMF_TEST_SOURCES_DIR}/testUtil.cxx
           )

    TestFiles("${STMMF_TEST_SOURCES_SIMPLE}" "${STMMF_TEST_WITH_SOURCES_SIMPLE}" "" "" FALSE)

    set(STMMF_TEST_WITH_SOURCES_GLIBMM
            ${STMMF_TEST_SOURCES_DIR}/forkingfixture.h
            ${STMMF_TEST_SOURCES_DIR}/forkingfixture.cc
            ${STMMF_TEST_SOURCES_DIR}/mainloopfixture.h
            ${STMMF_TEST_SOURCES_DIR}/mainloopfixture.cc
            ${STMMF_TEST_SOURCES_DIR}/tempfiletreefixture.h
            ${STMMF_TEST_SOURCES_DIR}/tempfiletreefixture.cc
            ${STMMF_TEST_SOURCES_DIR}/testingcommon.h
            ${STMMF_TEST_SOURCES_DIR}/testingutil.h
            ${STMMF_TEST_SOURCES_DIR}/testingutil.cc
            ${PROJECT_SOURCE_DIR}/src/util.h
            ${PROJECT_SOURCE_DIR}/src/util.cc
            ${PROJECT_SOURCE_DIR}/src/inotifiersource.h
            ${PROJECT_SOURCE_DIR}/src/inotifiersource.cc
            ${PROJECT_SOURCE_DIR}/src/fofimodel.h
            ${PROJECT_SOURCE_DIR}/src/fofimodel.cc
           )

    set(STMMF_TEST_SOURCES_GLIBMM
            ${STMMF_TEST_SOURCES_DIR}/testFofiModel.cxx
            ${STMMF_TEST_SOURCES_DIR}/testFofiModel01.cxx
            ${STMMF_TEST_SOURCES_DIR}/testFofiModel02.cxx
            ${STMMF_TEST_SOURCES_DIR}/testFofiModel03.cxx
            ${STMMF_TEST_SOURCES_DIR}/testFofiModel04.cxx
            ${STMMF_TEST_SOURCES_DIR}/testFofiModel05.cxx
            ${STMMF_TEST_SOURCES_DIR}/testFofiModel06.cxx
            ${STMMF_TEST_SOURCES_DIR}/testFofiModel07.cxx
            ${STMMF_TEST_SOURCES_DIR}/testFofiModel08.cxx
            ${STMMF_TEST_SOURCES_DIR}/testFofiModel09.cxx
            ${STMMF_TEST_SOURCES_DIR}/testFofiModel10.cxx
            ${STMMF_TEST_SOURCES_DIR}/testFofiModel11.cxx
            ${STMMF_TEST_SOURCES_DIR}/testUtil01.cxx
           )
    TestFiles("${STMMF_TEST_SOURCES_GLIBMM}" "${STMMF_TEST_WITH_SOURCES_GLIBMM}" "${GLIBMM_INCLUDE_DIRS}" "${GLIBMM_LIBRARIES}" FALSE)

    set(STMMF_TEST_WITH_SOURCES_FAKE
            ${STMMF_TEST_SOURCES_DIR}/fakesource.h
            ${STMMF_TEST_SOURCES_DIR}/fakesource.cc
            ${STMMF_TEST_SOURCES_DIR}/testingcommon.h
            ${STMMF_TEST_SOURCES_DIR}/testingutil.h
            ${STMMF_TEST_SOURCES_DIR}/testingutil.cc
            ${PROJECT_SOURCE_DIR}/src/util.h
            ${PROJECT_SOURCE_DIR}/src/util.cc
            ${PROJECT_SOURCE_DIR}/src/inotifiersource.h
            ${PROJECT_SOURCE_DIR}/src/inotifiersource.cc
            ${PROJECT_SOURCE_DIR}/src/fofimodel.h
            ${PROJECT_SOURCE_DIR}/src/fofimodel.cc
           )

    set(STMMF_TEST_SOURCES_FAKE
            ${STMMF_TEST_SOURCES_DIR}/testFofiModelF01.cxx
           )

    TestFiles("${STMMF_TEST_SOURCES_FAKE}" "${STMMF_TEST_WITH_SOURCES_FAKE}" "${GLIBMM_INCLUDE_DIRS}" "${GLIBMM_LIBRARIES}" TRUE)

    include(CTest)
endif()
