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
 * File:   main.cc
 */

#include "config.h"

#include "../fofimodel.h"
#include "../util.h"

#include <glibmm.h>

#include <cassert>
#include <iostream>
#include <cmath>
#include <numeric>
#include <fstream>

#include <sys/types.h>
#include <unistd.h>
#include <string.h>

namespace fofi
{

static constexpr int32_t s_nDefaultMaxToWatchDirectories = std::numeric_limits<int32_t>::max() - 1;
static constexpr int32_t s_nDefaultMaxResultPaths = std::numeric_limits<int32_t>::max() - 1;

void printVersion()
{
	std::cout << Config::getVersionString() << '\n';
}
void printUsage()
{
	std::cout << "Usage: fofimon [options]" << '\n';
	std::cout << "Monitor selected folders and files for modifications." << '\n';
	std::cout << "Options:" << '\n';
	std::cout << "  -h --help               Prints this message." << '\n';
	std::cout << "  -v --version            Prints version." << '\n';
	std::cout << "  --dont-watch            Doesn't start watching." << '\n';
	std::cout << "  -f --add-file FILEPATH  Adds file FILEPATH to watch (unaffected by zone filters)." << '\n';
	std::cout << "  -z --add-zone DIRPATH   Adds a watched zone with base directory DIRPATH." << '\n';
	std::cout << "Zone options (must follow --add-zone):" << '\n';
	std::cout << "  -m --max-depth DEPTH    Sets the max depth of a zone. Examples of DEPTH:" << '\n';
	std::cout << "                          0: just watches the base path of the zone (default)." << '\n';
	std::cout << "                          1: watches base path and its direct subdirectories." << '\n';
	std::cout << "                          N: watches base path and N levels of subdirectories." << '\n';
	std::cout << "                          -1: watches all the directories in the subtree." << '\n';
	std::cout << "  --pinned-file NAME      File name that can't be excluded by the zone's filters." << '\n';
	std::cout << "  --pinned-dir NAME       Directory name that can't be excluded by the zone's filters." << '\n';
	std::cout << "  --include-files REGEX   Includes file name filter REGEX (POSIX)." << '\n';
	std::cout << "  --include-dirs REGEX    Includes dir name filter REGEX (POSIX)." << '\n';
	std::cout << "  --exclude-files REGEX   Excludes file name filter REGEX (POSIX). Overrides includes." << '\n';
	std::cout << "  --exclude-dirs REGEX    Excludes dir name filter REGEX (POSIX). Overrides includes." << '\n';
	std::cout << "  --exclude-file NAME     Excludes file name. Overrides includes." << '\n';
	std::cout << "  --exclude-dir NAME      Excludes dir name. Overrides includes." << '\n';
	std::cout << "  --exclude-all           Excludes all dir and file names. Overrides includes." << '\n';
	std::cout << "                          Same as defining regular expression filters \".*\"." << '\n';
	std::cout << "Output options:" << '\n';
	std::cout << "  --print-zones [OUT]       Prints directory zones (to OUT file if given)." << '\n';
	std::cout << "  --print-watched [OUT]     Prints initial to be watched directories (to OUT file if given)." << '\n';
	std::cout << "  -l --live-events [OUT]    Prints single events as they happen" << '\n';
	std::cout << "                            (to OUT file if given)." << '\n';
	std::cout << "  -o --print-modified [OUT] Prints watched modifications after Control-D is pressed" << '\n';
	std::cout << "                            (to OUT file if given)." << '\n';
	std::cout << "  --skip-temporary          Don't show temporary files in watched modifications." << '\n';
	std::cout << "  --show-detail             Show more info (-l and -o outputs)." << '\n';
	std::cout << "Output codes:" << '\n';
	std::cout << "  Events (-l output):     State (-o output):" << '\n';
	std::cout << "    C: create               C: created" << '\n';
	std::cout << "    D: delete               D: deleted" << '\n';
	std::cout << "    M: modify               M: modified" << '\n';
	std::cout << "    A: attribute change     T: temporary" << '\n';
	std::cout << "    f: rename from          ?: inconsistent" << '\n';
	std::cout << "    t: rename to" << '\n';
}
void evalBoolArg(int& nArgC, char**& aArgV, const std::string& sOption1, const std::string& sOption2, std::string& sMatch, bool& bVar)
{
	sMatch.clear();
	if (aArgV[1] == nullptr) {
		return;
	}
	const bool bIsOption1 = (sOption1 == std::string(aArgV[1]));
	if (bIsOption1 || ((!sOption2.empty()) && (sOption2 == std::string(aArgV[1])))) {
		sMatch = (bIsOption1 ? sOption1 : sOption2);
		bVar = true;
		--nArgC;
		++aArgV;
	}
}
bool evalIntArg(int& nArgC, char**& aArgV, const std::string& sOption1, const std::string& sOption2, std::string& sMatch, int32_t& nVar, int32_t nMin)
{
	sMatch.clear();
	if (aArgV[1] == nullptr) {
		return true;
	}
	const bool bIsOption1 = (sOption1 == std::string(aArgV[1]));
	if (bIsOption1 || ((!sOption2.empty()) && (sOption2 == std::string(aArgV[1])))) {
		sMatch = (bIsOption1 ? sOption1 : sOption2);
		--nArgC;
		++aArgV;
		if (nArgC == 1) {
			std::cerr << "Error: " << sMatch << " missing argument" << '\n';
			return false; //----------------------------------------------------
		}
		try {
			const double fValue = Glib::Ascii::strtod(aArgV[1]);
			if (fValue < nMin) {
				nVar = nMin;
			} else {
				nVar = std::min(std::ceil(fValue), static_cast<double>(std::numeric_limits<int32_t>::max()));
			}
		} catch (const std::runtime_error& oErr) {
			std::cerr << "Error: " << sMatch << " " << oErr.what() << '\n';
			return false; //----------------------------------------------------
		}
		--nArgC;
		++aArgV;
	}
	return true;
}
bool evalPathNameArg(int& nArgC, char**& aArgV, bool bName, const std::string& sOption1, const std::string& sOption2
					, bool bPathMandatory, std::string& sMatch, std::string& sPath)
{
	sMatch.clear();
	if (aArgV[1] == nullptr) {
		return true;
	}
	const bool bIsOption1 = (sOption1 == std::string(aArgV[1]));
	if (bIsOption1 || ((!sOption2.empty()) && (sOption2 == std::string(aArgV[1])))) {
		sMatch = (bIsOption1 ? sOption1 : sOption2);
		--nArgC;
		++aArgV;
		if (nArgC == 1) {
			if (bPathMandatory) {
				std::cerr << "Error: " << sMatch << " missing argument" << '\n';
				return false; //------------------------------------------------
			}
			sPath = "";
			return true; //-----------------------------------------------------
		}
		sPath = aArgV[1];
		if (!bPathMandatory) {
			assert(! sPath.empty());
			if (sPath[0] == '-') {
				// it is the next option, not a file
				sPath = "";
				return true; //-------------------------------------------------
			}
		}
		if (bName) {
			if (sPath.find('/') != std::string::npos) {
				std::cerr << "Error: " << sMatch << " argument can't contain '/'" << '\n';
				return false; //--------------------------------------------
			}
		}
		--nArgC;
		++aArgV;
	}
	return true;
}

void printZones(std::ostream& oOut, const FofiModel& oFofiModel)
{
	const auto& aZones = oFofiModel.getDirectoryZones();
	oOut << "Directory zones:" << '\n';
	for (const auto& oDZ : aZones) {
		oOut << "  Directory zone path: " << Glib::filename_to_utf8(oDZ.m_sPath) << '\n';
		oOut << "            max depth: " << oDZ.m_nMaxDepth << '\n';
		const auto& aPinnedFiles = oDZ.m_aPinnedFiles;
		for (const auto& sPinnedFile : aPinnedFiles) {
			oOut << "          pinned file: " << Glib::filename_to_utf8(sPinnedFile) << '\n';
		}
		const auto& aPinnedSubDirs = oDZ.m_aPinnedSubDirs;
		for (const auto& sPinnedSubDir : aPinnedSubDirs) {
			oOut << "       pinned sub dir: " << Glib::filename_to_utf8(sPinnedSubDir) << '\n';
		}
		const auto& aFileIncludeFilters = oDZ.m_aFileIncludeFilters;
		for (const auto& oFileIncludeFilter : aFileIncludeFilters) {
			const bool bIsRegexp = (oFileIncludeFilter.m_eFilterType == FofiModel::FILTER_REGEX);
			const std::string sRE = (bIsRegexp ? "re" : "");
			oOut << "  file include filter: " << sRE << "'" << oFileIncludeFilter.m_sFilter << "'" << '\n';
		}
		const auto& aFileExcludeFilters = oDZ.m_aFileExcludeFilters;
		for (const auto& oFileExcludeFilter : aFileExcludeFilters) {
			const bool bIsRegexp = (oFileExcludeFilter.m_eFilterType == FofiModel::FILTER_REGEX);
			const std::string sRE = (bIsRegexp ? "re" : "");
			oOut << "  file exclude filter: " << sRE << "'" << oFileExcludeFilter.m_sFilter << "'" << '\n';
		}
		const auto& aSubDirIncludeFilters = oDZ.m_aSubDirIncludeFilters;
		for (const auto& oSubDirIncludeFilter : aSubDirIncludeFilters) {
			const bool bIsRegexp = (oSubDirIncludeFilter.m_eFilterType == FofiModel::FILTER_REGEX);
			const std::string sRE = (bIsRegexp ? "re" : "");
			oOut << "   dir include filter: " << sRE << "'" << oSubDirIncludeFilter.m_sFilter << "'" << '\n';
		}
		const auto& aSubDirExcludeFilters = oDZ.m_aSubDirExcludeFilters;
		for (const auto& oSubDirExcludeFilter : aSubDirExcludeFilters) {
			const bool bIsRegexp = (oSubDirExcludeFilter.m_eFilterType == FofiModel::FILTER_REGEX);
			const std::string sRE = (bIsRegexp ? "re" : "");
			oOut << "   dir exclude filter: " << sRE << "'" << oSubDirExcludeFilter.m_sFilter << "'" << '\n';
		}
	}
	oOut << "Total directory zones: " << aZones.size() << '\n';
}
void printToWatchDirs(std::ostream& oOut, const FofiModel& oFofiModel, bool bDontWatch)
{
	const auto& aToWatchDirs = oFofiModel.getToWatchDirectories();
	oOut << "Watched directories:" << '\n';
	const auto& oDZs = oFofiModel.getDirectoryZones();
	#ifdef STMF_DEBUG
	int32_t nC = 0;
	#endif //STMF_DEBUG
	for (const auto& oTWD : aToWatchDirs) {
		oOut << "  Path to watch     : " << oTWD.m_sPathName << '\n';
		#ifdef STMF_DEBUG
		oOut << "                idx : " << nC << '\n';
		++nC;
		#endif //STMF_DEBUG
		const int32_t nDZ = oTWD.getOwnerDirectoryZone();
		if (nDZ >= 0) {
			oOut << "                zone: " << oDZs[nDZ].m_sPath << '\n';
			oOut << "               depth: " << oTWD.m_nDepth << '\n';
		} else {
			oOut << "                zone: none (gap filler)" << '\n';
		}
		const auto& aPinnedFiles = oTWD.m_aPinnedFiles;
		for (const auto& sPinnedFile : aPinnedFiles) {
			oOut << "         pinned file: " << Glib::filename_to_utf8(sPinnedFile) << '\n';
		}
		const auto& aPinnedSubDirs = oTWD.m_aPinnedSubDirs;
		for (const auto& sPinnedSubDirs : aPinnedSubDirs) {
			oOut << "      pinned sub dir: " << Glib::filename_to_utf8(sPinnedSubDirs) << '\n';
		}
		if (bDontWatch) {
			oOut << "              exists: " << (oTWD.exists() ? "yes" : "no") << '\n';
		} else {
			if (oTWD.exists()) {
				oOut << "             watched: " << (oTWD.isWatched() ? "yes" : "no") << '\n';	
			} else {
				oOut << "              exists: no" << '\n';
			}
		}
	}
}
const char* getActionCodeString(INotifierSource::FOFI_ACTION eA)
{
	switch (eA) {
		case INotifierSource::FOFI_ACTION_CREATE:       return "C"; break;
		case INotifierSource::FOFI_ACTION_DELETE:       return "D"; break;
		case INotifierSource::FOFI_ACTION_MODIFY:       return "M"; break;
		case INotifierSource::FOFI_ACTION_ATTRIB:       return "A"; break;
		case INotifierSource::FOFI_ACTION_RENAME_FROM:  return "f"; break;
		case INotifierSource::FOFI_ACTION_RENAME_TO:    return "t"; break;
		default: return "???";
	}
}
const char* getActionString(INotifierSource::FOFI_ACTION eA)
{
	switch (eA) {
		case INotifierSource::FOFI_ACTION_CREATE:       return "Create     "; break;
		case INotifierSource::FOFI_ACTION_DELETE:       return "Delete     "; break;
		case INotifierSource::FOFI_ACTION_MODIFY:       return "Modify     "; break;
		case INotifierSource::FOFI_ACTION_ATTRIB:       return "Attribute  "; break;
		case INotifierSource::FOFI_ACTION_RENAME_FROM:  return "Rename from"; break;
		case INotifierSource::FOFI_ACTION_RENAME_TO:    return "Rename to  "; break;
		default: return "???";
	}
}
const char* getResultTypeCodeString(FofiModel::RESULT_TYPE eT)
{
	switch (eT) {
		case FofiModel::RESULT_CREATED: return "C"; break;
		case FofiModel::RESULT_DELETED: return "D"; break;
		case FofiModel::RESULT_MODIFIED: return "M"; break;
		case FofiModel::RESULT_TEMPORARY: return "T"; break;
		default: return "?";
	}
}
const char* getResultTypeString(FofiModel::RESULT_TYPE eT)
{
	switch (eT) {
		case FofiModel::RESULT_CREATED:   return "Created  "; break;
		case FofiModel::RESULT_DELETED:   return "Deleted  "; break;
		case FofiModel::RESULT_MODIFIED:  return "Modified "; break;
		case FofiModel::RESULT_TEMPORARY: return "Temporary"; break;
		default:                          return "None     ";
	}
}
void printCodeResult(std::ostream& oOut, const FofiModel::WatchedResult& oResult)
{
	const char* p0Status = getResultTypeCodeString(oResult.m_eResultType);
	oOut << p0Status << (oResult.m_bInconsistent ? "?" : " ") << Util::getPathFromDirAndName(oResult.m_sPath, oResult.m_sName) 
				<< ((oResult.m_bIsDir && oResult.m_sPath != "/") ? "/" : "") << '\n';
}
void printAction(std::ostream& oOut, const FofiModel::ActionData& oAction, int64_t nDurationUsec)
{
	const char* p0Action = getActionString(oAction.m_eAction);
	oOut << "        " << Util::getTimeString(oAction.m_nTimeUsec, nDurationUsec) << " " << p0Action;
	const bool bIsRenameFrom = (oAction.m_eAction == INotifierSource::FOFI_ACTION_RENAME_FROM);
	if (bIsRenameFrom || (oAction.m_eAction == INotifierSource::FOFI_ACTION_RENAME_TO)) {
		oOut << "  ";
		if (bIsRenameFrom) {
			oOut << "(to ";
		} else {
			oOut << "(from ";
		}
		oOut << (oAction.m_sOtherPath.empty() ? "unknown" : oAction.m_sOtherPath) << ")";
	}
	oOut << '\n';
}
void printDetailResult(std::ostream& oOut, const FofiModel::WatchedResult& oResult, int64_t nDurationUsec)
{
	oOut << "File: " << Util::getPathFromDirAndName(oResult.m_sPath, oResult.m_sName) 
				<< ((oResult.m_bIsDir && oResult.m_sPath != "/") ? "/" : "") << '\n';
	const char* p0Status = getResultTypeString(oResult.m_eResultType);
	oOut << "    Status: " << p0Status << (oResult.m_bInconsistent ? " (inconsistent)" : "") << '\n';
	oOut << "      Actions:" << '\n';
	const auto& aActions = oResult.m_aActions;
	for (const auto& oAction : aActions) {
		printAction(oOut, oAction, nDurationUsec);
	}
}
void printLiveAction(std::ostream& oOut, const FofiModel::WatchedResult& oResult)
{
	assert(! oResult.m_aActions.empty());
	const auto& oAction = oResult.m_aActions.back();
	const char* p0Action = getActionCodeString(oAction.m_eAction);
	oOut << p0Action;
	oOut << " " << Util::getPathFromDirAndName(oResult.m_sPath, oResult.m_sName) 
				<< ((oResult.m_bIsDir && oResult.m_sPath != "/") ? "/" : "");
	const bool bIsRenameFrom = (oAction.m_eAction == INotifierSource::FOFI_ACTION_RENAME_FROM);
	if (bIsRenameFrom || (oAction.m_eAction == INotifierSource::FOFI_ACTION_RENAME_TO)) {
		oOut << "  ";
		if (bIsRenameFrom) {
			oOut << "(T ";
		} else {
			oOut << "(F ";
		}
		oOut << (oAction.m_sOtherPath.empty() ? "unknown" : oAction.m_sOtherPath) << ")";
	}
	oOut << '\n';
}
void printDetailLiveAction(std::ostream& oOut, const FofiModel::WatchedResult& oResult)
{
	assert(! oResult.m_aActions.empty());
	const auto& oAction = oResult.m_aActions.back();
	oOut << Util::getTimeString(oAction.m_nTimeUsec, 1000 * 1000000);
	const char* p0Action = getActionString(oAction.m_eAction);
	oOut << " " << p0Action;
	oOut << " " << Util::getPathFromDirAndName(oResult.m_sPath, oResult.m_sName) 
				<< ((oResult.m_bIsDir && oResult.m_sPath != "/") ? "/" : "");
	const bool bIsRenameFrom = (oAction.m_eAction == INotifierSource::FOFI_ACTION_RENAME_FROM);
	if (bIsRenameFrom || (oAction.m_eAction == INotifierSource::FOFI_ACTION_RENAME_TO)) {
		oOut << "  ";
		if (bIsRenameFrom) {
			oOut << "(to ";
		} else {
			oOut << "(from ";
		}
		oOut << (oAction.m_sOtherPath.empty() ? "unknown" : oAction.m_sOtherPath) << ")";
	}
	oOut << '\n';
}
template<typename P>
void printOutput(const std::string& sOutFile, std::vector<std::string>& aOutFiles, P oP)
{
	if (! sOutFile.empty()) {
		const bool bAppend = Util::isInVector(aOutFiles, sOutFile);
		std::ofstream oOut(sOutFile, (bAppend ? std::ios::app : std::ios::out));
		if (! oOut) {
			std::cerr << "Error: opening " << sOutFile << '\n';
			return; //----------------------------------------------------------
		}
		oP(oOut);
		aOutFiles.push_back(sOutFile);
	} else {
		oP(std::cout);
	}
}

int fofimonMain(int nArgC, char** aArgV)
{
	const auto oEUID = ::geteuid();
	const bool bIsRoot = (static_cast<int32_t>(oEUID) == 0);

	int32_t nMaxToWatchDirectories = s_nDefaultMaxToWatchDirectories;
	int32_t nMaxResultPaths = s_nDefaultMaxResultPaths;
	bool bDontWatch = false;
	bool bSkipTemporary = false;
	bool bShowDetail = false;
	bool bPrintZones = false;
	bool bPrintToWatchDirs = false;
	bool bPrintToWatchAfterDirs = false;
	bool bPrintLiveActions = false;
	bool bPrintModified = false;
	std::string sOutFileZones;
	std::string sOutFileToWatchDirs;
	std::string sOutFileToWatchAfterDirs;
	std::string sOutFileLiveActions;
	std::string sOutFileModified;

	const auto oNoZoneError = [](const std::string& sMatch)
	{
		std::cerr << "Error: --add-zone must be defined before " << sMatch << '\n';
	};

	std::vector<std::string> aToWatchFiles;
	std::vector<FofiModel::DirectoryZone> aDZs;
	FofiModel::DirectoryZone oDZ;
	bool bHelp = false;
	bool bVersion = false;
	std::string sMatch;
	int32_t nRes = 0;
	bool bRes = false;
	std::string sRes;
	bool bOk;
	char* p0ArgVZeroSave = ((nArgC >= 1) ? aArgV[0] : nullptr);
	while (nArgC >= 2) {
		auto nOldArgC = nArgC;
		evalBoolArg(nArgC, aArgV, "--help", "-h", sMatch, bHelp);
		if (bHelp) {
			printUsage();
			return EXIT_SUCCESS; //---------------------------------------------
		}
		evalBoolArg(nArgC, aArgV, "--version", "-v", sMatch, bVersion);
		if (bVersion) {
			printVersion();
			return EXIT_SUCCESS; //---------------------------------------------
		}
		evalBoolArg(nArgC, aArgV, "--dont-watch", "", sMatch, bDontWatch);
		evalBoolArg(nArgC, aArgV, "--skip-temporary", "", sMatch, bSkipTemporary);
		evalBoolArg(nArgC, aArgV, "--show-detail", "", sMatch, bShowDetail);
		bOk = evalPathNameArg(nArgC, aArgV, false, "--print-zones", "", false, sMatch, sOutFileZones);
		if (!bOk) {
			return EXIT_FAILURE; //---------------------------------------------
		}
		if (! sMatch.empty()) {
			bPrintZones = true;
		}
		bOk = evalPathNameArg(nArgC, aArgV, false, "--print-watched", "", false, sMatch, sOutFileToWatchDirs);
		if (!bOk) {
			return EXIT_FAILURE; //---------------------------------------------
		}
		if (! sMatch.empty()) {
			bPrintToWatchDirs = true;
		}
		bOk = evalPathNameArg(nArgC, aArgV, false, "--print-watched-after", "", false, sMatch, sOutFileToWatchAfterDirs);
		if (!bOk) {
			return EXIT_FAILURE; //---------------------------------------------
		}
		if (! sMatch.empty()) {
			bPrintToWatchAfterDirs = true;
		}
		bOk = evalPathNameArg(nArgC, aArgV, false, "--live-actions", "-l", false, sMatch, sOutFileLiveActions);
		if (!bOk) {
			return EXIT_FAILURE; //---------------------------------------------
		}
		if (! sMatch.empty()) {
			bPrintLiveActions = true;
		}
		bOk = evalPathNameArg(nArgC, aArgV, false, "--print-modified", "-o", false, sMatch, sOutFileModified);
		if (!bOk) {
			return EXIT_FAILURE; //---------------------------------------------
		}
		if (! sMatch.empty()) {
			bPrintModified = true;
		}
		//
		bOk = evalIntArg(nArgC, aArgV, "--max-watched-dirs", "", sMatch, nMaxToWatchDirectories, 1);
		if (!bOk) {
			return EXIT_FAILURE; //---------------------------------------------
		}
		//
		bOk = evalIntArg(nArgC, aArgV, "--max-results", "", sMatch, nMaxResultPaths, 1);
		if (!bOk) {
			return EXIT_FAILURE; //---------------------------------------------
		}
		//
		bOk = evalPathNameArg(nArgC, aArgV, false, "--add-file", "-f", true, sMatch, sRes);
		if (!bOk) {
			return EXIT_FAILURE; //---------------------------------------------
		}
		if (! sMatch.empty()) {
			assert(! sRes.empty());
			aToWatchFiles.push_back(sRes);
		}
		//
		bOk = evalPathNameArg(nArgC, aArgV, false, "--add-zone", "-z", true, sMatch, sRes);
		if (!bOk) {
			return EXIT_FAILURE; //---------------------------------------------
		}
		if (! sMatch.empty()) {
			assert(! sRes.empty());
			// new directory zone
			if (! oDZ.m_sPath.empty()) {
				// add the old one
				aDZs.push_back(std::move(oDZ));
				// sets defaults
				oDZ = FofiModel::DirectoryZone{};
			}
			oDZ.m_sPath = sRes;
		}
		//
		bOk = evalIntArg(nArgC, aArgV, "--max-depth", "-m", sMatch, nRes, -1);
		if (!bOk) {
			return EXIT_FAILURE; //---------------------------------------------
		}
		if (! sMatch.empty()) {
			if (oDZ.m_sPath.empty()) {
				oNoZoneError(sMatch);
				return EXIT_FAILURE; //-----------------------------------------
			}
			oDZ.m_nMaxDepth = ((nRes >= 0) ? nRes : std::max(9999, PATH_MAX));
		}
		//
		bOk = evalPathNameArg(nArgC, aArgV, true, "--pinned-file", "", true, sMatch, sRes);
		if (!bOk) {
			return EXIT_FAILURE; //---------------------------------------------
		}
		if (! sMatch.empty()) {
			if (oDZ.m_sPath.empty()) {
				oNoZoneError(sMatch);
				return EXIT_FAILURE; //-----------------------------------------
			}
			oDZ.m_aPinnedFiles.push_back(sRes);
		}
		//
		bOk = evalPathNameArg(nArgC, aArgV, true, "--pinned-dir", "", true, sMatch, sRes);
		if (!bOk) {
			return EXIT_FAILURE; //---------------------------------------------
		}
		if (! sMatch.empty()) {
			if (oDZ.m_sPath.empty()) {
				oNoZoneError(sMatch);
				return EXIT_FAILURE; //-----------------------------------------
			}
			oDZ.m_aPinnedSubDirs.push_back(sRes);
		}
		//
		bOk = evalPathNameArg(nArgC, aArgV, false, "--include-files", "", true, sMatch, sRes);
		if (!bOk) {
			return EXIT_FAILURE; //---------------------------------------------
		}
		if (! sMatch.empty()) {
			if (oDZ.m_sPath.empty()) {
				oNoZoneError(sMatch);
				return EXIT_FAILURE; //-----------------------------------------
			}
			FofiModel::Filter oF;
			oF.m_sFilter = sRes;
			oF.m_eFilterType = FofiModel::FILTER_REGEX;
			oDZ.m_aFileIncludeFilters.push_back(std::move(oF));
		}
		bOk = evalPathNameArg(nArgC, aArgV, false, "--include-dirs", "", true, sMatch, sRes);
		if (!bOk) {
			return EXIT_FAILURE; //---------------------------------------------
		}
		if (! sMatch.empty()) {
			if (oDZ.m_sPath.empty()) {
				oNoZoneError(sMatch);
				return EXIT_FAILURE; //-----------------------------------------
			}
			FofiModel::Filter oF;
			oF.m_sFilter = sRes;
			oF.m_eFilterType = FofiModel::FILTER_REGEX;
			oDZ.m_aSubDirIncludeFilters.push_back(std::move(oF));
		}
		bOk = evalPathNameArg(nArgC, aArgV, false, "--exclude-files", "", true, sMatch, sRes);
		if (!bOk) {
			return EXIT_FAILURE; //---------------------------------------------
		}
		if (! sMatch.empty()) {
			if (oDZ.m_sPath.empty()) {
				oNoZoneError(sMatch);
				return EXIT_FAILURE; //-----------------------------------------
			}
			FofiModel::Filter oF;
			oF.m_sFilter = sRes;
			oF.m_eFilterType = FofiModel::FILTER_REGEX;
			oDZ.m_aFileExcludeFilters.push_back(std::move(oF));
		}
		bOk = evalPathNameArg(nArgC, aArgV, false, "--exclude-dirs", "", true, sMatch, sRes);
		if (!bOk) {
			return EXIT_FAILURE; //---------------------------------------------
		}
		if (! sMatch.empty()) {
			if (oDZ.m_sPath.empty()) {
				oNoZoneError(sMatch);
				return EXIT_FAILURE; //-----------------------------------------
			}
			FofiModel::Filter oF;
			oF.m_sFilter = sRes;
			oF.m_eFilterType = FofiModel::FILTER_REGEX;
			oDZ.m_aSubDirExcludeFilters.push_back(std::move(oF));
		}
		bOk = evalPathNameArg(nArgC, aArgV, true, "--exclude-file", "", true, sMatch, sRes);
		if (!bOk) {
			return EXIT_FAILURE; //---------------------------------------------
		}
		if (! sMatch.empty()) {
			if (oDZ.m_sPath.empty()) {
				oNoZoneError(sMatch);
				return EXIT_FAILURE; //-----------------------------------------
			}
			FofiModel::Filter oF;
			oF.m_sFilter = sRes;
			oDZ.m_aFileExcludeFilters.push_back(std::move(oF));
		}
		bOk = evalPathNameArg(nArgC, aArgV, true, "--exclude-dir", "", true, sMatch, sRes);
		if (!bOk) {
			return EXIT_FAILURE; //---------------------------------------------
		}
		if (! sMatch.empty()) {
			if (oDZ.m_sPath.empty()) {
				oNoZoneError(sMatch);
				return EXIT_FAILURE; //-----------------------------------------
			}
			FofiModel::Filter oF;
			oF.m_sFilter = sRes;
			oDZ.m_aSubDirExcludeFilters.push_back(std::move(oF));
		}
		evalBoolArg(nArgC, aArgV, "--exclude-all", "", sMatch, bRes);
		if (! sMatch.empty()) {
			if (oDZ.m_sPath.empty()) {
				oNoZoneError(sMatch);
				return EXIT_FAILURE; //-----------------------------------------
			}
			FofiModel::Filter oF;
			oF.m_sFilter = ".*";
			oDZ.m_aFileExcludeFilters.push_back(oF);
			oDZ.m_aSubDirExcludeFilters.push_back(std::move(oF));
		}
		//
		if (nOldArgC == nArgC) {
			std::cerr << "Unknown argument: " << ((aArgV[1] == nullptr) ? "(null)" : std::string(aArgV[1])) << '\n';
			return EXIT_FAILURE; //---------------------------------------------
		}
		aArgV[0] = p0ArgVZeroSave;
	}
	if (! oDZ.m_sPath.empty()) {
		// add last zone
		aDZs.push_back(std::move(oDZ));
	}

	Glib::RefPtr<Glib::MainLoop> refML = Glib::MainLoop::create();

	const int32_t nReserveWatchedDirs = INotifierSource::getSystemMaxUserWatches();

	FofiModel oFofiModel(std::make_unique<INotifierSource>(nReserveWatchedDirs), nMaxToWatchDirectories, nMaxResultPaths, bIsRoot);

	for (auto& sFile : aToWatchFiles) {
		const auto sRet = oFofiModel.addToWatchFile(std::move(sFile));
		if (! sRet.empty()) {
			std::cerr << sRet << '\n';
			return EXIT_FAILURE; //---------------------------------------------
		}
	}
	bool bMightTakeLong = false;
	for (auto& oDZ : aDZs) {
		if (oDZ.m_nMaxDepth > 1) {
			bMightTakeLong = true;
		}
		const auto sRet = oFofiModel.addDirectoryZone(std::move(oDZ));
		if (! sRet.empty()) {
			std::cerr << sRet << '\n';
			return EXIT_FAILURE; //---------------------------------------------
		}
	}

	//
	if (aToWatchFiles.empty() && oFofiModel.getDirectoryZones().empty()) {
		std::cout << "Warning: no directory zones or files defined ..." << '\n';
		std::cout << "         creating default zone with current working directory" << '\n';
		std::cout << "         as base path and max depth 0 (no recursion)" << '\n';
		char aAbs[PATH_MAX + 1];
		auto p0Abs = ::getcwd(aAbs, PATH_MAX);
		if (p0Abs == nullptr) {
			std::cerr << ::strerror(errno) << '\n';
			return EXIT_FAILURE; //---------------------------------------------
		}
		FofiModel::DirectoryZone oDZ;
		oDZ.m_sPath = p0Abs;
		const auto sRet = oFofiModel.addDirectoryZone(std::move(oDZ));
		if (! sRet.empty()) {
			std::cerr << sRet << '\n';
			return EXIT_FAILURE; //---------------------------------------------
		}
	}
	std::vector<std::string> aOutFiles;
	const auto& oPrintZones = [&]()
	{
		if (bPrintZones) {
			printOutput(sOutFileZones, aOutFiles, [&](std::ostream& oOut)
				{
					printZones(oOut, oFofiModel);
				});
		}
	};
	const auto& oPrintTotWatchedDirs = [&](bool bInitial)
	{
		const auto& aTWDs = oFofiModel.getToWatchDirectories();
		const int32_t nTotTWDs = std::accumulate(aTWDs.begin(), aTWDs.end(), 0
								, [](int32_t nCurTot, const FofiModel::ToWatchDir& oTWD)
			{
				return nCurTot + (oTWD.isWatched() ? 1 : 0);
			});
		std::cout << "Number of " << (bInitial ? "initially " : "") << "watched directories: " << nTotTWDs << '\n';
	};

	if (bDontWatch) {
		oPrintZones();
		if (bPrintToWatchDirs || !bPrintZones) {
			if (bMightTakeLong) {
				std::cout << "This may take a while ..." << '\n';
			}
			const auto sRet = oFofiModel.calcToWatchDirectories();
			if (! sRet.empty()) {
				std::cerr << sRet << '\n';
				return EXIT_FAILURE; //-----------------------------------------
			}
			if (bPrintToWatchDirs) {
				printOutput(sOutFileToWatchDirs, aOutFiles, [&](std::ostream& oOut)
					{
						printToWatchDirs(oOut, oFofiModel, true);
					});
			}
			const auto& aTWDs = oFofiModel.getToWatchDirectories();
			const int32_t nTotTWDs = std::accumulate(aTWDs.begin(), aTWDs.end(), 0
									, [](int32_t nCurTot, const FofiModel::ToWatchDir& oTWD)
				{
					return nCurTot + (oTWD.exists() ? 1 : 0);
				});
			std::cout << "Number of potentially watched directories: " << nTotTWDs << '\n';
		}
		return EXIT_SUCCESS; //-------------------------------------------------
	}
	if (bMightTakeLong) {
		std::cout << "Setting up initial watches: this may take a while ..." << '\n';
	}

	if ((!bPrintLiveActions) && (!bPrintModified)) {
		bPrintModified = true;
	}

	std::string sFatalError;
	oFofiModel.m_oAbortSignal.connect([&](const std::string& sError)
	{
		sFatalError = sError;
		refML->quit();
	});
	if (bPrintLiveActions) {
		oFofiModel.m_oWatchedResultActionSignal.connect([&](const FofiModel::WatchedResult& oWR)
		{
			printOutput(sOutFileLiveActions, aOutFiles, [&](std::ostream& oOut)
				{
					if (bShowDetail)  {
						printDetailLiveAction(oOut, oWR);
					} else {
						printLiveAction(oOut, oWR);
					}
				});
		});
	}
	oPrintZones();

	const auto sRet = oFofiModel.start();
	if (! sRet.empty()) {
		std::cerr << sRet << '\n';
		return EXIT_FAILURE; //-------------------------------------------------
	}

	if (bPrintToWatchDirs) {
		printOutput(sOutFileToWatchDirs, aOutFiles, [&](std::ostream& oOut)
			{
				printToWatchDirs(oOut, oFofiModel, false);
			});
	}

	oPrintTotWatchedDirs(true);
	std::cout << "Press 'Control-D' to stop watching ..." << '\n';

	Glib::RefPtr<Glib::IOChannel> refStdIn = Glib::IOChannel::create_from_fd(0 /*stdin*/);
	Glib::signal_io().connect([&](Glib::IOCondition oIOCondition) -> bool
	{
		const bool bContinue = true;
		if ((oIOCondition & Glib::IO_HUP) != 0) {
			refML->quit();
			return bContinue;
		}
		if ((oIOCondition & Glib::IO_IN) == 0) {
			return bContinue;
		}
		const auto c = getchar();
		if (c == EOF) {
			refML->quit();
		}
		return bContinue;
	}, refStdIn, Glib::IO_IN | Glib::IO_HUP);

	refML->run();

	oFofiModel.stop();

	const int64_t nDuration = oFofiModel.getDuration();
	std::cout << "Total time (seconds): " << Util::getTimeString(nDuration, nDuration) << '\n';

	if (oFofiModel.hasInconsistencies()) {
		std::cout << "Warning! Inconsistencies where detected. The results might not be accurate." << '\n';
	}
	if (oFofiModel.hasQueueOverflown()) {
		std::cout << "Warning! INotify event buffer did overflow." << '\n';
	}

	const bool bAborted = ! sFatalError.empty();
	if (bAborted) {
		std::cerr << "Aborted! " << sFatalError << '\n';
	}

	if (bPrintToWatchAfterDirs) {
		printOutput(sOutFileToWatchAfterDirs, aOutFiles, [&](std::ostream& oOut)
			{
				printToWatchDirs(oOut, oFofiModel, false);
				if (bAborted) {
					oOut << "Aborted! " << sFatalError << '\n';
				}
			});
		oPrintTotWatchedDirs(false);
	}

	if (bPrintModified) {
		printOutput(sOutFileModified, aOutFiles, [&](std::ostream& oOut)
			{
				const auto& aResults = oFofiModel.getWatchedResults();
				for (const auto& oResult : aResults) {
					if (bSkipTemporary && (oResult.m_eResultType == FofiModel::RESULT_TEMPORARY)) {
						continue; // for ----
					}
					if (bShowDetail) {
						printDetailResult(oOut, oResult, nDuration);
					} else {
						printCodeResult(oOut, oResult);
					}
				}
				if (bAborted) {
					oOut << "Aborted! " << sFatalError << '\n';
				}
			});
	}
	return EXIT_SUCCESS;
}

} // namespace fofi

int main(int nArgC, char** aArgV)
{
	return fofi::fofimonMain(nArgC, aArgV);
}

