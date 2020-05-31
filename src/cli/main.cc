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
 * File:   main.cc
 */

#include "config.h"

#include "evalargs.h"
#include "printout.h"

#include "../fofimodel.h"
#include "../util.h"
#include "../inotifiersource.h"

#include <glibmm.h>
#include <sigc++/sigc++.h>

#include <cassert>
#include <iostream>
#include <numeric>
#include <algorithm>
#include <deque>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include <limits.h>

namespace fofi
{

//using nlohmann::json;

static constexpr int32_t s_nDefaultMaxToWatchDirectories = std::numeric_limits<int32_t>::max() - 1;
static constexpr int32_t s_nDefaultMaxResultPaths = std::numeric_limits<int32_t>::max() - 1;

void printVersion() noexcept
{
	std::cout << Config::getVersionString() << '\n';
}
void printUsage() noexcept
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
	std::cout << "Output options (if OUT ends with '.json', json output is used):" << '\n';
	std::cout << "  --print-zones [OUT]       Prints directory zones (to OUT file if given)." << '\n';
	std::cout << "  --print-watched [OUT]     Prints initial to be watched directories (to OUT file if given)." << '\n';
	std::cout << "  -l --live-events [OUTL]   Prints single events as they happen" << '\n';
	std::cout << "                            (to OUTL file if given, no json)." << '\n';
	std::cout << "  -o --print-modified [OUT] Prints watched modifications after Control-D is pressed" << '\n';
	std::cout << "                            (to OUT file if given)." << '\n';
	std::cout << "  --skip-temporary          Don't show temporary files in watched modifications." << '\n';
	std::cout << "  --show-detail             Show more info (-l and -o outputs)." << '\n';
	std::cout << "Output codes:" << '\n';
	std::cout << "  Events (-l output):     State (-o output):" << '\n';
	std::cout << "    c: create               C: created" << '\n';
	std::cout << "    d: delete               D: deleted" << '\n';
	std::cout << "    m: modify               M: modified" << '\n';
	std::cout << "    a: attribute change     T: temporary" << '\n';
	std::cout << "    f: rename from          ?: inconsistent" << '\n';
	std::cout << "    t: rename to" << '\n';
}
struct OutputFile
{
	std::string m_sPathName;
	bool m_bCreated = false;
	bool m_bJSON = false;
};
bool isJSON(const std::string& sPathName) noexcept
{
	return (sPathName.size() > 5) && (sPathName.substr(sPathName.size() - 5) == ".json");
}
template<typename P>
void printOutput(OutputFile& oOutFile, P oP)
{
	if (! oOutFile.m_sPathName.empty()) {
		const bool bAppend = oOutFile.m_bCreated;
		std::ofstream oOut(oOutFile.m_sPathName, (bAppend ? std::ios::app : std::ios::out));
		if (! oOut) {
			std::cerr << "Error: opening " << oOutFile.m_sPathName << '\n';
			return; //----------------------------------------------------------
		}
		oP(oOut, oOutFile.m_bJSON);
		if (!bAppend) {
			oOutFile.m_bCreated = true;
		}
	} else {
		oP(std::cout, oOutFile.m_bJSON);
	}
}
void printNoZoneError(const std::string& sMatch) noexcept
{
	std::cerr << "Error: --add-zone must be defined before " << sMatch << '\n';
}

