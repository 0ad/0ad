#include "precompiled.h"

#include <string.h>
#include <time.h>

#include <string>
#include <vector>
#include <algorithm>

#include "../res.h"
#include "vfs_path.h"
#include "vfs_tree.h"


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


// CONTAINER RATIONALE (see philip discussion)


struct Mount;

// these must be defined before TNode because it puts them in a union.
// some TDir member functions access TNode members, so we have to define
// those later.

struct TFile
{
	// required:
	const Mount* m;
	// allocated and owned by caller (mount code)

	time_t mtime;
	off_t size;

	// note: this is basically the constructor (C++ can't call it directly
	// since this object is stored in a union)
	void init()
	{
		m = 0;
		mtime = 0;
		size = 0;
	}
};



struct TNode;

enum TNodeType
{
	N_NONE,
	N_DIR,
	N_FILE
};


//////////////////////////////////////////////////////////////////////////////
//
// "bucket" allocator for TNodes; used by DynHashTbl
//
//////////////////////////////////////////////////////////////////////////////


const size_t BUCKET_SIZE = 8*KiB;

static u8* bucket_pos;


TNode* node_alloc(size_t size)
{
	// would overflow a bucket
	if(size > BUCKET_SIZE-sizeof(u8*))
	{
		debug_warn("node_alloc: size doesn't fit in a bucket");
		return 0;
	}

	size = round_up(size, 8);
	// ensure alignment, since size includes a string
	const uintptr_t addr = (uintptr_t)bucket_pos;
	const size_t bytes_used = addr % BUCKET_SIZE;
	// addr = 0 on first call (no bucket yet allocated)
	// bytes_used == 0 if a node fit exactly into a bucket
	if(addr == 0 || bytes_used == 0 || bytes_used+size > BUCKET_SIZE)
	{
		u8* const prev_bucket = (u8*)addr - bytes_used;
		u8* bucket = (u8*)mem_alloc(BUCKET_SIZE, BUCKET_SIZE);
		if(!bucket)
			return 0;
		*(u8**)bucket = prev_bucket;
		bucket_pos = bucket+round_up(sizeof(u8*), 8);
	}

	TNode* node = (TNode*)bucket_pos;
	bucket_pos = (u8*)node+size;
	return node;
}


void node_free_all()
{
	const uintptr_t addr = (uintptr_t)bucket_pos;
	u8* bucket = bucket_pos - (addr % BUCKET_SIZE);

	// covers bucket_pos == 0 case
	while(bucket)
	{
		u8* prev_bucket = *(u8**)bucket;
		mem_free(bucket);
		bucket = prev_bucket;
	}
}



//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

typedef TNode* T;
typedef const char* Key;

static const size_t n = 16;

static inline Key GetKey(const T t);
static inline bool Eq(const Key k1, const Key k2);
static inline u32 Hash(const Key key);

class DynHashTbl
{
	T* tbl;
	short num_entries;
	short max_entries;	// when initialized, = 2**n for faster modulo

	bool expand_tbl()
	{
		// alloc a new table (but don't assign it to <tbl> unless successful)
		T* old_tbl = tbl;
		tbl = (T*)calloc(max_entries*2, sizeof(T));
		if(!tbl)
		{
			tbl = old_tbl;
			return false;
		}

		max_entries += max_entries;
		// must be set before get_slot

		// newly initialized, nothing to copy - done
		if(!old_tbl)
			return true;

		// re-hash from old table into the new one
		for(int i = 0; i < max_entries/2; i++)
		{
			T const t = old_tbl[i];
			if(t)
				*get_slot(GetKey(t)) = t;
		}
		free(old_tbl);

		return true;
	}


public:

	void init()
	{
		tbl = 0;
		num_entries = 0;
		max_entries = n/2;	// will be doubled in expand_tbl
		expand_tbl();
	}

	void clear()
	{
		free(tbl);
		tbl = 0;
		num_entries = max_entries = 0;
	}

	// note: add is only called once per file, so we can do the hash
	// here without duplication
	T* get_slot(Key key)
	{
		u32 hash = Hash(key);
		const uint mask = max_entries-1;
		T* p;
		for(;;)
		{
			p = &tbl[hash & mask];
			hash++;
			const T t = *p;
			if(!t)
				break;
			if(Eq(key, GetKey(t)))
				break;
		}

		return p;
	}

	bool add(const Key key, const T t)
	{
		// expand before determining slot; this will invalidate previous pnodes.
		if(num_entries*4 >= max_entries*3)
		{
			if(!expand_tbl())
				return false;
		}

		// commit
		*get_slot(key) = t;
		num_entries++;
		return true;
	}


