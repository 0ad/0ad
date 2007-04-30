/**
 * =========================================================================
 * File        : vfs_tree.cpp
 * Project     : 0 A.D.
 * Description : the actual 'filesystem' and its tree of directories.
 *
 * @author Jan.Wassenberg@stud.uni-karlsruhe.de
 * =========================================================================
 */

/*
 * Copyright (c) 2005 Jan Wassenberg
 *
 * Redistribution and/or modification are also permitted under the
 * terms of the GNU General Public License as published by the
 * Free Software Foundation (version 2 or later, at your option).
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "precompiled.h"
#include "vfs_tree.h"

#include <string.h>
#include <time.h>
#include <string>
#include <vector>
#include <algorithm>

#include "lib/posix/posix_pthread.h"
#include "lib/allocators.h"
#include "lib/adts.h"
#include "file_internal.h"


AT_STARTUP(\
	error_setDescription(ERR::TNODE_NOT_FOUND, "File/directory not found");\
	error_setDescription(ERR::TNODE_WRONG_TYPE, "Using a directory as file or vice versa");\
	\
	error_setEquivalent(ERR::TNODE_NOT_FOUND, ENOENT);\
)


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
	// allocated and owned by vfs_mount
	const Mount* m;

	// rationale: we store both entire path and name component.
	// this increases size of VFS (2 pointers needed here) and
	// filename storage, but allows getting path without having to
	// iterate over all dir name components.
	// we could retrieve name via strrchr(path, '/'), but that is slow.
	const char* V_path;
	// this is compared as a normal string (not pointer comparison), but
	// the pointer passed must obviously remain valid, so it is
	// usually an atom_fn.
	const char* name;

	TNode(TNodeType type_, const char* V_path_, const char* name_, const Mount* m_)
		: type(type_), V_path(V_path_), name(name_), m(m_)
	{
	}
};


class TFile : public TNode
{
public:
	off_t size;
	time_t mtime;

	uintptr_t memento;

	TFile(const char* V_path, const char* name, const Mount* m)
		: TNode(NT_FILE, V_path, name, m)
	{
		size = 0;
		mtime = 0;
		memento = 0;
	}
};


template<> class DHT_Traits<const char*, TNode*>
{
public:
	static const size_t initial_entries = 32;
	size_t hash(const char* key) const
	{
		return (size_t)fnv_lc_hash(key);
	}
	bool equal(const char* k1, const char* k2) const
	{
		// note: in theory, we could take advantage of the atom_fn
		// mechanism to only compare string pointers. however, we're
		// dealing with path *components* here. adding these as atoms would
		// about double the memory used (to ~1 MB) and require a bit of
		// care in the implementation of file_make_unique_path_copy
		// (must not early-out before checking the hash table).
		//
		// given that path components are rather short, string comparisons
		// are not expensive and we'll just go with that for simplicity.
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
//		if(!strcasecmp(k1, k2))
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
	uint flags;	// enum TDirFlags

	TChildren children;

public:
RealDir rd;	// HACK; removeme

	TDir(const char* V_path, const char* name, const Mount* m_)
		: TNode(NT_DIR, V_path, name, 0), children()
	{
		flags = 0;

		rd.m = m_;
		rd.watch = 0;
		mount_create_real_dir(V_path, rd.m);
	}

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

	TNode* find(const char* name) const
	{
		return children.find(name);
	}

	// must not be called if already exists! use find() first or
	// find_and_add instead.
	LibError add(const char* name_tmp, TNodeType type, TNode** pnode, const Mount* m_override = 0)
	{
		// note: must be done before path_append for security
		// (otherwise, '/' in <name_tmp> wouldn't be caught)
		RETURN_ERR(path_component_validate(name_tmp));

		char V_new_path_tmp[PATH_MAX];
		const uint flags = (type == NT_DIR)? PATH_APPEND_SLASH : 0;
		RETURN_ERR(path_append(V_new_path_tmp, V_path, name_tmp, flags));
		const char* V_new_path = file_make_unique_fn_copy(V_new_path_tmp);
		const char* name = path_name_only(V_new_path);
		// for directory nodes, V_path ends in slash, so name cannot be
		// derived via path_last_component. instead, we have to make an
		// atom_fn out of name_tmp.
		// this effectively doubles the amount of directory path text,
		// but it's not that bad.
		if(type == NT_DIR)
			name = file_make_unique_fn_copy(name_tmp);

		const Mount* m = rd.m;
		if(m_override)
			m = m_override;

		// note: if anything below fails, this mem remains allocated in the
		// pool, but that "can't happen" and is OK because pool is big enough.
		void* mem = node_alloc();
		if(!mem)
			WARN_RETURN(ERR::NO_MEM);
		TNode* node;
#include "lib/nommgr.h"
		if(type == NT_FILE)
			node = new(mem) TFile(V_new_path, name, m);
		else
			node = new(mem) TDir (V_new_path, name, m);
#include "lib/mmgr.h"

		children.insert(name, node);

		*pnode = node;
		return INFO::OK;
	}

	LibError find_and_add(const char* name, TNodeType type, TNode** pnode, const Mount* m = 0)
	{
		TNode* node = children.find(name);
		if(node)
		{
			// wrong type (dir vs. file)
			if(node->type != type)
				WARN_RETURN(ERR::TNODE_WRONG_TYPE);

			*pnode = node;
			return INFO::ALREADY_EXISTS;
		}

		return add(name, type, pnode, m);
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
			{
				((TDir*)node)->clearR();
				((TDir*)node)->~TDir();
			}
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


struct LookupCbParams : boost::noncopyable
{
	const bool create_missing;
	TDir* td;		// current dir; assigned from node
	TNode* node;	// latest node returned (dir or file)
	LookupCbParams(uint flags, TDir* td_)
		: create_missing((flags & LF_CREATE_MISSING) != 0), td(td_)
	{
		// init in case lookup's <path> is "".
		// this works because TDir is derived from TNode.
		node = (TNode*)td;
	}
};

static LibError lookup_cb(const char* component, bool is_dir, void* ctx)
{
	LookupCbParams* p = (LookupCbParams*)ctx;
	const TNodeType type = is_dir? NT_DIR : NT_FILE;

	p->td->populate();

	p->node = p->td->find(component);
	if(!p->node)
	{
		if(p->create_missing)
			RETURN_ERR(p->td->add(component, type, &p->node));
		else
			// complaining is left to callers; vfs_exists must be
			// able to fail quietly.
			return ERR::TNODE_NOT_FOUND;	// NOWARN
	}
	if(p->node->type != type)
		WARN_RETURN(ERR::TNODE_WRONG_TYPE);

	if(is_dir)
		p->td = (TDir*)p->node;

	return INFO::CB_CONTINUE;
}

static LibError lookup(TDir* td, const char* path, uint flags, TNode** pnode)
{
	// no undefined bits set
	debug_assert( (flags & ~(LF_CREATE_MISSING|LF_START_DIR)) == 0 );

	LookupCbParams p(flags, td);
	RETURN_ERR(path_foreach_component(path, lookup_cb, &p));

	// success.
	*pnode = p.node;
	return INFO::OK;
}


//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

// this is a pointer to node_alloc-ed memory instead of a static TDir for
// 2 reasons:
// - no NLSO shutdown order issues; validity is well defined
//   (namely between tree_init and tree_shutdown)
// - bonus: tree_init can use it when checking if called twice.
//
// this means we'll have to be extremely careful during tree_clear
// whether its memory remains valid.
static TDir* tree_root;

// make tree_root valid.
static void tree_root_init()
{
	// must not be called more than once without intervening tree_shutdown.
	debug_assert(!tree_root);

#include "lib/nommgr.h"	// placement new
	void* mem = node_alloc();
	if(mem)
		tree_root = new(mem) TDir("", "", 0);
#include "lib/mmgr.h"
}

// destroy the tree root node and free any extra memory held by it.
// note that its node memory still remains allocated.
static void tree_root_shutdown()
{
	// must not be called without previous tree_root_init.
	debug_assert(tree_root);

	// this frees the root node's hash table, which would otherwise leak.
	tree_root->~TDir();
	tree_root = 0;
}


// establish a root node and prepare node_allocator for use.
//
// rationale: calling this from every tree_add* is ugly, so require
// manual init.
void tree_init()
{
	node_init();
	tree_root_init();
}


// empty all directories and free their memory.
// however, node_allocator's DynArray still remains initialized and
// the root directory is usable (albeit empty).
// use when remounting.
void tree_clear()
{
	tree_root->clearR();
	tree_root_shutdown();	// must come before tree_root_init

	node_free_all();

	// note: this is necessary because node_free_all
	// pulls the rug out from under tree_root.
	tree_root_init();
}


// shut down entirely; destroys node_allocator. any further use after this
// requires another tree_init.
void tree_shutdown()
{
	// note: can't use tree_clear because that restores a root node
	// ready for use, which allocates memory.

	// wipe out all dirs (including root node), thus
	// freeing memory they hold.
	tree_root->clearR();

	tree_root_shutdown();

	// free memory underlying the nodes themselves.
	node_shutdown();
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
	LibError ret = td->find_and_add(name, NT_FILE, &node);
	RETURN_ERR(ret);
	if(ret == INFO::ALREADY_EXISTS)
	{
		TFile* tf = (TFile*)node;
		if(!mount_should_replace(tf->m, m, tf->size, size, tf->mtime, mtime))
			return INFO::ALREADY_EXISTS;

		stats_vfs_file_remove(tf->size);
	}

	TFile* tf = (TFile*)node;
	tf->m       = m;
	tf->mtime   = mtime;
	tf->size    = size;
	tf->memento = memento;
	stats_vfs_file_add(size);

	set_most_recent_if_newer(mtime);
	return INFO::OK;
}


LibError tree_add_dir(TDir* td, const char* name, TDir** ptd)
{
	TNode* node;
	RETURN_ERR(td->find_and_add(name, NT_DIR, &node));
	*ptd = (TDir*)node;
	return INFO::OK;
}



LibError tree_lookup_dir(const char* V_path, TDir** ptd, uint flags)
{
	// path is not a directory; TDir::lookup might return a file node
	if(!VFS_PATH_IS_DIR(V_path))
		WARN_RETURN(ERR::TNODE_WRONG_TYPE);

	TDir* td = (flags & LF_START_DIR)? *ptd : tree_root;
	TNode* node;
	CHECK_ERR(lookup(td, V_path, flags, &node));
		// directories should exist, so warn if this fails
	*ptd = (TDir*)node;
	return INFO::OK;
}


LibError tree_lookup(const char* V_path, TFile** pfile, uint flags)
{
	// path is not a file; TDir::lookup might return a directory node
	if(VFS_PATH_IS_DIR(V_path))
		WARN_RETURN(ERR::TNODE_WRONG_TYPE);

	TNode* node;
	LibError ret = lookup(tree_root, V_path, flags, &node);
	RETURN_ERR(ret);
	*pfile = (TFile*)node;
	return INFO::OK;
}


struct AddPathCbParams : boost::noncopyable
{
	const Mount* const m;
	TDir* td;
	AddPathCbParams(const Mount* m_)
		: m(m_), td(tree_root) {}
};

static LibError add_path_cb(const char* component, bool is_dir, void* ctx)
{
	AddPathCbParams* p = (AddPathCbParams*)ctx;

	// should only be called for directory paths, so complain if not dir.
	if(!is_dir)
		WARN_RETURN(ERR::TNODE_WRONG_TYPE);

	TNode* node;
	RETURN_ERR(p->td->find_and_add(component, NT_DIR, &node, p->m));

	p->td = (TDir*)node;
	return INFO::CB_CONTINUE;
}

// iterate over all components in V_dir_path (must reference a directory,
// i.e. end in slash). for any that are missing, add them with the
// specified mount point. this is useful for mounting directories.
//
// passes back the last directory encountered.
LibError tree_add_path(const char* V_dir_path, const Mount* m, TDir** ptd)
{
	debug_assert(VFS_PATH_IS_DIR(V_dir_path));

	AddPathCbParams p(m);
	RETURN_ERR(path_foreach_component(V_dir_path, add_path_cb, &p));
	*ptd = p.td;
	return INFO::OK;
}


//////////////////////////////////////////////////////////////////////////////


// rationale: see DirIterator definition in file.h.
struct TreeDirIterator
{
	TChildren::iterator it;

	// cache end() to avoid needless copies
	TChildren::iterator end;

	// the directory we're iterating over; this is used to lock/unlock it,
	// i.e. prevent modifications that would invalidate the iterator.
	TDir* td;
};

cassert(sizeof(TreeDirIterator) <= DIR_ITERATOR_OPAQUE_SIZE);


LibError tree_dir_open(const char* V_dir_path, DirIterator* di)
{
	debug_assert(VFS_PATH_IS_DIR(V_dir_path));

	TreeDirIterator* tdi = (TreeDirIterator*)di->opaque;

	TDir* td;
	CHECK_ERR(tree_lookup_dir(V_dir_path, &td));

	// we need to prevent modifications to this directory while an iterator is
	// active, otherwise entries may be skipped or no longer valid addresses
	// accessed. blocking other threads is much more convenient for callers
	// than having to check for ERR::AGAIN on every call, so we use a mutex
	// instead of a simple refcount. we don't bother with fine-grained locking
	// (e.g. per directory or read/write locks) because it would result in
	// more overhead (we have hundreds of directories) and is unnecessary.
	tree_lock();

	tdi->it  = td->begin();
	tdi->end = td->end();
	tdi->td  = td;
	return INFO::OK;
}


LibError tree_dir_next_ent(DirIterator* di, DirEnt* ent)
{
	TreeDirIterator* tdi = (TreeDirIterator*)di->opaque;

	if(tdi->it == tdi->end)
		return ERR::DIR_END;	// NOWARN

	const TNode* node = *(tdi->it++);
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

	return INFO::OK;
}


LibError tree_dir_close(DirIterator* UNUSED(d))
{
	tree_unlock();

	// no further cleanup needed. we could zero out d but that might
	// hide bugs; the iterator is safe (will not go beyond end) anyway.
	return INFO::OK;
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



void tfile_set_mount(TFile* tf, const Mount* m)
{
	tf->m = m;
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

	return INFO::OK;
}


RealDir* tree_get_real_dir(TDir* td)
{
	return &td->get_rd();
}
