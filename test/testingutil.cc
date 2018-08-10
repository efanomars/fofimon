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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>
 */
/*
 * File:   testingutil.cc
 */

#include "testingutil.h"

#include <glibmm.h>

#include <string>
#include <iostream>
#include <cassert>
#include <chrono>

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

namespace fofi
{
namespace testing
{

// Modified from https://stackoverflow.com/questions/675039/how-can-i-create-directory-tree-in-c-linux
typedef struct stat Stat;

namespace Private
{
void makeDir(const char* p0Dir)
{
	Stat oStat;

	if (::stat(p0Dir, &oStat) != 0) {
		// Directory does not exist. EEXIST for race condition.
		const auto nRet = ::mkdir(p0Dir, 0755); // rwx r-x r-x
		if (nRet != 0) {
			if (errno != EEXIST) {
				throw std::runtime_error("Error: Could not create directory " + std::string(p0Dir));
			}
		}
	} else if (!S_ISDIR(oStat.st_mode)) {
		throw std::runtime_error("Error: " + std::string(p0Dir) + " not a directory");
	}
}
}
void makePath(const std::string& sPath)
{
	std::string sWorkPath = sPath;
	std::string::size_type nBasePos = 0;
	do {
		const auto nNewPos = sWorkPath.find('/', nBasePos);
		if (nNewPos == std::string::npos) {
			Private::makeDir(sWorkPath.c_str());
			break; // do ------
		} else if (nBasePos != nNewPos) {
			// not root or double slash
			sWorkPath[nNewPos] = '\0';
			Private::makeDir(sWorkPath.c_str());
			sWorkPath[nNewPos] = '/';
		}
		nBasePos = nNewPos + 1;
	} while (true);
}

std::string getTempDir()
{
	static int32_t s_nCount = 0;
	++s_nCount;
	const int64_t nTimeNsec = std::chrono::duration_cast<std::chrono::nanoseconds>(
							std::chrono::steady_clock::now().time_since_epoch()).count();

	const std::string sSysTmpDir = Glib::get_tmp_dir();
	assert((! sSysTmpDir.empty()) && (sSysTmpDir != "/"));
	return sSysTmpDir + "/testingFofi/F" + std::to_string(s_nCount) + "_" + std::to_string(nTimeNsec);
}

std::vector<std::string> splitAbsolutePath(const std::string& sPath)
{
	assert(!sPath.empty());
	std::vector<std::string> aRes;
	if (sPath == "/") {
		return aRes;
	}
	assert(sPath[0] == '/');
	std::string::size_type nBasePos = 1;
	do {
		const auto nNewPos = sPath.find('/', nBasePos);
		if (nNewPos == std::string::npos) {
			auto sFolder = sPath.substr(nBasePos);
			if (! sFolder.empty()) {
				aRes.push_back(std::move(sFolder));
			}
			break;
		}
		if (nNewPos == nBasePos) {
			++nBasePos;
			continue;
		}
		// 0123456789
		// /AAA/BB
		//  ^  *
		auto sFolder = sPath.substr(nBasePos, nNewPos - nBasePos);
		if (! sFolder.empty()) {
			aRes.push_back(std::move(sFolder));
		}
		// 0123456789
		// /AAA/BB
		//      ^
		nBasePos = nNewPos + 1;
	} while (true);
	
	return aRes;
}

} // namespace testing
} // namespace fofi
