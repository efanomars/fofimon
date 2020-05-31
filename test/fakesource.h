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
 * File:   fakesource.h
 */

#ifndef FOFIMON_FAKE_SOURCE_H_
#define FOFIMON_FAKE_SOURCE_H_

#include "inotifiersource.h"

#include <cassert>
#include <vector>
#include <string>
#include <memory>


namespace fofi
{

using std::unique_ptr;

namespace testing
{

class FakeSource : public INotifierSource
{
public:
	std::vector<std::string> m_aInvalidPaths;
public:
	explicit FakeSource(int32_t nReserveSize) noexcept;
	virtual ~FakeSource() noexcept;

	void attach_override() noexcept override;

	std::vector<std::string> invalidPaths() noexcept override;
	std::pair<int32_t, int32_t> addPath(const std::string& sPath, int32_t nTag) noexcept override;
	int32_t removePath(int32_t nTag) noexcept override;
	int32_t removePath(int32_t nWatchIdx, int32_t nTag) noexcept override;
	int32_t renamePath(int32_t nFromTag, int32_t nToTag) noexcept override;
	int32_t renamePath(int32_t nFromWatchIdx, int32_t nFromTag, int32_t nToTag) noexcept override;
	int32_t clearAll() noexcept override;

	sigc::connection connect(const sigc::slot<FOFI_PROGRESS, const FofiData&>& oSlot) noexcept override;

	FOFI_PROGRESS callback(const FofiData& oData) noexcept;
private:
	// returns -1 or the index into m_aPathWatches and m_aWatchDescriptors
	int32_t findEntryByTag(int32_t nTag) noexcept;

private:
	sigc::signal<FOFI_PROGRESS, const FofiData&> m_oFofiDataCallback;
	//
	std::vector<int32_t> m_aPathWatchTags;
	//
	std::vector<int32_t> m_aFreeWatchIdxs;
	//
private:
	FakeSource(const FakeSource& oSource) = delete;
	FakeSource& operator=(const FakeSource& oSource) = delete;
};

} // namespace testing

} // namespace fofi

#endif /* FOFIMON_FAKE_SOURCE_H_ */
