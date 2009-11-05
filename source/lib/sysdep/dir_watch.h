/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * portable directory change notification API.
 */

#ifndef INCLUDED_DIR_WATCH
#define INCLUDED_DIR_WATCH

struct DirWatch;
typedef shared_ptr<DirWatch> PDirWatch;

/**
 * start watching a single directory for changes.
 *
 * @param path (must end in slash)
 * @param dirWatch opaque smart pointer to the watch state; used to
 * manage its lifetime (this is deemed more convenient than a
 * separate dir_watch_Remove interface).
 *
 * clients typically want to watch entire directory subtrees (e.g. a mod),
 * which is supported by Windows but not FAM. to reduce overhead, the
 * Windows backend always watches subtrees, but portable clients should
 * still add a watch for each subdirectory (the shared watch state is
 * reference-counted).
 * rationale: since the VFS has per-directory data structures, it is
 * convenient to store PDirWatch there instead of creating a second
 * tree structure here.
 **/
LIB_API LibError dir_watch_Add(const fs::wpath& path, PDirWatch& dirWatch);

class DirWatchNotification
{
public:
	enum Event
	{
		Created,
		Deleted,
		Changed
	};

	DirWatchNotification(const fs::wpath& pathname, Event type)
		: m_pathname(pathname), m_type(type)
	{
	}

	const fs::wpath& Pathname() const
	{
		return m_pathname;
	}

	Event Type() const
	{
		return m_type;
	}

private:
	fs::wpath m_pathname;
	Event m_type;
};

typedef std::vector<DirWatchNotification> DirWatchNotifications;

/**
 * return all pending directory watch notifications.
 *
 * @param notifications receives any pending notifications in unspecified order.
 * @return LibError (INFO::OK doesn't imply notifications were returned)
 *
 * note: the run time of this function is independent of the number of
 * directory watches and number of files.
 *
 * rationale for a polling interface: users (e.g. the main game loop)
 * typically want to receive change notifications at a single point,
 * rather than deal with the complexity of asynchronous notifications.
 **/
LIB_API LibError dir_watch_Poll(DirWatchNotifications& notifications);

#endif	// #ifndef INCLUDED_DIR_WATCH
