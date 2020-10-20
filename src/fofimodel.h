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
 * File:   fofimodel.h
 */

#ifndef FOFIMON_FOFI_MODEL_H_
#define FOFIMON_FOFI_MODEL_H_

#include "inotifiersource.h"

#include <sigc++/signal.h>

#include <vector>
#include <string>
#include <memory>
#include <regex>
#include <deque>

#include <stdint.h>

namespace fofi
{

class FofiModel
{
public:
	FofiModel(unique_ptr<INotifierSource> refSource, int32_t nMaxToWatchDirectories, int32_t nMaxResultPaths, bool bIsRoot);
	FofiModel(int32_t nMaxToWatchDirectories, int32_t nMaxResultPaths);
	~FofiModel();
	#ifdef STMF_TESTING_IFACE
	INotifierSource* getSource() { return m_refSource.get(); }
	#endif // STMF_TESTING_IFACE

	//TODO clear() // only when not watching

	enum FILTER_TYPE {
		FILTER_EXACT = 0 /**< Ex. "myfile.txt" */
		, FILTER_REGEX = 1 /**< Ex. ".*\.txt", "mydocnr.?\.conf" */
		//, FILTER_BLOB = 2 /* *< Ex. "*.txt", "mydocnr?.conf" */
	};
	struct Filter
	{
		FILTER_TYPE m_eFilterType = FILTER_EXACT; /**< Default: FILTER_EXACT */
		std::string m_sFilter;
		bool bApplyToPathName = false; /**< Ex. filter re".*B.*", path "/A/B/C":
										 * if true apply to "/A/B/C", otherwise to just "C".
										 * Default: false. */
	private:
		friend class FofiModel;
		std::regex m_oFilterRegex;
	};
	/** Zone of directories to be watched for which the same filters are applied.
	 * A zone with max depth 0 includes just the base path itself.
	 * A zone with max depth 1 also includes (filters and pinned permitting) its
	 * subdirectories.
	 *
	 * The leaf directories of a zone are those the subdirectories of which
	 * are not in the zone.
	 *
	 * Actual concrete directories are represented by ToWatchDir objects.
	 *
	 * If a sub dir or a file satisfy both the include and exclude filters they
	 * are excluded (exclude overrides include).
	 *
	 * Pinned sub dirs and files are always included.
	 *
	 * Two directory zones may overlap, but they can't have the same base path.
	 *
	 * If a watched directory is within two or more (overlapping) zones, the one
	 * with the base path closest to it is its owner zone.
	 */
	struct DirectoryZone
	{
		std::string m_sPath; /**< The base path. Mustn't necessarily exist. */
		int32_t m_nMaxDepth = 0; /**< 0: only watch the base directory itself, 1: also direct subdirectories, and so on. Must be &gt;= 0. */

