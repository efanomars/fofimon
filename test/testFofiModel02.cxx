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
 * File:   testFofiModel02.cxx
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

int testRenameDirectoryTree()
{
	TempFileTreeFixture oTempFileTreeFixture{};

	// create initial file structure
	oTempFileTreeFixture.createOrModifyRelFile("A1/B11/xx11.txt");
	oTempFileTreeFixture.createOrModifyRelFile("A1/B11/C111/xx111.txt");
	oTempFileTreeFixture.createOrModifyRelFile("A1/B12/C121/xx121.txt");
	oTempFileTreeFixture.createOrModifyRelFile("A1/B12/C122/xx122.txt");
	oTempFileTreeFixture.createOrModifyRelFile("A1/B12/C123/xx123.txt");
	oTempFileTreeFixture.createOrModifyRelFile("A2/B21/C211/xx211.txt");

	// create child process to perform additional operations while watching
	ForkingFixture oForkingFixture([&](){
		oTempFileTreeFixture.createOrModifyRelFile("A2/B21/C211/xx211.txt");
		oTempFileTreeFixture.createOrModifyRelFile("A1/B12/C123/xx123.txt");
		oTempFileTreeFixture.createOrModifyRelFile("A1/B12/C121/D1211/xx1211.txt");
		oTempFileTreeFixture.createOrModifyRelFile("A2/xx2.txt");
		oTempFileTreeFixture.renameRelPathName(    "A1/B12/C121/D1211", "AAA");

		oTempFileTreeFixture.sleepMillisec(500);

		oTempFileTreeFixture.removeRelFile("A2/xx2.txt");
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
	EXPECT_TRUE(oFofiModel.hasDirectoryZone(sBasePath));

	FofiModel::DirectoryZone oDZ2;
	oDZ2.m_sPath = sBasePath + "/A1/B12";
	oDZ2.m_nMaxDepth = 1;
	sErr = oFofiModel.addDirectoryZone(std::move(oDZ2));
	EXPECT_TRUE(sErr.empty());
	EXPECT_TRUE(oFofiModel.hasDirectoryZone(sBasePath + "/A1/B12"));

	// check the set up watched directories without watching yet
	oFofiModel.calcToWatchDirectories();
	const auto& aToWatchDirs = oFofiModel.getToWatchDirectories();

//for (const auto& oToWatch : aToWatchDirs) {
//std::cout << "  oToWatch.m_sPath=" << oToWatch.m_sPath << "  oToWatch.m_nMaxDepth=" << oToWatch.m_nMaxDepth << '\n';
//}
	EXPECT_TRUE(oFofiModel.getDirectoryZones().size() == 2);
//std::cout << "  aToWatchDirs.size()=" << aToWatchDirs.size() << '\n';
	EXPECT_TRUE(aToWatchDirs.size() == aBaseSplitPath.size() + 1 + 2 + 4);
	// + "/" + ("A1","A2") + ("/A1/B12", "/A1/B12/C121", "/A1/B12/C122", "/A1/B12/C123") 
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

	const auto& aToWatchDirsAfter = oFofiModel.getToWatchDirectories();
//std::cout << "  aToWatchDirsAfter.size()=" << aToWatchDirsAfter.size() << "  expect=" << (aBaseSplitPath.size() + 1 + 1 + 4 + 1 + 1) << '\n';
	EXPECT_TRUE(aToWatchDirsAfter.size() == aBaseSplitPath.size() + 1 + 1 + 4 + 1 + 1);
	// + "/" + ("A1") + ("/A1/B12", "/A1/B12/C121", "/A1/B12/C122", "/A1/B11/C123") + ("A2")
	// + ("AAA")

	// check generated results
	const auto& aResults = oFofiModel.getWatchedResults();
//std::cout << "   aResults.size()=" << aResults.size() << '\n';
	EXPECT_TRUE(aResults.size() == 5);

	const auto& oResult0 = aResults[0];
	EXPECT_TRUE(oResult0.m_sName == "xx123.txt");
	EXPECT_TRUE(! oResult0.m_bIsDir);
	EXPECT_TRUE(oResult0.m_aActions.size() == 1);
	EXPECT_TRUE(oResult0.m_aActions[0].m_eAction == INotifierSource::FOFI_ACTION_MODIFY);
	EXPECT_TRUE(oResult0.m_eResultType == FofiModel::RESULT_MODIFIED);

	const auto& oResult1 = aResults[1];
	EXPECT_TRUE(oResult1.m_sName == "D1211");
	EXPECT_TRUE(oResult1.m_bIsDir);
//std::cout << "   oResult1.m_aActions.size()=" << oResult1.m_aActions.size() << '\n';
	EXPECT_TRUE(oResult1.m_aActions.size() == 2);
	EXPECT_TRUE(oResult1.m_aActions[0].m_eAction == INotifierSource::FOFI_ACTION_CREATE);
	EXPECT_TRUE(oResult1.m_aActions[1].m_eAction == INotifierSource::FOFI_ACTION_RENAME_FROM);
	EXPECT_TRUE(oResult1.m_eResultType == FofiModel::RESULT_TEMPORARY);

	const auto& oResult2 = aResults[2];
	EXPECT_TRUE(oResult2.m_sName == "xx2.txt");
	EXPECT_TRUE(! oResult2.m_bIsDir);
	EXPECT_TRUE(oResult2.m_aActions.size() == 2);
	EXPECT_TRUE(oResult2.m_aActions[0].m_eAction == INotifierSource::FOFI_ACTION_CREATE);
	EXPECT_TRUE(oResult2.m_aActions[1].m_eAction == INotifierSource::FOFI_ACTION_DELETE);
	EXPECT_TRUE(oResult2.m_eResultType == FofiModel::RESULT_TEMPORARY);

	const auto& oResult3 = aResults[3];
//std::cout << "   oResult3.m_sName=" << oResult3.m_sName << '\n';
	EXPECT_TRUE(oResult3.m_sName == "AAA");
	EXPECT_TRUE(oResult3.m_bIsDir);
	EXPECT_TRUE(oResult3.m_aActions.size() == 1);
	EXPECT_TRUE(oResult3.m_aActions[0].m_eAction == INotifierSource::FOFI_ACTION_RENAME_TO);
	EXPECT_TRUE(oResult3.m_eResultType == FofiModel::RESULT_CREATED);

	const auto& oResult4 = aResults[4];
//std::cout << "   oResult4.m_sName=" << oResult4.m_sName << '\n';
//std::cout << "   oResult4.m_sPath=" << oResult4.m_sPath << '\n';
	EXPECT_TRUE(oResult4.m_sName == "xx1211.txt");
	EXPECT_TRUE(! oResult4.m_bIsDir);
	EXPECT_TRUE(oResult4.m_aActions.size() == 1);
	EXPECT_TRUE(oResult4.m_aActions[0].m_eAction == INotifierSource::FOFI_ACTION_CREATE);
	EXPECT_TRUE(oResult4.m_eResultType == FofiModel::RESULT_CREATED);

	return 0;
}

} // namespace testing
} // namespace fofi

int main(int /*argc*/, char** /*argv*/)
{
	std::cout << "FofiModel02 Tests:" << '\n';

	EXECUTE_TEST(fofi::testing::testRenameDirectoryTree());
	//
	std::cout << "FofiModel02 Tests successful!" << '\n';
	return 0;
}
