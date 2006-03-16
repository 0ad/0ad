#include "precompiled.h"

#include "sysdep/dir_watch.h"
#include "lib/res/h_mgr.h"
#include "file_internal.h"

#include <deque>
#include <list>

// location of a file: either archive or a real directory.
// not many instances => don't worry about efficiency.
struct Mount
{
	// mounting into this VFS directory;
	// must end in '/' (unless if root td, i.e. "")
	std::string V_mount_point;

	// real directory being mounted.
	// if this Mount represents an archive, this is the real directory
	// containing the Zip file (required so that this Mount is unmounted).
	std::string P_name;

	Handle archive;

	uint pri;

	// see enum VfsMountFlags
	uint flags;

	MountType type;

	Mount(const char* V_mount_point_, const char* P_name_, Handle archive_, uint flags_, uint pri_)
		: V_mount_point(V_mount_point_), P_name(P_name_)
	{
		archive = archive_;
		flags = flags_;
		pri = pri_;

		if(archive > 0)
		{
			h_add_ref(archive);
			type = MT_ARCHIVE;
		}
		else
			type = MT_FILE;
	}

	~Mount()
	{
		if(archive > 0)	// avoid h_mgr warning
			archive_close(archive);
	}

	Mount& operator=(const Mount& rhs)
	{
		V_mount_point = rhs.V_mount_point;
		P_name        = rhs.P_name;
		archive       = rhs.archive;
		pri           = rhs.pri;
		flags         = rhs.flags;
		type          = rhs.type;

		if(archive > 0)	// avoid h_mgr warning
			h_add_ref(archive);

		return *this;
	}

	struct equal_to : public std::binary_function<Mount, const char*, bool>
	{
		bool operator()(const Mount& m, const char* P_name) const
		{
			return (m.P_name == P_name);
		}
	};

private:
	Mount();
};



char mount_get_type(const Mount* m)
{
	switch(m->type)
	{
	case MT_ARCHIVE:
		return 'A';
	case MT_FILE:
		return 'F';
	default:
		return '?';
	}
}


bool mount_is_archivable(const Mount* m)
{
	return (m->flags & VFS_MOUNT_ARCHIVES) != 0;
}


bool mount_should_replace(const Mount* m_old, const Mount* m_new,
	size_t size_old, size_t size_new, time_t mtime_old, time_t mtime_new)
{
	// 1) "replace" if not yet associated with a Mount.
	if(!m_old)
		return true;

	// 2) keep old if new priority is lower.
	if(m_new->pri < m_old->pri)
		return false;

	// assume they're the same if size and last-modified time match.
	// note: FAT timestamp only has 2 second resolution
	const double mtime_diff = difftime(mtime_old, mtime_new);
	const bool identical = (size_old == size_new) &&
		fabs(mtime_diff) <= 2.0;

	// 3) go with more efficient source (if files are identical)
	//
	// since priority is not less, we really ought to always go with m_new.
	// however, there is one special case we handle for performance reasons:
	// if the file contents are the same, prefer the more efficient source.
	// note that priority doesn't automatically take care of this,
	// especially if set incorrectly.
	//
	// note: see MountType for explanation of type > type2.
	if(identical && m_old->type > m_new->type)
		return false;

	// 4) don't replace "old" file if modified more recently than "new".
	// (still provide for 2 sec. FAT tolerance - see above)
	if(mtime_diff > 2.0)
		return false;

	return true;
}


///////////////////////////////////////////////////////////////////////////////
//
// populate the directory being mounted with files from real subdirectories
// and archives.
//
///////////////////////////////////////////////////////////////////////////////

static const Mount& add_mount(const char* V_mount_point, const char* P_real_path, Handle archive,
	uint flags, uint pri);

// passed through dirent_cb's afile_enum to afile_cb
struct ZipCBParams
{
	// tree directory into which we are adding the archive's files
	TDir* const td;

	// archive's location; assigned to all files added from here
	const Mount* const m;

	// storage for directory lookup optimization (see below).
	// held across one afile_enum's afile_cb calls.
	const char* last_path;
	TDir* last_td;

	ZipCBParams(TDir* dir_, const Mount* loc_)
		: td(dir_), m(loc_)
	{
		last_path = 0;
		last_td = 0;
	}

