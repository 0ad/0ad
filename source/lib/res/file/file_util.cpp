#include "precompiled.h"

#include "file_internal.h"

#include <queue>

static bool dirent_less(const DirEnt& d1, const DirEnt& d2)
{
	return strcmp(d1.name, d2.name) < 0;
}

// enumerate all directory entries in <P_path>; add to container and
// then sort it by filename.
LibError file_get_sorted_dirents(const char* P_path, DirEnts& dirents)
{
	DirIterator d;
	RETURN_ERR(dir_open(P_path, &d));

	dirents.reserve(50);	// preallocate for efficiency

	DirEnt ent;
	for(;;)
	{
		LibError ret = dir_next_ent(&d, &ent);
		if(ret == ERR_DIR_END)
			break;
		RETURN_ERR(ret);

		ent.name = file_make_unique_fn_copy(ent.name);
		dirents.push_back(ent);
	}

	std::sort(dirents.begin(), dirents.end(), dirent_less);

	(void)dir_close(&d);
	return ERR_OK;
}


// call <cb> for each file and subdirectory in <dir> (alphabetical order),
// passing the entry name (not full path!), stat info, and <user>.
//
// first builds a list of entries (sorted) and remembers if an error occurred.
// if <cb> returns non-zero, abort immediately and return that; otherwise,
// return first error encountered while listing files, or 0 on success.
//
// rationale:
//   this makes file_enum and zip_enum slightly incompatible, since zip_enum
//   returns the full path. that's necessary because VFS zip_cb
//   has no other way of determining what VFS dir a Zip file is in,
//   since zip_enum enumerates all files in the archive (not only those
//   in a given dir). no big deal though, since add_ent has to
//   special-case Zip files anyway.
//   the advantage here is simplicity, and sparing callbacks the trouble
//   of converting from/to native path (we just give 'em the dirent name).
LibError file_enum(const char* P_path, const FileCB cb, const uintptr_t user)
{
	LibError stat_err = ERR_OK;	// first error encountered by stat()
	LibError cb_err   = ERR_OK;	// first error returned by cb

	DirEnts dirents;
	RETURN_ERR(file_get_sorted_dirents(P_path, dirents));

	// call back for each entry (now sorted);
	// first, expand each DirEnt to full struct stat (we store as such to
	// reduce memory use and therefore speed up sorting)
	struct stat s;
	memset(&s, 0, sizeof(s));
	// .. not needed for plain files (OS opens them; memento doesn't help)
	const uintptr_t memento = 0;
	for(DirEntCIt it = dirents.begin(); it != dirents.end(); ++it)
	{
		const DirEnt& dirent = *it;
		s.st_mode  = (dirent.size == -1)? S_IFDIR : S_IFREG;
		s.st_size  = dirent.size;
		s.st_mtime = dirent.mtime;
		LibError ret = cb(dirent.name, &s, memento, user);
		if(ret != INFO_CB_CONTINUE)
		{
			cb_err = ret;	// first error (since we now abort)
			break;
		}
	}

	if(cb_err != ERR_OK)
		return cb_err;
	return stat_err;
}



// retrieve the next (order is unspecified) dir entry matching <filter>.
// return 0 on success, ERR_DIR_END if no matching entry was found,
// or a negative error code on failure.
// filter values:
// - 0: anything;
// - "/": any subdirectory;
// - "/|<pattern>": any subdirectory, or as below with <pattern>;
// - <pattern>: any file whose name matches; ? and * wildcards are allowed.
//
// note that the directory entries are only scanned once; after the
// end is reached (-> ERR_DIR_END returned), no further entries can
// be retrieved, even if filter changes (which shouldn't happen - see impl).
//
// rationale: we do not sort directory entries alphabetically here.
// most callers don't need it and the overhead is considerable
// (we'd have to store all entries in a vector). it is left up to
// higher-level code such as VfsUtil.
LibError dir_filtered_next_ent(DirIterator* di, DirEnt* ent, const char* filter)
{
	// warn if scanning the directory twice with different filters
	// (this used to work with dir/file because they were stored separately).
	// it is imaginable that someone will want to change it, but until
	// there's a good reason, leave this check in. note: only comparing
	// pointers isn't 100% certain, but it's safe enough and easy.
	if(!di->filter_latched)
	{
		di->filter = filter;
		di->filter_latched = 1;
	}
	if(di->filter != filter)
		debug_warn("filter has changed for this directory. are you scanning it twice?");

	bool want_dir = true;
	if(filter)
	{
		// directory
		if(filter[0] == '/')
		{
			// .. and also files
			if(filter[1] == '|')
				filter += 2;
		}
		// file only
		else
			want_dir = false;
	}

	// loop until ent matches what is requested, or end of directory.
	for(;;)
	{
		RETURN_ERR(xdir_next_ent(di, ent));

		if(DIRENT_IS_DIR(ent))
		{
			if(want_dir)
				break;
		}
		else
		{
			// (note: filter = 0 matches anything)
			if(match_wildcard(ent->name, filter))
				break;
		}
	}

	return ERR_OK;
}


