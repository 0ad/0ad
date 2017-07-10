/* Copyright (C) 2010 Wildfire Games.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * portable directory change notification API.
 */

#ifndef INCLUDED_DIR_WATCH
#define INCLUDED_DIR_WATCH

#include "lib/os_path.h"

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
LIB_API Status dir_watch_Add(const OsPath& path, PDirWatch& dirWatch);

class DirWatchNotification
{
public:
	enum EType
	{
		Created,
		Deleted,
		Changed
	};

	DirWatchNotification(const OsPath& pathname, EType type)
		: pathname(pathname), type(type)
	{
	}

	const OsPath& Pathname() const
	{
		return pathname;
	}

	EType Type() const
	{
		return type;
	}

private:
	OsPath pathname;
	EType type;
};

typedef std::vector<DirWatchNotification> DirWatchNotifications;

/**
 * return all pending directory watch notifications.
 *
 * @param notifications receives any pending notifications in unspecified order.
 * @return Status (INFO::OK doesn't imply notifications were returned)
 *
 * note: the run time of this function is independent of the number of
 * directory watches and number of files.
 *
 * rationale for a polling interface: users (e.g. the main game loop)
 * typically want to receive change notifications at a single point,
 * rather than deal with the complexity of asynchronous notifications.
 **/
LIB_API Status dir_watch_Poll(DirWatchNotifications& notifications);

#endif	// #ifndef INCLUDED_DIR_WATCH
