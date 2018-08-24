# File: fofimon/CMakeLists.txt

cmake_minimum_required(VERSION 3.0)

option(STMM_BUILD_FOFIMON_CLI "Build fofimon" ON)
#option(STMM_BUILD_FOFIMON_GUI "Build fofimon-gtk" OFF)

#if (STMM_BUILD_FOFIMON_GUI)
#  option(STMM_INSTALL_LAUNCHER "Install launcher in share/applications/ (implies STMM_INSTALL_ICONS=ON)" ON)
#  option(STMM_INSTALL_ICONS "Install icons in share/icons/hicolor/(size)/apps/" ON)
#endif()

#if (STMM_BUILD_FOFIMON_GUI)
#  message(FATAL_ERROR "Sorry! GUI not implemented yet!")
#endif()

project(fofimon CXX)

set(RUNTIME_OUTPUT_DIRECTORY "build")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/share/cmake) 

include(CommonUtil)

CheckBinaryNotSourceTree()
CheckBuildType()
DefineCommonCompileOptions()

include(FindPkgConfig)
if (NOT PKG_CONFIG_FOUND)
  message(FATAL_ERROR "Mandatory 'pkg-config' not found!")
endif()

#
set(FOFIMON_MAJOR_VERSION 0)
set(FOFIMON_MINOR_VERSION 1)
set(FOFIMON_VERSION "${FOFIMON_MAJOR_VERSION}.${FOFIMON_MINOR_VERSION}.0")

set(GLIBMM_VERSION_REQ "2.50.0")
set(GTKMM_VERSION_REQ "3.14.0")
set(JSONCPP_VERSION_REQ "1.7.4")

if (STMM_BUILD_FOFIMON_CLI)
  pkg_check_modules(GLIBMM      REQUIRED  glibmm-2.4>=${GLIBMM_VERSION_REQ})  # used in CommonTesting.cmake

  list(APPEND STMMF_FOFIMON_CLI_EXTRA_INCLUDE_DIRS  ${GLIBMM_INCLUDE_DIRS})
  list(APPEND STMMF_FOFIMON_CLI_EXTRA_LIBS          ${GLIBMM_LIBRARIES}   )

  string(STRIP "${STMMF_FOFIMON_CLI_EXTRA_LIBS}"         STMMF_FOFIMON_CLI_EXTRA_LIBS)
  string(STRIP "${STMMF_FOFIMON_CLI_EXTRA_INCLUDE_DIRS}" STMMF_FOFIMON_CLI_EXTRA_INCLUDE_DIRS)
endif(STMM_BUILD_FOFIMON_CLI)

#if (STMM_BUILD_FOFIMON_GUI)
#  pkg_check_modules(GTKMM       REQUIRED  gtkmm-3.0>=${GTKMM_VERSION_REQ})

#  list(APPEND STMMF_FOFIMON_GUI_EXTRA_INCLUDE_DIRS  ${GTKMM_INCLUDE_DIRS})
#  list(APPEND STMMF_FOFIMON_GUI_EXTRA_LIBS          ${GTKMM_LIBRARIES}   )

#  string(STRIP "${STMMF_FOFIMON_GUI_EXTRA_LIBS}"         STMMF_FOFIMON_GUI_EXTRA_LIBS)
#  string(STRIP "${STMMF_FOFIMON_GUI_EXTRA_INCLUDE_DIRS}" STMMF_FOFIMON_GUI_EXTRA_INCLUDE_DIRS)
#endif(STMM_BUILD_FOFIMON_GUI)

# Source files (and headers only used for building)
set(STMMF_SOURCES_DIR ${PROJECT_SOURCE_DIR}/src)
set(STMMF_FOFIMON_SOURCES
        ${STMMF_SOURCES_DIR}/fofimodel.h
        ${STMMF_SOURCES_DIR}/fofimodel.cc
        ${STMMF_SOURCES_DIR}/inotifiersource.h
        ${STMMF_SOURCES_DIR}/inotifiersource.cc
        ${STMMF_SOURCES_DIR}/util.h
        ${STMMF_SOURCES_DIR}/util.cc
        )

