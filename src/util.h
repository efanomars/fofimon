/*
 * Copyright © 2018-2019  Stefano Marsili, <stemars@gmx.ch>
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
 * File:   util.h
 */

#ifndef FOFIMON_UTIL_H_
#define FOFIMON_UTIL_H_

#include <utility>
#include <vector>
#include <deque>
#include <string>
#include <algorithm>

#include <stdint.h>


namespace fofi
{

namespace Util
{

int64_t getNowTimeMicroseconds() noexcept;

std::string getTimeString(int64_t nTimeMicroseconds, int64_t nDurationMicroseconds) noexcept;

template <typename T>
void addVectorToVectorUniquely(std::vector<T>& aVs, const std::vector<T>& aVsAdd) noexcept
{
	//TODO use back_inserter?
	for (const auto& oT : aVsAdd) {
		const auto itFind = std::find(aVs.begin(), aVs.end(), oT);
		if (itFind == aVs.end()) {
			aVs.push_back(oT);
		}
	}
}

template <typename T>
void addValueToVectorUniquely(std::vector<T>& aVs, T oT) noexcept
{
	const auto itFind = std::find(aVs.begin(), aVs.end(), oT);
	if (itFind == aVs.end()) {
		aVs.push_back(std::move(oT));
	}
}

template <typename T>
void addValueToDequeUniquely(std::deque<T>& aVs, T oT) noexcept
{
	const auto itFind = std::find(aVs.begin(), aVs.end(), oT);
	if (itFind == aVs.end()) {
		aVs.push_back(std::move(oT));
	}
}

template <typename T>
bool isInVector(std::vector<T>& aVs, const T& oT) noexcept
{
	const auto itFind = std::find(aVs.begin(), aVs.end(), oT);
	return (itFind != aVs.end());
}

std::string cleanupPath(const std::string& sPath) noexcept;

// expects sPath == cleanupPath(sPath)
std::pair<bool, std::string> getPathDirName(const std::string& sPath) noexcept;
// expects sPath == cleanupPath(sPath)
std::pair<bool, std::string> getPathFileName(const std::string& sPath) noexcept;
// expects sPath == cleanupPath(sPath)
std::string getPathFromDirAndName(const std::string& sDirPath, const std::string& sName) noexcept;

// expects sPath == cleanupPath(sPath)
// throws
std::string getRealPathIfPossible(const std::string& sPath);

int32_t getPathDepth(const std::string& sChildPath, const std::string& sBasePath, int32_t nMaxDepth) noexcept;

class FileStat
{
public:
	static FileStat create(const std::string& sPath) noexcept;
	bool exists() const noexcept { return ((m_nFStat & FILE_STAT_EXISTS) == FILE_STAT_EXISTS); }
	bool isRegular() const noexcept { return ((m_nFStat & FILE_STAT_IS_REGULAR) == FILE_STAT_IS_REGULAR); }
	bool isDir() const noexcept { return ((m_nFStat & FILE_STAT_IS_DIR) == FILE_STAT_IS_DIR); }
	/** Whether the file is a symbolic link.
	 * Implies isRegular() and !isDir(). Might be dangling.
	 * @return Whether sym link.
	 */
	bool isSymLink() const noexcept { return ((m_nFStat & FILE_STAT_IS_SYM_LINK) == FILE_STAT_IS_SYM_LINK); }
private:
	enum FILE_STAT : int32_t
	{
		FILE_STAT_EXISTS = 1
		, FILE_STAT_IS_DIR = 16384
		, FILE_STAT_IS_REGULAR = 32768
		, FILE_STAT_IS_SYM_LINK = 8192
	};
	int32_t m_nFStat = 0;
private:
	FileStat() = default;
};

} // namespace Util

} // namespace fofi

#endif /* FOFIMON_UTIL_H_ */