	// no copy ctor because some members are const
private:
	ZipCBParams& operator=(const ZipCBParams&);
};

// called by add_ent's afile_enum for each file in the archive.
// we get the full path, since that's what is stored in Zip archives.
//
// [total time 21ms, with ~2000 file's (includes add_file cost)]
static LibError afile_cb(const char* atom_fn, const struct stat* s, uintptr_t memento, uintptr_t user)
{
	CHECK_PATH(atom_fn);

	const char* name = path_name_only(atom_fn);
	char path[VFS_MAX_PATH];
	path_dir_only(atom_fn, path);
	const char* atom_path = file_make_unique_fn_copy(path);
	if(!atom_path)
		return ERR_NO_MEM;

	ZipCBParams* params = (ZipCBParams*)user;
	TDir* td              = params->td;
	const Mount* m        = params->m;
	const char* last_path = params->last_path;
	TDir* last_td         = params->last_td;

	// into which directory should the file be inserted?
	// naive approach: tree_lookup_dir the path (slow!)
	// optimization: store the last file's path; if it's the same,
	//   use the directory we looked up last time (much faster!)
	// .. same as last time
	if(last_path == atom_path)
		td = last_td;
	// .. last != current: need to do lookup
	else
	{
		// we have to create them if missing, since we can't rely on the
		// archiver placing directories before subdirs or files that
		// reference them (WinZip doesn't always).
		// we also need to start at the mount point (td).
		const uint flags = LF_CREATE_MISSING|LF_START_DIR;
		CHECK_ERR(tree_lookup_dir(atom_path, &td, flags));

		params->last_path = atom_path;
		params->last_td = td;
	}

	WARN_ERR(tree_add_file(td, name, m, s->st_size, s->st_mtime, memento));
	vfs_opt_notify_non_loose_file(atom_fn);
	return INFO_CB_CONTINUE;
}


static bool archive_less(Handle hza1, Handle hza2)
{
	const char* fn1 = h_filename(hza1);
	const char* fn2 = h_filename(hza2);
	return strcmp(fn1, fn2) < 0;
}

typedef std::vector<Handle> Archives;
typedef Archives::const_iterator ArchiveCIt;

// return value is ERR_OK iff archives != 0 and the file should not be
// added to VFS (e.g. because it is an archive).
static LibError enqueue_archive(const char* name, const char* P_archive_dir, Archives* archives)
{
	// caller doesn't want us to check if this is a Zip file. this is the
	// case in all subdirectories of the mount point, since checking for all
	// mounted files would be slow. see mount_dir_tree.
	if(!archives)
		return INFO_SKIPPED;

	// get complete path for archive_open.
	// this doesn't (need to) work for subdirectories of the mounted td!
	// we can't use mount_get_path because we don't have the VFS path.
	char P_path[PATH_MAX];
	RETURN_ERR(vfs_path_append(P_path, P_archive_dir, name));

	// just open the Zip file and see if it's valid. we don't bother
	// checking the extension because archives won't necessarily be
	// called .zip (e.g. Quake III .pk3).
	Handle archive = archive_open(P_path);
	// .. special case: <name> is recognizable as a Zip file but is
	//    invalid and can't be opened. avoid adding it to
	//    archive list and/or VFS.
	if(archive == ERR_CORRUPTED)
		goto do_not_add_to_VFS_or_list;
	RETURN_ERR(archive);

	archives->push_back(archive);

	// avoid also adding the archive file itself to VFS.
	// (when caller sees ERR_OK, they skip the file)
do_not_add_to_VFS_or_list:
	return ERR_OK;
}

static LibError mount_archive(TDir* td, const Mount& m)
{
	ZipCBParams params(td, &m);
	archive_enum(m.archive, afile_cb, (uintptr_t)&params);
	return ERR_OK;
}

static LibError mount_archives(TDir* td, Archives* archives, const Mount* mount)
{
	// VFS_MOUNT_ARCHIVES flag wasn't set, or no archives present
	if(archives->empty())
		return ERR_OK;

	std::sort(archives->begin(), archives->end(), archive_less);

	for(ArchiveCIt it = archives->begin(); it != archives->end(); ++it)
	{
		Handle hza = *it;

		// add this archive to the mount list (address is guaranteed to
		// remain valid).
		const Mount& m = add_mount(mount->V_mount_point.c_str(), mount->P_name.c_str(), hza, mount->flags, mount->pri);

		mount_archive(td, m);
	}

	return ERR_OK;
}


