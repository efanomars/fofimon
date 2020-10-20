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
 * File:   evalargs.h
 */

#ifndef FOFIMON_EVAL_ARGS_H_
#define FOFIMON_EVAL_ARGS_H_

#include <string>

#include <stdint.h>

namespace fofi
{

void evalBoolArg(int& nArgC, char**& aArgV, const std::string& sOption1, const std::string& sOption2
				, std::string& sMatch, bool& bVar) noexcept;
bool evalIntArg(int& nArgC, char**& aArgV, const std::string& sOption1, const std::string& sOption2
				, std::string& sMatch, int32_t& nVar, int32_t nMin) noexcept;
bool evalPathNameArg(int& nArgC, char**& aArgV, bool bName, const std::string& sOption1, const std::string& sOption2
					, bool bPathMandatory, std::string& sMatch, std::string& sPath) noexcept;

} // namespace fofi

#endif /* FOFIMON_EVAL_ARGS_H_ */

