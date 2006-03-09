#include "precompiled.h"

#include <string.h>
#include <time.h>

#include <string>
#include <vector>
#include <algorithm>

#include "lib/allocators.h"
#include "lib/adts.h"
#include "file_internal.h"


// we add/cancel directory watches from the VFS mount code for convenience -
// it iterates through all subdirectories anyway (*) and provides storage for
// a key to identify the watch (obviates separate TDir -> watch mapping).
//
// define this to strip out that code - removes .watch from struct TDir,
// and calls to res_watch_dir / res_cancel_watch.
//
// *: the add_watch code would need to iterate through subdirs and watch
//    each one, because the monitor API (e.g. FAM) may only be able to
//    watch single directories, instead of a whole subdirectory tree.
//#define NO_DIR_WATCH


// Mount  = location of a file in the tree.
// TFile = all information about a file stored in the tree.
// TDir  = container holding TFile-s representing a dir. in the tree.


static void* node_alloc();


// remembers which VFS file is the most recently modified.
static time_t most_recent_mtime;
static void set_most_recent_if_newer(time_t mtime)
{
	most_recent_mtime = MAX(most_recent_mtime, mtime);
}
time_t tree_most_recent_mtime()
{
	return most_recent_mtime;
}

//-----------------------------------------------------------------------------
// locking
// these are exported to protect the vfs_mount list; apart from that, it is
// sufficient for VFS thread-safety to lock all of this module's APIs.

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void tree_lock()
{
	pthread_mutex_lock(&mutex);
}

void tree_unlock()
{
	pthread_mutex_unlock(&mutex);
}


//-----------------------------------------------------------------------------

enum TNodeType
{
	NT_DIR,
	NT_FILE
};

class TNode
{
public:
	TNodeType type;

	// rationale: we store both entire path and name component.
	// this increases size of VFS (2 pointers needed here) and
	// filename storage, but allows getting path without having to
	// iterate over all dir name components.
	// we could retrieve name via strrchr(path, '/'), but that is slow.
	const char* V_path;
	const char* name;

	TNode(TNodeType type_, const char* V_path_, const char* name_)
		: type(type_), V_path(V_path_), name(name_)
	{
	}
};


class TFile : public TNode
{
public:
	// required:
	const Mount* m;
	// allocated and owned by caller (mount code)

	off_t size;
	time_t mtime;

	uintptr_t memento;

	TFile(const char* V_path, const char* name, const Mount* m_)
		: TNode(NT_FILE, V_path, name)
	{
		m = m_;
		size = 0;
		mtime = 0;
		memento = 0;
	}
};


template<> class DHT_Traits<const char*, TNode*>
{
public:
	static const size_t initial_entries = 16;
	size_t hash(const char* key) const
	{
		return (size_t)fnv_lc_hash(key);
	}
	bool equal(const char* k1, const char* k2) const
	{
		// exact match
		if(!strcmp(k1, k2))
			return true;
#ifndef NDEBUG
		// matched except for case: this can have 2 causes:
		// - intentional. that would be legitimate but doesn't make much
		//   sense and isn't expected.
		// - bug, e.g. discarding filename case in a filelist.
		//   this risks not being able to find the file (since VFS and
		//   possibly OS are case-sensitive) and wastes memory here.
		// what we'll do is warn and treat as separate filename
		// (least surprise).
//		if(!stricmp(k1, k2))
//			debug_warn("filenames differ only in case: bug?");
#endif
		return false;
	}
	const char* get_key(TNode* t) const
	{
		return t->name;
	}
};
typedef DynHashTbl<const char*, TNode*, DHT_Traits<const char*, TNode*> > TChildren;
typedef TChildren::iterator TChildrenIt;

enum TDirFlags
{
	TD_POPULATED = 1
};

class TDir : public TNode
{
	int flags;	// enum TDirFlags

	RealDir rd;

	TChildren children;

public:
	TDir(const char* V_path, const char* name)
		: TNode(NT_DIR, V_path, name), children()
	{
		flags = 0;
		rd.m = 0;
		rd.watch = 0;
	}