//-----------------------------------------------------------------------------

struct TDirAndPath
{
	TDir* const td;
	const std::string path;

	TDirAndPath(TDir* d, const char* p)
		: td(d), path(p) {}
	// no copy ctor because some members are const
private:
	TDirAndPath& operator=(const TDirAndPath&);
};

typedef std::deque<TDirAndPath> DirQueue;



static LibError enqueue_dir(TDir* parent_td, const char* name,
	const char* P_parent_path, DirQueue* dir_queue)
{
	// caller doesn't want us to enqueue subdirectories; bail.
	if(!dir_queue)
		return ERR_OK;

	// skip versioning system directories - this avoids cluttering the
	// VFS with hundreds of irrelevant files.
	// we don't do this for Zip files because it's harder (we'd have to
	// strstr the entire path) and it is assumed the Zip file builder
	// will take care of it.
	if(!strcmp(name, "CVS") || !strcmp(name, ".svn"))
		return ERR_OK;

	// prepend parent path to get complete pathname.
	char P_path[PATH_MAX];
	CHECK_ERR(vfs_path_append(P_path, P_parent_path, name));

	// create subdirectory..
	TDir* td;
	CHECK_ERR(tree_add_dir(parent_td, name, &td));
	// .. and add it to the list of directories to visit.
	dir_queue->push_back(TDirAndPath(td, P_path));
	return ERR_OK;
}




// called by TDir::addR's file_enum for each entry in a real directory.
//
// if called for a real directory, it is added to VFS.
// else if called for a loose file that is a valid archive (*),
//   it is mounted (all of its files are added)
// else the file is added to VFS.
//
// * we only perform this check in the directory being mounted,
// i.e. passed in by tree_add_dir. to determine if a file is an archive,
// we have to open it and read the header, which is slow.
// can't just check extension, because it might not be .zip (e.g. Quake3 .pk3).

//
// td - tree td into which the dirent is to be added
// m - real td's location; assigned to all files added from this mounting
// archives - if the dirent is an archive, its Mount is added here.

static LibError add_ent(TDir* td, DirEnt* ent, const char* P_parent_path, const Mount* m,
	DirQueue* dir_queue, Archives* archives)
{
	const char* name = ent->name;

	// it's a directory entry.
	if(DIRENT_IS_DIR(ent))
		return enqueue_dir(td, name, P_parent_path, dir_queue);
	// else: it's a file (dir_next_ent discards everything except for
	// file and subdirectory entries).

	if(enqueue_archive(name, m->P_name.c_str(), archives) == ERR_OK)
		// return value indicates this file shouldn't be added to VFS
		// (see enqueue_archive)
		return ERR_OK;

	// prepend parent path to get complete pathname.
	char V_path[PATH_MAX];
	CHECK_ERR(vfs_path_append(V_path, tfile_get_atom_fn((TFile*)td), name));
	const char* atom_fn = file_make_unique_fn_copy(V_path);
	vfs_opt_notify_loose_file(atom_fn);

	// it's a regular data file; add it to the directory.
	return tree_add_file(td, name, m, ent->size, ent->mtime, 0);
}


// note: full path is needed for the dir watch.
static LibError populate_dir(TDir* td, const char* P_path, const Mount* m,
	DirQueue* dir_queue, Archives* archives, int flags)
{
	LibError err;

	RealDir* rd = tree_get_real_dir(td);
	WARN_ERR(mount_attach_real_dir(rd, P_path, m, flags));

	DirIterator d;
	RETURN_ERR(dir_open(P_path, &d));

	DirEnt ent;
	for(;;)
	{
		// don't RETURN_ERR since we need to close d.
		err = dir_next_ent(&d, &ent);
		if(err != ERR_OK)
			break;

		err = add_ent(td, &ent, P_path, m, dir_queue, archives);
		WARN_ERR(err);
	}

	WARN_ERR(dir_close(&d));
	return ERR_OK;
}


