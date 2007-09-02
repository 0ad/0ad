/**
 * =========================================================================
 * File        : vfs_mount.cpp
 * Project     : 0 A.D.
 * Description : mounts files and archives into VFS; provides x_* API
 *             : that dispatches to file or archive implementation.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "vfs_mount.h"

#include <deque>
#include <list>
#include <string>
#include <algorithm>
#include <ctime>

#include "lib/sysdep/dir_watch.h"
#include "lib/res/h_mgr.h"
#include "file_internal.h"


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


Handle mount_get_archive(const Mount* m)
{
	return m->archive;
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


// given Mount and V_path, return its actual location (portable path).
// works for any type of path: file or directory.
LibError mount_realpath(const char* V_path, const Mount* m, char* P_real_path)
{
	const char* remove = m->V_mount_point.c_str();
	const char* replace = m->P_name.c_str();	// P_parent_path
	CHECK_ERR(path_replace(P_real_path, V_path, remove, replace));

	// if P_real_path ends with '/' (a remnant from V_path), strip
	// it because that's not acceptable for portable paths.
	const size_t P_len = strlen(P_real_path);
	if(P_len != 0 && P_real_path[P_len-1] == '/')
		P_real_path[P_len-1] = '\0';

	return INFO::OK;
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
struct ZipCBParams : boost::noncopyable
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
};

// called by add_ent's afile_enum for each file in the archive.
// we get the full path, since that's what is stored in Zip archives.
//
// [total time 21ms, with ~2000 file's (includes add_file cost)]
static LibError afile_cb(const char* atom_fn, const struct stat* s, uintptr_t memento, uintptr_t user)
{
	CHECK_PATH(atom_fn);

	const char* name = path_name_only(atom_fn);
	char path[PATH_MAX];
	path_dir_only(atom_fn, path);
	const char* atom_path = file_make_unique_fn_copy(path);

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
	return INFO::CB_CONTINUE;
}


static bool archive_less(Handle hza1, Handle hza2)
{
	const char* fn1 = h_filename(hza1);
	const char* fn2 = h_filename(hza2);
	return strcmp(fn1, fn2) < 0;
}

typedef std::vector<Handle> Archives;
typedef Archives::const_iterator ArchiveCIt;

// return value is INFO::OK iff archives != 0 and the file should not be
// added to VFS (e.g. because it is an archive).
static LibError enqueue_archive(const char* name, const char* P_archive_dir, Archives* archives)
{
	// caller doesn't want us to check if this is a Zip file. this is the
	// case in all subdirectories of the mount point, since checking for all
	// mounted files would be slow. see mount_dir_tree.
	if(!archives)
		return INFO::SKIPPED;

	// get complete path for archive_open.
	// this doesn't (need to) work for subdirectories of the mounted td!
	// we can't use mount_get_path because we don't have the VFS path.
	char P_path[PATH_MAX];
	RETURN_ERR(path_append(P_path, P_archive_dir, name));

	// just open the Zip file and see if it's valid. we don't bother
	// checking the extension because archives won't necessarily be
	// called .zip (e.g. Quake III .pk3).
	Handle archive = archive_open(P_path);
	// .. special case: <name> is recognizable as a Zip file but is
	//    invalid and can't be opened. avoid adding it to
	//    archive list and/or VFS.
	if(archive == ERR::CORRUPTED)
		goto do_not_add_to_VFS_or_list;
	RETURN_ERR(archive);

	archives->push_back(archive);

	// avoid also adding the archive file itself to VFS.
	// (when caller sees INFO::OK, they skip the file)
do_not_add_to_VFS_or_list:
	return INFO::OK;
}

static LibError mount_archive(TDir* td, const Mount& m)
{
	ZipCBParams params(td, &m);
	archive_enum(m.archive, afile_cb, (uintptr_t)&params);
	return INFO::OK;
}

static LibError mount_archives(TDir* td, Archives* archives, const Mount* mount)
{
	// VFS_MOUNT_ARCHIVES flag wasn't set, or no archives present
	if(archives->empty())
		return INFO::OK;

	// load archives in alphabetical filename order to allow patches
	std::sort(archives->begin(), archives->end(), archive_less);

	for(ArchiveCIt it = archives->begin(); it != archives->end(); ++it)
	{
		Handle hza = *it;

		// add this archive to the mount list (address is guaranteed to
		// remain valid).
		const Mount& m = add_mount(mount->V_mount_point.c_str(), mount->P_name.c_str(), hza, mount->flags, mount->pri);

		mount_archive(td, m);
	}

	return INFO::OK;
}


//-----------------------------------------------------------------------------

struct TDirAndPath
{
	TDir* td;
	std::string path;

	TDirAndPath(TDir* d, const char* p)
	: td(d), path(p)
	{
	}
};

typedef std::deque<TDirAndPath> DirQueue;



static LibError enqueue_dir(TDir* parent_td, const char* name,
	const char* P_parent_path, DirQueue* dir_queue)
{
	// caller doesn't want us to enqueue subdirectories; bail.
	if(!dir_queue)
		return INFO::OK;

	// skip versioning system directories - this avoids cluttering the
	// VFS with hundreds of irrelevant files.
	// we don't do this for Zip files because it's harder (we'd have to
	// strstr the entire path) and it is assumed the Zip file builder
	// will take care of it.
	if(!strcmp(name, "CVS") || !strcmp(name, ".svn"))
		return INFO::OK;

	// prepend parent path to get complete pathname.
	char P_path[PATH_MAX];
	CHECK_ERR(path_append(P_path, P_parent_path, name));
	
	// create subdirectory..
	TDir* td;
	CHECK_ERR(tree_add_dir(parent_td, name, &td));
	// .. and add it to the list of directories to visit.
	dir_queue->push_back(TDirAndPath(td, P_path));
	return INFO::OK;
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

	if(enqueue_archive(name, m->P_name.c_str(), archives) == INFO::OK)
		// return value indicates this file shouldn't be added to VFS
		// (see enqueue_archive)
		return INFO::OK;

	// notify archive builder that this file could be archived but
	// currently isn't; if there are too many of these, archive will be
	// rebuilt.
	// note: check if archivable to exclude stuff like screenshots
	// from counting towards the threshold.
	if(mount_is_archivable(m))
	{
		// prepend parent path to get complete pathname.
		char V_path[PATH_MAX];
		CHECK_ERR(path_append(V_path, tfile_get_atom_fn((TFile*)td), name));
		const char* atom_fn = file_make_unique_fn_copy(V_path);

		vfs_opt_notify_loose_file(atom_fn);
	}

	// it's a regular data file; add it to the directory.
	return tree_add_file(td, name, m, ent->size, ent->mtime, 0);
}


// note: full path is needed for the dir watch.
static LibError populate_dir(TDir* td, const char* P_path, const Mount* m,
	DirQueue* dir_queue, Archives* archives, uint flags)
{
	LibError err;

	RealDir* rd = tree_get_real_dir(td);
	RETURN_ERR(mount_attach_real_dir(rd, P_path, m, flags));

	DirIterator d;
	RETURN_ERR(dir_open(P_path, &d));

	DirEnt ent;
	for(;;)
	{
		// don't RETURN_ERR since we need to close d.
		err = dir_next_ent(&d, &ent);
		if(err != INFO::OK)
			break;

		err = add_ent(td, &ent, P_path, m, dir_queue, archives);
		WARN_ERR(err);
	}

	WARN_ERR(dir_close(&d));
	return INFO::OK;
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
static LibError mount_dir_tree(TDir* td_start, const Mount& m)
{
	LibError err = INFO::OK;

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
	dir_queue.push_back(TDirAndPath(td_start, m.P_name.c_str()));

	do
	{
		TDir* const td     = dir_queue.front().td;
		const char* P_path = dir_queue.front().path.c_str();

		LibError ret = populate_dir(td, P_path, &m, pdir_queue, parchives, m.flags);
		if(err == INFO::OK)
			err = ret;

		// prevent searching for archives in subdirectories (slow!). this
		// is currently required by the implementation anyway.
		parchives = 0;

		dir_queue.pop_front();
		// pop at end of loop, because we hold a c_str() reference.
	}
	while(!dir_queue.empty());

	// do not pass parchives because that has been set to 0!
	mount_archives(td_start, &archives, &m);

	return INFO::OK;
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
	CHECK_ERR(tree_add_path(m.V_mount_point.c_str(), &m, &td));

	switch(m.type)
	{
	case MT_ARCHIVE:
		return mount_archive(td, m);
	case MT_FILE:
		return mount_dir_tree(td, m);
	default:
		WARN_RETURN(ERR::MOUNT_INVALID_TYPE);
	}
}

static void mount_unmount_all(void)
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
LibError vfs_mount(const char* V_mount_point, const char* P_real_path, uint flags, uint pri)
{
	// make sure caller didn't forget the required trailing '/'.
	debug_assert(VFS_PATH_IS_DIR(V_mount_point));

	// make sure it's not already mounted, i.e. in mounts.
	// also prevents mounting a parent directory of a previously mounted
	// directory, or vice versa. example: mount $install/data and then
	// $install/data/mods/official - mods/official would also be accessible
	// from the first mount point - bad.
	// no matter if it's an archive - still shouldn't be a "subpath".
	for(MountIt it = mounts.begin(); it != mounts.end(); ++it)
	{
		if(path_is_subpath(P_real_path, it->P_name.c_str()))
			WARN_RETURN(ERR::ALREADY_MOUNTED);
	}

	// disallow "." because "./" isn't supported on Windows.
	// it would also create a loophole for the parent td check above.
	// "./" and "/." are caught by CHECK_PATH.
	if(!strcmp(P_real_path, "."))
		WARN_RETURN(ERR::PATH_NON_CANONICAL);

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
	return INFO::OK;
}


struct IsArchiveMount
{
	bool operator()(const Mount& m) const
	{
		return (m.type == MT_ARCHIVE);
	}
};

// "backs off of" all archives - closes their files and allows them to
// be rewritten or deleted (required by archive builder).
// must call mount_rebuild when done with the rewrite/deletes,
// because this call leaves the VFS in limbo!!
//
// note: this works because archives are not "first-class" mount objects -
// they are added to the list whenever a real mount point's root directory
// contains archives. hence, we can just remove them from the list.
void mount_release_all_archives()
{
	mounts.remove_if(IsArchiveMount());
}


// unmount a previously mounted item, and rebuild the VFS afterwards.
LibError vfs_unmount(const char* P_name)
{
	// this removes all Mounts ensuing from the given mounting. their dtors
	// free all resources and there's no need to remove the files from
	// VFS (nor is this possible), since it is completely rebuilt afterwards.

	MountIt begin = mounts.begin(), end = mounts.end();
	MountIt last = std::remove_if(begin, end,
		std::bind2nd(Mount::equal_to(), P_name));
	// none were removed - need to complain so that the caller notices.
	if(last == end)
		WARN_RETURN(ERR::TNODE_NOT_FOUND);
	// trim list and actually remove 'invalidated' entries.
	mounts.erase(last, end);

	return mount_rebuild();
}






// if <path> or its ancestors are mounted,
// return a VFS path that accesses it.
// used when receiving paths from external code.
LibError mount_make_vfs_path(const char* P_path, char* V_path)
{
	debug_printf("mount_make_vfs_path %s %s\n", P_path, V_path);
	for(MountIt it = mounts.begin(); it != mounts.end(); ++it)
	{
		const Mount& m = *it;
		if(m.type != MT_FILE)
			continue;

		const char* remove = m.P_name.c_str();
		const char* replace = m.V_mount_point.c_str();

		if(path_replace(V_path, P_path, remove, replace) == INFO::OK)
			return INFO::OK;
	}

	WARN_RETURN(ERR::TNODE_NOT_FOUND);
}




static const Mount* write_target;

// 2006-05-09 JW note: we are wanting to move XMB files into a separate
// folder tree (no longer interspersed with XML), so that deleting them is
// easier and dirs are less cluttered.
//
// if several mods are active, VFS would have several RealDirs mounted
// and could no longer automatically determine the write target.
//
// one solution would be to use this set_write_target support to choose the
// correct dir; however, XMB files may be generated whilst editing
// (which also requires a write_target to write files that are actually
// currently in archives), so we'd need to save/restore write_target.
// this would't be thread-safe => disaster.
//
// a vfs_store_to(filename, flags, N_actual_path) API would work, but it'd
// impose significant burden on users (finding the actual native dir),
// and be prone to abuse. additionally, it would be difficult to
// propagate N_actual_path to VFile_reload where it is needed;
// this would end up messy.
//
// instead, we'll write XMB files into VFS path "mods/$MODNAME/..",
// into which the realdir of the same name (located in some writable folder)
// is mounted; VFS therefore can write without problems.
//
// however, other code (e.g. archive builder) doesn't know about this
// trick - it only sees the flat VFS namespace, which doesn't
// include mods/$MODNAME (that is hidden). to solve this, we also mount
// any active mod's XMB dir into VFS root for read access.


// set current "mod write directory" to P_target_dir, which must
// already have been mounted into the VFS.
// all files opened for writing with the FILE_WRITE_TO_TARGET flag set will
// be written into the appropriate subdirectory of this mount point.
//
// this allows e.g. the editor to write files that are already
// stored in archives, which are read-only.
LibError vfs_set_write_target(const char* P_target_dir)
{
	for(MountIt it = mounts.begin(); it != mounts.end(); ++it)
	{
		const Mount& m = *it;
		// skip if not a directory mounting
		if(m.type != MT_FILE)
			continue;

		// found it in list of mounted dirs
		if(!strcmp(m.P_name.c_str(), P_target_dir))
		{
			write_target = &m;
			return INFO::OK;
		}
	}

	WARN_RETURN(ERR::NOT_MOUNTED);
}


// 'relocate' tf to the mounting established by vfs_set_write_target.
// call if <tf> is being opened with FILE_WRITE_TO_TARGET flag set.
LibError set_mount_to_write_target(TFile* tf)
{
	if(!write_target)
		WARN_RETURN(ERR::NOT_MOUNTED);

	tfile_set_mount(tf, write_target);

	// invalidate the previous values. we don't need to be clever and
	// set size to that of the file in the new write_target mount point.
	// this is because we're only called for files that are being
	// opened for writing, which will change these values anyway.
	tree_update_file(tf, 0, 0);

	return INFO::OK;
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


static const Mount* MULTIPLE_MOUNTINGS = (const Mount*)-1;

// RDTODO: when should this be called? TDir ctor can already set this.
LibError mount_attach_real_dir(RealDir* rd, const char* P_path, const Mount* m, uint flags)
{
	// more than one real dir mounted into VFS dir
	// (=> can't create files for writing here)
	if(rd->m)
	{
		// HACK: until RealDir reorg is done, we're going to have to deal with
		// "attaching" to real dirs twice. don't mess up rd->m if m is the same.
		if(rd->m != m)
			rd->m = MULTIPLE_MOUNTINGS;
	}
	else
		rd->m = m;

#ifndef NO_DIR_WATCH
	if(flags & VFS_MOUNT_WATCH)
	{
		// 'watch' this directory for changes to support hotloading.
		// note: do not cause this function to return an error if
		// something goes wrong - this step is basically optional.
		char N_path[PATH_MAX];
		if(file_make_full_native_path(P_path, N_path) == INFO::OK)
			(void)dir_add_watch(N_path, &rd->watch);
	}
#endif

	return INFO::OK;
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


LibError mount_create_real_dir(const char* V_path, const Mount* m)
{
	debug_assert(VFS_PATH_IS_DIR(V_path));

	if(!m || m == MULTIPLE_MOUNTINGS || m->type != MT_FILE)
		return INFO::OK;

	char P_path[PATH_MAX];
	RETURN_ERR(mount_realpath(V_path, m, P_path));

	return dir_create(P_path);
}


LibError mount_populate(TDir* td, RealDir* rd)
{
	UNUSED2(td);
	UNUSED2(rd);
	return INFO::OK;
}