		std::vector<Filter> m_aSubDirIncludeFilters; /**< A logical OR is applied among the filters.
													 * If empty no filters are applied. Ex. "Config" watches only Config subdirectories. */
		std::vector<Filter> m_aSubDirExcludeFilters; /**< A logical OR is applied among the filters.
													 * If empty no filters are applied. Ex. "build","*.git" excludes build and git folders. */
		std::vector<std::string> m_aPinnedSubDirs; /**< Subdirs names (no paths) that are watched despite the filters. */
		//
		std::vector<Filter> m_aFileIncludeFilters; /**< A logical OR is applied among the filters. "*.txt","*.doc","*.dok" watches texts and documents. */
		std::vector<Filter> m_aFileExcludeFilters; /**< A logical OR is applied among the filters. "*.bak","*~" excludes backup files. */
		std::vector<std::string> m_aPinnedFiles; /**< Files names (no paths) that are watched despite the filters. */
	private:
		friend class FofiModel;
		bool m_bMightHaveInvalidDescendants = false;
	};
	/** A watched directory.
	 */
	struct ToWatchDir
	{
		std::string m_sPathName; /**< The absolute path of the watched directory. */
		int32_t m_nDepth = 0; /**< The depth relative to DirectoryZone */
		std::vector<std::string> m_aPinnedSubDirs;
			/**< Subdirs that are watched despite the filter.
			 * These include not only those defined in the owner directory zone,
			 * but also those defined in all other directory zones that cover m_sPath.
			 * In addition all the subfolders that lead to a directory zone base path
			 * are added.
			 */
		std::vector<std::string> m_aPinnedFiles;
			/**< Files that are watched despite the filter.
			 * These include not only those defined in the owner directory zone,
			 * but also those defined in all other directory zones that cover m_sPath.
			 */
		/** Whether the directory exists.
		 * @return Whether the directory exists.
		 */
		bool exists() const { return m_bExists; }
		/** Whether the directory (actually its contents) is watched.
		 * @return Whether the directory has an inotify watch for its contents.
		 */
		bool isWatched() const { return (m_nWatchedIdx >= 0); }
		/** The subdirectories ToWatchDir.
		 * @return The indexes into FofiModel::getToWatchDirectories() of the watched subdirectories.
		 */
		const std::deque<int32_t>& getToWatchSubDirIdxs() const { return m_aToWatchSubdirIdxs; }
		/** The parent ToWatchDir.
		 * @return The index into FofiModel::getToWatchDirectories() of the parent or -1 if m_sPathName is "/" (root).
		 */
		int32_t getParentIdx() const { return m_nParentTWDIdx; }
		/** The owner directory one.
		 * @return The index into FofiModel::getDirectoryZones() or -1 if gap filler directory.
		 */
		int32_t getOwnerDirectoryZone() const { return m_nIdxOwnerDirectoryZone; }
		/** The name part of the directory.
		 * @return The name of m_sPathName or null if root.
		 */
		const char* getName() const { return ((m_nNamePos < 0) ? nullptr : m_sPathName.c_str() + m_nNamePos); }
		/** Whether the directory is a leaf of the zone.
		 * @return Whether m_nMaxDepth == m_nDepth.
		 */
		bool isLeaf() const { return (m_nMaxDepth == m_nDepth); }
	private:
		friend class FofiModel;
		struct FileDir
		{
			std::string m_sName; /**< The name of the dir or file. Cannot be empty. */
			bool m_bIsDir = false; /**< Whether a dir or file. Default: false. */
			bool m_bRemoved = false; /**< Set to false when removed. Default: false. */
		};
		std::deque<FileDir>::iterator findInExisting(bool bIsDir, const std::string& sName);
	private:
		int32_t m_nNamePos = -1; // within m_sPathName. -1 if root.
		int32_t m_nIdxOwnerDirectoryZone = -1; // The directory zone from which this was generated or -1 (gap filler)
		int32_t m_nParentTWDIdx = -1; // The parent: -1 if m_sPath == "/"
		bool m_bExists = false; // Whether the dir exists
		int32_t m_nWatchedIdx = -1; // The INotifierSource watched index, -1 if not watched
		int32_t m_nMaxDepth = 0; /**< The max depth of watched directories relative to the path. If 0 just the path itself. */
		std::deque<int32_t> m_aToWatchSubdirIdxs; /**< Indexes into m_aToWatchDirs for faster access. */
		std::vector<int32_t> m_aWatchedResultIdxs; /**< Indexes into m_aWatchedResults for faster access.
												 * Files and subdirs that already where modified. */
		std::deque<FileDir> m_aExisting; /**< Names of files or (sub)directories that existed at startup.
											 * Once a WatchedResult is created the name is removed.
											 * A name can be removed in that it is set to empty.*/
	};
	/** Add directory zone.
	 * The base path of the directory zone must not already be used by an already added
	 * directory zone.
	 *
	 * The base path of the directory zone must not necessarily exist.
	 *
	 * Symbolic links are not followed. You might add a separate zone for
	 * any symbolic link target directory.
	 *
	 * If a directory zone base path is within the area (defined by its max depth)
	 * of another zone, it overrides its filters and it shadows it's children.
	 * That is a file or a directory always belong (if at all) to the directory
	 * zone with their closest base path.
	 *
	 * Example: given two directory zones DZ1=("/A", 10) and DZ2=("A/B/C", 1):
	 *     file "/A/X/Y/Z/f2.txt" belongs to DZ1.
	 *     file "/A/B/C/f1.txt" belongs to DZ2.
	 *     file "/A/B/C/D/E/f3.txt" belongs to no directory zone, because DZ2
	 *       is closer to it and shadows DZ1.
	 * @param oDirectoryZone The directory zone to add.
	 * @return Empty string or error.
	 */
	std::string addDirectoryZone(DirectoryZone&& oDirectoryZone);

