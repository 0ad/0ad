#include "ps/CStr.h"
#include "res/vfs.h"

namespace VFSUtil
{

typedef std::vector<CStr> FileList;

// Puts the list of files inside 'dirname' matching 'filter' into 'files'.
// 'dirname' shouldn't end with a slash.
// 'filter': see vfs_next_dirent
// 'files' is initially cleared, and undefined on failure.
// On failure, logs an error and returns false.
extern bool FindFiles(const CStr& dirname, const char* filter, FileList& files);


// called by EnumFiles for each file in a directory (optionally
// its subdirectories as well), passing user-specified context.
// note: path and ent parameters are only valid during the callback.
typedef void (*EnumDirEntsCB)(const char* path, const vfsDirEnt* ent,
	void* context);

// call <cb> for each file in the <start_path> directory;
// if <recursive>, files in subdirectories are also returned.
extern int EnumDirEnts(const CStr path, const char* filter, bool recursive,
	EnumDirEntsCB cb, void* context);

};	// namespace VFSUtil
