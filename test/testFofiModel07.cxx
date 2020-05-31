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
 * File:   testFofiModel07.cxx
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

int testDeepDirectoryReCreation()
{
	TempFileTreeFixture oTempFileTreeFixture{};

	// create initial file structure
	//oTempFileTreeFixture.createOrModifyRelFile("A1/B11/xx11.txt");

	// create child process to perform additional operations while watching
	ForkingFixture oForkingFixture([&](){
		oTempFileTreeFixture.createRelDir("A1/B12/C123/D1231/E12311/F123114/G1231142");
		oTempFileTreeFixture.createOrModifyRelFile("A1/B12/C123/D1231/E12311/F123114/G1231142/xx1231142.txt");
		oTempFileTreeFixture.createOrModifyRelFile("A1/B12/C123/D1231/E12311/F123114/G1231142/xy1231142.txt");
		oTempFileTreeFixture.createOrModifyRelFile("A2/B22/C223/D2231/E22311/F223117/xx223117.txt");
		oTempFileTreeFixture.createOrModifyRelFile("A3/B32/C323/D3231/E32311/F323117/G3231175/H32311756/xx32311756.txt");
		oTempFileTreeFixture.createOrModifyRelFile("A3/B32/C323/D3231/E32311/F323117/G3231175/H32311756/xy32311756.txt");
		oTempFileTreeFixture.createOrModifyRelFile("A3/B32/C323/D3231/E32311/F323117/G3231175/H32311756/xz32311756.txt");
		//
		oTempFileTreeFixture.sleepMillisec(500);
		//
		oTempFileTreeFixture.removeRelFile("A1/B12/C123/D1231/E12311/F123114/G1231142/xx1231142.txt");
		oTempFileTreeFixture.removeRelFile("A1/B12/C123/D1231/E12311/F123114/G1231142/xy1231142.txt");
		oTempFileTreeFixture.removeRelFile("A2/B22/C223/D2231/E22311/F223117/xx223117.txt");
		oTempFileTreeFixture.removeRelFile("A3/B32/C323/D3231/E32311/F323117/G3231175/H32311756/xx32311756.txt");
		oTempFileTreeFixture.removeRelFile("A3/B32/C323/D3231/E32311/F323117/G3231175/H32311756/xy32311756.txt");
		oTempFileTreeFixture.removeRelFile("A3/B32/C323/D3231/E32311/F323117/G3231175/H32311756/xz32311756.txt");
		oTempFileTreeFixture.removeRelDir( "A1/B12/C123/D1231/E12311/F123114/G1231142");
		oTempFileTreeFixture.removeRelDir( "A1/B12/C123/D1231/E12311/F123114");
		oTempFileTreeFixture.removeRelDir( "A1/B12/C123/D1231/E12311");
		oTempFileTreeFixture.removeRelDir( "A1/B12/C123/D1231");
		oTempFileTreeFixture.removeRelDir( "A1/B12/C123");
		oTempFileTreeFixture.removeRelDir( "A1/B12");
		oTempFileTreeFixture.removeRelDir( "A1");
		oTempFileTreeFixture.removeRelDir( "A2/B22/C223/D2231/E22311/F223117");
		oTempFileTreeFixture.removeRelDir( "A2/B22/C223/D2231/E22311");
		oTempFileTreeFixture.removeRelDir( "A2/B22/C223/D2231");
		oTempFileTreeFixture.removeRelDir( "A2/B22/C223");
		oTempFileTreeFixture.removeRelDir( "A2/B22");
		oTempFileTreeFixture.removeRelDir( "A2");
		oTempFileTreeFixture.removeRelDir( "A3/B32/C323/D3231/E32311/F323117/G3231175/H32311756");
		oTempFileTreeFixture.removeRelDir( "A3/B32/C323/D3231/E32311/F323117/G3231175");
		oTempFileTreeFixture.removeRelDir( "A3/B32/C323/D3231/E32311/F323117");
		oTempFileTreeFixture.removeRelDir( "A3/B32/C323/D3231/E32311");
		oTempFileTreeFixture.removeRelDir( "A3/B32/C323/D3231");
		oTempFileTreeFixture.removeRelDir( "A3/B32/C323");
		oTempFileTreeFixture.removeRelDir( "A3/B32");
		oTempFileTreeFixture.removeRelDir( "A3");
		//
		oTempFileTreeFixture.sleepMillisec(500);
		//
		oTempFileTreeFixture.createOrModifyRelFile("A1/B12/C123/D1231/E12311/F123114/G1231142/xx1231142.txt");
		oTempFileTreeFixture.createOrModifyRelFile("A1/B12/C123/D1231/E12311/F123114/G1231142/xy1231142.txt");
		oTempFileTreeFixture.createOrModifyRelFile("A2/B22/C223/D2231/E22311/F223117/xx223117.txt");
		oTempFileTreeFixture.createOrModifyRelFile("A3/B32/C323/D3231/E32311/F323117/G3231175/H32311756/xx32311756.txt");
		oTempFileTreeFixture.createOrModifyRelFile("A3/B32/C323/D3231/E32311/F323117/G3231175/H32311756/xy32311756.txt");
		oTempFileTreeFixture.createOrModifyRelFile("A3/B32/C323/D3231/E32311/F323117/G3231175/H32311756/xz32311756.txt");
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
	EXPECT_TRUE(aToWatchDirs.size() == aBaseSplitPath.size() + 1); // + "/"

	const int32_t nBaseTWDIdx = oTempFileTreeFixture.findToWatchPath(oFofiModel, sBasePath);
	EXPECT_TRUE(nBaseTWDIdx >= 0);
	const FofiModel::ToWatchDir& oBaseTWD = aToWatchDirs[nBaseTWDIdx];
//std::cout << "  oBaseTWD.m_aPinnedSubDirs.size()=" << oBaseTWD.m_aPinnedSubDirs.size() << '\n';
	EXPECT_TRUE(oBaseTWD.m_aPinnedSubDirs.size() == 0);
	EXPECT_TRUE(! oBaseTWD.isLeaf());
	EXPECT_TRUE(! oBaseTWD.isWatched());
	EXPECT_TRUE(oBaseTWD.getToWatchSubDirIdxs().size() == 0);

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
	EXPECT_TRUE(aResults.size() == 7 + 2 + 6 + 1 + 8 + 3);

	const auto& oResult0 = aResults[0];
	EXPECT_TRUE(oResult0.m_sName == "A1");
	EXPECT_TRUE(oResult0.m_bIsDir);

	EXPECT_TRUE( ! oFofiModel.hasInconsistencies());

	for (const auto& oResult : aResults) {
		EXPECT_TRUE(! oResult.m_bInconsistent);
		EXPECT_TRUE(oResult.m_eResultType == FofiModel::RESULT_CREATED);
		EXPECT_TRUE(oResult.m_aActions.size() == 3);
		EXPECT_TRUE(oResult.m_aActions[0].m_eAction == INotifierSource::FOFI_ACTION_CREATE);
		EXPECT_TRUE(oResult.m_aActions[1].m_eAction == INotifierSource::FOFI_ACTION_DELETE);
		EXPECT_TRUE(oResult.m_aActions[2].m_eAction == INotifierSource::FOFI_ACTION_CREATE);
		EXPECT_TRUE(! oResult.m_bInconsistent);
	}
//	const auto& oResult1 = aResults[1];
////std::cout << "  oResult1.m_aActions.size()=" << oResult1.m_aActions.size() << '\n';
////std::cout << "  action 0=" << static_cast<int32_t>(oResult1.m_aActions[0].m_eAction) << '\n';
//	EXPECT_TRUE(oResult1.m_aActions.size() == 1);
//	EXPECT_TRUE(oResult1.m_aActions[0].m_eAction == INotifierSource::FOFI_ACTION_CREATE);
//	EXPECT_TRUE(oResult1.m_eResultType == FofiModel::RESULT_CREATED);
//	EXPECT_TRUE(oResult1.m_sName == "C123");
//	EXPECT_TRUE(oResult1.m_bIsDir);
//
//	const auto& oResult2 = aResults[2];
//	EXPECT_TRUE(oResult2.m_aActions.size() == 1);
//	EXPECT_TRUE(oResult2.m_aActions[0].m_eAction == INotifierSource::FOFI_ACTION_CREATE);
//	EXPECT_TRUE(oResult2.m_eResultType == FofiModel::RESULT_CREATED);
//	EXPECT_TRUE(oResult2.m_sName == "xx123.txt");
//	EXPECT_TRUE(! oResult2.m_bIsDir);
	return 0;
}

} // namespace testing
} // namespace fofi

int main(int /*argc*/, char** /*argv*/)
{
	std::cout << "FofiModel07 Tests:" << '\n';

	EXECUTE_TEST(fofi::testing::testDeepDirectoryReCreation());
	//
	std::cout << "FofiModel07 Tests successful!" << '\n';
	return 0;
}
