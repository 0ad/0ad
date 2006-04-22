#include "precompiled.h"

#include "file_internal.h"


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