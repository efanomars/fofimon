# Copyright © 2019-2020  Stefano Marsili, <stemars@gmx.ch>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public
# License along with this program; if not, see <http://www.gnu.org/licenses/>

# File:   fofimon-defs.cmake


# Libtool CURRENT/REVISION/AGE: here
#   MAJOR is CURRENT interface
#   MINOR is REVISION (implementation of interface)
#   AGE is always 0
set(FOFIMON_MAJOR_VERSION 0)
set(FOFIMON_MINOR_VERSION 12) # !-U-!
set(FOFIMON_VERSION "${FOFIMON_MAJOR_VERSION}.${FOFIMON_MINOR_VERSION}.0")

set(FOFIMON_REQ_GLIBMM_VERSION "2.50.0")

if ("${CMAKE_SCRIPT_MODE_FILE}" STREQUAL "")
    include(FindPkgConfig)
    if (NOT PKG_CONFIG_FOUND)
        message(FATAL_ERROR "Mandatory 'pkg-config' not found!")
    endif()
    # Beware! The prefix passed to pkg_check_modules(PREFIX ...) shouldn't contain underscores!
    pkg_check_modules(GLIBMM   REQUIRED  glibmm-2.4>=${FOFIMON_REQ_GLIBMM_VERSION})
endif()

# include dirs
list(APPEND FOFIMON_EXTRA_INCLUDE_DIRS  "${GLIBMM_INCLUDE_DIRS}")

# libs
list(APPEND FOFIMON_EXTRA_LIBRARIES     "${GLIBMM_LIBRARIES}")