// actually mount the specified entry. split out of vfs_mount,
// because when invalidating (reloading) the VFS, we need to
// be able to mount without changing the mount list.
// add all loose files and subdirectories (recursive).
// also mounts all archives in P_real_path and adds to archives.

// add the contents of directory <p_path> to this TDir,
// marking the files' locations as <m>. flags: see VfsMountFlags.
//
// note: we are only able to add archives found in the root directory,
// due to dirent_cb implementation. that's ok - we don't want to check
// every single file to see if it's an archive (slow!).
static LibError mount_dir_tree(TDir* td, const Mount& m)
{
	LibError err = ERR_OK;

	// add_ent fills these queues with dirs/archives if the corresponding
	// flags are set.
	DirQueue dir_queue;	// don't preallocate (not supported by TDirAndPath)
	Archives archives;
	archives.reserve(8); // preallocate for efficiency.

	// instead of propagating flags down to add_dir, prevent recursing
	// and adding archives by setting the destination pointers to 0 (easier).
	DirQueue* const pdir_queue = (m.flags & VFS_MOUNT_RECURSIVE)? &dir_queue : 0;
	Archives* parchives = (m.flags & VFS_MOUNT_ARCHIVES)? &archives : 0;

	// kickoff (less efficient than goto, but c_str reference requires
	// pop to come at end of loop => this is easiest)
	dir_queue.push_back(TDirAndPath(td, m.P_name.c_str()));

	do
	{
		TDir* const td     = dir_queue.front().td;
		const char* P_path = dir_queue.front().path.c_str();

		LibError ret = populate_dir(td, P_path, &m, pdir_queue, parchives, m.flags);
		if(err == ERR_OK)
			err = ret;

		// prevent searching for archives in subdirectories (slow!). this
		// is currently required by the implementation anyway.
		parchives = 0;

		dir_queue.pop_front();
		// pop at end of loop, because we hold a c_str() reference.
	}
	while(!dir_queue.empty());

	// do not pass parchives because that has been set to 0!
	mount_archives(td, &archives, &m);

	return ERR_OK;
}







// the VFS stores the location (archive or directory) of each file;
// this allows multiple search paths without having to check each one
// when opening a file (slow).
//
// one Mount is allocated for each archive or directory mounted.
// therefore, files only /point/ to a (possibly shared) Mount.
// if a file's location changes (e.g. after mounting a higher-priority
// directory), the VFS entry will point to the new Mount; the priority
// of both locations is unchanged.
//
// allocate via mnt_create, passing the location. do not free!
// we keep track of all Locs allocated; they are freed at exit,
// and by mount_unmount_all (useful when rebuilding the VFS).
// this is much easier and safer than walking the VFS tree and
// freeing every location we find.







///////////////////////////////////////////////////////////////////////////////
//
// mount list (allows multiple mountings, e.g. for mods)
//
///////////////////////////////////////////////////////////////////////////////

// every mounting results in at least one Mount (and possibly more, e.g.
// if the directory contains Zip archives, which each get a Mount).
//
// requirements for container:
// - must not invalidate iterators after insertion!
//   (TFile holds a pointer to the Mount from which it was added)
// - must store items in order of insertion
//   xxx

typedef std::list<Mount> Mounts;
typedef Mounts::iterator MountIt;
static Mounts mounts;


static const Mount& add_mount(const char* V_mount_point, const char* P_real_path, Handle hza,
	uint flags, uint pri)
{
	mounts.push_back(Mount(V_mount_point, P_real_path, hza, flags, pri));
	return mounts.back();
}


// note: this is not a member function of Mount to avoid having to
// forward-declare mount_archive, mount_dir_tree.
static LibError remount(const Mount& m)
{
	TDir* td;
	CHECK_ERR(tree_lookup_dir(m.V_mount_point.c_str(), &td, LF_CREATE_MISSING));

	switch(m.type)
	{
	case MT_ARCHIVE:
		return mount_archive(td, m);
	case MT_FILE:
		return mount_dir_tree(td, m);
	default:
		debug_warn("invalid type");
		return ERR_CORRUPTED;
	}
}

