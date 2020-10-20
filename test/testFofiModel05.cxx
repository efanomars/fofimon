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
 * File:   testFofiModel05.cxx
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

int testNonExistingDirectoryZone()
{
	TempFileTreeFixture oTempFileTreeFixture{};

	// create initial file structure
	oTempFileTreeFixture.createOrModifyRelFile("A1/B11/xx11.txt");

	// create child process to perform additional operations while watching
	ForkingFixture oForkingFixture([&](){
		oTempFileTreeFixture.createOrModifyRelFile("A1/B12/C123/xx123.txt");
		//oTempFileTreeFixture.createOrModifyRelFile("A1/B12/C121/xx121.txt");

		//oTempFileTreeFixture.renameRelPathName("A2", "A1/B12/C124");
	});

	// only parent process gets here
	const auto& sBasePath = oTempFileTreeFixture.m_sTestBasePath;
	const auto aBaseSplitPath = testing::splitAbsolutePath(sBasePath);

	// create main loop needed by FofiModel
	MainLoopFixture oMainLoop;

	FofiModel oFofiModel(1000000, 1000000);

	FofiModel::DirectoryZone oDZ1;
	oDZ1.m_sPath = sBasePath;
	oDZ1.m_nMaxDepth = 1;
	auto sErr = oFofiModel.addDirectoryZone(std::move(oDZ1));
	EXPECT_TRUE(sErr.empty());

	FofiModel::DirectoryZone oDZ2;
	oDZ2.m_sPath = sBasePath + "/A1/B12";
	oDZ2.m_nMaxDepth = 1;
	sErr = oFofiModel.addDirectoryZone(std::move(oDZ2));
	EXPECT_TRUE(sErr.empty());

	// check the set up potentially watched directories without watching yet
	oFofiModel.calcToWatchDirectories();
	const auto& aToWatchDirs = oFofiModel.getToWatchDirectories();

//for (const auto& oToWatch : aToWatchDirs) {
//std::cout << "  oToWatch.m_sPath=" << oToWatch.m_sPath << "  oToWatch.m_nMaxDepth=" << oToWatch.m_nMaxDepth << '\n';
//}
	EXPECT_TRUE(oFofiModel.getDirectoryZones().size() == 2);
//std::cout << "  aToWatchDirs.size()=" << aToWatchDirs.size() << '\n';
	EXPECT_TRUE(aToWatchDirs.size() == aBaseSplitPath.size() + 1 + 1 + 1); // + "/" + ("A1") + ("/A1/B11")

	const int32_t nBaseTWDIdx = oTempFileTreeFixture.findToWatchPath(oFofiModel, sBasePath);
	EXPECT_TRUE(nBaseTWDIdx >= 0);
	const FofiModel::ToWatchDir& oBaseTWD = aToWatchDirs[nBaseTWDIdx];
//std::cout << "  oBaseTWD.m_aPinnedSubDirs.size()=" << oBaseTWD.m_aPinnedSubDirs.size() << '\n';
	EXPECT_TRUE(oBaseTWD.m_aPinnedSubDirs.size() == 1);
	EXPECT_TRUE(oBaseTWD.m_aPinnedSubDirs[0] == "A1");
	EXPECT_TRUE(! oBaseTWD.isLeaf());
	EXPECT_TRUE(! oBaseTWD.isWatched());
	EXPECT_TRUE(oBaseTWD.getToWatchSubDirIdxs().size() == 1);

	const int32_t nA1TWDIdx = oTempFileTreeFixture.findToWatchPath(oFofiModel, sBasePath + "/A1");
	EXPECT_TRUE(nA1TWDIdx >= 0);
	EXPECT_TRUE(oBaseTWD.getToWatchSubDirIdxs()[0] == nA1TWDIdx);
	const FofiModel::ToWatchDir& oA1TWD = aToWatchDirs[nA1TWDIdx];
	EXPECT_TRUE(oA1TWD.exists());
	EXPECT_TRUE(oA1TWD.getParentIdx() == nBaseTWDIdx);
	EXPECT_TRUE(oA1TWD.isLeaf());
	EXPECT_TRUE(! oA1TWD.isWatched());
	EXPECT_TRUE(oA1TWD.getToWatchSubDirIdxs().size() == 1);

	const int32_t nA1B11TWDIdx = oTempFileTreeFixture.findToWatchPath(oFofiModel, sBasePath + "/A1/B12");
	EXPECT_TRUE(nA1B11TWDIdx >= 0);
	EXPECT_TRUE(oA1TWD.getToWatchSubDirIdxs()[0] == nA1B11TWDIdx);
	const FofiModel::ToWatchDir& oA1B11TWD = aToWatchDirs[nA1B11TWDIdx];
	EXPECT_TRUE(! oA1B11TWD.exists());
	EXPECT_TRUE(oA1B11TWD.getParentIdx() == nA1TWDIdx);
	EXPECT_TRUE(! oA1B11TWD.isLeaf());
	EXPECT_TRUE(! oA1B11TWD.isWatched());
	EXPECT_TRUE(oA1B11TWD.getToWatchSubDirIdxs().size() == 0);

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
	EXPECT_TRUE(aResults.size() == 3);

	const auto& oResult0 = aResults[0];
	EXPECT_TRUE(oResult0.m_aActions.size() == 1);
	EXPECT_TRUE(oResult0.m_aActions[0].m_eAction == INotifierSource::FOFI_ACTION_CREATE);
	EXPECT_TRUE(oResult0.m_eResultType == FofiModel::RESULT_CREATED);
	EXPECT_TRUE(oResult0.m_sName == "B12");
	EXPECT_TRUE(oResult0.m_bIsDir);

	const auto& oResult1 = aResults[1];
//std::cout << "  oResult1.m_aActions.size()=" << oResult1.m_aActions.size() << '\n';
//std::cout << "  action 0=" << static_cast<int32_t>(oResult1.m_aActions[0].m_eAction) << '\n';
//if (oResult1.m_aActions.size() > 1) {
//std::cout << "  action 1=" << static_cast<int32_t>(oResult1.m_aActions[1].m_eAction) << '\n';
//}
	EXPECT_TRUE(oResult1.m_aActions.size() == 1);
	EXPECT_TRUE(oResult1.m_aActions[0].m_eAction == INotifierSource::FOFI_ACTION_CREATE);
	EXPECT_TRUE(oResult1.m_eResultType == FofiModel::RESULT_CREATED);
	EXPECT_TRUE(oResult1.m_sName == "C123");
	EXPECT_TRUE(oResult1.m_bIsDir);

	const auto& oResult2 = aResults[2];
//std::cout << "  oResult2.m_aActions.size()=" << oResult2.m_aActions.size() << '\n';
//std::cout << "  action 0=" << static_cast<int32_t>(oResult2.m_aActions[0].m_eAction) << '\n';
//if (oResult2.m_aActions.size() > 1) {
//std::cout << "  action 1=" << static_cast<int32_t>(oResult2.m_aActions[1].m_eAction) << '\n';
//}
	EXPECT_TRUE(oResult2.m_aActions.size() == 1);
	EXPECT_TRUE(oResult2.m_aActions[0].m_eAction == INotifierSource::FOFI_ACTION_CREATE);
	EXPECT_TRUE(oResult2.m_eResultType == FofiModel::RESULT_CREATED);
	EXPECT_TRUE(oResult2.m_sName == "xx123.txt");
	EXPECT_TRUE(! oResult2.m_bIsDir);
	return 0;
}

} // namespace testing
} // namespace fofi

int main(int /*argc*/, char** /*argv*/)
{
	std::cout << "FofiModel05 Tests:" << '\n';

	EXECUTE_TEST(fofi::testing::testNonExistingDirectoryZone());
	//
	std::cout << "FofiModel05 Tests successful!" << '\n';
	return 0;
}