	TNode* find(const char* name) const { return children.find(name); }
	TChildrenIt begin() const { return children.begin(); }
	TChildrenIt end() const { return children.end(); }

	// non-const - caller may change e.g. rd.watch
	RealDir& get_rd() { return rd; }

	void populate()
	{
		// the caller may potentially access this directory.
		// make sure it has been populated with loose files/directories.
		if(!(flags & TD_POPULATED))
		{
			WARN_ERR(mount_populate(this, &rd));
			flags |= TD_POPULATED;
		}
	}


	LibError add(const char* name_tmp, TNodeType type, TNode** pnode)
	{
		char V_new_path_tmp[VFS_MAX_PATH];
		vfs_path_append(V_new_path_tmp, V_path, name_tmp);
		const char* V_new_path = file_make_unique_fn_copy(V_new_path_tmp);
		const char* name = path_name_only(V_new_path);

		if(!path_component_valid(name))
			return ERR_PATH_INVALID;

		TNode* node = children.find(name);
		if(node)
		{
			if(node->type != type)
				return (type == NT_FILE)? ERR_NOT_FILE : ERR_NOT_DIR;

			*pnode = node;
			return INFO_ALREADY_PRESENT;
		}

		// note: if anything below fails, this mem remains allocated in the
		// pool, but that "can't happen" and is OK because pool is big enough.
		void* mem = node_alloc();
		if(!mem)
			return ERR_NO_MEM;
#include "nommgr.h"
		if(type == NT_FILE)
			node = new(mem) TFile(V_new_path, name, rd.m);
		else
			node = new(mem) TDir(V_new_path, name);
#include "mmgr.h"

		children.insert(name, node);

		*pnode = node;
		return ERR_OK;
	}

	// empty this directory and all subdirectories; used when rebuilding VFS.
	void clearR()
	{
		// recurse for all subdirs
		// (preorder traversal - need to do this before clearing the list)
		for(TChildrenIt it = children.begin(); it != children.end(); ++it)
		{
			TNode* node = *it;
			if(node->type == NT_DIR)
				((TDir*)node)->clearR();
		}

		// wipe out this directory
		children.clear();

		// the watch is restored when this directory is repopulated; we must
		// remove it in case the real directory backing this one was deleted.
		mount_detach_real_dir(&rd);
	}
};







static Pool node_pool;

static inline void node_init()
{
	const size_t el_size = MAX(sizeof(TDir), sizeof(TFile));
	(void)pool_create(&node_pool, VFS_MAX_FILES*el_size, el_size);
}

static inline void node_shutdown()
{
	(void)pool_destroy(&node_pool);
}

static void* node_alloc()
{
	return pool_alloc(&node_pool, 0);
}

static inline void node_free_all()
{
	pool_free_all(&node_pool);
}


//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////



static void displayR(TDir* td, int indent_level)
{
	const char indent[] = "    ";

	TChildrenIt it;

	// list all files in this dir
	for(it = td->begin(); it != td->end(); ++it)
	{
		TNode* node = (*it);
		if(node->type != NT_FILE)
			continue;
		const char* name = node->name;

		TFile& file = *((TFile*)node);
		char file_location = mount_get_type(file.m);
		char* timestamp = ctime(&file.mtime);
		timestamp[24] = '\0';	// remove '\n'
		const off_t size = file.size;

		// build format string: tell it how long the filename may be,
		// so that it takes up all space before file info column.
		char fmt[25];
		int chars = 80 - indent_level*(sizeof(indent)-1);
		sprintf(fmt, "%%-%d.%ds (%%c; %%6d; %%s)\n", chars, chars);

		for(int i = 0; i < indent_level; i++)
			printf(indent);
		printf(fmt, name, file_location, size, timestamp);
	}

	// recurse over all subdirs
	for(it = td->begin(); it != td->end(); ++it)
	{
		TNode* node = (*it);
		if(node->type != NT_DIR)
			continue;
		const char* subdir_name = node->name;

		// write subdir's name
		// note: do it now, instead of in recursive call so that:
		// - we don't have to pass dir_name parameter;
		// - the VFS root node isn't displayed.
		for(int i = 0; i < indent_level; i++)
			printf(indent);
		printf("[%s/]\n", subdir_name);

		TDir* subdir = ((TDir*)node);
		displayR(subdir, indent_level+1);
	}
}


