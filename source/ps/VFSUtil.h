#include "ps/CStr.h"
#include "lib/path_util.h"	// for convenience
#include "lib/res/file/vfs.h"

namespace VFSUtil
{

typedef std::vector<CStr> FileList;

// Puts the list of files inside 'dirname' matching 'filter' into 'files'.
// 'dirname' shouldn't end with a slash.
// 'filter': see vfs_next_dirent
// 'files' is initially cleared, and undefined on failure.
// On failure, logs an error and returns false.
extern bool FindFiles(const CStr& dirname, const char* filter, FileList& files);

}	// namespace VFSUtil
