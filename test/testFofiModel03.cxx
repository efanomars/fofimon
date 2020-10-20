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
 * File:   testFofiModel03.cxx
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

int testDirectoryZoneShadowing()
{
	TempFileTreeFixture oTempFileTreeFixture{};

	// create initial file structure
	oTempFileTreeFixture.createOrModifyRelFile("A1/B11/xx11.txt");
	oTempFileTreeFixture.createOrModifyRelFile("A1/B11/C111/xx111.txt");
	oTempFileTreeFixture.createOrModifyRelFile("A1/B11/C112/xx112.txt");
	oTempFileTreeFixture.createOrModifyRelFile("A1/B11/C113/xx113.txt");
	oTempFileTreeFixture.createOrModifyRelFile("A1/B12/C121/xx121.txt");
	oTempFileTreeFixture.createOrModifyRelFile("A1/B12/C122/xx122.txt");
	oTempFileTreeFixture.createOrModifyRelFile("A1/B12/C123/xx123.txt");

	// create child process to perform additional operations while watching
	ForkingFixture oForkingFixture([&](){
		oTempFileTreeFixture.createOrModifyRelFile("A1/B12/C123/xx123.txt");
		oTempFileTreeFixture.createOrModifyRelFile("A1/B12/C124/xx124.txt");
	});

	// only parent process gets here
	const auto& sBasePath = oTempFileTreeFixture.m_sTestBasePath;
	const auto aBaseSplitPath = testing::splitAbsolutePath(sBasePath);

	// create main loop needed by FofiModel
	MainLoopFixture oMainLoop;

	FofiModel oFofiModel(1000000, 1000000);

	FofiModel::DirectoryZone oDZ1;
	oDZ1.m_sPath = sBasePath;
	oDZ1.m_nMaxDepth = 3;
	auto sErr = oFofiModel.addDirectoryZone(std::move(oDZ1));
	EXPECT_TRUE(sErr.empty());

	FofiModel::DirectoryZone oDZ2;
	oDZ2.m_sPath = sBasePath + "/A1/B11";
	oDZ2.m_nMaxDepth = 1;
	sErr = oFofiModel.addDirectoryZone(std::move(oDZ2));
	EXPECT_TRUE(sErr.empty());

	FofiModel::DirectoryZone oDZ3;
	oDZ3.m_sPath = sBasePath + "/A1";
	oDZ3.m_nMaxDepth = 1;
	sErr = oFofiModel.addDirectoryZone(std::move(oDZ3));
	EXPECT_TRUE(sErr.empty());

	// check the set up watched directories without watching yet
	oFofiModel.calcToWatchDirectories();
	const auto& aToWatchDirs = oFofiModel.getToWatchDirectories();

//for (const auto& oToWatch : aToWatchDirs) {
//std::cout << "  oToWatch.m_sPath=" << oToWatch.m_sPath << "  oToWatch.m_nMaxDepth=" << oToWatch.m_nMaxDepth << '\n';
//}
	EXPECT_TRUE(oFofiModel.getDirectoryZones().size() == 3);
//std::cout << "  aToWatchDirs.size()=" << aToWatchDirs.size() << '\n';
	EXPECT_TRUE(aToWatchDirs.size() == aBaseSplitPath.size() + 1 + 1 + 4 + 1);
	// + "/" + ("A1") + ("/A1/B11", "/A1/B11/C111", "/A1/B11/C112", "/A1/B11/C113")
	// +  ("/A1/B12") 
	const int32_t nRootTWDIdx = oFofiModel.getRootToWatchDirectoriesIdx();
	EXPECT_TRUE(nRootTWDIdx >= 0);
	EXPECT_TRUE(aToWatchDirs[nRootTWDIdx].m_sPathName == "/");

	const int32_t nBaseTWDIdx = oTempFileTreeFixture.findToWatchPath(oFofiModel, sBasePath);
	EXPECT_TRUE(nBaseTWDIdx >= 0);
	const FofiModel::ToWatchDir& oBaseTWD = aToWatchDirs[nBaseTWDIdx];
//std::cout << "  oBaseTWD.m_aPinnedSubDirs.size()=" << oBaseTWD.m_aPinnedSubDirs.size() << '\n';
	EXPECT_TRUE(oBaseTWD.m_aPinnedSubDirs.size() == 1);
	EXPECT_TRUE(oBaseTWD.m_aPinnedSubDirs[0] == "A1");

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
//std::cout << "   aResults.size()=" << aResults.size() << '\n';
	EXPECT_TRUE(aResults.size() == 1);

	const auto& oResult0 = aResults[0];
	EXPECT_TRUE(oResult0.m_sName == "C124");
//std::cout << "   oResult0.m_aActions.size()=" << oResult0.m_aActions.size() << '\n';
	EXPECT_TRUE(oResult0.m_aActions.size() == 1);
	EXPECT_TRUE(oResult0.m_aActions[0].m_eAction == INotifierSource::FOFI_ACTION_CREATE);
	EXPECT_TRUE(oResult0.m_eResultType == FofiModel::RESULT_CREATED);

	return 0;
}

} // namespace testing
} // namespace fofi

int main(int /*argc*/, char** /*argv*/)
{
	std::cout << "FofiModel03 Tests:" << '\n';

	EXECUTE_TEST(fofi::testing::testDirectoryZoneShadowing());
	//
	std::cout << "FofiModel03 Tests successful!" << '\n';
	return 0;
}