void mount_unmount_all(void)
{
	mounts.clear();
}

static inline void remount_all()
{
	std::for_each(mounts.begin(), mounts.end(), remount);
}





// mount <P_real_path> into the VFS at <V_mount_point>,
//   which is created if it does not yet exist.
// files in that directory override the previous VFS contents if
//   <pri>(ority) is not lower.
// all archives in <P_real_path> are also mounted, in alphabetical order.
//
// flags determines extra actions to perform; see VfsMountFlags.
//
// P_real_path = "." or "./" isn't allowed - see implementation for rationale.
LibError vfs_mount(const char* V_mount_point, const char* P_real_path, int flags, uint pri)
{
	// callers have a tendency to forget required trailing '/';
	// complain if it's not there, unless path = "" (root td).
#ifndef NDEBUG
	const size_t len = strlen(V_mount_point);
	if(len && V_mount_point[len-1] != '/')
		debug_warn("path doesn't end in '/'");
#endif

	// make sure it's not already mounted, i.e. in mounts.
	// also prevents mounting a parent directory of a previously mounted
	// directory, or vice versa. example: mount $install/data and then
	// $install/data/mods/official - mods/official would also be accessible
	// from the first mount point - bad.
	// no matter if it's an archive - still shouldn't be a "subpath".
	for(MountIt it = mounts.begin(); it != mounts.end(); ++it)
	{
		if(file_is_subpath(P_real_path, it->P_name.c_str()))
			WARN_RETURN(ERR_ALREADY_MOUNTED);
	}

	// disallow "." because "./" isn't supported on Windows.
	// it would also create a loophole for the parent td check above.
	// "./" and "/." are caught by CHECK_PATH.
	if(!strcmp(P_real_path, "."))
		WARN_RETURN(ERR_PATH_INVALID);

	// (count this as "init" to obviate a separate timer)
	stats_vfs_init_start();
	const Mount& m = add_mount(V_mount_point, P_real_path, 0, flags, pri);
	LibError ret = remount(m);
	stats_vfs_init_finish();
	return ret;
}


// rebuild the VFS, i.e. re-mount everything. open files are not affected.
// necessary after loose files or directories change, so that the VFS
// "notices" the changes and updates file locations. res calls this after
// dir_watch reports changes; can also be called from the console after a
// rebuild command. there is no provision for updating single VFS dirs -
// it's not worth the trouble.
LibError mount_rebuild()
{
	tree_clear();
	remount_all();
	return ERR_OK;
}


// unmount a previously mounted item, and rebuild the VFS afterwards.
LibError vfs_unmount(const char* P_name)
{
	// this removes all Mounts ensuing from the given mounting. their dtors
	// free all resources and there's no need to remove the files from
	// VFS (nor is this feasible), since it is completely rebuilt afterwards.

	MountIt begin = mounts.begin(), end = mounts.end();
	MountIt last = std::remove_if(begin, end,
		std::bind2nd(Mount::equal_to(), P_name));
	// none were removed - need to complain so that the caller notices.
	if(last == end)
		return ERR_PATH_NOT_FOUND;
	// trim list and actually remove 'invalidated' entries.
	mounts.erase(last, end);

	return mount_rebuild();
}






// if <path> or its ancestors are mounted,
// return a VFS path that accesses it.
// used when receiving paths from external code.
LibError mount_make_vfs_path(const char* P_path, char* V_path)
{
	for(MountIt it = mounts.begin(); it != mounts.end(); ++it)
	{
		const Mount& m = *it;
		if(m.type != MT_FILE)
			continue;

		const char* remove = m.P_name.c_str();
		const char* replace = m.V_mount_point.c_str();

		if(path_replace(V_path, P_path, remove, replace) == 0)
			return ERR_OK;
	}

	return ERR_PATH_NOT_FOUND;
}


void mount_init()
{
	tree_init();
}


void mount_shutdown()
{
	tree_shutdown();
	mount_unmount_all();
}






