/*
 * Copyright © 2018  Stefano Marsili, <stemars@gmx.ch>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>
 */
/*
 * File:   util.cc
 */

#include "util.h"

#include <chrono>
#ifndef NDEBUG
#include <iostream>
#endif //NDEBUG
#include <cmath>

#include <stdlib.h>
#include <limits.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

namespace fofi
{

namespace Util
{

int64_t getNowTimeMicroseconds()
{
	const int64_t nTimeUsec = std::chrono::duration_cast<std::chrono::microseconds>(
								std::chrono::steady_clock::now().time_since_epoch()).count();
	return nTimeUsec;
}
std::string getTimeString(int64_t nTimeMicroseconds, int64_t nDurationMicroseconds)
{
	const int32_t nUSecSize = 1 + ((nDurationMicroseconds > 0) ? std::log10(nDurationMicroseconds) : 0);
	const int32_t nSecSize = std::max(1, nUSecSize - 6);
	const auto oSecs = std::div(nTimeMicroseconds, static_cast<int64_t>(1000000));
	std::string sUsecRes = "000000" + std::to_string(oSecs.rem);
	sUsecRes = sUsecRes.substr(sUsecRes.size() - 6, 6);
	std::string sSecRes = (std::string(nSecSize, ' ') + std::to_string(oSecs.quot));
	sSecRes = sSecRes.substr(sSecRes.size() - nSecSize, nSecSize);
	return sSecRes + "." + sUsecRes;
}

std::string cleanupPath(const std::string& sPath)
{
	if (sPath.empty()) {
		return "";
	}
	int32_t nCurPos = sPath.size() - 1;
	while ((nCurPos >= 0) && (sPath[nCurPos] == '/')) {
		--nCurPos;
	}
	if (nCurPos < 0) {
		return "/"; //----------------------------------------------------------
	}
	auto sCleanPath = sPath.substr(0, nCurPos + 1); // eliminate trailing slashes
	do {
		const auto nLastCharOfElPos = nCurPos;
		bool bContainsNonDot = false;
		while ((nCurPos >= 0) && (sPath[nCurPos] != '/')) { //  && (sPath[nCurPos] != '.')
			if (sPath[nCurPos] != '.') {
				bContainsNonDot = true;
			}
			--nCurPos;
		}
		if (! bContainsNonDot) {
			// all dots: simplify
			const auto nFirstCharOfElPos = nCurPos + 1;
			auto nTotDots = nLastCharOfElPos - nFirstCharOfElPos + 1;
			std::string sDots;
			if (nTotDots == 1) {
				sDots = ".";
			} else {
				assert(nTotDots >= 2);
				sDots = "..";
				for (int32_t nCount = 2; nCount < nTotDots; ++nCount) {
					sDots += "/..";
				}
			}
			sCleanPath = sCleanPath.substr(0, nFirstCharOfElPos) + sDots + sCleanPath.substr(nLastCharOfElPos + 1);
		}
		if (nCurPos < 0) {
			return sCleanPath; //-----------------------------------------------
		}
		const auto nSlashPos = nCurPos;
		--nCurPos;
		while ((nCurPos >= 0) && (sPath[nCurPos] == '/')) {
			--nCurPos;
		}
		const auto nPrevSlashPos = nCurPos + 1;
		if (nPrevSlashPos < nSlashPos) {
			sCleanPath = sCleanPath.substr(0, nPrevSlashPos) + sCleanPath.substr(nSlashPos);
		}
	} while (nCurPos >= 0);
	return sCleanPath;
}
std::pair<bool, std::string> getPathDirName(const std::string& sPath)
{
	//assert(cleanupPath(sPath) == sPath); // Call cleanupPath first
	const auto nLastSlashPos = sPath.find_last_of('/');
	if (nLastSlashPos == std::string::npos) {
		return std::make_pair(false, "");
	}
	if (nLastSlashPos == 0) {
		// root dir
		return std::make_pair(true, "/");
	}
	return std::make_pair(true, sPath.substr(0, nLastSlashPos));
}
std::pair<bool, std::string> getPathFileName(const std::string& sPath)
{
	//assert(cleanupPath(sPath) == sPath); // Call cleanupPath first
	const auto nLastSlashPos = sPath.find_last_of('/');
	if (nLastSlashPos == std::string::npos) {
		return std::make_pair(!sPath.empty(), sPath);
	}
	return std::make_pair((nLastSlashPos < (sPath.size() - 1)), sPath.substr(nLastSlashPos + 1));
}
std::string getPathFromDirAndName(const std::string& sPath, const std::string& sName)
{
	return sPath + ((sPath == "/") ? "" : "/") + sName;
}
std::string getRealPathIfPossible(const std::string& sPath)
{
	assert(cleanupPath(sPath) == sPath); // Call cleanupPath first
	std::string sCurrentSubPath = sPath;
	std::string sLongestAbsPath;
	std::string::size_type nLongestRestPosInPath = sPath.size();
	char aAbs[PATH_MAX + 1];
	do {
//std::cout << "sCurrentSubPath = " << sCurrentSubPath << '\n';
		auto p0Abs = ::realpath(sCurrentSubPath.c_str(), aAbs);
		if (p0Abs != nullptr) {
//std::cout << "  std::string{p0Abs} = " << std::string{p0Abs} << '\n';
			sLongestAbsPath = std::string{p0Abs};
			nLongestRestPosInPath = sCurrentSubPath.size();
			break;
		} else if (errno == ENAMETOOLONG) {
			throw std::runtime_error("Absolute path too long: " + sPath);
		} else if (errno == ELOOP) {
			throw std::runtime_error("Absolute path too many symlinks: " + sPath);
		} else if (errno == EACCES) {
			throw std::runtime_error("Absolute path has forbidden elements: " + sPath);
		} else {
			// ignore
		}
		if (sCurrentSubPath == "/") {
			break;
		} else if (sCurrentSubPath == "") {
			break;
		}
		sCurrentSubPath = Util::getPathDirName(sCurrentSubPath).second;
	} while (true);

	return sLongestAbsPath + sPath.substr(nLongestRestPosInPath);
}

int32_t getPathDepth(const std::string& sChildPath, const std::string& sBasePath, int32_t nMaxDepth)
{
	const int32_t nTargetSize = static_cast<int32_t>(sBasePath.size());
	if (sChildPath.substr(0, nTargetSize) != sBasePath) {
		return -1;
	}
//std::cout << "Util::getPathDepth 1  sBasePath=" << sBasePath << "  nMaxDepth=" << nMaxDepth << '\n';
	int32_t nDepth = 0;
	// Example:
	//              0123456789012345
	//  DZ.m_sPath: /AA/BB/CC
	//  sChildPath: /AA/BB/CC/DD/EE
	//  nDepth = 0
	//  nTargetSize = 9
	//  nSize = 15
	//  nFound = 13
	//  nDepth = 1
	//  nSize = 12
	//  nFound = 10
	//  nDepth = 2
	//  nSize = 9
	int32_t nSize = static_cast<int32_t>(sChildPath.size());
	while (nSize > nTargetSize) {
		const auto nFound = sChildPath.find_last_of('/', nSize);
		assert(nFound != std::string::npos);
//std::cout << "Util::getPathDepth 1  nFound=" << nFound << "  nSize=" << nSize << '\n';
		nSize = static_cast<int32_t>(nFound) - 1;
		++nDepth;
		if (nDepth > nMaxDepth) {
			return -1; //-------------------------------------------------------
		}
	}
	return nDepth;
}

FileStat FileStat::create(const std::string& sPath)
{
	FileStat oStatRes;
	struct ::stat oStat;
	const auto nRet = ::fstatat(AT_FDCWD, sPath.c_str(), &oStat, AT_SYMLINK_NOFOLLOW);
	if (nRet != 0) {
		return oStatRes; //-----------------------------------------------------
	}
	const bool bIsLink = ((oStat.st_mode & S_IFLNK) == S_IFLNK);
	oStatRes.m_nFStat = (1 | (oStat.st_mode & (S_IFREG | S_IFDIR)) | (bIsLink ? FILE_STAT_IS_SYM_LINK : 0));
	return oStatRes;
}

} // namespace Util

} // namespace fofi