	T find(Key key)
	{
		return *get_slot(key);
	}

	size_t size()
	{
		return num_entries;
	}





	class iterator
	{
	public:
		typedef std::forward_iterator_tag iterator_category;
		typedef ::T T;
		typedef T value_type;
		typedef ptrdiff_t difference_type;
		typedef const T* pointer;
		typedef const T& reference;

		iterator()
		{
		}
		iterator(T* pos_, T* end_) : pos(pos_), end(end_)
		{
		}
		T& operator*() const
		{
			return *pos;
		}
		iterator& operator++()	// pre
		{
			do
			pos++;
			while(pos != end && *pos == 0);
			return (*this);
		}
		bool operator==(const iterator& rhs) const
		{
			return pos == rhs.pos;
		}
		bool operator<(const iterator& rhs) const
		{
			return (pos < rhs.pos);
		}

		// derived
		const T* operator->() const
		{
			return &**this;
		}
		bool operator!=(const iterator& rhs) const
		{
			return !(*this == rhs);
		}
		iterator operator++(int)	// post
		{
			iterator tmp =  *this; ++*this; return tmp;
		}

	protected:
		T* pos;
		T* end;
		// only used when incrementing (avoid going beyond end of table)
	};

	iterator begin() const
	{
		T* pos = tbl;
		while(pos != tbl+max_entries && *pos == 0)
			pos++;
		return iterator(pos, tbl+max_entries);
	}
	iterator end() const
	{
		return iterator(tbl+max_entries, 0);
	}
};

typedef DynHashTbl::iterator TChildIt;







enum TDirFlags
{
	TD_POPULATED = 1
};

// must be declared before TNode
struct TDir
{
	int flags;	// enum TDirFlags

	RealDir rd;

	DynHashTbl children;

	void init();
	TNode* find(const char* name, TNodeType desired_type);
	int add(const char* name, TNodeType new_type, TNode** pnode);
	int attach_real_dir(const char* path, int flags, const Mount* new_m);
	int lookup(const char* path, uint flags, TNode** pnode, char* exact_path);
	void clearR();
	void displayR(int indent_level);
};




// can't inherit, since exact_name must come at end of record
struct TNode
{
	// must be at start of TNode to permit casting back and forth!
	// (see TDir::lookup)
	union TNodeUnion
	{
		TDir dir;
		TFile file;
	} u;

	TNodeType type;

	//used by callers needing the exact case,
	// e.g. for case-sensitive syscalls; also key for lookup
	// set by DynHashTbl
	char exact_name[1];
};



static inline bool Eq(const Key k1, const Key k2)
{
	return strcmp(k1, k2) == 0;
}

static u32 Hash(const Key key)
{
	return fnv_lc_hash(key);
}

static inline Key GetKey(const T t)
{
	return t->exact_name;
}








//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////


void TDir::init()
{
	rd.m = 0;
	rd.watch = 0;
	children.init();
}

TNode* TDir::find(const char* name, TNodeType desired_type)
{
	TNode* node = children.find(name);
	if(node && node->type != desired_type)
		return 0;
	return node;
}

int TDir::add(const char* name, TNodeType new_type, TNode** pnode)
{
	if(!path_component_valid(name))
		return ERR_PATH_INVALID;

	// this is legit - when looking up a directory, LF_CREATE_IF_MISSING
	// calls this *instead of* find (as opposed to only if not found)
	TNode* node = children.find(name);
	if(node)
		goto done;

	{
	const size_t size = sizeof(TNode)+strnlen(name, VFS_MAX_PATH)+1;
	node = node_alloc(size);
	if(!node)
		return 0;
	strcpy(node->exact_name, name);	// safe
	node->type = new_type;

	if(!children.add(name, node))
	{
		debug_warn("failed to expand table");
		// node will be freed by node_free_all
		return 0;
	}

	// note: this is called from lookup, which needs to create nodes.
	// therefore, we need to initialize here.
	if(new_type == N_FILE)
		node->u.file.init();
	else
		node->u.dir.init();
	}

done:
	*pnode = node;
	return 0;
}

