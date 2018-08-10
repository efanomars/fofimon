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
 * File:   inotifiersource.h
 */

#ifndef FOFIMON_INOTIFIER_SOURCE_H_
#define FOFIMON_INOTIFIER_SOURCE_H_

#include <glibmm.h>

#include <cassert>
#include <vector>
#include <string>
#include <memory>


namespace fofi
{

using std::unique_ptr;

/* INotify tracking of modified, added and removed files in a folder */
class INotifierSource : public Glib::Source
{
public:
	explicit INotifierSource(int32_t nReserveSize);
	virtual ~INotifierSource();

	#ifdef STMF_TESTING_IFACE
	virtual
	#endif // STMF_TESTING_IFACE
	void attach_override();
	static constexpr int32_t EXTENDED_ERRNO_FAKE_FS = 0x10000000;
	static constexpr int32_t EXTENDED_ERRNO_WATCH_NOT_FOUND = 0x20000000;

	static const char* s_sSystemMaxUserWatchesFile; /**< The file path name containing
														 * the max nr of watches per user. */
	/** The system maximum number of watches per user.
	 * @return The maximum number.
	 */
	static int32_t getSystemMaxUserWatches();
	/** The paths that should b automatically be excluded.
	 * If passed to addPath() will make it fail with error cause EXTENDED_ERRNO_FAKE_FS.
	 * @return The forbidden paths.
	 */
	#ifdef STMF_TESTING_IFACE
	virtual
	#endif // STMF_TESTING_IFACE
	std::vector<std::string> invalidPaths();
	/** Add a directory to watch.
	 * The return pair contains an error number (0 if no error) and  an index
	 * that can be passed to removePath and renamePath for faster access.
	 * Errors:
	 *     EACCES Read access to the given file is not permitted.
	 *     EFAULT pathname points outside of the process's accessible address space.
	 *     ENAMETOOLONG pathname is too long.
	 *     ENOENT A directory component in pathname does not exist or is a dangling symbolic link.
	 *     ENOMEM Insufficient kernel memory was available.
	 *     ENOSPC The user limit on the total number of inotify watches was reached or the kernel failed.
	 *
	 * @param sPath The path. Cannot be empty.
	 * @param nTag The tag associated with the directory. Should be unique for each directory.
	 * @return (0,nIndex) if succeeded, (errno,-1) if failed.
	 */
	#ifdef STMF_TESTING_IFACE
	virtual
	#endif // STMF_TESTING_IFACE
	std::pair<int32_t, int32_t> addPath(const std::string& sPath, int32_t nTag);
	/** Remove a watched path.
	 * @param nTag The tag associated with th directory passed when added.
	 * @return 0 if succeeded or (extended) errno if failed.
	 */
	#ifdef STMF_TESTING_IFACE
	virtual
	#endif // STMF_TESTING_IFACE
	int32_t removePath(int32_t nTag);
	/** Remove a watched path.
	 * @param nWatchIdx The index associated with th directory passed when added. Can be -1.
	 * @param nTag The tag associated with th directory passed when added.
	 * @return 0 if succeeded or (extended) errno if failed.
	 */
	#ifdef STMF_TESTING_IFACE
	virtual
	#endif // STMF_TESTING_IFACE
	int32_t removePath(int32_t nWatchIdx, int32_t nTag);
	/** Rename an existing inotify watch.
	 * @param nFromTag The tag associated with the "from" directory path.
	 * @param nToTag The tag associated with the "to" directory path.
	 * @return 0 if succeeded or an error number.
	 */
	#ifdef STMF_TESTING_IFACE
	virtual
	#endif // STMF_TESTING_IFACE
	int32_t renamePath(int32_t nFromTag, int32_t nToTag);
	/** Rename an existing inotify watch.
	 * @param nFromWatchIdx The tag associated with the "from" directory path. Can be -1.
	 * @param nFromTag The tag associated with the "from" directory path.
	 * @param nToTag The tag associated with the "to" directory path.
	 * @return 0 if succeeded or an error number.
	 */
	#ifdef STMF_TESTING_IFACE
	virtual
	#endif // STMF_TESTING_IFACE
	int32_t renamePath(int32_t nFromWatchIdx, int32_t nFromTag, int32_t nToTag);
	/** Clear all watches.
	 * @return 0 if succeeded or an error number.
	 */
	#ifdef STMF_TESTING_IFACE
	virtual
	#endif // STMF_TESTING_IFACE
	int32_t clearAll();

	enum FOFI_ACTION
	{
		FOFI_ACTION_INVALID = -1
		, FOFI_ACTION_CREATE = 0 // IN_CREATE
		, FOFI_ACTION_DELETE = 1 // IN_DELETE
//		, FOFI_ACTION_DELETE_SELF = 2 // IN_DELETE_SELF
		, FOFI_ACTION_MODIFY = 2 // IN_CLOSE_WRITE
		, FOFI_ACTION_ATTRIB = 3 // IN_ATTRIB
		, FOFI_ACTION_RENAME_FROM = 4 // IN_MOVED_FROM
		, FOFI_ACTION_RENAME_TO = 5 // IN_MOVED_TO
	};
	struct FofiData
	{
		int32_t m_nTag = -1; /**< The tag associated with the directory */
		std::string m_sName; /**< The file or subdirectory, empty if it involves the directory itself */
		bool m_bIsDir = false; /**< Whether m_sName is a directory. Default: false. */
		FOFI_ACTION m_eAction = FOFI_ACTION_CREATE; /**< The action. Default is FOFI_ACTION_CREATE. */
		int32_t m_nRenameCookie = 0;
		bool m_bOverflow = false; /**< Whether events were dropped. Default is false. */
	};
	//
	enum FOFI_PROGRESS
	{
		FOFI_PROGRESS_CONTINUE = 0 // go on watching
		, FOFI_PROGRESS_STOP = 1 // stop watching
	};
	// A source can have only one callback type, that is the slot given as parameter.
	// FOFI_PROGRESS = m_oCallback(oFofiData)
	#ifdef STMF_TESTING_IFACE
	virtual
	#endif // STMF_TESTING_IFACE
	sigc::connection connect(const sigc::slot<FOFI_PROGRESS, const FofiData&>& oSlot);

protected:
	bool prepare(int& nTimeout) override;
	bool check() override;
	bool dispatch(sigc::slot_base* oSlot) override;

private:
	// returns -1 or the index into m_aPathWatches and m_aWatchDescriptors
	int32_t findEntryByTag(int32_t nTag);
	// returns -1 or the index into m_aPathWatches and m_aWatchDescriptors
	int32_t findEntryByWatch(int32_t nWatchDescriptor);

private:
	//
	struct WatchItem
	{
		int32_t m_nDescriptor;
		int32_t m_nTag;
	};
	std::vector<WatchItem> m_aWatchItems;
	//
	std::vector<int32_t> m_aFreeWatchIdxs;
	//
	int32_t m_nINotifyFD;
	Glib::PollFD m_oINotifyPollFD;
	//
	static constexpr int32_t s_nBufferSize = 8192;
	char m_aBuffer[s_nBufferSize];
	//
private:
	INotifierSource(const INotifierSource& oSource) = delete;
	INotifierSource& operator=(const INotifierSource& oSource) = delete;
};

} // namespace fofi

#endif /* FOFIMON_INOTIFIER_SOURCE_H_ */
