/**
 * =========================================================================
 * File        : path.cpp
 * Project     : 0 A.D.
 * Description : manage paths relative to a root directory
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "path.h"

#include <string.h>

#include "lib/posix/posix_filesystem.h"
#include "lib/sysdep/sysdep.h"	// SYS_DIR_SEP
#include "lib/path_util.h"	// ERR::PATH_LENGTH


ERROR_ASSOCIATE(ERR::PATH_ROOT_DIR_ALREADY_SET, "Attempting to set FS root dir more than once", -1);
ERROR_ASSOCIATE(ERR::PATH_NOT_IN_ROOT_DIR, "Accessing a file that's outside of the root dir", -1);

bool exists(const Path& path)
{
	return fs::exists(path.external_directory_string());
}



// security check: only allow path_SetRoot once so that malicious code
// cannot circumvent the VFS checks that disallow access to anything above
// the current directory (set here).
// path_SetRoot is called early at startup, so any subsequent attempts
// are likely bogus.
// we provide for resetting this from the self-test to allow clean
// re-init of the individual tests.
static bool s_isRootPathEstablished;

static std::string s_rootPath;


/*static*/ PathTraits::external_string_type PathTraits::to_external(const Path&, const PathTraits::internal_string_type& src)
{
	std::string absolutePath = s_rootPath + src;
	std::replace(absolutePath.begin(), absolutePath.end(), '/', SYS_DIR_SEP);
	return absolutePath;
}

/*static*/ PathTraits::internal_string_type PathTraits::to_internal(const PathTraits::external_string_type& src)
{
	if(s_rootPath.compare(0, s_rootPath.length(), src) != 0)
		DEBUG_WARN_ERR(ERR::PATH_NOT_IN_ROOT_DIR);
	std::string relativePath = src.substr(s_rootPath.length(), src.length()-s_rootPath.length());
	std::replace(relativePath.begin(), relativePath.end(), SYS_DIR_SEP, '/');
	return relativePath;
}


LibError path_SetRoot(const char* argv0, const char* relativePath)
{
	if(s_isRootPathEstablished)
		WARN_RETURN(ERR::PATH_ROOT_DIR_ALREADY_SET);
	s_isRootPathEstablished = true;

	// get full path to executable
	char osPathname[PATH_MAX];
	// .. first try safe, but system-dependent version
	if(sys_get_executable_name(osPathname, PATH_MAX) < 0)
	{
		// .. failed; use argv[0]
		errno = 0;
		if(!realpath(argv0, osPathname))
			return LibError_from_errno();
	}

	// make sure it's valid
	errno = 0;
	if(access(osPathname, X_OK) < 0)
		return LibError_from_errno();

	// strip executable name
	char* name = (char*)path_name_only(osPathname);
	*name = '\0';

	strcat_s(osPathname, PATH_MAX, relativePath);

	// get actual root dir - previous osPathname may include ".."
	// (slight optimization, speeds up path lookup)
	errno = 0;
	char osRootPath[PATH_MAX];
	if(!realpath(osPathname, osRootPath))
		return LibError_from_errno();

	s_rootPath = osRootPath;
	s_rootPath.append(1, SYS_DIR_SEP);	// simplifies to_external

	return INFO::OK;
}


void path_ResetRootDir()
{
	debug_assert(s_isRootPathEstablished);	// see comment at s_isRootPathEstablished.
	s_rootPath.clear();
	s_isRootPathEstablished = false;
}
