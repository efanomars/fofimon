/*
 * Copyright © 2018-2019  Stefano Marsili, <stemars@gmx.ch>
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
 * File:   evalargs.cc
 */

#include "evalargs.h"

#include <glibmm.h>

#include <algorithm>
#include <limits>
#include <stdexcept>

#include <cassert>
#include <iostream>
#include <cmath>

namespace fofi
{

void evalBoolArg(int& nArgC, char**& aArgV, const std::string& sOption1, const std::string& sOption2, std::string& sMatch, bool& bVar) noexcept
{
	sMatch.clear();
	if (aArgV[1] == nullptr) {
		return;
	}
	const bool bIsOption1 = (sOption1 == std::string(aArgV[1]));
	if (bIsOption1 || ((!sOption2.empty()) && (sOption2 == std::string(aArgV[1])))) {
		sMatch = (bIsOption1 ? sOption1 : sOption2);
		bVar = true;
		--nArgC;
		++aArgV;
	}
}
bool evalIntArg(int& nArgC, char**& aArgV, const std::string& sOption1, const std::string& sOption2, std::string& sMatch, int32_t& nVar, int32_t nMin) noexcept
{
	sMatch.clear();
	if (aArgV[1] == nullptr) {
		return true;
	}
	const bool bIsOption1 = (sOption1 == std::string(aArgV[1]));
	if (bIsOption1 || ((!sOption2.empty()) && (sOption2 == std::string(aArgV[1])))) {
		sMatch = (bIsOption1 ? sOption1 : sOption2);
		--nArgC;
		++aArgV;
		if (nArgC == 1) {
			std::cerr << "Error: " << sMatch << " missing argument" << '\n';
			return false; //----------------------------------------------------
		}
		try {
			const double fValue = Glib::Ascii::strtod(aArgV[1]);
			if (fValue < nMin) {
				nVar = nMin;
			} else {
				nVar = std::min(std::ceil(fValue), static_cast<double>(std::numeric_limits<int32_t>::max()));
			}
		} catch (const std::runtime_error& oErr) {
			std::cerr << "Error: " << sMatch << " " << oErr.what() << '\n';
			return false; //----------------------------------------------------
		}
		--nArgC;
		++aArgV;
	}
	return true;
}
bool evalPathNameArg(int& nArgC, char**& aArgV, bool bName, const std::string& sOption1, const std::string& sOption2
					, bool bPathMandatory, std::string& sMatch, std::string& sPath) noexcept
{
	sMatch.clear();
	if (aArgV[1] == nullptr) {
		return true;
	}
	const bool bIsOption1 = (sOption1 == std::string(aArgV[1]));
	if (bIsOption1 || ((!sOption2.empty()) && (sOption2 == std::string(aArgV[1])))) {
		sMatch = (bIsOption1 ? sOption1 : sOption2);
		--nArgC;
		++aArgV;
		if (nArgC == 1) {
			if (bPathMandatory) {
				std::cerr << "Error: " << sMatch << " missing argument" << '\n';
				return false; //------------------------------------------------
			}
			sPath = "";
			return true; //-----------------------------------------------------
		}
		sPath = aArgV[1];
		if (!bPathMandatory) {
			assert(! sPath.empty());
			if (sPath[0] == '-') {
				// it is the next option, not a file
				sPath = "";
				return true; //-------------------------------------------------
			}
		}
		if (bName) {
			if (sPath.find('/') != std::string::npos) {
				std::cerr << "Error: " << sMatch << " argument can't contain '/'" << '\n';
				return false; //--------------------------------------------
			}
		}
		--nArgC;
		++aArgV;
	}
	return true;
}

} // namespace fofi

