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
 * File:   printout.h
 */

#ifndef FOFIMON_PRINT_OUT_H_
#define FOFIMON_PRINT_OUT_H_

#include "../fofimodel.h"

#include <fstream>

#include <stdint.h>

namespace fofi
{

void printZones(std::ostream& oOut, const FofiModel& oFofiModel) noexcept;
void printZonesJSon(std::ostream& oOut, const FofiModel& oFofiModel) noexcept;

void printToWatchDirs(std::ostream& oOut, const FofiModel& oFofiModel, bool bDontWatch) noexcept;
void printToWatchDirsJSon(std::ostream& oOut, const FofiModel& oFofiModel, bool bDontWatch) noexcept;

void printResult(std::ostream& oOut, const FofiModel::WatchedResult& oResult, bool bDetail, int64_t nDurationUsec) noexcept;
void printResultJSon(std::ostream& oOut, const FofiModel::WatchedResult& oResult, bool bDetail, int64_t nDurationUsec) noexcept;

void printLiveAction(std::ostream& oOut, const FofiModel::WatchedResult& oResult, bool bDetail) noexcept;

} // namespace fofi

#endif /* FOFIMON_PRINT_OUT_H_ */

