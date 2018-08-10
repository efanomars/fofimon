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
 * File:   testFofiModelF01.cxx
 */

#include "fofimodel.h"

#include "testingcommon.h"
#include "testingutil.h"
#include "tempfiletreefixture.h"

#include "fakesource.h"

#include <glibmm.h>

#include <iostream>
#include <cassert>

namespace fofi
{
namespace testing
{

int testMissingDeleteDir()
{
	TempFileTreeFixture oTempFileTreeFixture{};
	const auto& sBasePath = oTempFileTreeFixture.m_sTestBasePath;
	//const auto aBaseSplitPath = testing::splitAbsolutePath(sBasePath);

//std::cout << "sBasePath=" << sBasePath << '\n';
	oTempFileTreeFixture.createOrModifyRelFile("A/B/xx1.txt");

	FofiModel oFofiModel(std::make_unique<FakeSource>(0), 1000000, 1000000, false);
	FakeSource* p0Source = static_cast<FakeSource*>(oFofiModel.getSource());

	FofiModel::DirectoryZone oDZ1;
	oDZ1.m_sPath = sBasePath;
	oDZ1.m_nMaxDepth = 3;
	auto sErr = oFofiModel.addDirectoryZone(std::move(oDZ1));
	EXPECT_TRUE(sErr.empty());

	// start watching
	oFofiModel.start();

	//const auto& aTWDs = oFofiModel.getToWatchDirectories();
	const int32_t n_AB_TWDIdx = oTempFileTreeFixture.findToWatchPath(oFofiModel, sBasePath + "/A/B");
	const int32_t n_A_TWDIdx = oTempFileTreeFixture.findToWatchPath(oFofiModel, sBasePath + "/A");

	EXPECT_TRUE(n_AB_TWDIdx >= 0);
	EXPECT_TRUE(n_A_TWDIdx >= 0);

	oTempFileTreeFixture.removeRelFile("A/B/xx1.txt");
	{
	INotifierSource::FofiData oFD;
	oFD.m_nTag = n_AB_TWDIdx;
	oFD.m_sName = "xx1.txt";
	oFD.m_eAction = INotifierSource::FOFI_ACTION_DELETE;
	p0Source->callback(oFD);
	}

	oTempFileTreeFixture.removeRelDir("A/B");
	// missing action FOFI_ACTION_DELETE

	oTempFileTreeFixture.createRelDir("A/B");
	{
	INotifierSource::FofiData oFD;
	oFD.m_nTag = n_A_TWDIdx;
	oFD.m_sName = "B";
	oFD.m_eAction = INotifierSource::FOFI_ACTION_CREATE;
	oFD.m_bIsDir = true;
	p0Source->callback(oFD);
	}

	oFofiModel.stop();

	EXPECT_TRUE(oFofiModel.hasInconsistencies());

	const auto& aResults = oFofiModel.getWatchedResults();
	EXPECT_TRUE(aResults.size() == 2);

	const auto& oResult0 = aResults[0];
	EXPECT_TRUE(oResult0.m_aActions.size() == 1);
	EXPECT_TRUE(oResult0.m_aActions[0].m_eAction == INotifierSource::FOFI_ACTION_DELETE);
	EXPECT_TRUE(oResult0.m_eResultType == FofiModel::RESULT_DELETED);
	EXPECT_TRUE(oResult0.m_sName == "xx1.txt");
	EXPECT_TRUE(! oResult0.m_bIsDir);
	EXPECT_TRUE(! oResult0.m_bInconsistent);

	const auto& oResult1 = aResults[1];
	EXPECT_TRUE(oResult1.m_aActions.size() == 1);
	EXPECT_TRUE(oResult1.m_aActions[0].m_eAction == INotifierSource::FOFI_ACTION_CREATE);
//std::cout << "oResult1.m_eResultType=" << static_cast<int32_t>(oResult1.m_eResultType) << '\n';
	EXPECT_TRUE(oResult1.m_eResultType == FofiModel::RESULT_MODIFIED);
	EXPECT_TRUE(oResult1.m_sName == "B");
	EXPECT_TRUE(oResult1.m_bIsDir);
	EXPECT_TRUE(oResult1.m_bInconsistent);

	return 0;
}

int testCreatePresentFile()
{
	TempFileTreeFixture oTempFileTreeFixture{};
	const auto& sBasePath = oTempFileTreeFixture.m_sTestBasePath;
	//const auto aBaseSplitPath = testing::splitAbsolutePath(sBasePath);

//std::cout << "sBasePath=" << sBasePath << '\n';
	oTempFileTreeFixture.createOrModifyRelFile("A/B/xx1.txt");

	FofiModel oFofiModel(std::make_unique<FakeSource>(0), 1000000, 1000000, false);
	FakeSource* p0Source = static_cast<FakeSource*>(oFofiModel.getSource());

	FofiModel::DirectoryZone oDZ1;
	oDZ1.m_sPath = sBasePath;
	oDZ1.m_nMaxDepth = 3;
	auto sErr = oFofiModel.addDirectoryZone(std::move(oDZ1));
	EXPECT_TRUE(sErr.empty());

	// start watching
	oFofiModel.start();

	//const auto& aTWDs = oFofiModel.getToWatchDirectories();
	const int32_t n_AB_TWDIdx = oTempFileTreeFixture.findToWatchPath(oFofiModel, sBasePath + "/A/B");
	const int32_t n_A_TWDIdx = oTempFileTreeFixture.findToWatchPath(oFofiModel, sBasePath + "/A");

	EXPECT_TRUE(n_AB_TWDIdx >= 0);
	EXPECT_TRUE(n_A_TWDIdx >= 0);

	oTempFileTreeFixture.createOrModifyRelFile("A/B/xx1.txt");
	{
	INotifierSource::FofiData oFD;
	oFD.m_nTag = n_AB_TWDIdx;
	oFD.m_sName = "xx1.txt";
	oFD.m_eAction = INotifierSource::FOFI_ACTION_MODIFY;
	p0Source->callback(oFD);
	}

	oTempFileTreeFixture.removeRelFile("A/B/xx1.txt");
	// missing action FOFI_ACTION_DELETE

	oTempFileTreeFixture.createOrModifyRelFile("A/B/xx1.txt");
	{
	INotifierSource::FofiData oFD;
	oFD.m_nTag = n_AB_TWDIdx;
	oFD.m_sName = "xx1.txt";
	oFD.m_eAction = INotifierSource::FOFI_ACTION_CREATE;
	p0Source->callback(oFD);
	}

	oFofiModel.stop();

	EXPECT_TRUE(oFofiModel.hasInconsistencies());

	const auto& aResults = oFofiModel.getWatchedResults();
	EXPECT_TRUE(aResults.size() == 1);

	const auto& oResult0 = aResults[0];
	EXPECT_TRUE(oResult0.m_aActions.size() == 2);
	EXPECT_TRUE(oResult0.m_aActions[0].m_eAction == INotifierSource::FOFI_ACTION_MODIFY);
	EXPECT_TRUE(oResult0.m_aActions[1].m_eAction == INotifierSource::FOFI_ACTION_CREATE);
	EXPECT_TRUE(oResult0.m_eResultType == FofiModel::RESULT_MODIFIED);
	EXPECT_TRUE(oResult0.m_sName == "xx1.txt");
	EXPECT_TRUE(! oResult0.m_bIsDir);
	EXPECT_TRUE(oResult0.m_bInconsistent);

	return 0;
}

int testDeleteDeletedFile()
{
	TempFileTreeFixture oTempFileTreeFixture{};
	const auto& sBasePath = oTempFileTreeFixture.m_sTestBasePath;
	//const auto aBaseSplitPath = testing::splitAbsolutePath(sBasePath);

//std::cout << "sBasePath=" << sBasePath << '\n';
	oTempFileTreeFixture.createOrModifyRelFile("A/B/xx1.txt");

	FofiModel oFofiModel(std::make_unique<FakeSource>(0), 1000000, 1000000, false);
	FakeSource* p0Source = static_cast<FakeSource*>(oFofiModel.getSource());

	FofiModel::DirectoryZone oDZ1;
	oDZ1.m_sPath = sBasePath;
	oDZ1.m_nMaxDepth = 3;
	auto sErr = oFofiModel.addDirectoryZone(std::move(oDZ1));
	EXPECT_TRUE(sErr.empty());

	// start watching
	oFofiModel.start();

	//const auto& aTWDs = oFofiModel.getToWatchDirectories();
	const int32_t n_AB_TWDIdx = oTempFileTreeFixture.findToWatchPath(oFofiModel, sBasePath + "/A/B");
	const int32_t n_A_TWDIdx = oTempFileTreeFixture.findToWatchPath(oFofiModel, sBasePath + "/A");

	EXPECT_TRUE(n_AB_TWDIdx >= 0);
	EXPECT_TRUE(n_A_TWDIdx >= 0);

	oTempFileTreeFixture.removeRelFile("A/B/xx1.txt");
	{
	INotifierSource::FofiData oFD;
	oFD.m_nTag = n_AB_TWDIdx;
	oFD.m_sName = "xx1.txt";
	oFD.m_eAction = INotifierSource::FOFI_ACTION_DELETE;
	p0Source->callback(oFD);
	}

	oTempFileTreeFixture.createOrModifyRelFile("A/B/xx1.txt");
	// missing action FOFI_ACTION_CREATE

	oTempFileTreeFixture.removeRelFile("A/B/xx1.txt");
	{
	INotifierSource::FofiData oFD;
	oFD.m_nTag = n_AB_TWDIdx;
	oFD.m_sName = "xx1.txt";
	oFD.m_eAction = INotifierSource::FOFI_ACTION_DELETE;
	p0Source->callback(oFD);
	}

	oFofiModel.stop();

	EXPECT_TRUE(oFofiModel.hasInconsistencies());

	const auto& aResults = oFofiModel.getWatchedResults();
	EXPECT_TRUE(aResults.size() == 1);

	const auto& oResult0 = aResults[0];
	EXPECT_TRUE(oResult0.m_aActions.size() == 2);
	EXPECT_TRUE(oResult0.m_aActions[0].m_eAction == INotifierSource::FOFI_ACTION_DELETE);
	EXPECT_TRUE(oResult0.m_aActions[1].m_eAction == INotifierSource::FOFI_ACTION_DELETE);
	EXPECT_TRUE(oResult0.m_eResultType == FofiModel::RESULT_DELETED);
	EXPECT_TRUE(oResult0.m_sName == "xx1.txt");
	EXPECT_TRUE(! oResult0.m_bIsDir);
	EXPECT_TRUE(oResult0.m_bInconsistent);

	return 0;
}

int testModifyDeletedFile()
{
	TempFileTreeFixture oTempFileTreeFixture{};
	const auto& sBasePath = oTempFileTreeFixture.m_sTestBasePath;
	//const auto aBaseSplitPath = testing::splitAbsolutePath(sBasePath);

//std::cout << "sBasePath=" << sBasePath << '\n';
	oTempFileTreeFixture.createOrModifyRelFile("A/B/xx1.txt");

	FofiModel oFofiModel(std::make_unique<FakeSource>(0), 1000000, 1000000, false);
	FakeSource* p0Source = static_cast<FakeSource*>(oFofiModel.getSource());

	FofiModel::DirectoryZone oDZ1;
	oDZ1.m_sPath = sBasePath;
	oDZ1.m_nMaxDepth = 3;
	auto sErr = oFofiModel.addDirectoryZone(std::move(oDZ1));
	EXPECT_TRUE(sErr.empty());

	// start watching
	oFofiModel.start();

	//const auto& aTWDs = oFofiModel.getToWatchDirectories();
	const int32_t n_AB_TWDIdx = oTempFileTreeFixture.findToWatchPath(oFofiModel, sBasePath + "/A/B");
	const int32_t n_A_TWDIdx = oTempFileTreeFixture.findToWatchPath(oFofiModel, sBasePath + "/A");

	EXPECT_TRUE(n_AB_TWDIdx >= 0);
	EXPECT_TRUE(n_A_TWDIdx >= 0);

	oTempFileTreeFixture.removeRelFile("A/B/xx1.txt");
	{
	INotifierSource::FofiData oFD;
	oFD.m_nTag = n_AB_TWDIdx;
	oFD.m_sName = "xx1.txt";
	oFD.m_eAction = INotifierSource::FOFI_ACTION_DELETE;
	p0Source->callback(oFD);
	}

	oTempFileTreeFixture.createOrModifyRelFile("A/B/xx1.txt");
	// missing action FOFI_ACTION_CREATE

	oTempFileTreeFixture.createOrModifyRelFile("A/B/xx1.txt");
	{
	INotifierSource::FofiData oFD;
	oFD.m_nTag = n_AB_TWDIdx;
	oFD.m_sName = "xx1.txt";
	oFD.m_eAction = INotifierSource::FOFI_ACTION_MODIFY;
	p0Source->callback(oFD);
	}

	oFofiModel.stop();

	EXPECT_TRUE(oFofiModel.hasInconsistencies());

	const auto& aResults = oFofiModel.getWatchedResults();
	EXPECT_TRUE(aResults.size() == 1);

	const auto& oResult0 = aResults[0];
	EXPECT_TRUE(oResult0.m_aActions.size() == 2);
	EXPECT_TRUE(oResult0.m_aActions[0].m_eAction == INotifierSource::FOFI_ACTION_DELETE);
	EXPECT_TRUE(oResult0.m_aActions[1].m_eAction == INotifierSource::FOFI_ACTION_MODIFY);
	EXPECT_TRUE(oResult0.m_eResultType == FofiModel::RESULT_MODIFIED);
	EXPECT_TRUE(oResult0.m_sName == "xx1.txt");
	EXPECT_TRUE(! oResult0.m_bIsDir);
	EXPECT_TRUE(oResult0.m_bInconsistent);

	return 0;
}

} // namespace testing
} // namespace fofi

int main(int /*argc*/, char** /*argv*/)
{
	std::cout << "FofiModel Tests:" << '\n';

	EXECUTE_TEST(fofi::testing::testMissingDeleteDir());
	EXECUTE_TEST(fofi::testing::testCreatePresentFile());
	EXECUTE_TEST(fofi::testing::testDeleteDeletedFile());
	EXECUTE_TEST(fofi::testing::testModifyDeletedFile());
	//
	std::cout << "FofiModel Tests successful!" << '\n';
	return 0;
}
