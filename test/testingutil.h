/*
 * Copyright © 2018  Stefano Marsili, <stemars@gmx.ch>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, see <http://www.gnu.org/licenses/>
 */
/*
 * File:   testingutil.h
 */

#ifndef FOFIMON_TESTING_UTIL_H_
#define FOFIMON_TESTING_UTIL_H_

#include <string>
#include <cassert>
#include <iostream>
#include <vector>

namespace fofi
{
namespace testing
{

/** Get a unique directory usually something like /tmp/testingFofi/FXXX.
 * @return The dir. Is not empty, is not "/".
 */
std::string getTempDir();

/** Create the path if it doesn't exist.
 * @param sPath The path.
 * @throws std::runtime_error if failed to create all directories
 */
void makePath(const std::string& sPath);

/** Split an absolute path in its component directories.
 * Consecutive slashes are skipped.
 * @param sPath The path. Must start with '/'.
 * @return Empty vector if root. The components of the path otherwise.
 */
std::vector<std::string> splitAbsolutePath(const std::string& sPath);

} // namespace testing

} // namespace fofi

#endif	/* FOFIMON_TESTING_UTIL_H_ */

