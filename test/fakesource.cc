/*
 * Copyright © 2018  Stefano Marsili, <stemars@gmx.ch>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>
 */
/*
 * File:   fakesource.cc
 */

#include "fakesource.h"

#ifndef NDEBUG
#include <iostream>
#endif //NDEBUG
#include <cmath>
#include <limits>
#include <algorithm>

namespace fofi
{

namespace testing
{

FakeSource::FakeSource(int32_t nReserveSize)
: INotifierSource(nReserveSize)
{
	assert(nReserveSize >= 0);

	m_aPathWatchTags.reserve(nReserveSize);
}
FakeSource::~FakeSource()
{
//std::cout << "FakeSource::~FakeSource()" << '\n';
}
void FakeSource::attach_override()
{
}

int32_t FakeSource::findEntryByTag(int32_t nTag)
{
	const auto itFind = std::find(m_aPathWatchTags.begin(), m_aPathWatchTags.end(), nTag);
	if (itFind == m_aPathWatchTags.end()) {
		return -1;
	}
	return static_cast<int32_t>(std::distance(m_aPathWatchTags.begin(), itFind));
}
bool startsWithAnyOf(const std::string& sPath, const std::vector<std::string>& aStarters)
{
	for (const auto& sStart : aStarters) {
		if (sPath.substr(0, sStart.size()) == sStart) {
			return true; //-----------------------------------------------------
		}
	}
	return false;
}
std::vector<std::string> FakeSource::invalidPaths()
{
	return m_aInvalidPaths;
}
std::pair<int32_t, int32_t> FakeSource::addPath(const std::string& sPath, int32_t nTag)
{
//std::cout << "FakeSource::addPath  sPath=" << sPath << "  nTag=" << nTag << '\n';
	assert(Glib::path_is_absolute(sPath));
	if (startsWithAnyOf(sPath, m_aInvalidPaths)) {
		return std::make_pair(EXTENDED_ERRNO_FAKE_FS, -1); //-------------------
	}

	int32_t nWatchIdx;
	if (m_aFreeWatchIdxs.empty()) {
		nWatchIdx = static_cast<int32_t>(m_aPathWatchTags.size());
		m_aPathWatchTags.push_back(nTag);
	} else {
		nWatchIdx = m_aFreeWatchIdxs.back();
		m_aFreeWatchIdxs.pop_back();
		m_aPathWatchTags[nWatchIdx] = nTag;
	}
	return std::make_pair(0, nWatchIdx);
}
int32_t FakeSource::clearAll()
{
	m_aPathWatchTags.clear();
	m_aFreeWatchIdxs.clear();
	return 0;
}
int32_t FakeSource::removePath(int32_t nTag)
{
	return removePath(-1, nTag);
}
int32_t FakeSource::removePath(int32_t nWatchIdx, int32_t nTag)
{
	assert(nWatchIdx >= -1);
	#ifndef NDEBUG
	const int32_t nTotWatches = static_cast<int32_t>(m_aPathWatchTags.size());
	#endif //NDEBUG
	assert(nWatchIdx < nTotWatches);
	
	if (nWatchIdx < 0) {
		nWatchIdx = findEntryByTag(nTag);
		if (nWatchIdx < 0) {
			return EXTENDED_ERRNO_WATCH_NOT_FOUND; //---------------------------
		}
	} else {
		assert(m_aPathWatchTags[nWatchIdx] == nTag);
	}
	m_aPathWatchTags[nWatchIdx] = -1;
	m_aFreeWatchIdxs.push_back(nWatchIdx);
	return 0;
}
int32_t FakeSource::renamePath(int32_t nFromTag, int32_t nToTag)
{
	return renamePath(-1, nFromTag, nToTag);
}
int32_t FakeSource::renamePath(int32_t nFromWatchIdx, int32_t nFromTag, int32_t nToTag)
{
	assert(nFromWatchIdx >= -1);
	#ifndef NDEBUG
	const int32_t nTotWatches = static_cast<int32_t>(m_aPathWatchTags.size());
	#endif //NDEBUG
	assert(nFromWatchIdx < nTotWatches);
	
	if (nFromWatchIdx < 0) {
		nFromWatchIdx = findEntryByTag(nFromTag);
		if (nFromWatchIdx < 0) {
			return EXTENDED_ERRNO_WATCH_NOT_FOUND; //---------------------------
		}
	} else {
		assert(m_aPathWatchTags[nFromWatchIdx] == nFromTag);
	}
	m_aPathWatchTags[nFromWatchIdx] = nToTag;
	return 0;
}
sigc::connection FakeSource::connect(const sigc::slot<FOFI_PROGRESS, const FofiData&>& oSlot)
{
	assert(! oSlot.empty());
	return m_oFofiDataCallback.connect(oSlot);
}
INotifierSource::FOFI_PROGRESS FakeSource::callback(const FofiData& oData)
{
	assert(!m_oFofiDataCallback.empty());
	return m_oFofiDataCallback.emit(oData);
}

} // namespace testing

} // namespace fofi
