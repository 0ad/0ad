#include "ps/CStr.h"

namespace VFSUtil
{
	typedef std::vector<CStr> FileList;

	// Puts the list of files inside 'dirname' matching 'filter' into 'files'.
	// 'dirname' shouldn't end with a slash.
	// 'filter' is as in vfs_next_dirent:
	//   - 0: any file;
	//   - "/": any subdirectory
	//   - anything else: pattern for name (may include '?' and '*' wildcards)
	// 'files' is initially cleared, and undefined on failure.
	// On failure, logs an error and returns false.
	bool FindFiles(CStr& dirname, const char* filter, FileList& files);

};