	std::string removeDirectoryZone(const std::string& sPath);
	const std::vector<DirectoryZone>& getDirectoryZones() const;
	bool hasDirectoryZone(const std::string& sPath) const;

	/** Add single file to watch.
	 * The file must not necessarily exist.
	 *
	 * An inotify watch isn't added for the file but the containing dir (if existing)
	 * is, even if it doesn't belong to a directory zone.
	 * @param sPath The path of the file to watch. Cannot be empty.
	 * @return Empty string or error.
	 */
	std::string addToWatchFile(const std::string& sPath);
	std::string removeToWatchFile(const std::string& sPath);
	const std::vector<std::string>& getToWatchFiles() const;
	bool hasToWatchFile(const std::string& sPath) const;
	/** Calculate initially to be watched directories from the added directory
	 * zones and files.
	 *
	 * ToWatchDir objects are created for each directory from the base path of a
	 * directory zone to the root directory. Same for files.
	 *
	 * Additionally ToWatchDir objects are created for each existing directory
	 * in the zones.
	 *
	 * Cannot be called while running (between start and stop).
	 *
	 * When called erases the watched directories of the last run.
	 * @return Empty string or error.
	 */
	std::string calcToWatchDirectories();
	/** The watched directories.
	 * For this value to be set either calcToWatchDirectories() or start() must have been called.
	 * @return The directories.
	 */
	const std::deque<ToWatchDir>& getToWatchDirectories() const;
	/** The index into getToWatchDirectories() of "/".
	 * Recalculated by calcToWatchDirectories().
	 * @return The root index or -1 if calcToWatchDirectories() wasn't called.
	 */
	int32_t getRootToWatchDirectoriesIdx() const;

	/** Start watching the directory zones and files for modifications.
	 * Resets all the data of the last run.
	 * @return Empty string or error.
	 */
	std::string start();
	/** Stop watching.
	 * Must be called after a successful call of start().
	 */
	void stop();
	/** The duration in microseconds.
	 * If still watching the elapsed time since start() otherwise the duration of
	 * the last run.
	 * @return .
	 */
	int64_t getDuration() const;
	/** Whether watching.
	 * @return true if started and not stopeed yet.
	 */
	bool isWatching() const { return (m_nEventCounter > 0); }

