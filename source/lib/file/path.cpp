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


// set by path_SetRoot
static char osRootPath[PATH_MAX];
static size_t osRootPathLength;

// security check: only allow path_SetRoot once so that malicious code
// cannot circumvent the VFS checks that disallow access to anything above
// the current directory (set here).
// path_SetRoot is called early at startup, so any subsequent attempts
// are likely bogus.
// we provide for resetting this from the self-test to allow clean
// re-init of the individual tests.
static bool isRootDirEstablished;


LibError path_SetRoot(const char* argv0, const char* rel_path)
{
	if(isRootDirEstablished)
		WARN_RETURN(ERR::PATH_ROOT_DIR_ALREADY_SET);
	isRootDirEstablished = true;

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

	strcat_s(osPathname, ARRAY_SIZE(osRootPath), rel_path);

	// get actual root dir - previous osPathname may include ".."
	// (slight optimization, speeds up path lookup)
	errno = 0;
	if(!realpath(osPathname, osRootPath))
		return LibError_from_errno();

	// .. append SYS_DIR_SEP to simplify code that uses osRootPath
	osRootPathLength = strlen(osRootPath)+1;	// +1 for trailing SYS_DIR_SEP
	debug_assert((osRootPathLength+1) < sizeof(osRootPath)); // Just checking
	osRootPath[osRootPathLength-1] = SYS_DIR_SEP;
	// You might think that osRootPath is already 0-terminated, since it's
	// static - but that might not be true after calling path_ResetRootDir!
	osRootPath[osRootPathLength] = 0;

	return INFO::OK;
}


void path_ResetRootDir()
{
	// see comment at isRootDirEstablished.
	debug_assert(isRootDirEstablished);
	osRootPath[0] = '\0';
	osRootPathLength = 0;
	isRootDirEstablished = false;
}


// (this assumes SYS_DIR_SEP is a single character)
static void ConvertSlashCharacters(char* dst, const char* src, char from, char to)
{
	for(size_t len = 0; len < PATH_MAX; len++)
	{
		char c = *src++;
		if(c == from)
			c = to;
		*dst++ = c;

		// end of string - done
		if(c == '\0')
			return;
	}

	DEBUG_WARN_ERR(ERR::PATH_LENGTH);
}


void path_MakeAbsolute(const char* path, char* osPath)
{
	debug_assert(path != osPath);	// doesn't work in-place

	strcpy_s(osPath, PATH_MAX, osRootPath);
	ConvertSlashCharacters(osPath+osRootPathLength, path, '/', SYS_DIR_SEP);
}


void path_MakeRelative(const char* osPath, char* path)
{
	debug_assert(path != osPath);	// doesn't work in-place

	if(strncmp(osPath, osRootPath, osRootPathLength) != 0)
		DEBUG_WARN_ERR(ERR::PATH_NOT_IN_ROOT_DIR);
	ConvertSlashCharacters(path, osPath+osRootPathLength, SYS_DIR_SEP, '/');
}
