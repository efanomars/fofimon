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
 * File:   testFofiModel09.cxx
 */

#include "fofimodel.h"

#include "testingcommon.h"
#include "testingutil.h"
#include "tempfiletreefixture.h"
#include "forkingfixture.h"
#include "mainloopfixture.h"

#include <glibmm.h>

#include <iostream>
#include <cassert>

namespace fofi
{
namespace testing
{

int testDeepRenameAndBack()
{
	TempFileTreeFixture oTempFileTreeFixture{};

	const int32_t nGenFiles = 5;

	// create initial file structure
	for (int32_t nI = 0; nI < nGenFiles; ++ nI) {
		oTempFileTreeFixture.createOrModifyRelFile("A1/B12/C123/D1231/E12311/F123114/G1231142/xx1231142_" + std::to_string(nI) + ".txt");
	}
	for (int32_t nI = 0; nI < nGenFiles; ++ nI) {
		oTempFileTreeFixture.createOrModifyRelFile("A1/B12/C123/D1231/E12311/F123114/xx123114_" + std::to_string(nI) + ".txt");
	}
	for (int32_t nI = 0; nI < nGenFiles; ++ nI) {
		oTempFileTreeFixture.createOrModifyRelFile("A1/B12/C123/D1231/E12311/xx12311_" + std::to_string(nI) + ".txt");
	}
	for (int32_t nI = 0; nI < nGenFiles; ++ nI) {
		oTempFileTreeFixture.createOrModifyRelFile("A1/B12/C123/D1231/xx1231_" + std::to_string(nI) + ".txt");
	}
	for (int32_t nI = 0; nI < nGenFiles; ++ nI) {
		oTempFileTreeFixture.createOrModifyRelFile("A1/B12/C123/xx123_" + std::to_string(nI) + ".txt");
	}
	for (int32_t nI = 0; nI < nGenFiles; ++ nI) {
		oTempFileTreeFixture.createOrModifyRelFile("A1/B12/xx12_" + std::to_string(nI) + ".txt");
	}

	// create child process to perform additional operations while watching
	ForkingFixture oForkingFixture([&](){
		oTempFileTreeFixture.renameRelPathName("A1", "A2");
		oTempFileTreeFixture.sleepMillisec(500);
		oTempFileTreeFixture.renameRelPathName("A2", "A1");
	});
	// only parent process gets here!

	const auto& sBasePath = oTempFileTreeFixture.m_sTestBasePath;
	const auto aBaseSplitPath = testing::splitAbsolutePath(sBasePath);

	// create main loop needed by FofiModel
	MainLoopFixture oMainLoop;

	FofiModel oFofiModel(1000000, 1000000);

	FofiModel::DirectoryZone oDZ1;
	oDZ1.m_sPath = sBasePath;
	oDZ1.m_nMaxDepth = 9999;
	auto sErr = oFofiModel.addDirectoryZone(std::move(oDZ1));
	EXPECT_TRUE(sErr.empty());

	// check the set up potentially watched directories without watching yet
	oFofiModel.calcToWatchDirectories();
	const auto& aToWatchDirs = oFofiModel.getToWatchDirectories();

//for (const auto& oToWatch : aToWatchDirs) {
//std::cout << "  oToWatch.m_sPath=" << oToWatch.m_sPath << "  oToWatch.m_nMaxDepth=" << oToWatch.m_nMaxDepth << '\n';
//}
	EXPECT_TRUE(oFofiModel.getDirectoryZones().size() == 1);
//std::cout << "  aToWatchDirs.size()=" << aToWatchDirs.size() << '\n';
	EXPECT_TRUE(aToWatchDirs.size() == aBaseSplitPath.size() + 1 + 7); // + "/" + all of "A1/B12/C123/D1231/E12311/F123114/G1231142"

	const int32_t nBaseTWDIdx = oTempFileTreeFixture.findToWatchPath(oFofiModel, sBasePath);
	EXPECT_TRUE(nBaseTWDIdx >= 0);
	const FofiModel::ToWatchDir& oBaseTWD = aToWatchDirs[nBaseTWDIdx];
//std::cout << "  oBaseTWD.m_aPinnedSubDirs.size()=" << oBaseTWD.m_aPinnedSubDirs.size() << '\n';
	EXPECT_TRUE(oBaseTWD.m_aPinnedSubDirs.size() == 0);
	EXPECT_TRUE(! oBaseTWD.isLeaf());
	EXPECT_TRUE(! oBaseTWD.isWatched());
	EXPECT_TRUE(oBaseTWD.getToWatchSubDirIdxs().size() == 1);

	// start watching
	oFofiModel.start();
	// run the main loop that exits when child terminated
	const int32_t nTestIntervalMillisec = 100;
	int32_t nInitialCount = 4;
	int32_t nFinalCount = 4;
	oMainLoop.run([&]() -> bool
	{
		if (nInitialCount > 0) {
			--nInitialCount;
			return true;
		} else if (nInitialCount == 0) {
			oForkingFixture.startChild();
			--nInitialCount;
			return true;
		} else if (nInitialCount == -1) {
			const bool bChildFinished = oForkingFixture.isChildTerminated();
			if (bChildFinished) {
				// go to final count
				--nInitialCount;
			}
			return true;
		}
//std::cout << "  oMainLoop.run CALLBACK" << '\n';
		if (nFinalCount > 0) {
			--nFinalCount;
			return true;
		}
		return false;
	}, nTestIntervalMillisec);
	// child has terminated, stop watching
	oFofiModel.stop();
	// check generated results
	const auto& aResults = oFofiModel.getWatchedResults();
//std::cout << "  aResults.size()=" << aResults.size() << '\n';
//std::cout << "  oTempFileTreeFixture.m_sTestBasePath=" << oTempFileTreeFixture.m_sTestBasePath << '\n';
	EXPECT_TRUE(aResults.size() == 2 * (7 + (7-1) * nGenFiles));

	const auto& oResult0 = aResults[0];
	EXPECT_TRUE(oResult0.m_sName == "A1");
	EXPECT_TRUE(oResult0.m_bIsDir);

	uint32_t nCountA1 = 0;
	for (const auto& oResult : aResults) {
		const std::string sPathName = oResult.m_sPath + "/" + oResult.m_sName;
//std::cout << "  Result path=" << sPathName << (oResult.m_bIsDir ? "/" : "") << '\n';
//std::cout << "         oResult.m_aActions.size()=" << oResult.m_aActions.size() << '\n';
		EXPECT_TRUE(oResult.m_aActions.size() == 2);
		EXPECT_TRUE(! oResult.m_bInconsistent);
		if (sPathName.find(oTempFileTreeFixture.m_sTestBasePath + "/A1", 0) != std::string::npos) {
			EXPECT_TRUE(oResult.m_eResultType == FofiModel::RESULT_MODIFIED);
			EXPECT_TRUE(oResult.m_aActions[0].m_eAction == INotifierSource::FOFI_ACTION_RENAME_FROM);
			EXPECT_TRUE(oResult.m_aActions[1].m_eAction == INotifierSource::FOFI_ACTION_RENAME_TO);
			++nCountA1;
		} else {
			EXPECT_TRUE(oResult.m_eResultType == FofiModel::RESULT_TEMPORARY);
			EXPECT_TRUE(oResult.m_aActions[0].m_eAction == INotifierSource::FOFI_ACTION_RENAME_TO);
			EXPECT_TRUE(oResult.m_aActions[1].m_eAction == INotifierSource::FOFI_ACTION_RENAME_FROM);
		}
	}
//std::cout << "  nCountA1=" << nCountA1 << '\n';
	EXPECT_TRUE(nCountA1 == (aResults.size() / 2));
	return 0;
}

} // namespace testing
} // namespace fofi

int main(int /*argc*/, char** /*argv*/)
{
	std::cout << "FofiModel09 Tests:" << '\n';

	EXECUTE_TEST(fofi::testing::testDeepRenameAndBack());
	//
	std::cout << "FofiModel09 Tests successful!" << '\n';
	return 0;
}