if (STMM_BUILD_FOFIMON_CLI)
  set(STMMF_FOFIMON_CLI_SOURCES
          ${STMMF_FOFIMON_SOURCES}
          ${STMMF_SOURCES_DIR}/cli/config.h
          ${STMMF_SOURCES_DIR}/cli/evalargs.h
          ${STMMF_SOURCES_DIR}/cli/evalargs.cc
          ${STMMF_SOURCES_DIR}/cli/json.hpp
          ${STMMF_SOURCES_DIR}/cli/main.cc
          ${STMMF_SOURCES_DIR}/cli/printout.h
          ${STMMF_SOURCES_DIR}/cli/printout.cc
          )
endif(STMM_BUILD_FOFIMON_CLI)
#if (STMM_BUILD_FOFIMON_GUI)
#  set(STMMF_FOFIMON_GUI_SOURCES
#          ${STMMF_FOFIMON_SOURCES}
#          ${STMMF_SOURCES_DIR}/gui/config.h
#          ${STMMF_SOURCES_DIR}/gui/main.cc
#          ${STMMF_SOURCES_DIR}/gui/json.hpp
#          )
#endif(STMM_BUILD_FOFIMON_GUI)

#if (STMM_BUILD_FOFIMON_GUI)
#  set(STMMF_FOFIMON_GUI_DATA_DIR ${PROJECT_SOURCE_DIR}/data)
#endif(STMM_BUILD_FOFIMON_GUI)

if (STMM_BUILD_FOFIMON_CLI)
  add_executable(fofimon     ${STMMF_FOFIMON_CLI_SOURCES} ${PROJECT_BINARY_DIR}/config-cli.cc)
  target_include_directories(fofimon     SYSTEM PUBLIC ${STMMF_FOFIMON_CLI_EXTRA_INCLUDE_DIRS})
  DefineTargetPublicCompileOptions(fofimon)
endif(STMM_BUILD_FOFIMON_CLI)

#if (STMM_BUILD_FOFIMON_GUI)
#  add_executable(fofimon-gtk ${STMMF_FOFIMON_GUI_SOURCES} ${PROJECT_BINARY_DIR}/config-gui.cc)
#  target_include_directories(fofimon-gtk SYSTEM PUBLIC ${STMMF_FOFIMON_GUI_EXTRA_INCLUDE_DIRS})
#  DefineTargetPublicCompileOptions(fofimon-gtk)
#endif(STMM_BUILD_FOFIMON_GUI)

#if (STMM_BUILD_FOFIMON_GUI)
#  include(GNUInstallDirs)
#  set(FOFIMON_PKG_DATA_DIR     "${CMAKE_INSTALL_FULL_DATADIR}/fofimon-gtk")
#  set(FOFIMON_PKG_REL_DATA_DIR "${CMAKE_INSTALL_DATADIR}/fofimon-gtk")
#endif(STMM_BUILD_FOFIMON_GUI)

# Create config file for executables
if (STMM_BUILD_FOFIMON_CLI)
  configure_file("${STMMF_SOURCES_DIR}/cli/config.cc.in"
                 "${PROJECT_BINARY_DIR}/config-cli.cc" @ONLY)
endif(STMM_BUILD_FOFIMON_CLI)
#if (STMM_BUILD_FOFIMON_GUI)
#  configure_file("${STMMF_SOURCES_DIR}/gui/config.cc.in"
#                 "${PROJECT_BINARY_DIR}/config-gui.cc" @ONLY)
#endif(STMM_BUILD_FOFIMON_GUI)

enable_testing()
add_subdirectory(test)

# This allows config.cc to find the config.h include
if (STMM_BUILD_FOFIMON_CLI)
  target_include_directories(fofimon PUBLIC ${STMMF_SOURCES_DIR}/cli)
endif(STMM_BUILD_FOFIMON_CLI)
#if (STMM_BUILD_FOFIMON_GUI)
#  target_include_directories(fofimon-gtk PUBLIC ${STMMF_SOURCES_DIR}/gui)
#endif(STMM_BUILD_FOFIMON_GUI)