LibError mount_attach_real_dir(RealDir* rd, const char* P_path, const Mount* m, int flags)
{
	// more than one real dir mounted into VFS dir
	// (=> can't create files for writing here)
	if(rd->m)
		rd->m = (const Mount*)-1;
	else
		rd->m = m;

#ifndef NO_DIR_WATCH
	if(flags & VFS_MOUNT_WATCH)
	{
		char N_path[PATH_MAX];
		CHECK_ERR(file_make_full_native_path(P_path, N_path));
		CHECK_ERR(dir_add_watch(N_path, &rd->watch));
	}
#endif

	return ERR_OK;
}


void mount_detach_real_dir(RealDir* rd)
{
	rd->m = 0;

#ifndef NO_DIR_WATCH
	if(rd->watch)	// avoid dir_cancel_watch complaining
		WARN_ERR(dir_cancel_watch(rd->watch));
	rd->watch = 0;
#endif
}


LibError mount_populate(TDir* td, RealDir* rd)
{
	UNUSED2(td);
	UNUSED2(rd);
	return ERR_OK;
}


//-----------------------------------------------------------------------------






// rationale for not using virtual functions for file_open vs afile_open:
// it would spread out the implementation of each function and makes
// keeping them in sync harder. we will very rarely add new sources and
// all these functions are in one spot anyway.


// given a Mount, return the actual location (portable path) of
// <V_path>. used by vfs_realpath and VFile_reopen.
LibError x_realpath(const Mount* m, const char* V_path, char* P_real_path)
{
	const char* P_parent_path = 0;

	switch(m->type)
	{
	case MT_ARCHIVE:
		P_parent_path = h_filename(m->archive);
		break;
	case MT_FILE:
		P_parent_path = m->P_name.c_str();
		break;
	default:
		WARN_RETURN(ERR_INVALID_MOUNT_TYPE);
	}

	const char* remove = m->V_mount_point.c_str();
	const char* replace = P_parent_path;
	return path_replace(P_real_path, V_path, remove, replace);
}



LibError x_open(const Mount* m, const char* V_path, int flags, TFile* tf, XFile* xf)
{
	// declare variables used in the switch below to avoid needing {}.
	char N_path[PATH_MAX];
	uintptr_t memento = 0;

	switch(m->type)
	{
	case MT_ARCHIVE:
		if(flags & FILE_WRITE)
		{
			debug_warn("requesting write access to file in archive");
			return ERR_NOT_IMPLEMENTED;
		}
		memento = tfile_get_memento(tf);
		RETURN_ERR(afile_open(m->archive, V_path, memento, flags, &xf->u.zf));
		break;
	case MT_FILE:
		CHECK_ERR(x_realpath(m, V_path, N_path));
		RETURN_ERR(file_open(N_path, flags|FILE_DONT_SET_FN, &xf->u.f));
		// file_open didn't set fc.atom_fn due to FILE_DONT_SET_FN.
		xf->u.fc.atom_fn = file_make_unique_fn_copy(V_path);
		break;
	default:
		WARN_RETURN(ERR_INVALID_MOUNT_TYPE);
	}

	// success
	// note: don't assign these unless we succeed to avoid the
	// false impression that all is well.
	xf->type = m->type;
	xf->tf   = tf;
	return ERR_OK;
}


LibError x_close(XFile* xf)
{
	switch(xf->type)
	{
	// no file open (e.g. because x_open failed) -> nothing to do.
	case MT_NONE:
		return ERR_OK;

	case MT_ARCHIVE:
		(void)afile_close(&xf->u.zf);
		break;
	case MT_FILE:
		(void)file_close(&xf->u.f);
		break;
	default:
		WARN_RETURN(ERR_INVALID_MOUNT_TYPE);
	}

	// update file state in VFS tree
	// (must be done after close, since that calculates the size)
	if(xf->u.fc.flags & FILE_WRITE)
		tree_update_file(xf->tf, xf->u.fc.size, time(0));	// can't fail

	xf->type = MT_NONE;
	return ERR_OK;
}


LibError x_validate(const XFile* xf)
{
	switch(xf->type)
	{
	case MT_NONE:
		if(xf->tf != 0)
			return ERR_11;
		return ERR_OK;	// ok, nothing else to check

	case MT_FILE:
		if(xf->tf == 0)
			return ERR_12;
		return file_validate(&xf->u.f);

	case MT_ARCHIVE:
		// this could be set to 0 in x_open (since it's used to update the
		// VFS after newly written files are closed, but archive files
		// cannot be modified), but it's not ATM.
		if(xf->tf == 0)
			return ERR_13;
		return afile_validate(&xf->u.zf);

	default:
		return ERR_INVALID_MOUNT_TYPE;
	}
	UNREACHABLE;
}