	enum RESULT_TYPE
	{
		RESULT_NONE = 0
		, RESULT_CREATED = 1 /**< Non existing file created */
		, RESULT_DELETED = 2 /**< Existing file deleted */
		, RESULT_MODIFIED = 3 /**< Existing file modified */
		, RESULT_TEMPORARY = 4 /**< Initially created and finally deleted. */
	};
	struct ActionData
	{
		INotifierSource::FOFI_ACTION m_eAction = INotifierSource::FOFI_ACTION_INVALID;
		std::string m_sOtherPath; /**< Used for FOFI_ACTION_RENAME_FROM and FOFI_ACTION_RENAME_TO. */
		bool m_bImmediate = false; /**< True if action created manually (ex. by scanning a dir) or false if from inotify. Default: false. */
		bool m_bCausedByAttribChange = false; /**< Default: false. */
		int64_t m_nTimeUsec = 0; /**< Microseconds from start of watching. */
	};
	/** The modified file or directory class.
	 * Note: during the watching a file could be removed and a directory with the
	 * same name created, so the key is formed by the pair (m_sPath + "/" + m_sName, m_bIsDir)
	 */
	struct WatchedResult
	{
		RESULT_TYPE m_eResultType = RESULT_NONE; /**< The current state of the file or dir. */
		std::string m_sPath; /**< The parent path of the file or directory. */
		std::string m_sName; /**< The name of the file or directory. If m_sPath is "/" then this is empty. Otherwise not empty. */
		bool m_bIsDir = false; /**< Whether m_sName is a directory. */
		bool m_bInconsistent = false; /**< Whether the file or dir state might be inaccurate. */
		std::vector<ActionData> m_aActions; /**< The actions performed on the file or direcory. */
	private:
		friend class FofiModel;
		bool existedAtStart() const { return (m_eResultType == RESULT_DELETED) || (m_eResultType == RESULT_MODIFIED); }
		bool exists() const { return (m_eResultType == RESULT_CREATED) || (m_eResultType == RESULT_MODIFIED); }
		bool immediate() const { return ((! m_aActions.empty()) && (m_aActions.back().m_bImmediate)); }
	};
	/** The files and directories that where modified.
	 * @return The read-only result objects.
	 */
	const std::deque<WatchedResult>& getWatchedResults() const;
	/** Whether the result data might be inconsistent.
	 * @return Whether to trust the results.
	 */
	bool hasInconsistencies() const { return m_bHasInconsistencies; }
	/** Whether the inotify event buffer did overflow.
	 * This is set when inotify events overflow the buffer and are lost.
	 * @return Whether to trust the results.
	 */
	bool hasQueueOverflown() const { return m_bOverflow; }
	/* Emits when watched result is created has changes type. */
	sigc::signal<void, const WatchedResult&> m_oWatchedResultActionSignal;
	/** Abort request signal. The listener should call stop immediately.
	 * It might still use the partial results.
	 * The string describes the reason.
	 */
	sigc::signal<void, const std::string&> m_oAbortSignal;
private:
	struct OpenMove
	{
		int32_t m_nParentTWDIdx = -1; /**< The parent directory of the renaming subdir or file.
										 * used to build the full path name of the file or subdir. */
		int32_t m_nTWDIdx = -1; /**< The directory being renamed. If filtered out or
								 * in case of file it is -1.*/
		bool m_bIsDir = false; /**< Whether a directory (or a file) is being renamed. */
		std::string m_sName; /**< The name of the to be renamed file or subir. */
		std::string m_sPathName; /**< The path name of the to be renamed file or subir. */
		int32_t m_nRenameCookie = -1; /**< Used to match move from and move to */
		int64_t m_nMoveFromTimeUsec; /**< When the move from was received in microseconds. */
		bool m_bFilteredOut = false; /**< The move from must not be watched. */
	};
	int32_t findDirectoryZone(const std::string& sPath) const;
	int32_t findToWatchDir(const std::string& sPath) const;
	int32_t findToWatchDir(int32_t nParentTWDIdx, const std::string& sPathName) const;
	int32_t findToWatchFile(const std::string& sPath) const;
	int32_t findResult(const std::string& sPath, const std::string& sName, bool bIsDir) const;
	int32_t findResult(int32_t nTWDIdx, const std::string& sName, bool bIsDir) const;
	int32_t findRootResult() const;
	void setDirectoryZone(ToWatchDir& oToWatch);
	std::string internalCalcToWatchDirectories();
	// throws Max number of ToWatchDir structs reached
	// throws Max number of INotify watches reached
	void initialSetup();
	// throws Max number of ToWatchDir structs reached
	// throws Max number of INotify watches reached
	int32_t initialFillTheGaps(int32_t nChildToTWDIdx, const std::string& sPath);
	// throws Max number of ToWatchDir structs reached
	// throws Max number of INotify watches reached
	void initialCreateToWatchDir(int32_t nParentToTWDIdx);

	void addExistingContent(ToWatchDir& oTWD);

	void createImmediateChildren(int32_t nParentToTWDIdx, bool bWasAttrib, int64_t nNowUsec, const std::vector<ToWatchDir::FileDir>& aExcept);
	void createImmediateChildren(int32_t nParentToTWDIdx, bool bWasAttrib, int64_t nNowUsec);
	// throws Max number of ToWatchDir structs reached
	int32_t addExistingToWatchDir(const std::string& sPath);
	// throws Max number of INotify watches reached
	void createINotifyWatch(int32_t nTWDIdx, ToWatchDir& oTWD);

