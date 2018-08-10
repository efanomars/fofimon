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
 * File:   config.h
 */

#ifndef FOFIMON_CONFIG_H_
#define FOFIMON_CONFIG_H_

#include <string>

namespace fofi
{

namespace Config
{

const std::string& getVersionString();

const std::string& getDataDir();

} // namespace Config

} // namespace fofi

#endif /* FOFIMON_CONFIG_H_ */

