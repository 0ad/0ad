#include "precompiled.h"
#include "vfs_path.h"

bool vfs_path_IsDirectory(const VfsPath& pathname)
{
	return pathname.empty() || pathname.leaf() == ".";
}
