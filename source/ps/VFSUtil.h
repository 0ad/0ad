#include "ps/CStr.h"
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


// called by EnumDirEnts for each entry in a directory (optionally those in
// its subdirectories as well), passing their complete path+name, the info
// that would be returned by vfs_next_dirent, and user-specified context.
// note: path and ent parameters are only valid during the callback.
typedef void (*EnumDirEntsCB)(const char* path, const DirEnt* ent,
	void* context);

enum EnumDirEntsFlags
{
	RECURSIVE = 1
};

// call <cb> for each entry matching <user_filter> (see vfs_next_dirent) in
// directory <path>; if flags & RECURSIVE, entries in subdirectories are
// also returned.
extern LibError EnumDirEnts(const CStr path, int flags, const char* filter,
	EnumDirEntsCB cb, void* context);

};	// namespace VFSUtil