static LibError lookup(TDir* td, const char* path, uint flags, TNode** pnode)
{
	// early out: "" => return this directory (usually VFS root)
	if(path[0] == '\0')
	{
		*pnode = (TNode*)td;	// HACK: TDir is at start of TNode
		return ERR_OK;
	}

	CHECK_PATH(path);
	debug_assert( (flags & ~(LF_CREATE_MISSING|LF_START_DIR)) == 0 );
	// no undefined bits set

	const bool create_missing = !!(flags & LF_CREATE_MISSING);

	// copy into (writeable) buffer so we can 'tokenize' path components
	// by replacing '/' with '\0'.
	char V_path[VFS_MAX_PATH];
	strcpy_s(V_path, sizeof(V_path), path);
	char* cur_component = V_path;

	TNodeType type = NT_DIR;

	// successively navigate to the next component in <path>.
	TNode* node = 0;
	for(;;)
	{
		// "extract" cur_component string (0-terminate by replacing '/')
		char* slash = (char*)strchr(cur_component, '/');
		if(!slash)
		{
			// string ended in slash => return the current dir node.
			if(*cur_component == '\0')
				break;

			// it's a filename
			type = NT_FILE;
		}
		// normal operation (cur_component is a directory)
		else
		{
			td->populate();

			*slash = '\0';
		}

		// create <cur_component> (no-op if it already exists)
		if(create_missing)
			RETURN_ERR(td->add(V_path, type, &node));
		else
		{
			node = td->find(cur_component);
			if(!node)
				return slash? ERR_PATH_NOT_FOUND : ERR_FILE_NOT_FOUND;
			if(node->type != type)
				return slash? ERR_NOT_DIR : ERR_NOT_FILE;
		}

		// cur_component was a filename => we're done
		if(!slash)
			break;
		// else: it was a directory; advance
		// .. undo having replaced '/' with '\0' - this means V_path will
		//    store the complete path up to and including cur_component.
		*slash = '/';
		cur_component = slash+1;
		td = (TDir*)node;
	}

	// success.
	*pnode = node;
	return ERR_OK;
}


//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

static TDir* tree_root;

// rationale: can't do this in tree_shutdown - we'd leak at exit.
// calling from tree_add* is ugly as well, so require manual init.
void tree_init()
{
	node_init();

	void* mem = node_alloc();
	if(mem)
	{
#include "nommgr.h"
		tree_root = new(mem) TDir("", "");
#include "mmgr.h"
	}
}


void tree_shutdown()
{
	node_shutdown();
}


void tree_clear()
{
	tree_root->clearR();
	node_free_all();
}


// write a representation of the VFS tree to stdout.
void tree_display()
{
	displayR(tree_root, 0);
}


LibError tree_add_file(TDir* td, const char* name,
	const Mount* m, off_t size, time_t mtime, uintptr_t memento)
{
	TNode* node;
	LibError ret = td->add(name, NT_FILE, &node);
	RETURN_ERR(ret);
	if(ret == INFO_ALREADY_PRESENT)
	{
		TFile* tf = (TFile*)node;
		if(!mount_should_replace(tf->m, m, tf->size, size, tf->mtime, mtime))
			return INFO_ALREADY_PRESENT;

		stats_vfs_file_remove(tf->size);
	}

	TFile* tf = (TFile*)node;
	tf->m       = m;
	tf->mtime   = mtime;
	tf->size    = size;
	tf->memento = memento;
	stats_vfs_file_add(size);

	set_most_recent_if_newer(mtime);
	return ERR_OK;
}


LibError tree_add_dir(TDir* td, const char* name, TDir** ptd)
{
	TNode* node;
	RETURN_ERR(td->add(name, NT_DIR, &node));
	*ptd = (TDir*)node;
	return ERR_OK;
}