// call <cb> for each entry matching <user_filter> (see vfs_next_dirent) in
// directory <path>; if flags & VFS_DIR_RECURSIVE, entries in
// subdirectories are also returned.
//
// note: EnumDirEntsCB path and ent are only valid during the callback.
LibError vfs_dir_enum(const char* start_path, uint flags, const char* user_filter,
	DirEnumCB cb, void* context)
{
	debug_assert((flags & ~(VFS_DIR_RECURSIVE)) == 0);
	const bool recursive = (flags & VFS_DIR_RECURSIVE) != 0;

	char filter_buf[PATH_MAX];
	const char* filter = user_filter;
	bool user_filter_wants_dirs = true;
	if(user_filter)
	{
		if(user_filter[0] != '/')
			user_filter_wants_dirs = false;

		// we need subdirectories and the caller hasn't already requested them
		if(recursive && !user_filter_wants_dirs)
		{
			snprintf(filter_buf, sizeof(filter_buf), "/|%s", user_filter);
			filter = filter_buf;
		}
	}


	// note: FIFO queue instead of recursion is much more efficient
	// (less stack usage; avoids seeks by reading all entries in a
	// directory consecutively)

	std::queue<const char*> dir_queue;
	dir_queue.push(file_make_unique_fn_copy(start_path));

	// for each directory:
	do
	{
		// get current directory path from queue
		// note: can't refer to the queue contents - those are invalidated
		// as soon as a directory is pushed onto it.
		PathPackage pp;
		(void)path_package_set_dir(&pp, dir_queue.front());
		dir_queue.pop();

		Handle hdir = vfs_dir_open(pp.path);
		if(hdir <= 0)
		{
			debug_warn("vfs_open_dir failed");
			continue;
		}

		// for each entry (file, subdir) in directory:
		DirEnt ent;
		while(vfs_dir_next_ent(hdir, &ent, filter) == 0)
		{
			// build complete path (DirEnt only stores entry name)
			(void)path_package_append_file(&pp, ent.name);
			const char* atom_path = file_make_unique_fn_copy(pp.path);

			if(DIRENT_IS_DIR(&ent))
			{
				if(recursive)
					dir_queue.push(atom_path);

				if(user_filter_wants_dirs)
					cb(atom_path, &ent, context);
			}
			else
				cb(atom_path, &ent, context);
		}

		vfs_dir_close(hdir);
	}
	while(!dir_queue.empty());

	return ERR_OK;
}


// fill V_next_fn (which must be big enough for PATH_MAX chars) with
// the next numbered filename according to the pattern defined by V_fn_fmt.
// <nfi> must be initially zeroed (e.g. by defining as static) and passed
// each time.
// if <use_vfs> (default), the paths are treated as VFS paths; otherwise,
// file.cpp's functions are used. this is necessary because one of
// our callers needs a filename for VFS archive files.
//
// this function is useful when creating new files which are not to
// overwrite the previous ones, e.g. screenshots.
// example for V_fn_fmt: "screenshots/screenshot%04d.png".
void next_numbered_filename(const char* fn_fmt,
	NextNumberedFilenameInfo* nfi, char* next_fn, bool use_vfs)
{
	// (first call only:) scan directory and set next_num according to
	// highest matching filename found. this avoids filling "holes" in
	// the number series due to deleted files, which could be confusing.
	// example: add 1st and 2nd; [exit] delete 1st; [restart]
	// add 3rd -> without this measure it would get number 1, not 3. 
	if(nfi->next_num == 0)
	{
		char dir[PATH_MAX];
		path_dir_only(fn_fmt, dir);
		const char* name_fmt = path_name_only(fn_fmt);

		int max_num = -1; int num;
		DirEnt ent;

		if(use_vfs)
		{
			Handle hd = vfs_dir_open(dir);
			if(hd > 0)
			{
				while(vfs_dir_next_ent(hd, &ent, 0) == ERR_OK)
				{
					if(!DIRENT_IS_DIR(&ent) && sscanf(ent.name, name_fmt, &num) == 1)
						max_num = MAX(num, max_num);
				}
				(void)vfs_dir_close(hd);
			}
		}
		else
		{
			DirIterator it;
			if(dir_open(dir, &it) == ERR_OK)
			{
				while(dir_next_ent(&it, &ent) == ERR_OK)
					if(!DIRENT_IS_DIR(&ent) && sscanf(ent.name, name_fmt, &num) == 1)
						max_num = MAX(num, max_num);
				(void)dir_close(&it);
			}
		}

		nfi->next_num = max_num+1;
	}

	bool (*exists)(const char* fn) = use_vfs? vfs_exists : file_exists;

	// now increment number until that file doesn't yet exist.
	// this is fairly slow, but typically only happens once due
	// to scan loop above. (we still need to provide for looping since
	// someone may have added files in the meantime)
	// binary search isn't expected to improve things.
	do
		snprintf(next_fn, PATH_MAX, fn_fmt, nfi->next_num++);
	while(exists(next_fn));
}