int TDir::lookup(const char* path, uint flags, TNode** pnode, char* exact_path)
{
	// cleared on failure / if returning root dir node (= "")
	if(exact_path)
		exact_path[0] = '\0';

	// early out: "" => return this directory (usually VFS root)
	if(path[0] == '\0')
	{
		*pnode = (TNode*)this;	// HACK: TDir is at start of TNode
		return 0;
	}

	CHECK_PATH(path);
	debug_assert( (flags & ~(LF_CREATE_MISSING|LF_START_DIR)) == 0 );
	// no undefined bits set

	const bool create_missing = !!(flags & LF_CREATE_MISSING);

	// copy into (writeable) buffer so we can 'tokenize' path components
	// by replacing '/' with '\0'.
	char v_path[VFS_MAX_PATH];
	strcpy_s(v_path, sizeof(v_path), path);
	char* cur_component = v_path;

	TDir* td = this;
	TNodeType type = N_DIR;

	// successively navigate to the next component in <path>.
	TNode* node = 0;
	for(;;)
	{
		// "extract" cur_component string (0-terminate by replacing '/')
		char* slash = (char*)strchr(cur_component, '/');
		if(!slash)
		{
			// all other node assignments are checked, so this must have
			// been the first iteration and there's no slash =>
			// pathname is incorrect.
			if(!node)
				return ERR_INVALID_PARAM;

			// string ended in slash => return the current dir node.
			if(*cur_component == '\0')
				break;

			// it's a filename
			type = N_FILE;
		}
		// normal operation (cur_component is a directory)
		else
		{
			// the caller may potentially access this directory.
			// make sure it has been populated with loose files/directories.
			if(!(td->flags & TD_POPULATED))
			{
				WARN_ERR(mount_populate(td, &td->rd));
				td->flags |= TD_POPULATED;
			}

			*slash = '\0';
		}

		// create <cur_component> (no-op if it already exists)
		if(create_missing)
		{
			RETURN_ERR(td->add(cur_component, type, &node));
			// this is a hack, but I don't see a better way.
			// tree_add_file does special "should override" checks and
			// we are creating a TNode (not TFile or TDir) here,
			// so we special-case its init.
			if(type == N_FILE)
			{
				node->u.file.m = td->rd.m;
			}
		}
		else
		{
			node = td->find(cur_component, type);
			if(!node)
				return slash? ERR_PATH_NOT_FOUND : ERR_FILE_NOT_FOUND;
		}
		td = &node->u.dir;

		if(exact_path)
			exact_path += sprintf(exact_path, "%s/", node->exact_name);
		// no length check needed: length is the same as path

		// cur_component was a filename => we're done
		if(!slash)
		{
			// strip trailing '/' that was added above
			if(exact_path)
				exact_path[-1] = '\0';
			break;
		}
		// else: it was a directory; advance
		cur_component = slash+1;
	}

	// success.
	*pnode = node;
	return 0;
}

// empty this directory and all subdirectories; used when rebuilding VFS.
void TDir::clearR()
{
	// recurse for all subdirs
	// (preorder traversal - need to do this before clearing the list)
	for(TChildIt it = children.begin(); it != children.end(); ++it)
	{
		TNode* node = *it;
		if(node->type == N_DIR)
			node->u.dir.clearR();
	}

	// wipe out this directory
	children.clear();

	// the watch is restored when this directory is repopulated; we must
	// remove it in case the real directory backing this one was deleted.
	mount_detach_real_dir(&rd);
}

void TDir::displayR(int indent_level)
{
	const char indent[] = "    ";

	TChildIt it;

	// list all files in this dir
	for(it = children.begin(); it != children.end(); ++it)
	{
		TNode* node = (*it);
		if(node->type != N_FILE)
			continue;

		TFile& file = node->u.file;
		const char* name = node->exact_name;
		char type = mount_get_type(file.m);
		char* timestamp = ctime(&file.mtime);
		timestamp[24] = '\0';	// remove '\n'
		const off_t size = file.size;

		for(int i = 0; i < indent_level; i++)
			printf(indent);
		char fmt[25];
		int chars = 80 - indent_level*(sizeof(indent)-1);
		sprintf(fmt, "%%-%d.%ds (%%c; %%6d; %%s)\n", chars, chars);
		// build format string: tell it how long the filename may be,
		// so that it takes up all space before file info column.
		printf(fmt, name, type, size, timestamp);
	}

	// recurse over all subdirs
	for(it = children.begin(); it != children.end(); ++it)
	{
		TNode* node = (*it);
		if(node->type != N_DIR)
			continue;

		TDir& subdir = node->u.dir;
		const char* subdir_name = node->exact_name;

		// write subdir's name
		// note: do it now, instead of in recursive call so that:
		// - we don't have to pass dir_name parameter;
		// - the VFS root node isn't displayed.
		for(int i = 0; i < indent_level; i++)
			printf(indent);
		printf("[%s/]\n", subdir_name);

		subdir.displayR(indent_level+1);
	}
}


//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