LibError tree_lookup_dir(const char* path, TDir** ptd, uint flags)
{
	// path is not a directory; TDir::lookup might return a file node
	if(path[0] != '\0' && path[strlen(path)-1] != '/')
		return ERR_NOT_DIR;

	TDir* td = (flags & LF_START_DIR)? *ptd : tree_root;
	TNode* node;
	CHECK_ERR(lookup(td, path, flags, &node));
		// directories should exist, so warn if this fails
	*ptd = (TDir*)node;
	return ERR_OK;
}


LibError tree_lookup(const char* path, TFile** pfile, uint flags)
{
	// path is not a file; TDir::lookup might return a directory node
	if(path[0] == '\0' || path[strlen(path)-1] == '/')
		return ERR_NOT_FILE;

	TNode* node;
	LibError ret = lookup(tree_root, path, flags, &node);
	RETURN_ERR(ret);
	*pfile = (TFile*)node;
	return ERR_OK;
}


//////////////////////////////////////////////////////////////////////////////




// rationale: see DirIterator definition in file.h.
struct TreeDirIterator_
{
	TChildren::iterator it;

	// cache end() to avoid needless copies
	TChildren::iterator end;

	// the directory we're iterating over; this is used to lock/unlock it,
	// i.e. prevent modifications that would invalidate the iterator.
	TDir* td;
};

cassert(sizeof(TreeDirIterator_) <= sizeof(TreeDirIterator));


LibError tree_dir_open(const char* path_slash, TreeDirIterator* d_)
{
	TreeDirIterator_* d = (TreeDirIterator_*)d_;

	TDir* td;
	CHECK_ERR(tree_lookup_dir(path_slash, &td));

	// we need to prevent modifications to this directory while an iterator is
	// active, otherwise entries may be skipped or no longer valid addresses
	// accessed. blocking other threads is much more convenient for callers
	// than having to check for ERR_AGAIN on every call, so we use a mutex
	// instead of a simple refcount. we don't bother with fine-grained locking
	// (e.g. per directory or read/write locks) because it would result in
	// more overhead (we have hundreds of directories) and is unnecessary.
	tree_lock();

	d->it  = td->begin();
	d->end = td->end();
	d->td  = td;
	return ERR_OK;
}


LibError tree_dir_next_ent(TreeDirIterator* d_, DirEnt* ent)
{
	TreeDirIterator_* d = (TreeDirIterator_*)d_;

	if(d->it == d->end)
		return ERR_DIR_END;

	const TNode* node = *(d->it++);
	ent->name = node->name;

	// set size and mtime fields depending on node type:
	switch(node->type)
	{
	case NT_DIR:
		ent->size = -1;
		ent->mtime = 0;	// not currently supported for dirs
		ent->tf    = 0;
		break;
	case NT_FILE:
	{
		TFile* tf = (TFile*)node;
		ent->size  = tf->size;
		ent->mtime = tf->mtime;
		ent->tf    = tf;
		break;
	}
	default:
		debug_warn("invalid TNode type");
	}

	return ERR_OK;
}


LibError tree_dir_close(TreeDirIterator* UNUSED(d))
{
	tree_unlock();

	// no further cleanup needed. we could zero out d but that might
	// hide bugs; the iterator is safe (will not go beyond end) anyway.
	return ERR_OK;
}


//-----------------------------------------------------------------------------
// get/set

const Mount* tfile_get_mount(const TFile* tf)
{
	return tf->m;
}

uintptr_t tfile_get_memento(const TFile* tf)
{
	return tf->memento;
}

const char* tfile_get_atom_fn(const TFile* tf)
{
	return ((TNode*)tf)->V_path;
}



void tree_update_file(TFile* tf, off_t size, time_t mtime)
{
	tf->size  = size;
	tf->mtime = mtime;
}


// get file status (mode, size, mtime). output param is undefined on error.
LibError tree_stat(const TFile* tf, struct stat* s)
{
	// all stat members currently supported are stored in TFile, so we
	// can return them directly without having to call file|zip_stat.
	s->st_mode  = S_IFREG;
	s->st_size  = tf->size;
	s->st_mtime = tf->mtime;

	return ERR_OK;
}


RealDir* tree_get_real_dir(TDir* td)
{
	return &td->get_rd();
}
