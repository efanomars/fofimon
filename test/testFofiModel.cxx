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
 * File:   testFofiModel.cxx
 */

#include "fofimodel.h"

#include "testingcommon.h"
#include "testingutil.h"
#include "tempfiletreefixture.h"

#include <glibmm.h>

#include <iostream>
#include <cassert>

namespace fofi
{
namespace testing
{

int testAddDirectoryZone()
{
	TempFileTreeFixture oTempFileTreeFixture{};
	const auto& sBasePath = oTempFileTreeFixture.m_sTestBasePath;
	const auto aBaseSplitPath = testing::splitAbsolutePath(sBasePath);
//std::cout << "sBasePath=" << sBasePath << '\n';
	oTempFileTreeFixture.createOrModifyRelFile("A/B/xx1.txt");
	FofiModel oFofiModel(1000000, 1000000);

	FofiModel::DirectoryZone oDZ1;
	oDZ1.m_sPath = sBasePath;
	oDZ1.m_nMaxDepth = 3;
	auto sErr = oFofiModel.addDirectoryZone(std::move(oDZ1));
	EXPECT_TRUE(sErr.empty());
	EXPECT_TRUE(oFofiModel.hasDirectoryZone(sBasePath));

	oFofiModel.calcToWatchDirectories();
	const auto& aToWatchDirs = oFofiModel.getToWatchDirectories();

	EXPECT_TRUE(oFofiModel.getDirectoryZones().size() == 1);
	EXPECT_TRUE(aToWatchDirs.size() == aBaseSplitPath.size() + 1 + 2);

	const int32_t nRootTWDIdx = oFofiModel.getRootToWatchDirectoriesIdx();
	EXPECT_TRUE(nRootTWDIdx >= 0);
	const FofiModel::ToWatchDir& oRootTWD = aToWatchDirs[nRootTWDIdx];
	EXPECT_TRUE(oRootTWD.m_sPathName == "/");
	EXPECT_TRUE( oRootTWD.exists());
	EXPECT_TRUE(oRootTWD.m_aPinnedSubDirs.size() == 1);
	EXPECT_TRUE(oRootTWD.m_aPinnedSubDirs[0] == aBaseSplitPath[0]);
	EXPECT_TRUE(oRootTWD.getToWatchSubDirIdxs().size() == 1);

	int32_t nCurTWDIdx = oRootTWD.getToWatchSubDirIdxs()[0];
	const auto nBasePathEls = static_cast<int32_t>(aBaseSplitPath.size());
	for (int32_t nPathEl = 0; nPathEl < nBasePathEls; ++nPathEl ) {
		const auto& oCurTWD = aToWatchDirs[nCurTWDIdx];
		// calc partial path
		std::string sPartiaPath;
		for (int32_t nPartialEl = 0; nPartialEl <= nPathEl; ++nPartialEl) {
			sPartiaPath += "/" + aBaseSplitPath[nPartialEl];
		}
//std::cout << "sPartiaPath=" << sPartiaPath << '\n';
		EXPECT_TRUE(oCurTWD.m_sPathName == sPartiaPath);
		EXPECT_TRUE( oCurTWD.exists());
		if (nPathEl != nBasePathEls - 1) {
			EXPECT_TRUE(oCurTWD.m_aPinnedSubDirs.size() == 1);
			EXPECT_TRUE(oCurTWD.m_aPinnedSubDirs[0] == aBaseSplitPath[nPathEl + 1]);
			EXPECT_TRUE(oCurTWD.getToWatchSubDirIdxs().size() == 1);
			nCurTWDIdx = oCurTWD.getToWatchSubDirIdxs()[0];
		}
	}

	const auto& oCurTWD = aToWatchDirs[nCurTWDIdx];
	EXPECT_TRUE(oCurTWD.getToWatchSubDirIdxs().size() == 1);
	const int32_t nATWDIdx = oCurTWD.getToWatchSubDirIdxs()[0];
	const auto& oATWD = aToWatchDirs[nATWDIdx];
	EXPECT_TRUE(oATWD.m_sPathName == sBasePath + "/A");
	
	return 0;
}

int testAddTwoDirectoryZones()
{
	TempFileTreeFixture oTempFileTreeFixture{};
	const auto& sBasePath = oTempFileTreeFixture.m_sTestBasePath;
	const auto aBaseSplitPath = testing::splitAbsolutePath(sBasePath);

	oTempFileTreeFixture.createOrModifyRelFile("A1/B11/xx11.txt");
	oTempFileTreeFixture.createOrModifyRelFile("A1/B12/C121/xx121.txt");
	oTempFileTreeFixture.createOrModifyRelFile("A1/B12/C122/xx122.txt");
	oTempFileTreeFixture.createOrModifyRelFile("A1/B12/C123/xx123.txt");
	oTempFileTreeFixture.createOrModifyRelFile("A2/B21/C211/xx211.txt");

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

	oFofiModel.calcToWatchDirectories();
	const auto& aToWatchDirs = oFofiModel.getToWatchDirectories();

	EXPECT_TRUE(oFofiModel.getDirectoryZones().size() == 2);
//std::cout << "  aToWatchDirs.size()=" << aToWatchDirs.size() << '\n';
	EXPECT_TRUE(aToWatchDirs.size() == aBaseSplitPath.size() + 1 + 2 + 4); // + "/" + ("A1","A2") + ("/A1/B12", "/A1/B12"/C121", "/A1/B12"/C122", "/A1/B12"/C123")
	const int32_t nRootTWDIdx = oFofiModel.getRootToWatchDirectoriesIdx();
	EXPECT_TRUE(nRootTWDIdx >= 0);
	EXPECT_TRUE(aToWatchDirs[nRootTWDIdx].m_sPathName == "/");

	const int32_t nBaseTWDIdx = oTempFileTreeFixture.findToWatchPath(oFofiModel, sBasePath);
	EXPECT_TRUE(nBaseTWDIdx >= 0);
	const FofiModel::ToWatchDir& oBaseTWD = aToWatchDirs[nBaseTWDIdx];
//std::cout << "  oBaseTWD.m_aPinnedSubDirs.size()=" << oBaseTWD.m_aPinnedSubDirs.size() << '\n';
	EXPECT_TRUE(oBaseTWD.m_aPinnedSubDirs.size() == 1);
	EXPECT_TRUE(oBaseTWD.m_aPinnedSubDirs[0] == "A1");

	return 0;
}

} // namespace testing
} // namespace fofi

int main(int /*argc*/, char** /*argv*/)
{
	std::cout << "FofiModel Tests:" << '\n';

	EXECUTE_TEST(fofi::testing::testAddDirectoryZone());
	EXECUTE_TEST(fofi::testing::testAddTwoDirectoryZones());
	//
	std::cout << "FofiModel Tests successful!" << '\n';
	return 0;
}
