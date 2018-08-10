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
 * File:   tempfiletreefixture.h
 */

#ifndef FOFIMON_TEMP_FILE_TREE_FIXTURE_H_
#define FOFIMON_TEMP_FILE_TREE_FIXTURE_H_

#include "testingutil.h"
#include "util.h"
#include "fofimodel.h"

#include <glibmm.h>

#include <string>
#include <cassert>
#include <iostream>
#include <vector>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

namespace fofi
{
namespace testing
{

class TempFileTreeFixture
{
public:
	TempFileTreeFixture()
	: m_sTestBasePath(testing::getTempDir())
	{
		makePath(m_sTestBasePath);
	}

	void makeRelPath(const std::string& sRelPath)
	{
		makePath(m_sTestBasePath + "/" + sRelPath);
	}

	void createOrModifyRelFile(const std::string& sRelPathName)
	{
		const auto sAbsPathName = m_sTestBasePath + "/" + sRelPathName;
		std::string sParentDir = Util::getPathDirName(Util::cleanupPath(sAbsPathName)).second;
		//std::string sFileName = Glib::path_get_basename(sAbsPathName);
		makePath(sParentDir);
		const auto nFD = ::open(sAbsPathName.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
		const std::string sData = "Xyz";
		::write(nFD, sData.c_str(), sData.size());
		::close(nFD);
	}
	void createRelDir(const std::string& sRelPathName)
	{
		const auto sAbsPathName = m_sTestBasePath + "/" + sRelPathName;
		makePath(sAbsPathName);
	}
	void removeRelFile(const std::string& sRelPathName)
	{
		const auto sAbsPathName = m_sTestBasePath + "/" + sRelPathName;
		assert(! Glib::file_test(sAbsPathName, Glib::FILE_TEST_IS_DIR));
		::unlink(sAbsPathName.c_str());
	}
	void removeRelDir(const std::string& sRelPathName)
	{
		const auto sAbsPathName = m_sTestBasePath + "/" + sRelPathName;
		::rmdir(sAbsPathName.c_str());
	}
	void renameRelPathName(const std::string& sOldRelPathName, const std::string& sNewRelPathName)
	{
		if (sOldRelPathName == sNewRelPathName) {
			return;
		}
		const auto sOldAbsPathName = m_sTestBasePath + "/" + sOldRelPathName;
		const auto sNewAbsPathName = m_sTestBasePath + "/" + sNewRelPathName;
		#ifndef NDEBUG
		const auto nRet =
		#endif //NDEBUG
		::rename(sOldAbsPathName.c_str(), sNewAbsPathName.c_str());
		assert(nRet == 0);
	}
	/** Make symbolic link.
	 * @param sRelPathName The source link path name.
	 * @param bRelToSource Whether the target is relative to source or to m_sTestBasePath.
	 * @param sRelTargetPathName The target path name (File or dir).
	 */
	void makeRelSymlink(const std::string& sRelPathName, bool bRelToSource, const std::string& sRelTargetPathName)
	{
		const auto sAbsPathName = m_sTestBasePath + "/" + sRelPathName;
		const auto sTargetPathName = (bRelToSource ? sRelTargetPathName : m_sTestBasePath + "/" + sRelTargetPathName);
//std::cout << "makeRelSymlink()  sAbsPathName=" << sAbsPathName << "  sTargetPathName=" << sTargetPathName << '\n';

		#ifndef NDEBUG
		const auto nRet =
		#endif //NDEBUG
		::symlink(sTargetPathName.c_str(), sAbsPathName.c_str());
//if (nRet != 0) {
//std::cout << "makeRelSymlink()  errno=" << Glib::strerror(errno) << '\n';
//}
		assert(nRet == 0);
	}
	void chmodRelPathName(const std::string& sRelPathName, bool bSetX)
	{
		const auto sAbsPathName = m_sTestBasePath + "/" + sRelPathName;
		struct ::stat oStat;
		#ifndef NDEBUG
		auto nRet =
		#endif //NDEBUG
		::stat(sAbsPathName.c_str(), &oStat);
		assert(nRet == 0);
		#ifndef NDEBUG
		nRet =
		#endif //NDEBUG
		::chmod(sAbsPathName.c_str(), (oStat.st_mode & ~S_IXUSR) | (bSetX ? S_IXUSR : 0));
		assert(nRet == 0);
	}
	void sleepMillisec(int32_t nMillisec)
	{
		::timespec oTimeToSleep{};
		oTimeToSleep.tv_sec = 0;
		oTimeToSleep.tv_nsec = nMillisec * 1000 * 1000;
		::nanosleep(&oTimeToSleep, nullptr);
	}
	int32_t findToWatchPath(const FofiModel& oFofiModel, const std::string& sDirPath) const
	{
		const auto& aTWDirs = oFofiModel.getToWatchDirectories();
		const auto itFind = std::find_if(aTWDirs.begin(), aTWDirs.end(), [&](const FofiModel::ToWatchDir& oTWD)
		{
			return (oTWD.m_sPathName == sDirPath);
		});
		if (itFind == aTWDirs.end()) {
			return -1;
		}
		return std::distance(aTWDirs.begin(), itFind);
	}
	/** The unique test base directory (based on /tmp).
	 */
	const std::string m_sTestBasePath;
private:
};

} // namespace testing

} // namespace fofi

#endif	/* FOFIMON_TEMP_FILE_TREE_FIXTURE_H_ */

