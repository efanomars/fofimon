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
 * File:   printout.cc
 */

#include "printout.h"

#include "util.h"

#include "inotifiersource.h"  // for INotifierSource, INotifierSource::FO...

#include "nlohmann/json.hpp"

#include <glibmm.h>

#include <deque>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <cassert>
//#include <iostream>

namespace fofi
{

using nlohmann::json;

void printZone(std::ostream& oOut, const FofiModel::DirectoryZone& oDZ) noexcept
{
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
void printZones(std::ostream& oOut, const FofiModel& oFofiModel) noexcept
{
	const auto& aZones = oFofiModel.getDirectoryZones();
	oOut << "Directory zones:" << '\n';
	for (const auto& oDZ : aZones) {
		printZone(oOut, oDZ);
	}
	oOut << "Total directory zones: " << aZones.size() << '\n';
}
void printZoneJSon(std::ostream& oOut, const FofiModel::DirectoryZone& oDZ) noexcept
{
	const char* const p0PinnedFiles = "Pinned files";
	const char* const p0PinnedSubdirs = "Pinned subdirs";
	const char* const p0FileIncludeFilters = "File include filters";
	const char* const p0FileExcludeFilters = "File exclude filters";
	const char* const p0SubdirIncludeFilters = "Subdir include filters";
	const char* const p0SubdirExcludeFilters = "Subdir exclude filters";

	json oJZ;
	oJZ["Path"] = std::string{Glib::filename_to_utf8(oDZ.m_sPath)};
	oJZ["Max depth"] = oDZ.m_nMaxDepth;
	oJZ[p0PinnedFiles] = json::array();
	auto& oJPinnedFiles = oJZ[p0PinnedFiles];
	const auto& aPinnedFiles = oDZ.m_aPinnedFiles;
	for (const auto& sPinnedFile : aPinnedFiles) {
		oJPinnedFiles.push_back(std::string{Glib::filename_to_utf8(sPinnedFile)});
	}
	oJZ[p0PinnedSubdirs] = json::array();
	auto& oJPinnedSubdirs = oJZ[p0PinnedSubdirs];
	const auto& aPinnedSubDirs = oDZ.m_aPinnedSubDirs;
	for (const auto& sPinnedSubDir : aPinnedSubDirs) {
		oJPinnedSubdirs.push_back(std::string{Glib::filename_to_utf8(sPinnedSubDir)});
	}
	oJZ[p0FileIncludeFilters] = json::array();
	auto& oJFileIncludeFilters = oJZ[p0FileIncludeFilters];
	const auto& aFileIncludeFilters = oDZ.m_aFileIncludeFilters;
	for (const auto& oFileIncludeFilter : aFileIncludeFilters) {
		const bool bIsRegexp = (oFileIncludeFilter.m_eFilterType == FofiModel::FILTER_REGEX);
		json oJFIF;
		oJFIF["Filter"] = oFileIncludeFilter.m_sFilter;
		oJFIF["Regexp"] = bIsRegexp;
		oJFileIncludeFilters.push_back(std::move(oJFIF));
	}
	oJZ[p0FileExcludeFilters] = json::array();
	auto& oJFileExcludeFilters = oJZ[p0FileExcludeFilters];
	const auto& aFileExcludeFilters = oDZ.m_aFileExcludeFilters;
	for (const auto& oFileExcludeFilter : aFileExcludeFilters) {
		const bool bIsRegexp = (oFileExcludeFilter.m_eFilterType == FofiModel::FILTER_REGEX);
		json oJFEF;
		oJFEF["Filter"] = oFileExcludeFilter.m_sFilter;
		oJFEF["Regexp"] = bIsRegexp;
		oJFileExcludeFilters.push_back(std::move(oJFEF));
	}
	oJZ[p0SubdirIncludeFilters] = json::array();
	auto& oJSubdirIncludeFilters = oJZ[p0SubdirIncludeFilters];
	const auto& aSubDirIncludeFilters = oDZ.m_aSubDirIncludeFilters;
	for (const auto& oSubDirIncludeFilter : aSubDirIncludeFilters) {
		const bool bIsRegexp = (oSubDirIncludeFilter.m_eFilterType == FofiModel::FILTER_REGEX);
		json oJSIF;
		oJSIF["Filter"] = oSubDirIncludeFilter.m_sFilter;
		oJSIF["Regexp"] = bIsRegexp;
		oJSubdirIncludeFilters.push_back(std::move(oJSIF));
	}
	oJZ[p0SubdirExcludeFilters] = json::array();
	auto& oJSubdirExcludeFilters = oJZ[p0SubdirExcludeFilters];
	const auto& aSubDirExcludeFilters = oDZ.m_aSubDirExcludeFilters;
	for (const auto& oSubDirExcludeFilter : aSubDirExcludeFilters) {
		const bool bIsRegexp = (oSubDirExcludeFilter.m_eFilterType == FofiModel::FILTER_REGEX);
		json oJSEF;
		oJSEF["Filter"] = oSubDirExcludeFilter.m_sFilter;
		oJSEF["Regexp"] = bIsRegexp;
		oJSubdirExcludeFilters.push_back(std::move(oJSEF));
	}
	oOut << oJZ.dump(2) << '\n';
}
void printZonesJSon(std::ostream& oOut, const FofiModel& oFofiModel) noexcept
{
	oOut << "[" << '\n';
	bool bFirst = true;
	const auto& aZones = oFofiModel.getDirectoryZones();
	for (const auto& oDZ : aZones) {
		if (! bFirst) {
			bFirst = false;
			oOut << "," << '\n';
		}
		printZoneJSon(oOut, oDZ);
	}
	oOut << "]" << '\n';
}
void printToWatchDirs(std::ostream& oOut, const FofiModel& oFofiModel, bool bDontWatch) noexcept
{
	const auto& aToWatchDirs = oFofiModel.getToWatchDirectories();
	oOut << "Watched directories:" << '\n';
	const auto& oDZs = oFofiModel.getDirectoryZones();
	#ifdef STMM_TRACE_DEBUG
	int32_t nC = 0;
	#endif //STMM_TRACE_DEBUG
	for (const auto& oTWD : aToWatchDirs) {
		oOut << "  Path to watch     : " << oTWD.m_sPathName << '\n';
		#ifdef STMM_TRACE_DEBUG
		oOut << "                idx : " << nC << '\n';
		++nC;
		#endif //STMM_TRACE_DEBUG
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
void printToWatchDirsJSon(std::ostream& oOut, const FofiModel& oFofiModel, bool bDontWatch) noexcept
{
	const char * const p0PinnedFiles = "Pinned files";
	const char * const p0PinnedSubdirs = "Pinned subdirs";

	oOut << "[" << '\n';
	const auto& aToWatchDirs = oFofiModel.getToWatchDirectories();
	const auto& oDZs = oFofiModel.getDirectoryZones();
	int32_t nC = 0;
	for (const auto& oTWD : aToWatchDirs) {
		if (nC > 0) {
			oOut << "," << '\n';
		}
		json oJTWD;
		oJTWD["Path"] = std::string{Glib::filename_to_utf8(oTWD.m_sPathName)};
		#ifdef STMM_TRACE_DEBUG
		oJTWD["Idx"] = nC;
		#endif //STMM_TRACE_DEBUG
		++nC;
		const int32_t nDZ = oTWD.getOwnerDirectoryZone();
		if (nDZ >= 0) {
			oJTWD["Zone"] = std::string{Glib::filename_to_utf8(oDZs[nDZ].m_sPath)};
			oJTWD["Depth"] = oTWD.m_nDepth;
		} else {
			oJTWD["Zone"] = "";
		}
		oJTWD[p0PinnedFiles] = json::array();
		auto& oJPinnedFiles = oJTWD[p0PinnedFiles];
		const auto& aPinnedFiles = oTWD.m_aPinnedFiles;
		for (const auto& sPinnedFile : aPinnedFiles) {
			oJPinnedFiles.push_back(std::string{Glib::filename_to_utf8(sPinnedFile)});
		}
		oJTWD[p0PinnedSubdirs] = json::array();
		auto& oJPinnedSubdirs = oJTWD[p0PinnedSubdirs];
		const auto& aPinnedSubDirs = oTWD.m_aPinnedSubDirs;
		for (const auto& sPinnedSubDirs : aPinnedSubDirs) {
			oJPinnedSubdirs.push_back(std::string{Glib::filename_to_utf8(sPinnedSubDirs)});
		}
		oJTWD["exists"] = oTWD.exists();
		if (! bDontWatch) {
			oJTWD["watched"] = oTWD.isWatched();
		}
		oOut << oJTWD.dump(2) << '\n';
	}
	oOut << "]" << '\n';
}
const char* getActionCodeString(INotifierSource::FOFI_ACTION eA) noexcept
{
	switch (eA) {
		case INotifierSource::FOFI_ACTION_CREATE:       return "c"; break;
		case INotifierSource::FOFI_ACTION_DELETE:       return "d"; break;
		case INotifierSource::FOFI_ACTION_MODIFY:       return "m"; break;
		case INotifierSource::FOFI_ACTION_ATTRIB:       return "a"; break;
		case INotifierSource::FOFI_ACTION_RENAME_FROM:  return "f"; break;
		case INotifierSource::FOFI_ACTION_RENAME_TO:    return "t"; break;
		default: return "???";
	}
}
const char* getActionString(INotifierSource::FOFI_ACTION eA) noexcept
{
	switch (eA) {
		case INotifierSource::FOFI_ACTION_CREATE:       return "Create"; break;
		case INotifierSource::FOFI_ACTION_DELETE:       return "Delete"; break;
		case INotifierSource::FOFI_ACTION_MODIFY:       return "Modify"; break;
		case INotifierSource::FOFI_ACTION_ATTRIB:       return "Attribute"; break;
		case INotifierSource::FOFI_ACTION_RENAME_FROM:  return "Rename from"; break;
		case INotifierSource::FOFI_ACTION_RENAME_TO:    return "Rename to"; break;
		default: return "???";
	}
}
const char* getResultTypeCodeString(FofiModel::RESULT_TYPE eT) noexcept
{
	switch (eT) {
		case FofiModel::RESULT_CREATED: return "C"; break;
		case FofiModel::RESULT_DELETED: return "D"; break;
		case FofiModel::RESULT_MODIFIED: return "M"; break;
		case FofiModel::RESULT_TEMPORARY: return "T"; break;
		default: return "?";
	}
}
const char* getResultTypeString(FofiModel::RESULT_TYPE eT) noexcept
{
	switch (eT) {
		case FofiModel::RESULT_CREATED:   return "Created"; break;
		case FofiModel::RESULT_DELETED:   return "Deleted"; break;
		case FofiModel::RESULT_MODIFIED:  return "Modified"; break;
		case FofiModel::RESULT_TEMPORARY: return "Temporary"; break;
		default:                          return "None";
	}
}
void printAction(std::ostream& oOut, const FofiModel::ActionData& oAction, int64_t nDurationUsec) noexcept
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
	oOut << "File: " << Glib::filename_to_utf8(Util::getPathFromDirAndName(oResult.m_sPath, oResult.m_sName))
				<< ((oResult.m_bIsDir && (oResult.m_sPath != "/")) ? "/" : "") << '\n';
	const char* p0Status = getResultTypeString(oResult.m_eResultType);
	oOut << "    Status: " << p0Status << (oResult.m_bInconsistent ? " (inconsistent)" : "") << '\n';
	oOut << "      Actions:" << '\n';
	const auto& aActions = oResult.m_aActions;
	for (const auto& oAction : aActions) {
		printAction(oOut, oAction, nDurationUsec);
	}
}
void printCodeResult(std::ostream& oOut, const FofiModel::WatchedResult& oResult) noexcept
{
	const char* p0Status = getResultTypeCodeString(oResult.m_eResultType);
	oOut << p0Status << (oResult.m_bInconsistent ? "?" : " ")
				<< Glib::filename_to_utf8(Util::getPathFromDirAndName(oResult.m_sPath, oResult.m_sName))
				<< ((oResult.m_bIsDir && (oResult.m_sPath != "/")) ? "/" : "") << '\n';
}
void printResult(std::ostream& oOut, const FofiModel::WatchedResult& oResult, bool bDetail, int64_t nDurationUsec) noexcept
{
	if (bDetail) {
		printDetailResult(oOut, oResult, nDurationUsec);
	} else {
		printCodeResult(oOut, oResult);
	}
}
void printCodeResultJSon(std::ostream& oOut, const FofiModel::WatchedResult& oResult, json& oJRes) noexcept
{
	oJRes["Path"] = std::string{Glib::filename_to_utf8(Util::getPathFromDirAndName(oResult.m_sPath, oResult.m_sName))};
	oJRes["Dir"] = oResult.m_bIsDir;
	const char* p0Status = getResultTypeString(oResult.m_eResultType);
	oJRes["Status"] = p0Status;
	oJRes["Inconsistent"] = oResult.m_bInconsistent;
	oOut << oJRes.dump(2) << '\n';
}
void printDetailResultJSon(std::ostream& oOut, const FofiModel::WatchedResult& oResult, int64_t nDurationUsec, json& oJRes) noexcept
{
	printCodeResultJSon(oOut, oResult, oJRes);
	const char * const p0Actions = "Actions";
	oJRes[p0Actions] = json::array();
	auto& oJActions = oJRes[p0Actions];
	const auto& aActions = oResult.m_aActions;
	for (const auto& oAction : aActions) {
		json oJAction;
		const char* p0Action = getActionString(oAction.m_eAction);
		oJAction["Action"] = p0Action;
		oJAction["Time"] = Util::getTimeString(oAction.m_nTimeUsec, nDurationUsec);
		const bool bIsRenameFrom = (oAction.m_eAction == INotifierSource::FOFI_ACTION_RENAME_FROM);
		if (bIsRenameFrom || (oAction.m_eAction == INotifierSource::FOFI_ACTION_RENAME_TO)) {
			if (! oAction.m_sOtherPath.empty()) {
				oJAction[(bIsRenameFrom ? "Renamed to" : "Renamed from")] = std::string{Glib::filename_to_utf8(oAction.m_sOtherPath)};
			}
		}
		oJActions.push_back(oJAction);
	}
	oOut << oJRes.dump(2) << '\n';
}
void printResultJSon(std::ostream& oOut, const FofiModel::WatchedResult& oResult, bool bDetail, int64_t nDurationUsec) noexcept
{
	json oJRes;
	if (bDetail) {
		printDetailResultJSon(oOut, oResult, nDurationUsec, oJRes);
	} else {
		printCodeResultJSon(oOut, oResult, oJRes);
	}
}
void printCodeLiveAction(std::ostream& oOut, const FofiModel::WatchedResult& oResult) noexcept
{
	assert(! oResult.m_aActions.empty());
	const auto& oAction = oResult.m_aActions.back();
	const char* p0Action = getActionCodeString(oAction.m_eAction);
	oOut << p0Action;
	oOut << " " << Glib::filename_to_utf8(Util::getPathFromDirAndName(oResult.m_sPath, oResult.m_sName))
				<< ((oResult.m_bIsDir && oResult.m_sPath != "/") ? "/" : "");
	const bool bIsRenameFrom = (oAction.m_eAction == INotifierSource::FOFI_ACTION_RENAME_FROM);
	if (bIsRenameFrom || (oAction.m_eAction == INotifierSource::FOFI_ACTION_RENAME_TO)) {
		oOut << "  ";
		if (bIsRenameFrom) {
			oOut << "(T ";
		} else {
			oOut << "(F ";
		}
		oOut << (oAction.m_sOtherPath.empty() ? "unknown" : Glib::filename_to_utf8(oAction.m_sOtherPath)) << ")";
	}
	oOut << '\n';
}
void printDetailLiveAction(std::ostream& oOut, const FofiModel::WatchedResult& oResult) noexcept
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
void printLiveAction(std::ostream& oOut, const FofiModel::WatchedResult& oResult, bool bDetail) noexcept
{
	if (bDetail) {
		printDetailLiveAction(oOut, oResult);
	} else {
		printCodeLiveAction(oOut, oResult);
	}
}

} // namespace fofi

