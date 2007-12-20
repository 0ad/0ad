#include "precompiled.h"
#include "vfs_path.h"

bool IsDirectory(const VfsPath& pathname)
{
	return pathname.empty() || pathname.leaf() == ".";
}

std::string Basename(const VfsPath& pathname)
{
	const std::string name(pathname.leaf());
	const size_t pos = name.rfind('.');
	return name.substr(0, pos);
}

std::string Extension(const VfsPath& pathname)
{
	const std::string name(pathname.leaf());
	const size_t pos = name.rfind('.');
	return (pos == std::string::npos)? std::string() : name.substr(pos);
}