bool x_is_open(const XFile* xf)
{
	return (xf->type != MT_NONE);
}


// VFile was exceeding HDATA_USER_SIZE. flags and size (required
// in File as well as VFile) are now moved into the union.
// use the functions below to insulate against change a bit.

off_t x_size(const XFile* xf)
{
	return xf->u.fc.size;
}


void x_set_flags(XFile* xf, uint flags)
{
	xf->u.fc.flags = flags;
}

uint x_flags(const XFile* xf)
{
	return xf->u.fc.flags;
}


LibError x_io_issue(XFile* xf, off_t ofs, size_t size, void* buf, XIo* xio)
{
	xio->type = xf->type;
	switch(xio->type)
	{
	case MT_ARCHIVE:
		return afile_io_issue(&xf->u.zf, ofs, size, buf, &xio->u.zio);
	case MT_FILE:
		return file_io_issue(&xf->u.f, ofs, size, buf, &xio->u.fio);
	default:
		WARN_RETURN(ERR_INVALID_MOUNT_TYPE);
	}
}


int x_io_has_completed(XIo* xio)
{
	switch(xio->type)
	{
	case MT_ARCHIVE:
		return afile_io_has_completed(&xio->u.zio);
	case MT_FILE:
		return file_io_has_completed(&xio->u.fio);
	default:
		WARN_RETURN(ERR_INVALID_MOUNT_TYPE);
	}
}


LibError x_io_wait(XIo* xio, void*& p, size_t& size)
{
	switch(xio->type)
	{
	case MT_ARCHIVE:
		return afile_io_wait(&xio->u.zio, p, size);
	case MT_FILE:
		return file_io_wait(&xio->u.fio, p, size);
	default:
		WARN_RETURN(ERR_INVALID_MOUNT_TYPE);
	}
}


LibError x_io_discard(XIo* xio)
{
	switch(xio->type)
	{
	case MT_ARCHIVE:
		return afile_io_discard(&xio->u.zio);
	case MT_FILE:
		return file_io_discard(&xio->u.fio);
	default:
		WARN_RETURN(ERR_INVALID_MOUNT_TYPE);
	}
}


LibError x_io_validate(const XIo* xio)
{
	switch(xio->type)
	{
	case MT_ARCHIVE:
		return afile_io_validate(&xio->u.zio);
	case MT_FILE:
		return file_io_validate(&xio->u.fio);
	default:
		return ERR_INVALID_MOUNT_TYPE;
	}
	UNREACHABLE;
}


ssize_t x_io(XFile* xf, off_t ofs, size_t size, FileIOBuf* pbuf, FileIOCB cb, uintptr_t ctx)
{
	switch(xf->type)
	{
	case MT_ARCHIVE:
		// (vfs_open makes sure it's not opened for writing if zip)
		return afile_read(&xf->u.zf, ofs, size, pbuf, cb, ctx);

	case MT_FILE:
		// normal file:
		// let file_io alloc the buffer if the caller didn't (i.e. p = 0),
		// because it knows about alignment / padding requirements
		return file_io(&xf->u.f, ofs, size, pbuf, cb, ctx);

	default:
		WARN_RETURN(ERR_INVALID_MOUNT_TYPE);
	}
}


LibError x_map(XFile* xf, void*& p, size_t& size)
{
	switch(xf->type)
	{
	case MT_ARCHIVE:
		return afile_map(&xf->u.zf, p, size);
	case MT_FILE:
		return file_map(&xf->u.f, p, size);
	default:
		WARN_RETURN(ERR_INVALID_MOUNT_TYPE);
	}
}


LibError x_unmap(XFile* xf)
{
	switch(xf->type)
	{
	case MT_ARCHIVE:
		return afile_unmap(&xf->u.zf);
	case MT_FILE:
		return file_unmap(&xf->u.f);
	default:
		WARN_RETURN(ERR_INVALID_MOUNT_TYPE);
	}
}