if ($ENV{STMM_CMAKE_COMMENTS})
message(STATUS "")
message(STATUS "fofimon(-gtk) was configured with the following options:")
message(STATUS " STMMF_FOFIMON_SOURCES:                 ${STMMF_FOFIMON_SOURCES}")
if (STMM_BUILD_FOFIMON_CLI)
message(STATUS " STMMF_FOFIMON_CLI_EXTRA_LIBS:          ${STMMF_FOFIMON_CLI_EXTRA_LIBS}")
message(STATUS " STMMF_FOFIMON_CLI_EXTRA_INCLUDE_DIRS:  ${STMMF_FOFIMON_CLI_EXTRA_INCLUDE_DIRS}")
endif(STMM_BUILD_FOFIMON_CLI)
#if (STMM_BUILD_FOFIMON_GUI)
#message(STATUS " STMMF_FOFIMON_GUI_EXTRA_LIBS:          ${STMMF_FOFIMON_GUI_EXTRA_LIBS}")
#message(STATUS " STMMF_FOFIMON_GUI_EXTRA_INCLUDE_DIRS:  ${STMMF_FOFIMON_GUI_EXTRA_INCLUDE_DIRS}")
#endif(STMM_BUILD_FOFIMON_GUI)
message(STATUS " CMAKE_BUILD_TYPE:              ${CMAKE_BUILD_TYPE}")
message(STATUS " CMAKE_CXX_COMPILER_ID:         ${CMAKE_CXX_COMPILER_ID}")
message(STATUS " CMAKE_CXX_COMPILER_VERSION:    ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS " install prefix:                ${CMAKE_INSTALL_PREFIX}")
endif()

if (STMM_BUILD_FOFIMON_CLI)
  target_link_libraries(fofimon     ${STMMF_FOFIMON_CLI_EXTRA_LIBS}) # ${STMMF_FOFIMON_EXTRA_LDFLAGS}
  install(TARGETS fofimon     RUNTIME DESTINATION "bin")
endif(STMM_BUILD_FOFIMON_CLI)
#if (STMM_BUILD_FOFIMON_GUI)
#  target_link_libraries(fofimon-gtk ${STMMF_FOFIMON_GUI_EXTRA_LIBS}) # ${STMMF_FOFIMON_EXTRA_LDFLAGS}
#  install(TARGETS fofimon-gtk RUNTIME DESTINATION "bin")
#endif(STMM_BUILD_FOFIMON_GUI)

#if (STMM_BUILD_FOFIMON_GUI)
#if (STMM_INSTALL_LAUNCHER)
#    install(FILES         "${STMMF_FOFIMON_GUI_DATA_DIR}/applications/fofimon-gtk.desktop"
#            DESTINATION "${CMAKE_INSTALL_DATADIR}/applications/")
#endif()
#if (STMM_INSTALL_ICONS OR STMM_INSTALL_LAUNCHER)
#    install(FILES       "${STMMF_FOFIMON_GUI_DATA_DIR}/icons/hicolor/24x24/apps/fofimon-gtk.png"
#            DESTINATION      "${CMAKE_INSTALL_DATADIR}/icons/hicolor/24x24/apps/")
#    install(FILES       "${STMMF_FOFIMON_GUI_DATA_DIR}/icons/hicolor/32x32/apps/fofimon-gtk.png"
#            DESTINATION      "${CMAKE_INSTALL_DATADIR}/icons/hicolor/32x32/apps/")
#    install(FILES       "${STMMF_FOFIMON_GUI_DATA_DIR}/icons/hicolor/48x48/apps/fofimon-gtk.png"
#            DESTINATION      "${CMAKE_INSTALL_DATADIR}/icons/hicolor/48x48/apps/")
#    install(FILES       "${STMMF_FOFIMON_GUI_DATA_DIR}/icons/hicolor/scalable/apps/fofimon-gtk.svg"
#            DESTINATION      "${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps/")
#endif()
#endif(STMM_BUILD_FOFIMON_GUI)
 