	/** Traverse a subtree renaming.
	 * @param nFromParentTWDIdx The parent ToWatchDir of the renamed from file or subdir. If -1 parent not watched.
	 * @param sFromParentPath The from parent path. If empty means the renamed from is unknown.
	 * @param sFromName The file or subdir name that is renamed. If empty means the renamed from is unknown.
	 * @param sFromPath The full path name of the file or subdir that is renamed, If empty means the renamed from is unknown.
	 * @param bIsDir Whether renaming a subdir or a regular file.
	 * @param nToParentTWDIdx The parent ToWatchDir of the renamed to file or subdir. If -1 parent not watched.
	 * @param sToParentPath The from parent path. If empty means the renamed from is unknown.
	 * @param sToName The file or subdir name that is renamed to. If empty means the renamed to is unknown.
	 * @param sToPath The full path name of the file or subdir that is renamed to, If empty means the renamed to is unknown.
	 * @param nNowUsec The time stamp.
	 * @throws Max number of ToWatchDir structs reached
	 * @throws Max number of INotify watches reached
	 */
	void traverseRename(int32_t nFromParentTWDIdx
						, const std::string& sFromParentPath, const std::string& sFromName, const std::string& sFromPath
						, bool bIsDir
						, int32_t nToParentTWDIdx
						, const std::string& sToParentPath, const std::string& sToName, const std::string& sToPath
						, int64_t nNowUsec);

	int32_t addWatchedResultRoot();
	int32_t addWatchedResult(ToWatchDir& oParentTWD, const std::string& sName, bool bIsDir);
	int32_t addWatchedResult(ToWatchDir& oParentTWD, const std::string& sPath, const std::string& sName, bool bIsDir);
	void setInconsistent(WatchedResult& oWR);
	void setNotImmediate(WatchedResult& oWR);

	ActionData& addActionData(WatchedResult& oWR, INotifierSource::FOFI_ACTION eAction, int64_t nTimeUsec);

	// throws Max numb<r of ToWatchDir structs reached
	void checkThrowMaxToWatchDirsReached();
	// throws Max number of INotify watches reached
	void checkThrowMaxInotifyUserWatches(int32_t nErrno);

	bool isFilteredOut(bool bIsDir, const ToWatchDir& oToWatch, const std::string& sName, const std::string& sPathName) const;
	bool isFilteredOutSubDir(const ToWatchDir& oToWatch, const std::string& sName, const std::string& sPath) const;
	bool isFilteredOutFile(const ToWatchDir& oToWatch, const std::string& sName, const std::string& sPath) const;

	INotifierSource::FOFI_PROGRESS onFileModified(const INotifierSource::FofiData& oFofiData);
	bool onCheckOpenMoves();

	void calcFiltersRegex(std::vector<Filter> aFilters);
	// returns true if one of the filters matches
	bool isMatchedByFilters(const std::vector<Filter>& aFilters
							, const std::string& sName, const std::string& sPathName) const;

	// returns the depth within the zone or -1 if not in zone
	int32_t getPathDepthInZone(const std::string& sChildPath, const DirectoryZone& oDZ) const;
	static int64_t getNowTimeMicroseconds();
private:
	static constexpr int32_t s_nCheckOpenMovesMillisec = 1;
	static constexpr int32_t s_nOpenMovesFailedAfterUsec = 200;

	int32_t m_nMaxToWatchDirectories;
	int32_t m_nMaxResultPaths;
	const bool m_bIsUserRoot;

	std::unique_ptr<INotifierSource> m_refSource;
	std::vector<std::string> m_aInvalidPaths;
	std::vector<DirectoryZone> m_aDirectoryZones;
	std::vector<std::string> m_aToWatchFiles; // the watched files
	//
	std::deque<ToWatchDir> m_aToWatchDirs; // the directory zones + their subtree according to depth
	int32_t m_nRootTWDIdx; // points into m_aToWatchDirs after calcToWatchDirectories() or is -1
	int64_t m_nEventCounter; // 0 means not watching
	int64_t m_nStartTimeUsec;
	int64_t m_nStopTimeUsec;
	bool m_bInitialSetup;

	int32_t m_nRootResultIdx;
	bool m_bOverflow;
	bool m_bHasInconsistencies;
	std::deque<WatchedResult> m_aWatchedResults;

	std::vector<OpenMove> m_aOpenMoves;

	const std::string m_sES;
	const std::vector<ToWatchDir::FileDir> m_aEFD;
private:
	FofiModel() = delete;
	FofiModel(const FofiModel& oSource) = delete;
	FofiModel& operator=(const FofiModel& oSource) = delete;
};

} // namespace fofi

#endif /* FOFIMON_FOFI_MODEL_H_ */

