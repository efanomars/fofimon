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
 * File:   testUtil01.cxx
 */

#include "util.h"

#include "testingcommon.h"
#include "tempfiletreefixture.h"

#include <iostream>
#include <cassert>

namespace fofi
{
namespace testing
{

using std::shared_ptr;
using std::weak_ptr;

int testRealPath()
{
	TempFileTreeFixture oTempFileTreeFixture{};
	const auto& sBasePath = oTempFileTreeFixture.m_sTestBasePath;
//std::cout << "sBasePath=" << sBasePath << '\n';
	oTempFileTreeFixture.createOrModifyRelFile("A/B1/xx1.txt");
	oTempFileTreeFixture.createOrModifyRelFile("A/B2/xx2.txt");
	oTempFileTreeFixture.makeRelSymlink("T1", false, "A/B2");
	oTempFileTreeFixture.makeRelSymlink("T2", true, "A");

//std::cout << "------------>" << Util::getRealPathIfPossible("") << '\n';
	EXPECT_TRUE(Util::getRealPathIfPossible(sBasePath + "/A/B1/C") == sBasePath + "/A/B1/C");
	EXPECT_TRUE(Util::getRealPathIfPossible(sBasePath + "/A/../X") == sBasePath + "/X");
//std::cout << "------------>" << Util::getRealPathIfPossible(sBasePath + "/T/xx2.txt") << '\n';
	EXPECT_TRUE(Util::getRealPathIfPossible(sBasePath + "/T1/xx2.txt") == sBasePath + "/A/B2/xx2.txt");
	EXPECT_TRUE(Util::getRealPathIfPossible(sBasePath + "/T2/boh") == sBasePath + "/A/boh");

	return 0;
}

int testFileStat()
{
	TempFileTreeFixture oTempFileTreeFixture{};
	const auto& sBasePath = oTempFileTreeFixture.m_sTestBasePath;
//std::cout << "sBasePath=" << sBasePath << '\n';
	oTempFileTreeFixture.createOrModifyRelFile("A/B1/xx1.txt");
	oTempFileTreeFixture.createOrModifyRelFile("A/B2/xx2.txt");
	oTempFileTreeFixture.makeRelSymlink("T1", false, "A/B2");
	oTempFileTreeFixture.makeRelSymlink("yy1.txt", false, "A/B1/xx1.txt");
	oTempFileTreeFixture.makeRelSymlink("yy2.txt", false, "A/B2/xx2.txt");
	oTempFileTreeFixture.removeRelFile("A/B2/xx2.txt");

	{
	const auto oFStat = Util::FileStat::create(sBasePath + "/A/B1");
	EXPECT_TRUE(oFStat.exists());
	EXPECT_TRUE(oFStat.isDir());
	EXPECT_TRUE(! oFStat.isRegular());
	EXPECT_TRUE(! oFStat.isSymLink());
	}

	{
	const auto oFStat = Util::FileStat::create(sBasePath + "/T1");
	EXPECT_TRUE(oFStat.exists());
	EXPECT_TRUE(! oFStat.isDir());
	EXPECT_TRUE(oFStat.isRegular());
	EXPECT_TRUE(oFStat.isSymLink());
	}

	{
	const auto oFStat = Util::FileStat::create(sBasePath + "/A/B1/xx1.txt");
	EXPECT_TRUE(oFStat.exists());
	EXPECT_TRUE(! oFStat.isDir());
	EXPECT_TRUE(oFStat.isRegular());
	EXPECT_TRUE(! oFStat.isSymLink());
	}
	{
	const auto oFStat = Util::FileStat::create(sBasePath + "A/B2/xx2.txt");
	EXPECT_TRUE(! oFStat.exists());
	}

	{
	const auto oFStat = Util::FileStat::create(sBasePath + "/yy1.txt");
	EXPECT_TRUE(oFStat.exists());
	EXPECT_TRUE(! oFStat.isDir());
	EXPECT_TRUE(oFStat.isRegular());
	EXPECT_TRUE(oFStat.isSymLink());
	}

	{ // dangling
	const auto oFStat = Util::FileStat::create(sBasePath + "/yy2.txt");
	EXPECT_TRUE(oFStat.exists());
	EXPECT_TRUE(! oFStat.isDir());
	EXPECT_TRUE(oFStat.isRegular());
	EXPECT_TRUE(oFStat.isSymLink());
	}

	return 0;
}

} // namespace testing
} // namespace fofi

int main(int /*argc*/, char** /*argv*/)
{
	std::cout << "Util01 Tests:" << '\n';

	EXECUTE_TEST(fofi::testing::testRealPath());
	EXECUTE_TEST(fofi::testing::testFileStat());
	//
	std::cout << "Util01 Tests successful!" << '\n';
	return 0;
}