static TNode tree_root;
	// => exact_name = ""
static TDir* tree_root_dir = &tree_root.u.dir;


void tree_clear()
{
	tree_root_dir->clearR();
}

// rationale: can't do this in tree_shutdown - we'd leak at exit.
// calling from tree_add* is ugly as well, so require manual init.
void tree_init()
{
	tree_root_dir->init();
}


// write a representation of the VFS tree to stdout.
void vfs_display()
{
	tree_root_dir->displayR(0);
}





int tree_add_file(TDir* td, const char* name, const Mount* m,
	off_t size, time_t mtime)
{
	TNode* node;
	RETURN_ERR(td->add(name, N_FILE, &node));
	TFile* tf = &node->u.file;

	// assume they're the same if size and last-modified time match.
	// note: FAT timestamp only has 2 second resolution
	const bool is_same = (tf->size == size) &&
		fabs(difftime(tf->mtime, mtime)) <= 2.0;
	if(!mount_should_replace(tf->m, m, is_same))
		return 1;

	tf->m     = m;
	tf->mtime = mtime;
	tf->size  = size;
	return 0;
}


int tree_add_dir(TDir* td, const char* name, TDir** ptd)
{
	TNode* node;
	RETURN_ERR(td->add(name, N_DIR, &node));
	*ptd = &node->u.dir;
	return 0;
}





int tree_lookup_dir(const char* path, TDir** ptd, uint flags, char* exact_path)
{
	// TDir::lookup would return a file node
	if(path[0] != '\0' && path[strlen(path)-1] != '/')
		return -1;

	TDir* td = (flags & LF_START_DIR)? *ptd : tree_root_dir;
	TNode* node;
	CHECK_ERR(td->lookup(path, flags, &node, exact_path));
		// directories should exist, so warn if this fails
	*ptd = &node->u.dir;
	return 0;
}


int tree_lookup(const char* path, TFile** pfile, uint flags, char* exact_path)
{
	// TDir::lookup would return a directory node
	if(path[0] == '\0' || path[strlen(path)-1] == '/')
		return -1;

	TNode* node;
	int ret = tree_root_dir->lookup(path, flags, &node, exact_path);
	RETURN_ERR(ret);
	*pfile = &node->u.file;
	return 0;
}


//////////////////////////////////////////////////////////////////////////////




// rationale: see DirIterator definition in file.h.
struct TreeDirIterator_
{
	DynHashTbl::iterator it;

	// cache end() to avoid needless copies
	DynHashTbl::iterator end;

	// the directory we're iterating over; this is used to lock/unlock it,
	// i.e. prevent modifications that would invalidate the iterator.
	TDir* td;
};

cassert(sizeof(TreeDirIterator_) <= sizeof(TreeDirIterator));


int tree_dir_open(const char* path_slash, TreeDirIterator* d_)
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

	d->it  = td->children.begin();
	d->end = td->children.end();
	d->td  = td;
	return 0;
}


int tree_dir_next_ent(TreeDirIterator* d_, DirEnt* ent)
{
	TreeDirIterator_* d = (TreeDirIterator_*)d_;

	if(d->it == d->end)
		return ERR_DIR_END;

	const TNode* node = *(d->it++);
	ent->name = node->exact_name;

	// set size and mtime fields depending on node type:
	switch(node->type)
	{
	case N_DIR:
		ent->size = -1;
		ent->mtime = 0;	// not currently supported for dirs
		break;
	case N_FILE:
		ent->size  = node->u.file.size;
		ent->mtime = node->u.file.mtime;
		break;
	default:
		debug_warn("invalid TNode type");
	}

	return 0;	// success
}


int tree_dir_close(TreeDirIterator* UNUSED(d))
{
	tree_unlock();

	// no further cleanup needed. we could zero out d but that might
	// hide bugs; the iterator is safe (will not go beyond end) anyway.
	return 0;
}


//-----------------------------------------------------------------------------
// get/set

const Mount* tree_get_mount(const TFile* tf)
{
	return tf->m;
}


void tree_update_file(TFile* tf, off_t size, time_t mtime)
{
	tf->size  = size;
	tf->mtime = mtime;
}


// get file status (mode, size, mtime). output param is undefined on error.
int tree_stat(const TFile* tf, struct stat* s)
{
	// all stat members currently supported are stored in TFile, so we
	// can return them directly without having to call file|zip_stat.
	s->st_mode  = S_IFREG;
	s->st_size  = tf->size;
	s->st_mtime = tf->mtime;

	return 0;
}


RealDir* tree_get_real_dir(TDir* td)
{
	return &td->rd;
}
