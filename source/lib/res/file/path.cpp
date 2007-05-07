/**
 * =========================================================================
 * File        : path.cpp
 * Project     : 0 A.D.
 * Description : helper functions for VFS paths.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "path.h"

#include <string.h>

#include "lib/posix/posix_filesystem.h"
#include "lib/lib.h"
#include "lib/adts.h"
#include "lib/allocators.h"
#include "lib/sysdep/sysdep.h"
#include "file_internal.h"


AT_STARTUP(\
	error_setDescription(ERR::ROOT_DIR_ALREADY_SET, "Attempting to set FS root dir more than once");\
)


// path types:
// p_*: posix (e.g. mount object name or for open())
// v_*: vfs (e.g. mount point)
// fn : filename only (e.g. from readdir)
// dir_name: directory only, no path (e.g. subdir name)
//
// all paths must be relative (no leading '/'); components are separated
// by '/'; no ':', '\\', "." or ".." allowed; root dir is "".
//
// grammar:
// path ::= dir*file?
// dir  ::= name/
// file ::= name
// name ::= [^/]

enum Conversion
{
	TO_NATIVE,
	TO_PORTABLE
};

static LibError convert_path(char* dst, const char* src, Conversion conv = TO_NATIVE)
{
	// SYS_DIR_SEP is assumed to be a single character!

	const char* s = src;
	char* d = dst;

	char from = SYS_DIR_SEP, to = '/';
	if(conv == TO_NATIVE)
		from = '/', to = SYS_DIR_SEP;

	size_t len = 0;

	for(;;)
	{
		len++;
		if(len >= PATH_MAX)
			WARN_RETURN(ERR::PATH_LENGTH);

		char c = *s++;

		if(c == from)
			c = to;

		*d++ = c;

		// end of string - done
		if(c == '\0')
			return INFO::OK;
	}
}


// set by file_set_root_dir
static char n_root_dir[PATH_MAX];
static size_t n_root_dir_len;


// return the native equivalent of the given relative portable path
// (i.e. convert all '/' to the platform's directory separator)
// makes sure length < PATH_MAX.
LibError file_make_native_path(const char* path, char* n_path)
{
	return convert_path(n_path, path, TO_NATIVE);
}

// return the portable equivalent of the given relative native path
// (i.e. convert the platform's directory separators to '/')
// makes sure length < PATH_MAX.
LibError file_make_portable_path(const char* n_path, char* path)
{
	return convert_path(path, n_path, TO_PORTABLE);
}


// return the native equivalent of the given portable path
// (i.e. convert all '/' to the platform's directory separator).
// also prepends current directory => n_full_path is absolute.
// makes sure length < PATH_MAX.
LibError file_make_full_native_path(const char* path, char* n_full_path)
{
	debug_assert(path != n_full_path);	// doesn't work in-place

	strcpy_s(n_full_path, PATH_MAX, n_root_dir);
	return convert_path(n_full_path+n_root_dir_len, path, TO_NATIVE);
}

// return the portable equivalent of the given relative native path
// (i.e. convert the platform's directory separators to '/')
// n_full_path is absolute; if it doesn't match the current dir, fail.
// (note: portable paths are always relative to the file root dir).
// makes sure length < PATH_MAX.
LibError file_make_full_portable_path(const char* n_full_path, char* path)
{
	debug_assert(path != n_full_path);	// doesn't work in-place

	if(strncmp(n_full_path, n_root_dir, n_root_dir_len) != 0)
		WARN_RETURN(ERR::TNODE_NOT_FOUND);
	return convert_path(path, n_full_path+n_root_dir_len, TO_PORTABLE);
}


// security check: only allow attempting to chdir once, so that malicious
// code cannot circumvent the VFS checks that disallow access to anything
// above the current directory (set here).
// this routine is called early at startup, so any subsequent attempts
// are likely bogus.
// we provide for resetting this from the self-test to allow clean
// re-init of the individual tests.
static bool root_dir_established;


// establish the root directory from <rel_path>, which is treated as
// relative to the executable's directory (determined via argv[0]).
// all relative file paths passed to this module will be based from
// this root dir.
//
// example: executable in "$install_dir/system"; desired root dir is
// "$install_dir/data" => rel_path = "../data".
//
// argv[0] is necessary because the current directory is unknown at startup
// (e.g. it isn't set when invoked via batch file), and this is the
// easiest portable way to find our install directory.
//
// can only be called once, by design (see below). rel_path is trusted.
LibError file_set_root_dir(const char* argv0, const char* rel_path)
{
	if(root_dir_established)
		WARN_RETURN(ERR::ROOT_DIR_ALREADY_SET);
	root_dir_established = true;

	// get full path to executable
	char n_path[PATH_MAX];
	// .. first try safe, but system-dependent version
	if(sys_get_executable_name(n_path, PATH_MAX) < 0)
	{
		// .. failed; use argv[0]
		if(!realpath(argv0, n_path))
			return LibError_from_errno();
	}

	// make sure it's valid
	if(access(n_path, X_OK) < 0)
		return LibError_from_errno();

	// strip executable name, append rel_path, convert to native
	char* start_of_fn = (char*)path_name_only(n_path);
	RETURN_ERR(file_make_native_path(rel_path, start_of_fn));

	// get actual root dir - previous n_path may include ".."
	// (slight optimization, speeds up path lookup)
	if(!realpath(n_path, n_root_dir))
		return LibError_from_errno();

	// .. append SYS_DIR_SEP to simplify code that uses n_root_dir
	n_root_dir_len = strlen(n_root_dir)+1;	// +1 for trailing SYS_DIR_SEP
	debug_assert((n_root_dir_len+1) < sizeof(n_root_dir)); // Just checking
	n_root_dir[n_root_dir_len-1] = SYS_DIR_SEP;
	// You might think that n_root_dir is already 0-terminated, since it's
	// static - but that might not be true after calling file_reset_root_dir!
	n_root_dir[n_root_dir_len] = 0;
	
	return INFO::OK;
}


void path_reset_root_dir()
{
	// see comment at root_dir_established.
	debug_assert(root_dir_established);
	n_root_dir[0] = '\0';
	n_root_dir_len = 0;
	root_dir_established = false;
}


//-----------------------------------------------------------------------------
// storage for path strings
//-----------------------------------------------------------------------------

// rationale: we want a constant-time IsAtomFn(string pointer) lookup:
// this avoids any overhead of calling file_make_unique_fn_copy on
// already-atomized strings. that requires allocating from one contiguous
// arena, which is also more memory-efficient than the heap (no headers).
static Pool atom_pool;

typedef DynHashTbl<const char*, const char*> AtomMap;
static AtomMap atom_map;

bool path_is_atom_fn(const char* fn)
{
	return pool_contains(&atom_pool, (void*)fn);
}

// allocate a copy of P_fn in our string pool. strings are equal iff
// their addresses are equal, thus allowing fast comparison.
//
// if the (generous) filename storage is full, 0 is returned.
// this is not ever expected to happen; callers need not check the
// return value because a warning is raised anyway.
const char* file_make_unique_fn_copy(const char* P_fn)
{
	// early out: if already an atom, return immediately.
	if(path_is_atom_fn(P_fn))
		return P_fn;

	const size_t fn_len = strlen(P_fn);
	const char* unique_fn;

	// check if already allocated; return existing copy if so.
	//
	// rationale: the entire storage could be done via container,
	// rather than simply using it as a lookup mapping.
	// however, DynHashTbl together with Pool (see above) is more efficient.
	unique_fn = atom_map.find(P_fn);
	if(unique_fn)
		return unique_fn;

	unique_fn = (const char*)pool_alloc(&atom_pool, fn_len+1);
	if(!unique_fn)
	{
		DEBUG_WARN_ERR(ERR::NO_MEM);
		return 0;
	}
	cpu_memcpy((void*)unique_fn, P_fn, fn_len);
	((char*)unique_fn)[fn_len] = '\0';

	atom_map.insert(unique_fn, unique_fn);

	stats_unique_name(fn_len);
	return unique_fn;
}


static ModuleInitState init_state;

void path_init()
{
	moduleInit_assertCanInit(init_state);

	pool_create(&atom_pool, 8*MiB, POOL_VARIABLE_ALLOCS);

	moduleInit_markInitialized(&init_state);
}

void path_shutdown()
{
	moduleInit_assertCanShutdown(init_state);

	atom_map.clear();
	(void)pool_destroy(&atom_pool);

	moduleInit_markShutdown(&init_state);
}


const char* file_get_random_name()
{
	// there had better be names in atom_pool, else this will fail.
	debug_assert(atom_pool.da.pos != 0);

again:
	const size_t start_ofs = (size_t)rand(0, (uint)atom_pool.da.pos);

	// scan back to start of string (don't scan ahead; this must
	// work even if atom_pool only contains one entry).
	const char* start = (const char*)atom_pool.da.base+start_ofs;
	for(size_t i = 0; i < start_ofs; i++)
	{
		if(*start == '\0')
			break;
		start--;
	}

	// skip past the '\0' we found. loop is needed because there may be
	// several if we land in padding (due to pool alignment).
	size_t chars_left = atom_pool.da.pos - start_ofs;
	for(; *start == '\0'; start++)
	{
		// we had landed in padding at the end of the buffer.
		if(chars_left-- == 0)
			goto again;
	}

	const char* next_name = start;
	return next_name;
}
