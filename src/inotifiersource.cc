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
 * File:   inotifiersource.cc
 */

#include "inotifiersource.h"

#ifndef NDEBUG
//#include <iostream>
#endif //NDEBUG
#include <cmath>
#include <limits>
#include <algorithm>
#include <cassert>
#include <iterator>
#include <stdexcept>
#include <array>

#include <sys/inotify.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

namespace fofi
{

constexpr int32_t INotifierSource::EXTENDED_ERRNO_FAKE_FS; // = 0x10000000;
constexpr int32_t INotifierSource::EXTENDED_ERRNO_WATCH_NOT_FOUND; // = 0x20000000;

const char* INotifierSource::s_sSystemMaxUserWatchesFile = "/proc/sys/fs/inotify/max_user_watches";

int32_t INotifierSource::getSystemMaxUserWatches() noexcept
{
	const auto nFD = ::open(s_sSystemMaxUserWatchesFile, O_RDONLY);
	if (nFD < 0) {
		return -1;
	}
	int32_t nMaxWatches = -1;
	constexpr int32_t nBufSize = 30;
	char aBuf[nBufSize];
	const auto nRead = ::read(nFD, aBuf, nBufSize);
	if (nRead >= 0) {
		try {
			double fMaxWatches = Glib::Ascii::strtod(aBuf);
			nMaxWatches = std::min(std::ceil(fMaxWatches), static_cast<double>(std::numeric_limits<int32_t>::max()));
		} catch (const std::runtime_error& oErr) {
		}
	}
	::close(nFD);
	return nMaxWatches;
}

INotifierSource::INotifierSource(int32_t nReserveSize) noexcept
: Glib::Source()
, m_nINotifyFD(-1)
{
	static_assert(sizeof(int) <= sizeof(int32_t), "");
	static_assert(false == FALSE, "");
	static_assert(true == TRUE, "");
	static_assert(s_nBufferSize >= sizeof(struct inotify_event) + NAME_MAX + 1, "");
	assert(nReserveSize >= 0);

	m_aWatchItems.reserve(nReserveSize);

	m_aFreeWatchIdxs.reserve(1000);
}
INotifierSource::~INotifierSource() noexcept
{
	if (m_nINotifyFD != -1) {
		::close(m_nINotifyFD);
	}
//std::cout << "INotifierSource::~INotifierSource()" << '\n';
}
void INotifierSource::attach_override() noexcept
{
	m_nINotifyFD = ::inotify_init1(IN_NONBLOCK);
//std::cout << "INotifierSource::INotifierSource()  m_nINotifyFD=" << m_nINotifyFD << '\n';
	if (m_nINotifyFD == -1) {
		return; //--------------------------------------------------------------
	}

	m_oINotifyPollFD.set_fd(m_nINotifyFD);
	m_oINotifyPollFD.set_events(Glib::IO_IN);
	add_poll(m_oINotifyPollFD);

	// priority higher than normal
	set_priority( (Glib::PRIORITY_DEFAULT > Glib::PRIORITY_LOW) ? Glib::PRIORITY_DEFAULT + 1 : Glib::PRIORITY_DEFAULT - 1);
	set_can_recurse(false);

	Glib::Source::attach();
}

int32_t INotifierSource::findEntryByTag(int32_t nTag) const noexcept
{
	const auto itFind = std::find_if(m_aWatchItems.begin(), m_aWatchItems.end(), [&](const WatchItem& oWI)
	{
		return (nTag == oWI.m_nTag);
	});
	if (itFind == m_aWatchItems.end()) {
		return -1;
	}
	return static_cast<int32_t>(std::distance(m_aWatchItems.begin(), itFind));
}
int32_t INotifierSource::findEntryByWatch(int32_t nWatchDescriptor) const noexcept
{
	const auto itFind = std::find_if(m_aWatchItems.begin(), m_aWatchItems.end(), [&](const WatchItem& oWI)
	{
		return (nWatchDescriptor == oWI.m_nDescriptor);
	});
	if (itFind == m_aWatchItems.end()) {
		return -1;
	}
	return static_cast<int32_t>(std::distance(m_aWatchItems.begin(), itFind));
}
template <int32_t N>
bool startsWithAnyOf(const std::string& sPath, std::array<const char*, N>& aStarters) noexcept
{
	const auto nTotStarters = static_cast<int32_t>(aStarters.size());
	for (int32_t nIdx = 0; nIdx < nTotStarters; ++nIdx) {
		const char* p0Str = aStarters[nIdx];
		assert(p0Str != nullptr);
		const int32_t nSize = ::strlen(p0Str);
		if (sPath.substr(0, nSize) == p0Str) {
			return true; //-----------------------------------------------------
		}
	}
	return false;
}
std::array<const char*, 3> s_aForbiddenPaths{{"/proc", "/sys", "/dev/pts"}};

std::vector<std::string> INotifierSource::invalidPaths() noexcept
{
	std::vector<std::string> aInvalidPaths;
	const auto nTotStarters = static_cast<int32_t>(s_aForbiddenPaths.size());
	for (int32_t nIdx = 0; nIdx < nTotStarters; ++nIdx) {
		const char* p0Str = s_aForbiddenPaths[nIdx];
		assert(p0Str != nullptr);
		aInvalidPaths.push_back(p0Str);
	}
	return aInvalidPaths;
}
std::pair<int32_t, int32_t> INotifierSource::addPath(const std::string& sPath, int32_t nTag) noexcept
{
//std::cout << "INotifierSource::addPath  sPath=" << sPath << "  nTag=" << nTag << '\n';
	assert(Glib::path_is_absolute(sPath));
	if (startsWithAnyOf<3>(sPath, s_aForbiddenPaths)) {
		return std::make_pair(EXTENDED_ERRNO_FAKE_FS, -1); //-------------------
	}

	const int32_t nWatchFD = ::inotify_add_watch(m_nINotifyFD, sPath.c_str()
									, IN_CREATE | IN_MOVED_TO
									| IN_DELETE | IN_MOVED_FROM
									| IN_CLOSE_WRITE | IN_ATTRIB
									| IN_DONT_FOLLOW | IN_EXCL_UNLINK | IN_ONLYDIR);
	if (nWatchFD == -1) {
		return std::make_pair(errno, -1); //------------------------------------
	}
	assert(-1 == findEntryByWatch(nWatchFD));
	int32_t nWatchIdx;
	if (m_aFreeWatchIdxs.empty()) {
		nWatchIdx = static_cast<int32_t>(m_aWatchItems.size());
		WatchItem oWI;
		oWI.m_nDescriptor = nWatchFD;
		oWI.m_nTag = nTag;
		m_aWatchItems.push_back(oWI);
	} else {
		nWatchIdx = m_aFreeWatchIdxs.back();
		m_aFreeWatchIdxs.pop_back();
		WatchItem& oWI = m_aWatchItems[nWatchIdx];
		oWI.m_nDescriptor = nWatchFD;
		oWI.m_nTag = nTag;
	}
	return std::make_pair(0, nWatchIdx);
}
int32_t INotifierSource::clearAll() noexcept
{
	int32_t nErrno = 0;
	for (const WatchItem& oWI : m_aWatchItems) {
		const auto nRet = ::inotify_rm_watch(m_nINotifyFD, oWI.m_nDescriptor);
		if ((nRet == -1) && (nErrno == 0)) {
			nErrno = errno;
		}
	}
	m_aWatchItems.clear();
	m_aFreeWatchIdxs.clear();
	return nErrno;
}
int32_t INotifierSource::removePath(int32_t nTag) noexcept
{
	return removePath(-1, nTag);
}
int32_t INotifierSource::removePath(int32_t nWatchIdx, int32_t nTag) noexcept
{
	assert(nWatchIdx >= -1);
	#ifndef NDEBUG
	const int32_t nTotWatches = static_cast<int32_t>(m_aWatchItems.size());
	#endif //NDEBUG
	assert(nWatchIdx < nTotWatches);
	if (nWatchIdx < 0) {
		nWatchIdx = findEntryByTag(nTag);
		if (nWatchIdx < 0) {
			return EXTENDED_ERRNO_WATCH_NOT_FOUND; //---------------------------
		}
	} else {
		assert(m_aWatchItems[nWatchIdx].m_nTag == nTag);
	}
	const int32_t nWatchFD = m_aWatchItems[nWatchIdx].m_nDescriptor;
	int32_t nErrno = 0;
	const auto nRet = ::inotify_rm_watch(m_nINotifyFD, nWatchFD);
	if (nRet == -1) {
		nErrno = errno;
	}
	m_aWatchItems[nWatchIdx].m_nDescriptor = -1;
	m_aWatchItems[nWatchIdx].m_nTag = -1;
	m_aFreeWatchIdxs.push_back(nWatchIdx);
	return nErrno;
}
int32_t INotifierSource::renamePath(int32_t nFromTag, int32_t nToTag) noexcept
{
	return renamePath(-1, nFromTag, nToTag);
}
int32_t INotifierSource::renamePath(int32_t nFromWatchIdx, int32_t nFromTag, int32_t nToTag) noexcept
{
	assert(nFromWatchIdx >= -1);
	#ifndef NDEBUG
	const int32_t nTotWatches = static_cast<int32_t>(m_aWatchItems.size());
	#endif //NDEBUG
	assert(nFromWatchIdx < nTotWatches);
	if (nFromWatchIdx < 0) {
		nFromWatchIdx = findEntryByTag(nFromTag);
		if (nFromWatchIdx < 0) {
			return EXTENDED_ERRNO_WATCH_NOT_FOUND; //---------------------------
		}
	} else {
		assert(m_aWatchItems[nFromWatchIdx].m_nTag == nFromTag);
	}
	m_aWatchItems[nFromWatchIdx].m_nTag = nToTag;
	return 0;
}
sigc::connection INotifierSource::connect(const sigc::slot<FOFI_PROGRESS, const FofiData&>& oSlot) noexcept
{
	if (m_nINotifyFD == -1) {
		// File error, return an empty connection
		return sigc::connection();
	}
	return connect_generic(oSlot);
}

bool INotifierSource::prepare(int& nTimeout) noexcept
{
	nTimeout = -1;

	return false;
}
bool INotifierSource::check() noexcept
{
	bool bRet = false;

	if ((m_oINotifyPollFD.get_revents() & Glib::IO_IN) != 0) {
		bRet = true;
	}

	return bRet;
}
bool INotifierSource::dispatch(sigc::slot_base* p0Slot) noexcept
{
//std::cout << "INotifierSource::dispatch" << '\n';
	bool bContinue = true;

	if (p0Slot == nullptr) {
		return bContinue; //----------------------------------------------------
	}

	const ssize_t nLen = ::read(m_oINotifyPollFD.get_fd(), m_aBuffer, s_nBufferSize);
	if ((nLen == -1) && (errno != EAGAIN)) {
		return bContinue; //----------------------------------------------------
	}
	// If the nonblocking read() found no events to read, then
	// it returns -1 with errno set to EAGAIN. In that case,
	// we exit the loop.
	if (nLen <= 0) {
		return bContinue; //----------------------------------------------------
	}


	struct inotify_event* p0Event = nullptr;
	char* p0Cur = m_aBuffer;
	for (; p0Cur < (m_aBuffer + nLen); p0Cur += sizeof(struct inotify_event) + p0Event->len) {
		p0Event = reinterpret_cast<struct inotify_event *>(p0Cur);
		const int32_t nMask = p0Event->mask;
		const int32_t nWatchFD = p0Event->wd;
		const int32_t nWatchIdx = findEntryByWatch(nWatchFD);
		if (nWatchIdx < 0) {
			// event's watch from directory or file already removed
			continue; // for(p0Event --------
		}
		FOFI_ACTION eAction;
		const int32_t nActionMask = (nMask & (IN_CREATE | IN_DELETE | IN_CLOSE_WRITE | IN_ATTRIB | IN_MOVED_FROM | IN_MOVED_TO));
		switch (nActionMask) {
		case IN_CREATE:
			{
				eAction = FOFI_ACTION_CREATE;
			}
			break;
		case IN_DELETE:
			{
				eAction = FOFI_ACTION_DELETE;
			}
			break;
		//case IN_DELETE_SELF:
		//	{
		//		eAction = FOFI_ACTION_DELETE_SELF;
		//	}
		//	break;
		case IN_CLOSE_WRITE:
			{
				eAction = FOFI_ACTION_MODIFY;
			}
			break;
		case IN_ATTRIB:
			{
				eAction = FOFI_ACTION_ATTRIB;
			}
			break;
		case IN_MOVED_FROM:
			{
				eAction = FOFI_ACTION_RENAME_FROM;
			}
			break;
		case IN_MOVED_TO:
			{
				eAction = FOFI_ACTION_RENAME_TO;
			}
			break;
		default:
			{
				eAction = FOFI_ACTION_INVALID;
			}
			break;
		}
		if ((nActionMask > 0) && (eAction != FOFI_ACTION_INVALID)) {
			FofiData oData;
			oData.m_bOverflow = (nMask & IN_Q_OVERFLOW);
			oData.m_bIsDir = (nMask & IN_ISDIR);
			oData.m_eAction = eAction;
			oData.m_nRenameCookie = p0Event->cookie;
			oData.m_sName = ((p0Event->len > 0) ? std::string{p0Event->name} : "");
			oData.m_nTag = m_aWatchItems[nWatchIdx].m_nTag;
			FOFI_PROGRESS eProg = (*static_cast<sigc::slot<FOFI_PROGRESS, const FofiData&>*>(p0Slot))(oData);
			bContinue = (eProg == FOFI_PROGRESS_CONTINUE);
		}
	}
	assert(p0Cur == (m_aBuffer + nLen));
	return bContinue;
}


} // namespace fofi
