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
 * File:   testUtil.cxx
 */

#include "util.h"

#include "testingcommon.h"

#include <iostream>
#include <cassert>

namespace fofi
{
namespace testing
{

using std::shared_ptr;
using std::weak_ptr;

int testGetTimeString()
{
	EXPECT_TRUE(Util::getTimeString(0, 0) ==         "0.000000");
	EXPECT_TRUE(Util::getTimeString(0, 100000) ==    "0.000000");
	EXPECT_TRUE(Util::getTimeString(0, 1000000) ==   "0.000000");
	EXPECT_TRUE(Util::getTimeString(0, 9999999) ==   "0.000000");
	EXPECT_TRUE(Util::getTimeString(0, 10000000) == " 0.000000");
	//
	EXPECT_TRUE(Util::getTimeString(1000000, 0) ==         "1.000000");
	EXPECT_TRUE(Util::getTimeString(1000000, 100000) ==    "1.000000");
	EXPECT_TRUE(Util::getTimeString(1000000, 1000000) ==   "1.000000");
	EXPECT_TRUE(Util::getTimeString(1000000, 9999999) ==   "1.000000");
	EXPECT_TRUE(Util::getTimeString(1000000, 10000000) == " 1.000000");
	//
	EXPECT_TRUE(Util::getTimeString(0, 100000000) ==         "  0.000000");
	EXPECT_TRUE(Util::getTimeString(999999, 100000000) ==    "  0.999999");
	EXPECT_TRUE(Util::getTimeString(9999999, 100000000) ==   "  9.999999");
	EXPECT_TRUE(Util::getTimeString(100000000, 100000000) == "100.000000");
	//
	return 0;
}

int testAddValueToVectorUniquely()
{
	std::vector<int32_t> aV{1,3,5,7};
	Util::addValueToVectorUniquely(aV, 5);
	const std::vector<int32_t> aVRes{1,3,5,7};
	EXPECT_TRUE(aV == aVRes);

	Util::addValueToVectorUniquely(aV, 2);
	const std::vector<int32_t> aVRes2{1,3,5,7, 2};
	EXPECT_TRUE(aV == aVRes2);

	return 0;
}
int testAddVectorToVectorUniquely()
{
	std::vector<int32_t> aV{1,3,5,7};
	const std::vector<int32_t> aVAdd{1,4,6,7};
	Util::addVectorToVectorUniquely(aV, aVAdd);
	const std::vector<int32_t> aVRes{1,3,5,7, 4,6};
	EXPECT_TRUE(aV == aVRes);

	return 0;
}
int testAddValueToDequeUniquely()
{
	std::deque<int32_t> aV{1,3,5,7};
	Util::addValueToDequeUniquely(aV, 5);
	const std::deque<int32_t> aVRes{1,3,5,7};
	EXPECT_TRUE(aV == aVRes);

	Util::addValueToDequeUniquely(aV, 2);
	const std::deque<int32_t> aVRes2{1,3,5,7, 2};
	EXPECT_TRUE(aV == aVRes2);

	return 0;
}

int testCleanupPath()
{
	EXPECT_TRUE(Util::cleanupPath("") == "");
	EXPECT_TRUE(Util::cleanupPath("/") == "/");
	EXPECT_TRUE(Util::cleanupPath("///") == "/");
	EXPECT_TRUE(Util::cleanupPath(".") == ".");
	EXPECT_TRUE(Util::cleanupPath("..") == "..");
	EXPECT_TRUE(Util::cleanupPath("...") == "../..");
	EXPECT_TRUE(Util::cleanupPath("A...") == "A...");
	EXPECT_TRUE(Util::cleanupPath("/A/") == "/A");
	EXPECT_TRUE(Util::cleanupPath("/A//B") == "/A/B");
	EXPECT_TRUE(Util::cleanupPath("A//B") == "A/B");
	EXPECT_TRUE(Util::cleanupPath("//.././B/..") == "/.././B/..");
	EXPECT_TRUE(Util::cleanupPath("AAA") == "AAA");
	EXPECT_TRUE(Util::cleanupPath("/AAA.A/./BB//CC//") == "/AAA.A/./BB/CC");
	return 0;
}
int testPathDirName()
{
	EXPECT_TRUE(Util::getPathDirName("").first == false);
	EXPECT_TRUE(Util::getPathDirName("AAA").first == false);
	EXPECT_TRUE(Util::getPathDirName("/AAA").first == true);
	EXPECT_TRUE(Util::getPathDirName("/AAA").second == "/");
	EXPECT_TRUE(Util::getPathDirName("AAA/BBB").first == true);
	EXPECT_TRUE(Util::getPathDirName("AAA/BBB").second == "AAA");
	return 0;
}
int testPathFileName()
{
	EXPECT_TRUE(Util::getPathFileName("").first == false);
	EXPECT_TRUE(Util::getPathFileName("AAA").first == true);
	EXPECT_TRUE(Util::getPathFileName("AAA").second == "AAA");
	EXPECT_TRUE(Util::getPathFileName("/AAA").first == true);
	EXPECT_TRUE(Util::getPathFileName("/AAA").second == "AAA");
	EXPECT_TRUE(Util::getPathFileName("AAA/BBB").first == true);
	EXPECT_TRUE(Util::getPathFileName("AAA/BBB").second == "BBB");
	return 0;
}
int testPathDepth()
{
	EXPECT_TRUE(Util::getPathDepth("/A/BB/CCC", "/A/BB", 2) == 1);
	EXPECT_TRUE(Util::getPathDepth("/A/BB/CCC", "/a/BB", 2) == -1);
	EXPECT_TRUE(Util::getPathDepth("/A/BB/CCC", "/A/BB", 0) == -1);
	EXPECT_TRUE(Util::getPathDepth("/", "/", 2) == 0);
	EXPECT_TRUE(Util::getPathDepth("/AA", "/", 2) == 1);
	EXPECT_TRUE(Util::getPathDepth("/AA/B", "/", 2) == 2);
	return 0;
}

} // namespace testing
} // namespace fofi

int main(int /*argc*/, char** /*argv*/)
{
	std::cout << "Util Tests:" << '\n';

	EXECUTE_TEST(fofi::testing::testGetTimeString());
	EXECUTE_TEST(fofi::testing::testAddValueToVectorUniquely());
	EXECUTE_TEST(fofi::testing::testAddVectorToVectorUniquely());
	EXECUTE_TEST(fofi::testing::testAddValueToDequeUniquely());
	EXECUTE_TEST(fofi::testing::testCleanupPath());
	EXECUTE_TEST(fofi::testing::testPathDirName());
	EXECUTE_TEST(fofi::testing::testPathFileName());
	EXECUTE_TEST(fofi::testing::testPathDepth());
	//
	std::cout << "Util Tests successful!" << '\n';
	return 0;
}
