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
 * File:   testingcommon.h
 */

#include <iostream>

/* from the fit library (github.com/pfultz2/Fit) */
#define EXPECT_TRUE(...) if (!(__VA_ARGS__)) { std::cout << "***FAILED:  EXPECT_TRUE(" << #__VA_ARGS__ << ")\n     File: " << __FILE__ << ": " << __LINE__ << '\n'; return 1; }

#define EXECUTE_TEST(...) if ((__VA_ARGS__) != 0) { return 1; } else { std::cout << "ok " << #__VA_ARGS__ << '\n'; }
