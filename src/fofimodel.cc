/*
 * Copyright © 2018-2020  Stefano Marsili, <stemars@gmx.ch>
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
 * File:   fofimodel.cc
 */
#include "fofimodel.h"

#include "inotifiersource.h"
#include "util.h"

#include <glibmm.h>

#include <sigc++/sigc++.h>

#include <cassert>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <stdexcept>
#include <utility>

#include <errno.h>

namespace fofi
{

FofiModel::FofiModel(int32_t nMaxToWatchDirectories, int32_t nMaxResultPaths)
: FofiModel(std::make_unique<INotifierSource>(0), nMaxToWatchDirectories, nMaxResultPaths, false)
{
}
FofiModel::FofiModel(unique_ptr<INotifierSource> refSource, int32_t nMaxToWatchDirectories, int32_t nMaxResultPaths, bool bIsRoot)
: m_nMaxToWatchDirectories(nMaxToWatchDirectories)
, m_nMaxResultPaths(nMaxResultPaths)
, m_bIsUserRoot(bIsRoot)
, m_refSource(std::move(refSource))
, m_nRootTWDIdx(-1)
, m_nEventCounter(0)
, m_nStartTimeUsec(-1)
, m_nStopTimeUsec(-1)
, m_bInitialSetup(false)
, m_nRootResultIdx(-1)
, m_bOverflow(false)
, m_bHasInconsistencies(false)
, m_sES()
{
	assert(nMaxToWatchDirectories > 0);
	assert(nMaxResultPaths > 0);
	assert(m_refSource);
	//
	m_refSource->attach_override();
	m_refSource->connect(sigc::mem_fun(this, &FofiModel::onFileModified));

	m_aInvalidPaths = m_refSource->invalidPaths();

	Glib::signal_timeout().connect(
			sigc::mem_fun(*this, &FofiModel::onCheckOpenMoves), s_nCheckOpenMovesMillisec);
}
FofiModel::~FofiModel()
{
}
void FofiModel::calcFiltersRegex(std::vector<Filter> aFilters)
{
	for (auto& oF : aFilters) {
		assert(!oF.m_sFilter.empty());
		if (oF.m_eFilterType != FILTER_EXACT) {
			oF.m_oFilterRegex = std::regex(oF.m_sFilter, std::regex_constants::basic);
		}
	}
}
std::string FofiModel::addDirectoryZone(DirectoryZone&& oDZ)
{
	assert(m_nEventCounter == 0); // can't add directory zones while watching
	assert(oDZ.m_nMaxDepth >= 0);
	try {
		calcFiltersRegex(oDZ.m_aSubDirIncludeFilters);
		calcFiltersRegex(oDZ.m_aSubDirExcludeFilters);
		calcFiltersRegex(oDZ.m_aFileIncludeFilters);
		calcFiltersRegex(oDZ.m_aFileExcludeFilters);
	} catch (const std::regex_error& oErr) {
		return oErr.what(); //--------------------------------------------------
	}
	oDZ.m_sPath = Util::cleanupPath(oDZ.m_sPath);
	try {
		oDZ.m_sPath = Util::getRealPathIfPossible(oDZ.m_sPath);
	} catch (const std::runtime_error& oErr) {
		return oErr.what(); //--------------------------------------------------
	}
	if (oDZ.m_sPath.find("/./") != std::string::npos) {
		const std::string sError = "Base path contains '/./': " + Glib::filename_to_utf8(oDZ.m_sPath);
		return sError; //-------------------------------------------------------
	}
	if (oDZ.m_sPath.find("/../") != std::string::npos) {
		const std::string sError = "Base path contains '/../': " + Glib::filename_to_utf8(oDZ.m_sPath);
		return sError; //-------------------------------------------------------
	}
	const auto nFoundIdx = findDirectoryZone(oDZ.m_sPath);
	if (nFoundIdx >= 0) {
		const std::string sError = "Base path already defined: " + Glib::filename_to_utf8(oDZ.m_sPath);
		return sError; //-------------------------------------------------------
	}
	for (const auto& sInvalidPath : m_aInvalidPaths) {
		if (oDZ.m_sPath.substr(0, sInvalidPath.size()) == sInvalidPath) {
			const std::string sError = "Base path is an unwatchable directory: " + Glib::filename_to_utf8(oDZ.m_sPath);
			return sError; //---------------------------------------------------
		}
		if (sInvalidPath.substr(0, oDZ.m_sPath.size()) == oDZ.m_sPath) {
			// the base path is an ancestor of one of an invalid path
			oDZ.m_bMightHaveInvalidDescendants = true;
		}
	}
	for (std::string& sPinnedFile : oDZ.m_aPinnedFiles) {
		if (sPinnedFile.find("/") != std::string::npos) {
			const std::string sError = "Pinned file can't contain a '/': " + Glib::filename_to_utf8(sPinnedFile);
			return sError; //---------------------------------------------------
		}
	}
	for (std::string& sPinnedSubDir : oDZ.m_aPinnedSubDirs) {
		if (sPinnedSubDir.find("/") != std::string::npos) {
			const std::string sError = "Pinned sub dir can't contain a '/': " + Glib::filename_to_utf8(sPinnedSubDir);
			return sError; //---------------------------------------------------
		}
	}
	m_aDirectoryZones.push_back(std::move(oDZ));
	return "";
}
std::string FofiModel::removeDirectoryZone(const std::string& sPath)
{
	assert(m_nEventCounter == 0); // can't remove directory zones while watching
	const auto nFoundIdx = findDirectoryZone(sPath);
	if (nFoundIdx < 0) {
		return "Path not defined: " + Glib::filename_to_utf8(sPath);
	}
	m_aDirectoryZones.erase(m_aDirectoryZones.begin() + nFoundIdx);
	return "";
}
const std::vector<FofiModel::DirectoryZone>& FofiModel::getDirectoryZones() const
{
	return m_aDirectoryZones;
}
bool FofiModel::hasDirectoryZone(const std::string& sPath) const
{
	return (findDirectoryZone(sPath) >= 0);
}
int32_t FofiModel::findDirectoryZone(const std::string& sPath) const
{
	const auto itFind = std::find_if(m_aDirectoryZones.begin(), m_aDirectoryZones.end(), [&](const DirectoryZone& oBaseDir)
	{
		return (oBaseDir.m_sPath == sPath);
	});
	if (itFind == m_aDirectoryZones.end()) {
		return -1;
	}
	return static_cast<int32_t>(std::distance(m_aDirectoryZones.begin(), itFind));
}
int32_t FofiModel::findToWatchDir(const std::string& sPath) const
{
	const auto itFind = std::find_if(m_aToWatchDirs.begin(), m_aToWatchDirs.end(), [&](const ToWatchDir& oTWD)
	{
		return (oTWD.m_sPathName == sPath);
	});
	if (itFind == m_aToWatchDirs.end()) {
		return -1;
	}
	return static_cast<int32_t>(std::distance(m_aToWatchDirs.begin(), itFind));
}
int32_t FofiModel::findToWatchDir(int32_t nParentTWDIdx, const std::string& sPathName) const
{
	const auto& oToWatch = m_aToWatchDirs[nParentTWDIdx];
	const auto itFind = std::find_if(oToWatch.m_aToWatchSubdirIdxs.begin(), oToWatch.m_aToWatchSubdirIdxs.end(), [&](int32_t nCurTWDIdx)
	{
		const auto& oCurToWatch = m_aToWatchDirs[nCurTWDIdx];
		return (oCurToWatch.m_sPathName == sPathName);
	});
	if (itFind == oToWatch.m_aToWatchSubdirIdxs.end()) {
		return -1;
	}
	return *itFind;
}
std::string FofiModel::addToWatchFile(const std::string& sPath)
{
	assert(m_nEventCounter == 0); // can't add files while watching
	const auto nFoundIdx = findToWatchFile(sPath);
	if (nFoundIdx >= 0) {
		return "File already defined: " + Glib::filename_to_utf8(sPath); //-----
	}
	m_aToWatchFiles.push_back(sPath);
	return "";
}
std::string FofiModel::removeToWatchFile(const std::string& sPath)
{
	assert(m_nEventCounter == 0); // can't remove files while watching
	const auto nFoundIdx = findToWatchFile(sPath);
	if (nFoundIdx < 0) {
		return "File not defined: " + Glib::filename_to_utf8(sPath); //---------
	}
	m_aToWatchFiles.erase(m_aToWatchFiles.begin() + nFoundIdx);
	return "";
}
const std::vector<std::string>& FofiModel::getToWatchFiles() const
{
	return m_aToWatchFiles;
}
bool FofiModel::hasToWatchFile(const std::string& sPath) const
{
	return (findToWatchFile(sPath) >= 0);
}
int32_t FofiModel::findToWatchFile(const std::string& sPath) const
{
	const auto itFind = std::find_if(m_aToWatchFiles.begin(), m_aToWatchFiles.end(), [&](const std::string& sCurPath)
	{
		return (sCurPath == sPath);
	});
	if (itFind == m_aToWatchFiles.end()) {
		return -1; //-----------------------------------------------------------
	}
	return static_cast<int32_t>(std::distance(m_aToWatchFiles.begin(), itFind));
}
void FofiModel::checkThrowMaxToWatchDirsReached()
{
	const int32_t nTotWatchedDirs = static_cast<int32_t>(m_aToWatchDirs.size());
	if (nTotWatchedDirs >= m_nMaxToWatchDirectories) {
		const std::string sError = "Reached limit of " + std::to_string(nTotWatchedDirs) + " watched directories";
		throw std::runtime_error(sError);
	}
}
void FofiModel::checkThrowMaxInotifyUserWatches(int32_t nErrno)
{
	if (nErrno == ENOSPC) {
		const int32_t nMaxWatches = INotifierSource::getSystemMaxUserWatches();
		const std::string sError = "Maximum number of inotify watches reached ("
									+ ((nMaxWatches >= 0) ? std::to_string(nMaxWatches) : "unknown")
									+ ")\nConsider increasing the value in file '" + INotifierSource::s_sSystemMaxUserWatchesFile + "'"
									+ "\nExample: sudo sh -c 'echo " + std::to_string(std::max(nMaxWatches + 25000, 65536))
									+ " >" + INotifierSource::s_sSystemMaxUserWatchesFile + "'";
		throw std::runtime_error(sError);
	} else if ((nErrno == ENOMEM) || (nErrno == ENAMETOOLONG)) {
		throw std::runtime_error(Glib::strerror(nErrno)); //----
	}
}

bool FofiModel::isFilteredOut(bool bIsDir, const ToWatchDir& oToWatch, const std::string& sName, const std::string& sPathName) const
{
	return (bIsDir ? isFilteredOutSubDir(oToWatch, sName, sPathName) : isFilteredOutFile(oToWatch, sName, sPathName));
}
bool FofiModel::isFilteredOutSubDir(const ToWatchDir& oToWatch, const std::string& sName, const std::string& sPathName) const
{
	assert(!sName.empty());
	assert(!sPathName.empty());
	const auto& aNames = oToWatch.m_aPinnedSubDirs;
	const auto itFindName = std::find(aNames.begin(), aNames.end(), sName);
	const bool bPinned = (itFindName != aNames.end());
	if (bPinned) {
		return false; //--------------------------------------------------------
	}
	if (oToWatch.m_nIdxOwnerDirectoryZone < 0) {
		// Gap filler: as if filtered out
		return true; //---------------------------------------------------------
	}
	const auto& oDZ = m_aDirectoryZones[oToWatch.m_nIdxOwnerDirectoryZone];
	if (oDZ.m_bMightHaveInvalidDescendants) {
		for (const auto& sInvalidPath : m_aInvalidPaths) {
			if (sInvalidPath == sPathName) {
				// sort of filtered out
				return true; //-------------------------------------------------
			}
		}
	}
	// filtered out?
	const auto& aIncludeFilters = oDZ.m_aSubDirIncludeFilters;
	if (! aIncludeFilters.empty()) {
		const bool bIsMatched = isMatchedByFilters(aIncludeFilters, sName, sPathName);
		if (!bIsMatched) {
			return true; //-----------------------------------------------------
		}
	}
	const auto& aExcludeFilters = oDZ.m_aSubDirExcludeFilters;
	if (! aExcludeFilters.empty()) {
		const bool bIsMatched = isMatchedByFilters(aExcludeFilters, sName, sPathName);
		if (bIsMatched) {
			return true; //-----------------------------------------------------
		}
	}
	return false;
}
bool FofiModel::isFilteredOutFile(const ToWatchDir& oToWatch, const std::string& sName, const std::string& sPathName) const
{
	assert(!sName.empty());
	assert(!sPathName.empty());
	const auto& aNames = oToWatch.m_aPinnedFiles;
	const auto itFindName = std::find(aNames.begin(), aNames.end(), sName);
	const bool bPinned = (itFindName != aNames.end());
	if (bPinned) {
		return false; //--------------------------------------------------------
	}
	if (oToWatch.m_nIdxOwnerDirectoryZone < 0) {
		// Gap filler: as if filtered out
		return true; //---------------------------------------------------------
	}
	// filtered out?
	const auto& oDZ = m_aDirectoryZones[oToWatch.m_nIdxOwnerDirectoryZone];
	const auto& aIncludeFilters = oDZ.m_aFileIncludeFilters;
	if (! aIncludeFilters.empty()) {
		const bool bIsMatched = isMatchedByFilters(aIncludeFilters, sName, sPathName);
		if (!bIsMatched) {
			return true; //-----------------------------------------------------
		}
	}
	const auto& aExcludeFilters = oDZ.m_aFileExcludeFilters;
	if (! aExcludeFilters.empty()) {
		const bool bIsMatched = isMatchedByFilters(aExcludeFilters, sName, sPathName);
		if (bIsMatched) {
			return true; //-----------------------------------------------------
		}
	}
	return false;
}
void FofiModel::setDirectoryZone(ToWatchDir& oTWD)
{
	assert(oTWD.m_nIdxOwnerDirectoryZone < 0);
	// inverse loop order because if a path is within multiple zones the one
	// with the closest base path is chosen as owner,
	// that is the owner is set by the directory zone that creates the
	// ToWatchDir instance first
	const int32_t nTotDirectoryZones = static_cast<int32_t>(m_aDirectoryZones.size());
	for (int32_t nDZIdx = nTotDirectoryZones - 1; nDZIdx >= 0; --nDZIdx) {
		DirectoryZone& oDZ = m_aDirectoryZones[nDZIdx];
		// Note the base path is allowed not to exist
		const int32_t nDepth = Util::getPathDepth(oTWD.m_sPathName, oDZ.m_sPath, oDZ.m_nMaxDepth);
		const bool bIsInZone = (nDepth >= 0);
		if (bIsInZone) {
			oTWD.m_nIdxOwnerDirectoryZone = nDZIdx;
			oTWD.m_nDepth = nDepth;
			oTWD.m_nMaxDepth = oDZ.m_nMaxDepth;
			return; //----------------------------------------------------------
		}
	}
}
void FofiModel::addExistingContent(ToWatchDir& oTWD)
{
	assert(oTWD.m_aExisting.empty());
	try {
		Glib::Dir oDir(oTWD.m_sPathName);
		for (const auto& sChildName : oDir) {
			const std::string sChildPath = Util::getPathFromDirAndName(oTWD.m_sPathName, sChildName);
			const auto oChildFStat = Util::FileStat::create(sChildPath);
			const bool bChildExists = oChildFStat.exists();
			if (! bChildExists) {
				continue; //----
			}
			const bool bChildIsDir = oChildFStat.isDir();
			oTWD.m_aExisting.push_back({sChildName, bChildIsDir, false});
		}
	} catch (const Glib::FileError& oErr) {
	}
}
int32_t FofiModel::initialFillTheGaps(int32_t nChildTWDIdx, const std::string& sPath)
{
	int32_t nTWDIdx = findToWatchDir(sPath);
	const bool bAlreadyDefined = [&]() -> bool
	{
		if (nTWDIdx >= 0) {
			return true;
		} else {
			checkThrowMaxToWatchDirsReached();
			nTWDIdx = static_cast<int32_t>(m_aToWatchDirs.size());
			m_aToWatchDirs.emplace_back();
			return false;
		}
	}();
	auto& oTWD = m_aToWatchDirs[nTWDIdx];
	if (nChildTWDIdx >= 0) {
		auto& oChildTWD = m_aToWatchDirs[nChildTWDIdx];
		const std::string sChildDirName{oChildTWD.getName()};
		Util::addValueToVectorUniquely(oTWD.m_aPinnedSubDirs, sChildDirName);
		Util::addValueToDequeUniquely(oTWD.m_aToWatchSubdirIdxs, nChildTWDIdx);
		if (oChildTWD.m_nParentTWDIdx < 0) {
			oChildTWD.m_nParentTWDIdx = nTWDIdx;
		}
	}
	if (bAlreadyDefined) {
		return nTWDIdx; //------------------------------------------------------
	}
	oTWD.m_sPathName = sPath;
	const auto oFStat = Util::FileStat::create(sPath);
	const bool bExists = oFStat.exists();
	const bool bExistsAndDir = bExists && oFStat.isDir();
	oTWD.m_bExists = bExistsAndDir;
	const bool bIsRoot = (sPath == "/");
	if (! bIsRoot) {
		const auto nFoundSlashPos = sPath.find_last_of('/');
		assert(nFoundSlashPos != std::string::npos);
		oTWD.m_nNamePos = static_cast<int32_t>(nFoundSlashPos) + 1;
	}
	setDirectoryZone(oTWD);

	if (! bIsRoot) {
		std::string sParentDir = Util::getPathDirName(sPath).second;
		// recurse first so that iwatches are created for the parent (and its ancestors)
		initialFillTheGaps(nTWDIdx, sParentDir);
	}

	const bool bRunning = (m_nEventCounter > 0);
	if (bRunning) {
		createINotifyWatch(nTWDIdx, oTWD);
	}
	if (bRunning && bExistsAndDir) {
		if (oTWD.isWatched()) {
			addExistingContent(oTWD);
		}
	}
	return nTWDIdx;
}
void FofiModel::initialCreateToWatchDir(int32_t nParentTWDIdx)
{
	assert(nParentTWDIdx >= 0);
	auto& oParentTWD = m_aToWatchDirs[nParentTWDIdx];
	const bool bParentIsLeaf = (oParentTWD.m_nDepth == oParentTWD.m_nMaxDepth);
	if (bParentIsLeaf) {
		return; //--------------------------------------------------------------
	}
	const bool bRunning = (m_nEventCounter > 0);
	try {
		Glib::Dir oDir(oParentTWD.m_sPathName);
		for (const auto& sChildName : oDir) {
			const std::string sChildPath = Util::getPathFromDirAndName(oParentTWD.m_sPathName, sChildName);
			const auto oFStat = Util::FileStat::create(sChildPath);
			const bool bExists = oFStat.exists();
			const bool bExistsAndDir = bExists && oFStat.isDir();
			if (! bExistsAndDir) {
				continue; //-----
			}
			if (isFilteredOutSubDir(oParentTWD, sChildName, sChildPath)) {
				continue; //-----
			}
			int32_t nTWDIdx = findToWatchDir(nParentTWDIdx, sChildPath);
			if (nTWDIdx >= 0) {
				ToWatchDir& oTWD = m_aToWatchDirs[nTWDIdx];
				assert(oParentTWD.m_nIdxOwnerDirectoryZone >= 0);
				assert(oTWD.m_nIdxOwnerDirectoryZone >= 0);
				if (oTWD.m_nIdxOwnerDirectoryZone != oParentTWD.m_nIdxOwnerDirectoryZone) {
					continue;  //-----
				}
			} else {
				nTWDIdx = addExistingToWatchDir(sChildPath);
				ToWatchDir& oTWD = m_aToWatchDirs[nTWDIdx];
				oTWD.m_nParentTWDIdx = nParentTWDIdx;
				Util::addValueToDequeUniquely(oParentTWD.m_aToWatchSubdirIdxs, nTWDIdx);
			}
			ToWatchDir& oTWD = m_aToWatchDirs[nTWDIdx];
			if (bRunning && oTWD.m_bExists && !oTWD.isWatched()) {
				createINotifyWatch(nTWDIdx, oTWD);
				if (! oTWD.isWatched()) {
					continue; //-----
				}
				//
				addExistingContent(oTWD);
			}
			// recurse
			initialCreateToWatchDir(nTWDIdx);
		}
	} catch (const Glib::FileError& oErr) {
	}
}
void FofiModel::initialSetup()
{
	m_aToWatchDirs.clear();
	// order related (possibly overlapping) directory zones by increasing depth
	// (this is achieved by ordering by name)
	std::sort(m_aDirectoryZones.begin(), m_aDirectoryZones.end(), [](const DirectoryZone& oDZ1, const DirectoryZone& oDZ2)
	{
		return (oDZ1.m_sPath < oDZ2.m_sPath);
	});
	// every directory zone base path needs it's ancestors (if existing)
	// to be watched
	const int32_t nTotDirectoryZones = static_cast<int32_t>(m_aDirectoryZones.size());
	for (int32_t nDZIdx = nTotDirectoryZones - 1; nDZIdx >= 0; --nDZIdx) {
		DirectoryZone& oDZ = m_aDirectoryZones[nDZIdx];
		initialFillTheGaps(-1, oDZ.m_sPath);
	}
	// every watched file needs it's parent and ancestors (if existing)
	// to be watched
	for (const std::string& sFilePath : m_aToWatchFiles) {
		std::string sParentDir = Util::getPathDirName(sFilePath).second;
		std::string sFileName = Util::getPathFileName(sFilePath).second;
		const int32_t nParentTWDIdx = initialFillTheGaps(-1, sParentDir);
		assert(nParentTWDIdx >= 0);
		auto& oParentTWD = m_aToWatchDirs[nParentTWDIdx];
		Util::addValueToVectorUniquely(oParentTWD.m_aPinnedFiles, sFileName);
	}
	// create ToWatchDir object for each existing directory in the zones
	for (int32_t nDZIdx = nTotDirectoryZones - 1; nDZIdx >= 0; --nDZIdx) {
		DirectoryZone& oDZ = m_aDirectoryZones[nDZIdx];
		initialCreateToWatchDir(findToWatchDir(oDZ.m_sPath));
	}
	m_nRootTWDIdx = findToWatchDir("/");
	assert(m_nRootTWDIdx >= 0);
}
int32_t FofiModel::addWatchedResult(ToWatchDir& oParentTWD, const std::string& sName, bool bIsDir)
{
	return addWatchedResult(oParentTWD, oParentTWD.m_sPathName, sName, bIsDir);
}
int32_t FofiModel::addWatchedResultRoot()
{
	ToWatchDir oDummyTWD;
	return addWatchedResult(oDummyTWD, "/", "", true);
}
std::deque<FofiModel::ToWatchDir::FileDir>::iterator FofiModel::ToWatchDir::findInExisting(bool bIsDir, const std::string& sName)
{
	auto itFind = std::find_if(m_aExisting.begin(), m_aExisting.end(), [&](const ToWatchDir::FileDir& oFD)
	{
		return ((!oFD.m_bRemoved) && (oFD.m_sName == sName) && (oFD.m_bIsDir == bIsDir));
	});
	return itFind;
}
int32_t FofiModel::addWatchedResult(ToWatchDir& oParentTWD, const std::string& sPath, const std::string& sName, bool bIsDir)
{
	int32_t nResultIdx = static_cast<int32_t>(m_aWatchedResults.size());
	if (! sName.empty()) {
		oParentTWD.m_aWatchedResultIdxs.push_back(nResultIdx);
		//
		const auto itFind = oParentTWD.findInExisting(bIsDir, sName);
		if (itFind != oParentTWD.m_aExisting.end()) {
			// When for a file or dir a result is created
			// it is marked as removed from the existing list
			(*itFind).m_bRemoved = true;
		}
	}
	m_aWatchedResults.emplace_back();
	WatchedResult& oWatchedResult = m_aWatchedResults[nResultIdx];
	oWatchedResult.m_sPath = sPath;
	oWatchedResult.m_sName = sName;
	oWatchedResult.m_bIsDir = bIsDir;
	return nResultIdx;
}
void FofiModel::setInconsistent(WatchedResult& oWR)
{
#ifdef STMM_TRACE_DEBUG
//	std::cout << "FofiModel::setInconsistent =" << oWR.m_sPath << "/" << oWR.m_sName << (oWR.m_bIsDir ? "/" : "") << '\n';
#endif //STMM_TRACE_DEBUG
	oWR.m_bInconsistent = true;
	m_bHasInconsistencies = true;
}
void FofiModel::setNotImmediate(WatchedResult& oWR)
{
	assert(! oWR.m_aActions.empty());
	ActionData& oActionData = oWR.m_aActions.back();
	oActionData.m_bImmediate = false;
}
FofiModel::ActionData& FofiModel::addActionData(WatchedResult& oWR, INotifierSource::FOFI_ACTION eAction, int64_t nTimeUsec)
{
	oWR.m_aActions.emplace_back();
	ActionData& oActionData = oWR.m_aActions.back();
	oActionData.m_eAction = eAction;
	oActionData.m_nTimeUsec = nTimeUsec;
	return oActionData;
}

void FofiModel::createImmediateChildren(int32_t nParentTWDIdx, bool bWasAttrib, int64_t nNowUsec)
{
	createImmediateChildren(nParentTWDIdx, bWasAttrib, nNowUsec, m_aEFD);
}
void FofiModel::createImmediateChildren(int32_t nParentTWDIdx, bool bWasAttrib, int64_t nNowUsec, const std::vector<ToWatchDir::FileDir>& aExcepts)
{
//std::cout << "FofiModel::createImmediateChildren nParentTWDIdx=" << nParentTWDIdx << '\n';
	assert(nParentTWDIdx >= 0);
	const bool bHasExcepts = ! aExcepts.empty();
	ToWatchDir& oParentTWD = m_aToWatchDirs[nParentTWDIdx];
	const bool bParentIsLeaf = oParentTWD.isLeaf();
	try {
		Glib::Dir oDir(oParentTWD.m_sPathName);
		for (const auto& sChildName : oDir) {
			const std::string sChildPath = Util::getPathFromDirAndName(oParentTWD.m_sPathName, sChildName);
			const auto oFStat = Util::FileStat::create(sChildPath);
			const bool bIsDir = oFStat.isDir();
			if (bHasExcepts) {
				const auto itFind = std::find_if(aExcepts.begin(), aExcepts.end(), [&](const ToWatchDir::FileDir& oViFiDi) {
					return ((oViFiDi.m_bIsDir == bIsDir) && (oViFiDi.m_sName == sChildName));
				});
				if (itFind != aExcepts.end()) {
					continue; //for ---
				}
			}
			const bool bFilteredOut = isFilteredOut(bIsDir, oParentTWD, sChildName, sChildPath);
			if (bFilteredOut) {
				oParentTWD.m_aExisting.push_back({sChildName, bIsDir, false});
				continue; //-----
			}
			bool bExistedAtStart = false;
			bool bWasCreatedImmediately = false;
			bool bInconsistent = false;
			bool bEmitWatchedResult = true;
			int32_t nResultIdx = findResult(nParentTWDIdx, sChildName, bIsDir);
			bool bWatchedResultExists = (nResultIdx >= 0);
			if (! bWatchedResultExists) {
				nResultIdx = addWatchedResult(oParentTWD, sChildName, bIsDir);
#ifdef STMM_TRACE_DEBUG
	std::cout << "FofiModel::createImmediateChildren Created immediately (N) sChildPath=" << sChildPath << '\n';
#endif //STMM_TRACE_DEBUG
			} else {
				WatchedResult& oWatchedResult = m_aWatchedResults[nResultIdx];
				bWasCreatedImmediately = oWatchedResult.immediate();
#ifdef STMM_TRACE_DEBUG
	std::cout << "FofiModel::createImmediateChildren Created immediately (E) sChildPath=" << sChildPath << '\n';
#endif //STMM_TRACE_DEBUG
				if (bWasCreatedImmediately) {
					assert(oWatchedResult.exists());
				}
				// from RESULT_DELETED to RESULT_MODIFIED
				// from RESULT_TEMPORARY to RESULT_CREATED
				// from RESULT_CREATED to RESULT_CREATED: error
				// from RESULT_MODIFIED to RESULT_MODIFIED: error
				bExistedAtStart = oWatchedResult.existedAtStart();
				bInconsistent = ((!bWasCreatedImmediately) && oWatchedResult.exists());
				if (bInconsistent) {
#ifdef STMM_TRACE_DEBUG
	std::cout << "FofiModel::createImmediateChildren Inconsistent nParentTWDIdx=" << nParentTWDIdx << "  sChildName=" << sChildName << '\n';
	std::cout << "FofiModel::createImmediateChildren            1 oWatchedResult.m_eResultType=" << static_cast<int32_t>(oWatchedResult.m_eResultType) << '\n';
#endif //STMM_TRACE_DEBUG
					setInconsistent(oWatchedResult);
				}
			}
			WatchedResult& oWatchedResult = m_aWatchedResults[nResultIdx];
			if ((! bWasCreatedImmediately) || bInconsistent) {
				ActionData& oActionData = addActionData(oWatchedResult, INotifierSource::FOFI_ACTION_CREATE, nNowUsec);
				oActionData.m_bCausedByAttribChange = bWasAttrib;
				oActionData.m_bImmediate = ! bInconsistent;
			} else {
				bEmitWatchedResult = false;
			}
			//
			oWatchedResult.m_eResultType = (bExistedAtStart ? RESULT_MODIFIED : RESULT_CREATED);

			if (! bIsDir) {
				if (bEmitWatchedResult) {
					m_oWatchedResultActionSignal.emit(oWatchedResult);
				}
				continue; //-----
			}
			// check whether a TWD already exists
			int32_t nTWDIdx = findToWatchDir(nParentTWDIdx, sChildPath);
			if (nTWDIdx < 0) {
				if (bParentIsLeaf) {
					// if a structural TWD is not present this dir isn't watched
					if (bEmitWatchedResult) {
						m_oWatchedResultActionSignal.emit(oWatchedResult);
					}
					continue; //-----
				}
				nTWDIdx = addExistingToWatchDir(sChildPath);
				ToWatchDir& oTWD = m_aToWatchDirs[nTWDIdx];
				oTWD.m_nParentTWDIdx = nParentTWDIdx;
				Util::addValueToDequeUniquely(oParentTWD.m_aToWatchSubdirIdxs, nTWDIdx);
			} else {
				ToWatchDir& oTWD = m_aToWatchDirs[nTWDIdx];
				if (oTWD.m_bExists) {
					// creating a dir that already exists
					if ((! bWasCreatedImmediately) || bInconsistent) {
						// inconsistent state: a delete event was missed
						if (oTWD.isWatched()) {
							// if it's watched it's permission might have changed
							// and might no longer be watchable
							m_refSource->removePath(oTWD.m_nWatchedIdx, nTWDIdx);
							oTWD.m_nWatchedIdx = -1;
						}
						if (bWatchedResultExists) {
							// If WR existed and TWD is inconsistent
							// the WR must be inconsistent too
							assert(bInconsistent);
						} else { // WR just created
							if (!bInconsistent) {
#ifdef STMM_TRACE_DEBUG
//	std::cout << "FofiModel::createImmediateChildren Inconsistent nParentTWDIdx=" << nParentTWDIdx << "  sChildName=" << sChildName << '\n';
//	std::cout << "FofiModel::createImmediateChildren           1b oWatchedResult.m_eResultType=" << static_cast<int32_t>(oWatchedResult.m_eResultType) << '\n';
#endif //STMM_TRACE_DEBUG
								setInconsistent(oWatchedResult);
								assert(oWatchedResult.m_eResultType == RESULT_CREATED);
								// when a directory is deleted and then recreated mark it as modified
								oWatchedResult.m_eResultType = RESULT_MODIFIED;
							}
						}
					} else {
						// the creation was already emitted by createImmediateChildren
						// it would be a duplicate
						bEmitWatchedResult = false;
					}
				} else {
					assert(! oTWD.isWatched());
					oTWD.m_bExists = true;
				}
			}
			ToWatchDir& oTWD = m_aToWatchDirs[nTWDIdx];
			if (! oTWD.isWatched()) {
				// It is important that the inotify watch is created before
				// looking for already created sub dirs (createImmediateChildren)
				createINotifyWatch(nTWDIdx, oTWD);
			}
			//
			if (bEmitWatchedResult) {
				m_oWatchedResultActionSignal.emit(oWatchedResult);
			}
			if (oTWD.isWatched()) {
				// only recurse if could create a watch
				createImmediateChildren(nTWDIdx, bWasAttrib, nNowUsec);
			}
		}
	} catch (const Glib::FileError& oErr) {
		//TODO set inconsistent of paernt WR (maybe it's root!)
	}
}
std::string FofiModel::calcToWatchDirectories()
{
	assert(m_nEventCounter == 0); // Can't call this while running
	return internalCalcToWatchDirectories();
}
std::string FofiModel::internalCalcToWatchDirectories()
{
	m_bInitialSetup = true;
	try {
		initialSetup();
	} catch (const std::runtime_error& oErr) {
		m_bInitialSetup = false;
		return oErr.what(); //--------------------------------------------------
	}

	m_bInitialSetup = false;
	return "";
}
const std::deque<FofiModel::ToWatchDir>& FofiModel::getToWatchDirectories() const
{
	return m_aToWatchDirs;
}
int32_t FofiModel::getRootToWatchDirectoriesIdx() const
{
	return m_nRootTWDIdx;
}
std::string FofiModel::start()
{
	assert(m_nEventCounter == 0);
	m_nEventCounter = 1; // marks start watching
	// create ToWatchDir and add to INotifierSource
	const std::string sError = internalCalcToWatchDirectories();
	if (! sError.empty()) {
		m_nEventCounter = 0;
		return sError; //-------------------------------------------------------
	}

	m_nStartTimeUsec = Util::getNowTimeMicroseconds();
	m_nStopTimeUsec = -1;
	//
	m_nRootResultIdx = -1;
	m_aWatchedResults.clear();
	m_bOverflow = false;
	m_bHasInconsistencies = false;
	//
	assert(m_aOpenMoves.empty());
	return "";
}
void FofiModel::stop()
{
	assert(m_nEventCounter > 0);
	m_nStopTimeUsec = Util::getNowTimeMicroseconds();
	m_aOpenMoves.clear();
	m_nEventCounter = 0; // stop watching
	m_refSource->clearAll();
}
int64_t FofiModel::getDuration() const
{
	if (m_nStartTimeUsec < 0) {
		return 0;
	} else if (m_nStopTimeUsec >= m_nStartTimeUsec) {
		return m_nStopTimeUsec - m_nStartTimeUsec;
	} else {
		return Util::getNowTimeMicroseconds() - m_nStartTimeUsec;
	}
}
bool FofiModel::isMatchedByFilters(const std::vector<Filter>& aFilters
									, const std::string& sName, const std::string& sPathName) const
{
	for (const auto& oIF : aFilters) {
//std::cout << "FofiModel::isMatchedByFilters sPathName=" << sPathName << "  oIF.m_sFilter=" << oIF.m_sFilter << '\n';
		const std::string& sStr = (oIF.bApplyToPathName ? sPathName : sName);
		if (oIF.m_eFilterType == FILTER_EXACT) {
			if (sStr == oIF.m_sFilter) {
				return true; //-------------------------------------------------
			}
		} else {
			const bool bMatch = std::regex_match(sStr, oIF.m_oFilterRegex);
			if (! bMatch) {
				return true; //-------------------------------------------------
			}
		}
	}
	return false;
}
int32_t FofiModel::addExistingToWatchDir(const std::string& sPath)
{
	checkThrowMaxToWatchDirsReached(); //-------------------------------
	const int32_t nTotWatchedDirs = static_cast<int32_t>(m_aToWatchDirs.size());
	const int32_t nTWDIdx = nTotWatchedDirs;
	m_aToWatchDirs.emplace_back();
	ToWatchDir& oTWD = m_aToWatchDirs[nTWDIdx];
	oTWD.m_sPathName = sPath;
	const auto nFoundSlashPos = sPath.find_last_of('/');
	assert(nFoundSlashPos != std::string::npos);
	oTWD.m_nNamePos = static_cast<int32_t>(nFoundSlashPos) + 1;
	oTWD.m_bExists = true;
	setDirectoryZone(oTWD);
	return nTWDIdx;
}
void FofiModel::createINotifyWatch(int32_t nTWDIdx, ToWatchDir& oTWD)
{
	const auto oPair = m_refSource->addPath(oTWD.m_sPathName, nTWDIdx);
	int32_t nErrno = oPair.first;
	if (nErrno == 0) {
		oTWD.m_nWatchedIdx = oPair.second;
		return; //--------------------------------------------------------------
	}
	assert(nErrno != INotifierSource::EXTENDED_ERRNO_FAKE_FS);
	assert(nErrno != INotifierSource::EXTENDED_ERRNO_WATCH_NOT_FOUND);
	checkThrowMaxInotifyUserWatches(nErrno); //---------------------------------
	// probably permission denied
	oTWD.m_nWatchedIdx = -1;
}
INotifierSource::FOFI_PROGRESS FofiModel::onFileModified(const INotifierSource::FofiData& oFofiData)
{
	++m_nEventCounter;
	if (oFofiData.m_bOverflow) {
		m_bOverflow = true;
		return INotifierSource::FOFI_PROGRESS_CONTINUE; //----------------------
	}
	INotifierSource::FOFI_ACTION eAction = oFofiData.m_eAction;
	const int32_t nParentTWDIdx = oFofiData.m_nTag;
	const bool bIsDir = oFofiData.m_bIsDir;
	const auto& sName = oFofiData.m_sName;
	ToWatchDir& oParentTWD = m_aToWatchDirs[nParentTWDIdx];
	//
#ifdef STMM_TRACE_DEBUG
//	std::cout << "FofiModel::onFileModified tag=" << oFofiData.m_nTag << "    name=\"" << oFofiData.m_sName << "\"  " << (oFofiData.m_bIsDir ? "DIR" : "FILE") << '\n';
//	std::cout << "               action=" << static_cast<int32_t>(oFofiData.m_eAction) << " cookie=" << oFofiData.m_nRenameCookie << '\n';
#endif //STMM_TRACE_DEBUG
	const auto nNowUsec = Util::getNowTimeMicroseconds() - m_nStartTimeUsec;
	//
	if (sName.empty()) {
		assert(eAction == INotifierSource::FOFI_ACTION_ATTRIB);
		if (nParentTWDIdx != m_nRootTWDIdx) {
			return INotifierSource::FOFI_PROGRESS_CONTINUE; //------------------
		}
		// The only directory for which we are interested in a self attrib change
		// is the root directory since it has no parent
		if (m_nRootResultIdx < 0) {
			m_nRootResultIdx = findRootResult();
		}
		if (m_nRootResultIdx < 0) {
			m_nRootResultIdx = addWatchedResultRoot();
			WatchedResult& oWatchedResult = m_aWatchedResults[m_nRootResultIdx];
			oWatchedResult.m_eResultType = RESULT_MODIFIED;
//std::cout << "FofiModel::onFileModified RESULT 2 sChildPath=/ (root)" << '\n';
			addActionData(oWatchedResult, eAction, nNowUsec);
		} else {
			#ifndef NDEBUG
			WatchedResult& oWatchedResult = m_aWatchedResults[m_nRootResultIdx];
			#endif //NDEBUG
			assert(oWatchedResult.m_eResultType == RESULT_MODIFIED);
		}
		return INotifierSource::FOFI_PROGRESS_CONTINUE; //----------------------
	}

	const std::string sChildPathName = Util::getPathFromDirAndName(oParentTWD.m_sPathName, sName);

	const bool bFilteredOut = isFilteredOut(bIsDir, oParentTWD, sName, sChildPathName);
	bool bWasAttrib = false;
	//
	const bool bRenameFrom = (eAction == INotifierSource::FOFI_ACTION_RENAME_FROM);
	if (bRenameFrom || (eAction == INotifierSource::FOFI_ACTION_RENAME_TO)) {
		if (bRenameFrom) {
			m_aOpenMoves.emplace_back();
			OpenMove& oOpenMove = m_aOpenMoves.back();
//std::cout << "onFileModified RENAME FROM nNowUsec=" << nNowUsec << "  cookie=" << oFofiData.m_nRenameCookie << " &Move=" << reinterpret_cast<int64_t>(&oOpenMove) << '\n';
			oOpenMove.m_nParentTWDIdx = nParentTWDIdx;
			oOpenMove.m_nTWDIdx = -1; // The renamed (watched) directory, filled below if bIsDir == true
			oOpenMove.m_bIsDir = bIsDir;
			oOpenMove.m_sName = sName;
			oOpenMove.m_sPathName = sChildPathName;
			oOpenMove.m_nRenameCookie = oFofiData.m_nRenameCookie;
			oOpenMove.m_bFilteredOut = bFilteredOut;
			if (bIsDir && !bFilteredOut) {
				int32_t nChildTWDIdx = findToWatchDir(nParentTWDIdx, sChildPathName);
				if ((nChildTWDIdx < 0) && !oParentTWD.isLeaf()) {
					// inconsistent: renaming from a (theoretically) existing folder
					// that has no ToWatchDir associated with it
					// maybe missed a create event or race condition during initial setup
					// Check other objects are coherent
					assert(findResult(nParentTWDIdx, sName, true) < 0);
					#ifndef NDEBUG
					auto itFindE = oParentTWD.findInExisting(bIsDir, sName);
					#endif //NDEBUG
					assert(itFindE == oParentTWD.m_aExisting.end());
					try {
						// just create a non iwatched but marked as existing (falsely, since moved from) TWD
						nChildTWDIdx = addExistingToWatchDir(sChildPathName);
					} catch (const std::runtime_error& oErr) {
						m_oAbortSignal.emit(oErr.what());
						return INotifierSource::FOFI_PROGRESS_CONTINUE; //------
					}
					assert(nChildTWDIdx >= 0);
					ToWatchDir& oChildTWD = m_aToWatchDirs[nChildTWDIdx];
					oChildTWD.m_nParentTWDIdx = nParentTWDIdx;
					Util::addValueToDequeUniquely(oParentTWD.m_aToWatchSubdirIdxs, nChildTWDIdx);
				} else {
					// Setting to non existing is done in the rename to?
					//ToWatchDir& oChildTWD = m_aToWatchDirs[nChildTWDIdx];
					//oChildTWD.m_bExists = false;
				}
				oOpenMove.m_nTWDIdx = nChildTWDIdx;
				// since might have been a long operation
				oOpenMove.m_nMoveFromTimeUsec = Util::getNowTimeMicroseconds() - m_nStartTimeUsec;
			} else {
				oOpenMove.m_nMoveFromTimeUsec = nNowUsec;
			}
		} else { // rename to
//std::cout << "onFileModified RENAME TO nNowUsec=" << nNowUsec << "  cookie=" << oFofiData.m_nRenameCookie <<  '\n';
			const auto itFind = std::find_if(m_aOpenMoves.begin(), m_aOpenMoves.end(), [&](const OpenMove& oOpenMove)
			{
//std::cout << "onFileModified RENAME TO cookie=" << oOpenMove.m_nRenameCookie <<  '\n';
//std::cout << "              oOpenMove.m_sName=" << oOpenMove.m_sName <<  '\n';
				return (oOpenMove.m_nRenameCookie == oFofiData.m_nRenameCookie);
			});
			if (itFind != m_aOpenMoves.end()) {
//std::cout << "onFileModified RENAME TO FOUND! m_aOpenMoves.size()=" << m_aOpenMoves.size() << '\n';
				OpenMove& oOpenMove = *itFind;
				if (bFilteredOut && oOpenMove.m_bFilteredOut) {
					return INotifierSource::FOFI_PROGRESS_CONTINUE; //----------
				}
				// traverse the source and dest in parallel
				// trying to keep watches for the same inode by renaming the tag (nParentTWDIdx)
				// of directories. Note that the destination zone might have different
				// depth, filters and some folders and files not be created in the destination
				// On the other hand the destination zone might create ToWatchDir for
				// actual folders that were filtered out in the source or (alas) were
				// created immediately after the inode rename (generating fake file renames)
				const auto& sFromParentPath = m_aToWatchDirs[oOpenMove.m_nParentTWDIdx].m_sPathName;
				try {
					traverseRename((oOpenMove.m_bFilteredOut ? -1 : oOpenMove.m_nParentTWDIdx)
									, sFromParentPath, oOpenMove.m_sName, oOpenMove.m_sPathName
									, bIsDir
									, (bFilteredOut ? -1 : nParentTWDIdx)
									, oParentTWD.m_sPathName, sName, sChildPathName
									, nNowUsec);
				} catch (const std::runtime_error& oErr) {
					m_oAbortSignal.emit(oErr.what());
					return INotifierSource::FOFI_PROGRESS_CONTINUE; //----------
				}
				if (m_aOpenMoves.size() > 1) {
					std::swap(oOpenMove, m_aOpenMoves.back());
				}
				m_aOpenMoves.pop_back();
			} else {
//std::cout << "onFileModified RENAME TO !NOT!FOUND! m_aOpenMoves.size()=" << m_aOpenMoves.size() << '\n';
				// rename to from outside watched area
				if (bFilteredOut) {
					return INotifierSource::FOFI_PROGRESS_CONTINUE; //----------
				}
				try {
					traverseRename(-1
									, "", "", ""
									, bIsDir
									, nParentTWDIdx
									, oParentTWD.m_sPathName, sName, sChildPathName
									, nNowUsec);
				} catch (const std::runtime_error& oErr) {
					m_oAbortSignal.emit(oErr.what());
					return INotifierSource::FOFI_PROGRESS_CONTINUE; //----------
				}
			}
		}
	} else if (bFilteredOut) {
//std::cout << "FofiModel::onFileModified FILTERED OUT" << '\n';
		return INotifierSource::FOFI_PROGRESS_CONTINUE; //----------------------
	} else {
		bool bResultIdxFindCalled = false;
		int32_t nResultIdx = -1;
		if ((eAction == INotifierSource::FOFI_ACTION_ATTRIB) && !m_bIsUserRoot) {
			// Did an attribute change cause the file or dir to become visible?
			// or to disappear?
			// (root user can see everything)
			nResultIdx = findResult(oParentTWD.m_sPathName, sName, bIsDir);
			bResultIdxFindCalled = true;
			if (nResultIdx < 0) {
				auto itFindE = oParentTWD.findInExisting(bIsDir, sName);
				if (itFindE == oParentTWD.m_aExisting.end()) {
					// popped into existence (visibility from root only to normal user)
					eAction = INotifierSource::FOFI_ACTION_CREATE;
					bWasAttrib = true;
				}
			} else {
				WatchedResult& oWatchedResult = m_aWatchedResults[nResultIdx];
				if (! oWatchedResult.exists()) {
					// popped into existence (visibility from root only to normal user)
					eAction = INotifierSource::FOFI_ACTION_CREATE;
					bWasAttrib = true;
				}
			}
		}
#ifdef STMM_TRACE_DEBUG
		if (bWasAttrib) {
//			std::cout << "WAS ATTRIB!!!   sChildPathName=" << sChildPathName << "  action=" << static_cast<int32_t>(eAction)<< '\n';
		}
#endif //STMM_TRACE_DEBUG
		if (! bResultIdxFindCalled) {
			nResultIdx = findResult(nParentTWDIdx, sName, bIsDir);
		}
		bool bWatchedResultExists = (nResultIdx >= 0);
		bool bEmitWatchedResult = true;
		bool bWasCreatedImmediately = false;
		bool bInconsistent = false;
		if (! bWatchedResultExists) {
			const auto itFindE = oParentTWD.findInExisting(bIsDir, sName);
			const bool bExisted = (itFindE != oParentTWD.m_aExisting.end());
			nResultIdx = addWatchedResult(oParentTWD, sName, bIsDir);
			WatchedResult& oWatchedResult = m_aWatchedResults[nResultIdx];
//std::cout << "FofiModel::onFileModified RESULT 3 sChildPath=" << sChildPathName << '\n';
			if (eAction == INotifierSource::FOFI_ACTION_CREATE) {
#ifdef STMM_TRACE_DEBUG
//	std::cout << "FofiModel::onFileModified  CREATE (N) nParentTWDIdx=" << nParentTWDIdx << "  sName=" << sName << '\n';
#endif //STMM_TRACE_DEBUG
				oWatchedResult.m_eResultType = RESULT_CREATED;
				if (bExisted) {
					// Inconsistent: existing file or dir is created! Missed remove
					// or race condition happened during initial setup
#ifdef STMM_TRACE_DEBUG
//	std::cout << "FofiModel::onFileModified  Inconsistent nParentTWDIdx=" << nParentTWDIdx << "  sName=" << sName << '\n';
//	std::cout << "FofiModel::onFileModified            1x oWatchedResult.m_eResultType=" << static_cast<int32_t>(oWatchedResult.m_eResultType) << '\n';
#endif //STMM_TRACE_DEBUG
					setInconsistent(oWatchedResult);
				}
			} else if (eAction == INotifierSource::FOFI_ACTION_DELETE) {
#ifdef STMM_TRACE_DEBUG
//	std::cout << "FofiModel::onFileModified  DELETE (N) nParentTWDIdx=" << nParentTWDIdx << "  sName=" << sName << '\n';
#endif //STMM_TRACE_DEBUG
				oWatchedResult.m_eResultType = RESULT_DELETED;
				if (! bExisted) {
					// Inconsistent: non existing file or dir is deleted! Missed create
					// or race condition happened during initial setup
#ifdef STMM_TRACE_DEBUG
//	std::cout << "FofiModel::onFileModified  Inconsistent nParentTWDIdx=" << nParentTWDIdx << "  sName=" << sName << '\n';
//	std::cout << "FofiModel::onFileModified            1y oWatchedResult.m_eResultType=" << static_cast<int32_t>(oWatchedResult.m_eResultType) << '\n';
#endif //STMM_TRACE_DEBUG
					setInconsistent(oWatchedResult);
				}
			} else {
				assert((eAction == INotifierSource::FOFI_ACTION_MODIFY) || (eAction == INotifierSource::FOFI_ACTION_ATTRIB));
				oWatchedResult.m_eResultType = RESULT_MODIFIED;
				if (! bExisted) {
					// Inconsistent: non existing file or dir is modified! Missed create
					// or race condition happened during initial setup
#ifdef STMM_TRACE_DEBUG
//	std::cout << "FofiModel::onFileModified  Inconsistent nParentTWDIdx=" << nParentTWDIdx << "  sName=" << sName << '\n';
//	std::cout << "FofiModel::onFileModified            1z oWatchedResult.m_eResultType=" << static_cast<int32_t>(oWatchedResult.m_eResultType) << '\n';
#endif //STMM_TRACE_DEBUG
					setInconsistent(oWatchedResult);
				}
			}
			ActionData& oActionData = addActionData(oWatchedResult, eAction, nNowUsec);
			oActionData.m_bCausedByAttribChange = bWasAttrib;
		} else {
			WatchedResult& oWatchedResult = m_aWatchedResults[nResultIdx];
			const bool bExistedAtStart = oWatchedResult.existedAtStart();
			if (eAction == INotifierSource::FOFI_ACTION_CREATE) {
				bWasCreatedImmediately = oWatchedResult.immediate();
#ifdef STMM_TRACE_DEBUG
//	std::cout << "FofiModel::onFileModified  CREATE (E) nParentTWDIdx=" << nParentTWDIdx << "  sName=" << sName << "  bWasCreatedImmediately=" << bWasCreatedImmediately << '\n';
#endif //STMM_TRACE_DEBUG
				// from RESULT_DELETED to RESULT_MODIFIED
				// from RESULT_TEMPORARY to RESULT_CREATED
				// from RESULT_CREATED to RESULT_CREATED: error
				// from RESULT_MODIFIED to RESULT_MODIFIED: error
				bInconsistent = ((! bWasCreatedImmediately) && oWatchedResult.exists());
				if (bInconsistent) {
#ifdef STMM_TRACE_DEBUG
//	std::cout << "FofiModel::onFileModified Inconsistent nParentTWDIdx=" << nParentTWDIdx << "  sName=" << sName << '\n';
//	std::cout << "FofiModel::onFileModified            2 oWatchedResult.m_eResultType=" << static_cast<int32_t>(oWatchedResult.m_eResultType) << '\n';
#endif //STMM_TRACE_DEBUG
					setInconsistent(oWatchedResult);
				}
				if ((! bWasCreatedImmediately) || bInconsistent) {
					ActionData& oActionData = addActionData(oWatchedResult, INotifierSource::FOFI_ACTION_CREATE, nNowUsec);
					oActionData.m_bCausedByAttribChange = bWasAttrib;
				} else {
					setNotImmediate(oWatchedResult);
					//#ifndef NDEBUG
					//ActionData& oActionData = oWatchedResult.m_aActions.back();
					//#endif //NDEBUG
					//assert(oActionData.m_eAction == INotifierSource::FOFI_ACTION_CREATE);
					////TODO could also be MOVED_TO !!!
					bEmitWatchedResult = false;
				}
				//
				oWatchedResult.m_eResultType = (bExistedAtStart ? RESULT_MODIFIED : RESULT_CREATED);
				//
			} else if (eAction == INotifierSource::FOFI_ACTION_DELETE) {
#ifdef STMM_TRACE_DEBUG
//	std::cout << "FofiModel::onFileModified  DELETE (E) nParentTWDIdx=" << nParentTWDIdx << "  sName=" << sName << '\n';
#endif //STMM_TRACE_DEBUG
				// from RESULT_DELETED to RESULT_DELETED error
				// from RESULT_TEMPORARY to RESULT_TEMPORARY error
				// from RESULT_CREATED to RESULT_TEMPORARY
				// from RESULT_MODIFIED to RESULT_DELETED
				if (! oWatchedResult.exists()) {
#ifdef STMM_TRACE_DEBUG
//	std::cout << "FofiModel::onFileModified Inconsistent nParentTWDIdx=" << nParentTWDIdx << "  sName=" << sName << '\n';
//	std::cout << "FofiModel::onFileModified            3 oWatchedResult.m_eResultType=" << static_cast<int32_t>(oWatchedResult.m_eResultType) << '\n';
#endif //STMM_TRACE_DEBUG
					setInconsistent(oWatchedResult);
				}
				ActionData& oActionData = addActionData(oWatchedResult, eAction, nNowUsec);
				oActionData.m_bCausedByAttribChange = bWasAttrib;
				//
				oWatchedResult.m_eResultType = (bExistedAtStart ? RESULT_DELETED : RESULT_TEMPORARY);
			} else {
				assert((eAction == INotifierSource::FOFI_ACTION_MODIFY) || (eAction == INotifierSource::FOFI_ACTION_ATTRIB));
				// from RESULT_DELETED to RESULT_MODIFIED error
				// from RESULT_TEMPORARY to RESULT_MODIFIED error
				// from RESULT_CREATED to RESULT_MODIFIED ignore
				// from RESULT_MODIFIED to RESULT_MODIFIED ignore
				if (! oWatchedResult.exists()) {
#ifdef STMM_TRACE_DEBUG
//	std::cout << "FofiModel::onFileModified Inconsistent nParentTWDIdx=" << nParentTWDIdx << "  sName=" << sName << '\n';
//	std::cout << "FofiModel::onFileModified            4 oWatchedResult.m_eResultType=" << static_cast<int32_t>(oWatchedResult.m_eResultType) << '\n';
#endif //STMM_TRACE_DEBUG
					setInconsistent(oWatchedResult);
					ActionData& oActionData = addActionData(oWatchedResult, eAction, nNowUsec);
					oActionData.m_bCausedByAttribChange = bWasAttrib;
					//
					oWatchedResult.m_eResultType = (bExistedAtStart ? RESULT_MODIFIED : RESULT_CREATED);
				} else {
					// no significant action
					setNotImmediate(oWatchedResult);
					//TODO maybe the attribute change made a directory unwatchable?
					bEmitWatchedResult = false;
				}
			}
		}
//std::cout << "FofiModel::onFileModified added result: sName=" << sName << "  oParentTWD.m_sPath=" << oParentTWD.m_sPathName << '\n';
		WatchedResult& oWatchedResult = m_aWatchedResults[nResultIdx];
		assert(! oWatchedResult.immediate());
		//
		if (! bIsDir) {
			if (bEmitWatchedResult) {
				m_oWatchedResultActionSignal.emit(oWatchedResult);
			}
			return INotifierSource::FOFI_PROGRESS_CONTINUE; //------------------
		}

		const bool bParentIsLeaf = oParentTWD.isLeaf();
//std::cout << "FofiModel::onFileModified parent=" << nParentTWDIdx << "  is leaf=" << bParentIsLeaf << '\n';
		//
		if (eAction == INotifierSource::FOFI_ACTION_CREATE) {
			//
			try {
				// check whether a TWD already exists!
				int32_t nChildTWDIdx = findToWatchDir(nParentTWDIdx, sChildPathName);
//std::cout << "FofiModel::onFileModified nChildTWDIdx=" << nChildTWDIdx << '\n';
				if (nChildTWDIdx < 0) {
					if (bParentIsLeaf) {
						// if a structural TWD is not present this dir isn't watched
						if (bEmitWatchedResult) {
							m_oWatchedResultActionSignal.emit(oWatchedResult);
						}
						return INotifierSource::FOFI_PROGRESS_CONTINUE; //------
					}
					nChildTWDIdx = addExistingToWatchDir(sChildPathName);
					ToWatchDir& oChildTWD = m_aToWatchDirs[nChildTWDIdx];
					oChildTWD.m_nParentTWDIdx = nParentTWDIdx;
					Util::addValueToDequeUniquely(oParentTWD.m_aToWatchSubdirIdxs, nChildTWDIdx);
				} else {
					ToWatchDir& oChildTWD = m_aToWatchDirs[nChildTWDIdx];
					if (oChildTWD.m_bExists) {
						// creating a dir that already exists
						if ((! bWasCreatedImmediately) || bInconsistent) {
							// inconsistent: missed delete dir event
							if (oChildTWD.isWatched()) {
								// the watch has probably gone anyway
								/*const int32_t nErrno =*/
								m_refSource->removePath(oChildTWD.m_nWatchedIdx, nChildTWDIdx);
								oChildTWD.m_nWatchedIdx = -1;
							}
							if (bWatchedResultExists) {
								// If WR existed and TWD is inconsistent
								// the WR must be inconsistent too
								assert(bInconsistent);
							} else {
								// Since having a TWD doesn't imply a WR (for the same dir) is also present
								if (!bInconsistent) {
#ifdef STMM_TRACE_DEBUG
//	std::cout << "FofiModel::onFileModified Inconsistent nParentTWDIdx=" << nParentTWDIdx << "  sName=" << sName << '\n';
//	std::cout << "FofiModel::onFileModified            6 oWatchedResult.m_eResultType=" << static_cast<int32_t>(oWatchedResult.m_eResultType) << '\n';
#endif //STMM_TRACE_DEBUG
									setInconsistent(oWatchedResult);
									assert(oWatchedResult.m_eResultType == RESULT_CREATED);
									// when a directory is deleted and then recreated mark it as modified
									oWatchedResult.m_eResultType = RESULT_MODIFIED;
								}
							}
							assert(bEmitWatchedResult);
						} else {
//std::cout << "FofiModel::onFileModified IMM sChildPathName" << sChildPathName << '\n';
							// the creation was already emitted by createImmediateChildren
							// it would be a duplicate
							bEmitWatchedResult = false;
						}
					} else {
						assert(! oChildTWD.isWatched());
						oChildTWD.m_bExists = true;
					}
				}
				ToWatchDir& oChildTWD = m_aToWatchDirs[nChildTWDIdx];
				if (! oChildTWD.isWatched()) {
					// It is important that the inotify watch is created before
					// looking for already created sub dirs (createImmediateChildren)
					createINotifyWatch(nChildTWDIdx, oChildTWD);
				}
				//
				if (bEmitWatchedResult) {
					m_oWatchedResultActionSignal.emit(oWatchedResult);
				}
				//
				if (oChildTWD.isWatched()) {
					createImmediateChildren(nChildTWDIdx, bWasAttrib, nNowUsec);
				}
			} catch (const std::runtime_error& oErr) {
				m_oAbortSignal.emit(oErr.what());
				return INotifierSource::FOFI_PROGRESS_CONTINUE; //--------------
			}
		} else if (eAction == INotifierSource::FOFI_ACTION_DELETE) {
			const int32_t nChildTWDIdx = findToWatchDir(nParentTWDIdx, sChildPathName);
			if (nChildTWDIdx >= 0) {
				ToWatchDir& oChildTWD = m_aToWatchDirs[nChildTWDIdx];
				if (oChildTWD.m_bExists) {
					if (oChildTWD.isWatched()) {
						m_refSource->removePath(oChildTWD.m_nWatchedIdx, nChildTWDIdx);
						oChildTWD.m_nWatchedIdx = -1;
					}
					oChildTWD.m_bExists = false;
					oChildTWD.m_aExisting.clear();
					//TODO check whether child TWDs (of oChildTWD) are still
					//TODO marked as existing and watched recursively
					//TODO and go through the WR and possibly add the missed delete action
					//TODO and set as inconsistent
					//TODO traverseRemove(oChildTWD);
				}
			} else {
				// deleting a subdir that was never created?
			}
			if (bEmitWatchedResult) {
				m_oWatchedResultActionSignal.emit(oWatchedResult);
			}
		}
	}
	return INotifierSource::FOFI_PROGRESS_CONTINUE;
}
void FofiModel::traverseRename(int32_t nFromParentTWDIdx
								, const std::string& sFromParentPath, const std::string& sFromName, const std::string& sFromPath
								, bool bIsDir
								, int32_t nToParentTWDIdx
								, const std::string& sToParentPath, const std::string& sToName, const std::string& sToPath
								, int64_t nNowUsec)
{
//std::cout << "FofiModel::traverseRename  sFromPath=" << sFromPath << "  sToPath=" << sToPath << '\n';
	const bool bFromParentWatched = (nFromParentTWDIdx >= 0);
	const bool bToParentWatched = (nToParentTWDIdx >= 0);
	assert(bFromParentWatched || bToParentWatched);
	assert((!sFromParentPath.empty()) || !bFromParentWatched); // sFromParentPath.empty() implies !bFromParentWatched
	assert((!sToParentPath.empty()) || !bToParentWatched); // sToParentPath.empty() implies !bToParentWatched
	assert(sFromParentPath.empty() == sFromName.empty());
	assert(sFromParentPath.empty() == sFromPath.empty());
	assert(sToParentPath.empty() == sToName.empty());
	assert(sToParentPath.empty() == sToPath.empty());
	if (bFromParentWatched) {
		int32_t nFromResultIdx = findResult(nFromParentTWDIdx, sFromName, bIsDir);
		const bool bNoFromResult = (nFromResultIdx < 0);
//std::cout << "FofiModel::traverseRename bNoFromResult=" << bNoFromResult << "  sFromName=" << sFromName << "  bIsDir=" << bIsDir<< '\n';
		if (bNoFromResult) {
			auto& oFromParentTWD = m_aToWatchDirs[nFromParentTWDIdx];
			nFromResultIdx = addWatchedResult(oFromParentTWD, sFromParentPath, sFromName, bIsDir);
//std::cout << "FofiModel::traverseRename RESULT 4 sChildPath=" << sFromParentPath << "/" << sFromName << '\n';
		}
		WatchedResult& oWatchedResult = m_aWatchedResults[nFromResultIdx];
//if (! oWatchedResult.m_aActions.empty()) {
//std::cout << "FofiModel::traverseRename action 0=" << static_cast<int32_t>(oWatchedResult.m_aActions[0].m_eAction) << "  size=" << oWatchedResult.m_aActions.size() << '\n';
//}
		if (bNoFromResult) {
			oWatchedResult.m_eResultType = RESULT_DELETED;
		} else {
			// from RESULT_DELETED to RESULT_DELETED error
			// from RESULT_TEMPORARY to RESULT_TEMPORARY error
			// from RESULT_CREATED to RESULT_TEMPORARY
			// from RESULT_MODIFIED to RESULT_DELETED
			const bool bFromExistedAtStart = oWatchedResult.existedAtStart();
			const bool bFromInconsistent = ! oWatchedResult.exists();
			if (bFromInconsistent) {
				// missed a create dir?
#ifdef STMM_TRACE_DEBUG
//	std::cout << "FofiModel::traverseRename Inconsistent nFromParentTWDIdx=" << nFromParentTWDIdx << "  sFromName=" << sFromName << '\n';
//	std::cout << "FofiModel::traverseRename            7 oWatchedResult.m_eResultType=" << static_cast<int32_t>(oWatchedResult.m_eResultType) << '\n';
#endif //STMM_TRACE_DEBUG
				setInconsistent(oWatchedResult);
			}
			oWatchedResult.m_eResultType = (bFromExistedAtStart ? RESULT_DELETED : RESULT_TEMPORARY);
		}
		ActionData& oActionData = addActionData(oWatchedResult, INotifierSource::FOFI_ACTION_RENAME_FROM, nNowUsec);
		oActionData.m_sOtherPath = sToPath;
		m_oWatchedResultActionSignal.emit(oWatchedResult);
	}

	//
	int32_t nToResultIdx = -1;
	if (bToParentWatched) {
		nToResultIdx = findResult(nToParentTWDIdx, sToName, bIsDir);
		const bool bNoToResult = (nToResultIdx < 0);
//std::cout << "FofiModel::traverseRename bNoToResult=" << bNoToResult << "  sToName=" << sToName << "  bIsDir=" << bIsDir << '\n';
		if (bNoToResult) {
			auto& oToParentTWD = m_aToWatchDirs[nToParentTWDIdx];
			nToResultIdx = addWatchedResult(oToParentTWD, sToParentPath, sToName, bIsDir);
//std::cout << "FofiModel::traverseRename RESULT 5 sChildPath=" << sToParentPath << "/" << sToName << '\n';
		}
		WatchedResult& oWatchedResult = m_aWatchedResults[nToResultIdx];
		if (bNoToResult) {
			oWatchedResult.m_eResultType = RESULT_CREATED;
		} else {
			// from RESULT_DELETED to RESULT_MODIFIED
			// from RESULT_TEMPORARY to RESULT_CREATED
			// from RESULT_CREATED to RESULT_CREATED: NO error!
			// from RESULT_MODIFIED to RESULT_MODIFIED: NO error!
			const bool bToExistedAtStart = oWatchedResult.existedAtStart();
			oWatchedResult.m_eResultType = (bToExistedAtStart ? RESULT_MODIFIED : RESULT_CREATED);
		}
		ActionData& oActionData = addActionData(oWatchedResult, INotifierSource::FOFI_ACTION_RENAME_TO, nNowUsec);
		oActionData.m_sOtherPath = sFromPath;
		m_oWatchedResultActionSignal.emit(oWatchedResult);
	}
	if (! bIsDir) {
		return; //--------------------------------------------------------------
	}
	const int32_t nFromTWDIdx = (bFromParentWatched ? findToWatchDir(nFromParentTWDIdx, sFromPath) : -1);
	const bool bFromExists = (nFromTWDIdx >= 0) && m_aToWatchDirs[nFromTWDIdx].m_bExists;
	//
	int32_t nToTWDIdx = (bToParentWatched ? findToWatchDir(nToParentTWDIdx, sToPath) : -1);
	if (bToParentWatched) {
		if (nToTWDIdx < 0) {
			// from function invariant we know sToPath not filtered out
			// but if parent is leaf the TWD should either already exist because base path
			// of another DZ or because it's a gap filler, otherwise recursion (as
			// far as the destination is concerned) stops.
			const bool bToParentIsLeaf = m_aToWatchDirs[nToParentTWDIdx].isLeaf();
			if (!bToParentIsLeaf) {
				// create a TWD!
				nToTWDIdx = addExistingToWatchDir(sToPath);
				auto& oToTWD = m_aToWatchDirs[nToTWDIdx];
				oToTWD.m_nParentTWDIdx = nToParentTWDIdx;
				auto& oToParentTWD = m_aToWatchDirs[nToParentTWDIdx];
				Util::addValueToDequeUniquely(oToParentTWD.m_aToWatchSubdirIdxs, nToTWDIdx);
				// not adding iwatch yet because it might be transfered to
				// destination path by from path further down
			}
		} else {
			auto& oToTWD = m_aToWatchDirs[nToTWDIdx];
			if (oToTWD.m_bExists) {
				// a delete event was missed
				if (oToTWD.isWatched()) {
					/*const int32_t nErrno =*/
					m_refSource->removePath(oToTWD.m_nWatchedIdx, nToTWDIdx);
					oToTWD.m_nWatchedIdx = -1;
				}
				WatchedResult& oWatchedResult = m_aWatchedResults[nToResultIdx];
#ifdef STMM_TRACE_DEBUG
//	std::cout << "FofiModel::traverseRename Inconsistent nToParentTWDIdx=" << nToParentTWDIdx << "  sToName=" << sToName << '\n';
//	std::cout << "FofiModel::traverseRename            9 oWatchedResult.m_eResultType=" << static_cast<int32_t>(oWatchedResult.m_eResultType) << '\n';
#endif //STMM_TRACE_DEBUG
				setInconsistent(oWatchedResult);
				setNotImmediate(oWatchedResult);
				oToTWD.m_aExisting.clear();
			} else {
				// pretend the directory exists even though oFStat.exixts()
				// might already be false causing of delete event following in the inotify queue
				oToTWD.m_bExists = true;
			}
		}
	}
	const bool bToExists = (nToTWDIdx >= 0) && m_aToWatchDirs[nToTWDIdx].m_bExists;
//std::cout << "FofiModel::traverseRename bFromExists=" << bFromExists << " bToExists=" << bToExists << '\n';
//std::cout << "                          sFromPath=" << sFromPath << " sToPath=" << sToPath << '\n';
//std::cout << "                          nFromTWDIdx=" << nFromTWDIdx << " nToTWDIdx=" << nToTWDIdx << '\n';
	if ((!bFromExists) && !bToExists) {
		return; //--------------------------------------------------------------
	}
	std::vector<ToWatchDir::FileDir> aVisitedNames;
	if (bFromExists) {
		ToWatchDir& oFromTWD = m_aToWatchDirs[nFromTWDIdx];
		if (oFromTWD.isWatched()) {
			if (bToExists) {
				// transfer iwatch to destination
//std::cout << "FofiModel::traverseRename RENAME iWatch!" << '\n';
				#ifndef NDEBUG
				const int32_t nErrno =
				#endif //NDEBUG
				m_refSource->renamePath(oFromTWD.m_nWatchedIdx, nFromTWDIdx, nToTWDIdx);
				assert(nErrno != INotifierSource::EXTENDED_ERRNO_WATCH_NOT_FOUND);
				auto& oToTWD = m_aToWatchDirs[nToTWDIdx];
				assert(oToTWD.m_nWatchedIdx < 0);
				oToTWD.m_nWatchedIdx = oFromTWD.m_nWatchedIdx;
				oFromTWD.m_nWatchedIdx = -1;
			} else {
				// Cannot transfer iwatch to destination, just remove it
				#ifndef NDEBUG
				const int32_t nErrno =
				#endif //NDEBUG
				m_refSource->removePath(oFromTWD.m_nWatchedIdx, nFromTWDIdx);
				assert(nErrno != INotifierSource::EXTENDED_ERRNO_WATCH_NOT_FOUND);
				oFromTWD.m_nWatchedIdx = -1;
			}
		}
		oFromTWD.m_bExists = false;
		for (const int32_t& nFromChildTWDIdx : oFromTWD.m_aToWatchSubdirIdxs) {
			auto& oFromChildTWD = m_aToWatchDirs[nFromChildTWDIdx];
			// if the TWD for the from child exists, we know it isn't filtered out
			char const* p0FromChildName = oFromChildTWD.getName();
			assert(p0FromChildName != nullptr);
			const std::string sFromChildName{p0FromChildName};
			aVisitedNames.push_back({sFromChildName, true, false});
			if (! oFromChildTWD.m_bExists) {
				// oFromChildTWD.m_bExists equals child WR.exists() (if WR object present)
				continue; // for----
			}
			//
			bool bToChildDefined = bToExists;
			if (bToExists) {
				const ToWatchDir& oToTWD = m_aToWatchDirs[nToTWDIdx];
				const auto itFind = std::find_if(oToTWD.m_aToWatchSubdirIdxs.begin(), oToTWD.m_aToWatchSubdirIdxs.end(), [&](int32_t nCurTWDIdx)
				{
					auto& oToChildTWD = m_aToWatchDirs[nCurTWDIdx];
					char const* p0ToName = oToChildTWD.getName();
					assert(p0ToName != nullptr);
					return (sFromChildName == p0ToName);
				});
				const bool bNameWasWatched = (itFind != oToTWD.m_aToWatchSubdirIdxs.end());
				if (bNameWasWatched) {
					// the TWD for the to child found, therefore can't possibly be filtered out
					// even though it's not necessarily part of oToTWD's zone
					const int32_t nToChildTWDIdx = *itFind;
					ToWatchDir& oToChildTWD = m_aToWatchDirs[nToChildTWDIdx];
					if (oToChildTWD.m_bExists) {
						oToChildTWD.m_bExists = false;
						oToChildTWD.m_aExisting.clear();
						// inconsistent: we missed a remove event
						if (oToChildTWD.isWatched()) {
							// if it's watched it's permission might have changed
							m_refSource->removePath(oToChildTWD.m_nWatchedIdx, nToChildTWDIdx);
							oToChildTWD.m_nWatchedIdx = -1;
						}
					}
				} else {
					// check whether filtered out in the destination
					//TODO is this recalculated below?
					const auto sToChildPath = Util::getPathFromDirAndName(oToTWD.m_sPathName, sFromChildName);
					const bool bFilteredOut = isFilteredOutSubDir(oToTWD, sFromChildName, sToChildPath);
					if (bFilteredOut) {
						bToChildDefined = false;
					}
				}
			}
//std::cout << "FofiModel::traverseRename DIR1 sFromChildPath=" << oFromChildTWD.m_sPathName << '\n';
			const std::string sToChildPath = (bToChildDefined ? Util::getPathFromDirAndName(sToPath, sFromChildName) : m_sES);
			traverseRename(nFromTWDIdx, oFromTWD.m_sPathName, sFromChildName, oFromChildTWD.m_sPathName
							, true
							, (bToChildDefined ? nToTWDIdx : -1)
							, (bToChildDefined ? sToPath : m_sES), (bToChildDefined ? sFromChildName : m_sES), sToChildPath
							, nNowUsec);
			oFromChildTWD.m_bExists = false;
		}
		//
		for (const int32_t nWRIdx : oFromTWD.m_aWatchedResultIdxs) {
			WatchedResult& oWR = m_aWatchedResults[nWRIdx];
			const std::string& sFromChildName = oWR.m_sName;
			const bool bIsFromChildDir = oWR.m_bIsDir;
			if (bIsFromChildDir) {
				const auto itFind = std::find_if(aVisitedNames.begin(), aVisitedNames.end(), [&](const ToWatchDir::FileDir& oViFiDi) {
					return (oViFiDi.m_bIsDir && (oViFiDi.m_sName == sFromChildName));
				});
				if (itFind != aVisitedNames.end()) {
					continue; //for ---
				}
			}
			aVisitedNames.push_back({sFromChildName, bIsFromChildDir, false});
			if (! oWR.exists()) {
				continue; //for ---
			}
			// If WR exists from child can't be filtered out
			const auto sFromChildPath = Util::getPathFromDirAndName(oWR.m_sPath, sFromChildName);
			// make sure destination child isn't filtered out
			bool bToChildDefined = bToExists;
			const auto sToChildPathTemp = (bToExists ? Util::getPathFromDirAndName(sToPath, sFromChildName) : m_sES);
			if (bToExists) {
				const ToWatchDir& oToTWD = m_aToWatchDirs[nToTWDIdx];
				const bool bToFilteredOut = isFilteredOut(bIsFromChildDir, oToTWD, sFromChildName, sToChildPathTemp);
				if (bToFilteredOut) {
					bToChildDefined = false;
				}
			}
			const std::string& sToChildPath = (bToChildDefined ? sToChildPathTemp : m_sES);
			traverseRename(nFromTWDIdx, oWR.m_sPath, sFromChildName, sFromChildPath
							, bIsFromChildDir
							, (bToChildDefined ? nToTWDIdx : -1)
							, (bToChildDefined ? sToPath : m_sES), (bToChildDefined ? sFromChildName : m_sES), sToChildPath
							, nNowUsec);
		}
		//
		for (const ToWatchDir::FileDir& oFiDi : oFromTWD.m_aExisting) {
			if (oFiDi.m_bRemoved) {
				continue; //for ---
			}
			const std::string& sFromChildName = oFiDi.m_sName;
			const bool bIsFromChildDir = oFiDi.m_bIsDir;
			const auto itFind = std::find_if(aVisitedNames.begin(), aVisitedNames.end(), [&](const ToWatchDir::FileDir& oViFiDi) {
				return ((oViFiDi.m_bIsDir == bIsFromChildDir) && (oViFiDi.m_sName == sFromChildName));
			});
			if (itFind != aVisitedNames.end()) {
				continue; //for ---
			}
			aVisitedNames.push_back({sFromChildName, bIsFromChildDir, false});
			// make sure the child isn't filtered out in both source and destination
			bool bFromChildDefined = true;
			const auto sFromChildPathTemp = Util::getPathFromDirAndName(oFromTWD.m_sPathName, sFromChildName);
			const bool bFromFilteredOut = isFilteredOut(bIsFromChildDir, oFromTWD, sFromChildName, sFromChildPathTemp);
			if (bFromFilteredOut) {
				bFromChildDefined = false;
			}
			//
			bool bToChildDefined = bToExists;
			const auto sToChildPathTemp = (bToExists ? Util::getPathFromDirAndName(sToPath, sFromChildName) : m_sES);
			if (bToExists) {
				ToWatchDir& oToTWD = m_aToWatchDirs[nToTWDIdx];
				const bool bToFilteredOut = isFilteredOut(bIsFromChildDir, oToTWD, sFromChildName, sToChildPathTemp);
				if (bToFilteredOut) {
					bToChildDefined = false;
					// Since filtered out a WR won't be created for this name in the destination
					// so the name must be added to the exising of the destination
					oToTWD.m_aExisting.push_back({sFromChildName, bIsFromChildDir, false});
				}
			}
			if (! (bFromChildDefined || bToChildDefined)) {
				continue; //for ---
			}
			const std::string& sFromChildPath = (bFromChildDefined ? sFromChildPathTemp : m_sES);
			const std::string& sToChildPath = (bToChildDefined ? sToChildPathTemp : m_sES);
			traverseRename(nFromTWDIdx, oFromTWD.m_sPathName, sFromChildName, sFromChildPath
							, bIsFromChildDir
							, (bToChildDefined ? nToTWDIdx : -1)
							, (bToChildDefined ? sToPath : m_sES), (bToChildDefined ? sFromChildName : m_sES), sToChildPath
							, nNowUsec);
		}
		oFromTWD.m_aExisting.clear();
	}
	if (bToExists) {
		bool bWatchCreatedNow = false;
		auto& oToTWD = m_aToWatchDirs[nToTWDIdx];
		if (! oToTWD.isWatched()) {
			createINotifyWatch(nToTWDIdx, oToTWD);
			bWatchCreatedNow = true;
		}
		if (! oToTWD.isWatched()) {
			// permission denied
			return; //----------------------------------------------------------
		}
		// The files and dirs found in the destination have probably been created
		// immediately after the rename and shouldn't be part of the rename event
		// But if renamed from unknown might consider them renamed to files???
		if (bWatchCreatedNow) {
			createImmediateChildren(nToTWDIdx, false, nNowUsec, aVisitedNames);
		} else {
			// iwatch was renamed => possible immediate creation of files is caught
		}
	}
}
bool FofiModel::onCheckOpenMoves()
{
	if (m_aOpenMoves.size() == 0) {
		return true;
	}
	const auto nNowUsec = Util::getNowTimeMicroseconds() - m_nStartTimeUsec;
//std::cout << "onCheckOpenMoves   nNowUsec=" << nNowUsec<< '\n';
	auto itCurMove = m_aOpenMoves.begin();
	while (itCurMove != m_aOpenMoves.end()) {
		if (nNowUsec < itCurMove->m_nMoveFromTimeUsec + s_nOpenMovesFailedAfterUsec) {
			++itCurMove;
			continue; // while---
		}
		// moved out of watched area
		auto& oOpenMove = *itCurMove;
//std::cout << "onCheckOpenMoves   oOpenMove.m_nParentTWDIdx=" << oOpenMove.m_nParentTWDIdx << '\n';
//std::cout << "onCheckOpenMoves   oOpenMove.m_sName=" << oOpenMove.m_sName << '\n';
//std::cout << "onCheckOpenMoves   oOpenMove.m_sPathName=" << oOpenMove.m_sPathName << '\n';
//std::cout << "onCheckOpenMoves   oOpenMove.m_nRenameCookie=" << oOpenMove.m_nRenameCookie << '\n';
		if (! oOpenMove.m_bFilteredOut) {
			const auto& sFromParentPath = m_aToWatchDirs[oOpenMove.m_nParentTWDIdx].m_sPathName;
			traverseRename(oOpenMove.m_nParentTWDIdx
							, sFromParentPath, oOpenMove.m_sName, oOpenMove.m_sPathName
							, oOpenMove.m_bIsDir
							, -1
							, "", "", ""
							, nNowUsec);
		}
		itCurMove = m_aOpenMoves.erase(itCurMove);
	}
	return true;
}
int32_t FofiModel::findRootResult() const
{
	const auto itFind = std::find_if(m_aWatchedResults.begin(), m_aWatchedResults.end(), [&](const WatchedResult& oWR)
	{
		return (oWR.m_sPath == "/") && oWR.m_sName.empty();
	});
	if (itFind == m_aWatchedResults.end()) {
		return -1;
	}
	return static_cast<int32_t>(std::distance(m_aWatchedResults.begin(), itFind));
}
int32_t FofiModel::findResult(const std::string& sPath, const std::string& sName, bool bIsDir) const
{
	const int32_t nTWDIdx = findToWatchDir(sPath);
	if (nTWDIdx < 0) {
		return -1;
	}
	return findResult(nTWDIdx, sName, bIsDir);
}
int32_t FofiModel::findResult(int32_t nTWDIdx, const std::string& sName, bool bIsDir) const
{
	assert(nTWDIdx >= 0);
	assert(!sName.empty());
	const ToWatchDir& oTWD = m_aToWatchDirs[nTWDIdx];
	const auto& aResultIdxs = oTWD.m_aWatchedResultIdxs;
	const auto itFind = std::find_if(aResultIdxs.begin(), aResultIdxs.end(), [&](int32_t nResultIdx)
	{
		const WatchedResult& oWatchedResult = m_aWatchedResults[nResultIdx];
		return ((oWatchedResult.m_sName == sName) && (oWatchedResult.m_bIsDir == bIsDir));
	});
	if (itFind == aResultIdxs.end()) {
		return -1;
	}
	const auto nResultIdx = *itFind;
	return nResultIdx;
}
const std::deque<FofiModel::WatchedResult>& FofiModel::getWatchedResults() const
{
	return m_aWatchedResults;
}


} // namespace fofi