int fofimonMain(int nArgC, char** aArgV) noexcept
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

	std::vector<std::string> aToWatchFiles;
	std::vector<FofiModel::DirectoryZone> aDZs;
	FofiModel::DirectoryZone oDZ;
	bool bHelp = false;
	bool bVersion = false;
	std::string sMatch;
	int32_t nRes = 0;
	bool bRes = false;
	std::string sRes;
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
		bool bOk = evalPathNameArg(nArgC, aArgV, false, "--print-zones", "", false, sMatch, sOutFileZones);
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
				printNoZoneError(sMatch);
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
				printNoZoneError(sMatch);
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
				printNoZoneError(sMatch);
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
				printNoZoneError(sMatch);
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
				printNoZoneError(sMatch);
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
				printNoZoneError(sMatch);
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
				printNoZoneError(sMatch);
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
				printNoZoneError(sMatch);
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
				printNoZoneError(sMatch);
				return EXIT_FAILURE; //-----------------------------------------
			}
			FofiModel::Filter oF;
			oF.m_sFilter = sRes;
			oDZ.m_aSubDirExcludeFilters.push_back(std::move(oF));
		}
		evalBoolArg(nArgC, aArgV, "--exclude-all", "", sMatch, bRes);
		if (! sMatch.empty()) {
			if (oDZ.m_sPath.empty()) {
				printNoZoneError(sMatch);
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
	//
	OutputFile oZonesOF;
	oZonesOF.m_sPathName = sOutFileZones;
	oZonesOF.m_bJSON = isJSON(sOutFileZones);
	//
	OutputFile oTWDOF;
	oTWDOF.m_sPathName = sOutFileToWatchDirs;
	oTWDOF.m_bJSON = isJSON(sOutFileToWatchDirs);
	//
	OutputFile oLiveOF;
	oLiveOF.m_sPathName = sOutFileLiveActions;
	oLiveOF.m_bJSON = isJSON(sOutFileLiveActions);
	//
	OutputFile oTWDAfterOF;
	oTWDAfterOF.m_sPathName = sOutFileToWatchAfterDirs;
	oTWDAfterOF.m_bJSON = isJSON(sOutFileToWatchAfterDirs);
	//
	OutputFile oModifiedOF;
	oModifiedOF.m_sPathName = sOutFileModified;
	oModifiedOF.m_bJSON = isJSON(sOutFileModified);

	const auto& oPrintZones = [&]()
	{
		if (bPrintZones) {
			printOutput(oZonesOF, [&](std::ostream& oOut, bool bJSon)
				{
					if (bJSon) {
						printZonesJSon(oOut, oFofiModel);
					} else {
						printZones(oOut, oFofiModel);
					}
				});
		}
	};
	const auto& oPrintTotalWatchedDirs = [&](bool bInitial)
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
				printOutput(oTWDOF, [&](std::ostream& oOut, bool bJSON)
					{
						if (bJSON){
							printToWatchDirsJSon(oOut, oFofiModel, true);
						} else {
							printToWatchDirs(oOut, oFofiModel, true);
						}
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
			printOutput(oLiveOF, [&](std::ostream& oOut, bool /*bJSON*/)
				{
					printLiveAction(oOut, oWR, bShowDetail);
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
		printOutput(oTWDOF, [&](std::ostream& oOut, bool bJSON)
			{
				if (bJSON){
					printToWatchDirsJSon(oOut, oFofiModel, false);
				} else {
					printToWatchDirs(oOut, oFofiModel, false);
				}
			});
	}

	oPrintTotalWatchedDirs(true);
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
		const auto c = ::getchar();
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
		printOutput(oTWDAfterOF, [&](std::ostream& oOut, bool bJSON)
			{
				if (bJSON){
					printToWatchDirsJSon(oOut, oFofiModel, false);
				} else {
					printToWatchDirs(oOut, oFofiModel, false);
					if (bAborted) {
						oOut << "Aborted! " << sFatalError << '\n';
					}
				}
			});
		oPrintTotalWatchedDirs(false);
	}

	if (bPrintModified) {
		printOutput(oModifiedOF, [&](std::ostream& oOut, bool bJSON)
			{
				if (bJSON) {
					oOut << "[" << '\n';
				}
				bool bFirst = true;
				const auto& aResults = oFofiModel.getWatchedResults();
				for (const auto& oResult : aResults) {
					if (bSkipTemporary && (oResult.m_eResultType == FofiModel::RESULT_TEMPORARY)) {
						continue; // for ----
					}
					if (bJSON) {
						if (! bFirst) {
							bFirst = false;
							oOut << "," << '\n';
						}
						printResultJSon(oOut, oResult, bShowDetail, nDuration);
					} else {
						printResult(oOut, oResult, bShowDetail, nDuration);
					}
				}
				if (bJSON) {
					oOut << "]" << '\n';
				} else {
					if (bAborted) {
						oOut << "Aborted! " << sFatalError << '\n';
					}
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
