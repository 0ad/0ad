#include "precompiled.h"

#include <string.h>

#include <string>
#include <vector>

#include "lib.h"
#include "res.h"
#include "vfs_path.h"
#include "vfs_tree.h"
#include "hotload.h"	// see NO_DIR_WATCH


// TMountPoint  = location of a file in the tree.
// TFile = all information about a file stored in the tree.
// TDir  = container holding TFile-s representing a dir. in the tree.




// CONTAINER RATIONALE (see philip discussion)


struct TNode;

enum TNodeType
{
	N_NONE,
	N_DIR,
	N_FILE
};


//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////


class TChildren
{
public: // xxx
	TNode** tbl;
	short num_entries;
	short max_entries;	// when initialized, = 2**n for faster modulo

public:
	void init();
	void clear();
	TNode** get_slot(const char* fn);
	bool expand_tbl();
	TNode* add(const char* fn);
	TNode* find(const char* fn);

	size_t size()
	{
		return num_entries;
	}

	class iterator;
	iterator begin() const;
	iterator end() const;
};

typedef TChildren::iterator TChildIt;




class TDir
{
public:

	// if exactly one real directory is mounted into this virtual dir,
	// this points to its location. used to add files to VFS when writing.
	//
	// the TMountPoint is actually in the mount info and is invalid when
	// that's unmounted, but the VFS would then be rebuilt anyway.
	//
	// = 0 if no real dir mounted here; = -1 if more than one.
	const TMountPoint* mount_point;

#ifndef NO_DIR_WATCH
	intptr_t watch;
#endif
	TChildren children;

	// documented below
	void init();
	TNode* find(const char* fn, TNodeType desired_type);
	TNode* add(const char* fn, TNodeType new_type);
	int mount(const char* path, const TMountPoint*, bool watch);
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
	// set by TChildren
	char exact_name[1];
};




//////////////////////////////////////////////////////////////////////////////
//
// "bucket" allocator for TNodes; used by TChildren
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
// TChildren implementation
//
//////////////////////////////////////////////////////////////////////////////



class TChildren::iterator
{
public:
	typedef std::forward_iterator_tag iterator_category;
	typedef TNode* T;
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

TChildren::iterator TChildren::begin() const
{
	TNode** pos = tbl;
	while(pos != tbl+max_entries && *pos == 0)
		pos++;
	return iterator(pos, tbl+max_entries);
}
TChildren::iterator TChildren::end() const
{
	return iterator(tbl+max_entries, 0);
}


void TChildren::init()
{
	tbl = 0;
	num_entries = 0;
	max_entries = 16;	// will be doubled in expand_tbl
	expand_tbl();
}


void TChildren::clear()
{
	free(tbl);
	tbl = 0;
	num_entries = max_entries = 0;
}


// note: add is only called once per file, so we can do the hash
// here without duplication
TNode** TChildren::get_slot(const char* fn)
{
	const uint mask = max_entries-1;
	u32 hash = fnv_lc_hash(fn);
	TNode** pnode;
	for(;;)
	{
		pnode = &tbl[hash & mask];
		hash++;
		TNode* const node = *pnode;
		if(!node)
			break;
		if(!stricmp(node->exact_name, fn))
			break;
	}

	return pnode;
}


bool TChildren::expand_tbl()
{
	// alloc a new table (but don't assign it to <tbl> unless successful)
	TNode** old_tbl = tbl;
	tbl = (TNode**)calloc(max_entries*2, sizeof(TNode*));
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
		TNode* const node = old_tbl[i];
		if(node)
			*get_slot(node->exact_name) = node;
	}
	free(old_tbl);

	return true;
}


// return existing, or add if not present
TNode* TChildren::add(const char* fn)
{
	// expand before determining slot; this will invalidate previous pnodes.
	if(num_entries*2 >= max_entries)
	{
		if(!expand_tbl())
			return 0;
	}

	TNode** pnode = get_slot(fn);
	if(*pnode)
		return *pnode;

	const size_t size = sizeof(TNode)+strlen_s(fn, VFS_MAX_PATH)+1;
	TNode* node = node_alloc(size);
	if(!node)
		return 0;

	// commit
	*pnode = node;
	num_entries++;
	strcpy(node->exact_name, fn);	// safe
	node->type = N_NONE;
	return node;
}


TNode* TChildren::find(const char* fn)
{
	return *get_slot(fn);
}



//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

void TDir::init()
{
	mount_point = 0;
	children.init();
}


TNode* TDir::find(const char* fn, TNodeType desired_type)
{
	TNode* node = children.find(fn);
	if(node && node->type != desired_type)
		return 0;
	return node;
}


TNode* TDir::add(const char* name, TNodeType new_type)
{
	if(!path_component_valid(name))
		return 0;

	TNode* node = children.add(name);
	if(!node)
		return 0;
	// already initialized
	if(node->type != N_NONE)
		return node;

	node->type = new_type;

	// note: this is called from lookup, which needs to create nodes.
	// therefore, we need to initialize here.
	if(new_type == N_FILE)
		node->u.file.init();
	else
		node->u.dir.init();

	return node;
}


// note: full VFS path is needed for the dir watch.
int TDir::mount(const char* path, const TMountPoint* mp, bool watch)
{
	// more than one real dir mounted into VFS dir
	// (=> can't create files for writing here)
	if(mount_point)
		mount_point = (TMountPoint*)-1;
	else
		mount_point = mp;

#ifndef NO_DIR_WATCH
	if(watch)
		CHECK_ERR(res_watch_dir(path, &this->watch));
#endif

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
	assert( (flags & ~(LF_CREATE_MISSING|LF_START_DIR)) == 0 );
		// no undefined bits set

	const bool create_missing = !!(flags & LF_CREATE_MISSING);

	// copy into (writeable) buffer so we can 'tokenize' path components
	// by replacing '/' with '\0'.
	char v_path[VFS_MAX_PATH];
	strcpy_s(v_path, sizeof(v_path), path);
	char* cur_component = v_path;

	TDir* dir = this;
	TNodeType type = N_DIR;

	// successively navigate to the next component in <path>.
	TNode* node;
	for(;;)
	{
		// "extract" cur_component string (0-terminate by replacing '/')
		char* slash = (char*)strchr(cur_component, '/');
		if(!slash)
		{
			// string ended in slash => return the current dir node
			if(*cur_component == '\0')
				break;
			// it's a filename
			type = N_FILE;
		}
		// normal operation (cur_component is a directory)
		else
			*slash = '\0';

		// create <cur_component> (no-op if it already exists)
		if(create_missing)
		{
			node = dir->add(cur_component, type);
			if(!node)
				return ERR_NO_MEM;
			if(type == N_FILE)	// xxx move to ctor i.e. init()?
			{
				node->u.file.mount_point = dir->mount_point;
				node->u.file.pri = 0;
				node->u.file.in_archive = 0;
			}
		}
		else
		{
			node = dir->find(cur_component, type);
			if(!node)
				return slash? ERR_PATH_NOT_FOUND : ERR_FILE_NOT_FOUND;
		}
		dir = &node->u.dir;

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
#ifndef NO_DIR_WATCH
	res_cancel_watch(watch);
	watch = 0;
#endif
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
		char is_archive = file.in_archive? 'A' : 'L';
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
		printf(fmt, name, is_archive, size, timestamp);
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


// rationale: can't do this in tree_clear - we'd leak at exit.
// calling from tree_add* is ugly as well, so require manual init.
void tree_init()
{
	tree_root_dir->init();
}

inline void tree_clear()
{
	tree_root_dir->clearR();
}


// write a representation of the VFS tree to stdout.
inline void tree_display()
{
	tree_root_dir->displayR(0);
}


TFile* tree_add_file(TDir* dir, const char* name)
{
	TNode* node = dir->add(name, N_FILE);
	return node? &node->u.file : 0;
}


TDir* tree_add_dir(TDir* dir, const char* name)
{
	TNode* node = dir->add(name, N_DIR);
	return node? &node->u.dir: 0;
}



int tree_mount(TDir* dir, const char* path, const TMountPoint* mount_point, bool watch)
{
	return dir->mount(path, mount_point, watch);
}


int tree_lookup_dir(const char* path, TDir** pdir, uint flags, char* exact_path)
{
	// TDir::lookup would return a file node
	if(path[0] != '\0' && path[strlen(path)-1] != '/')
		return -1;

	TDir* dir = (flags & LF_START_DIR)? *pdir : tree_root_dir;
	TNode* node;
	CHECK_ERR(dir->lookup(path, flags, &node, exact_path));
		// directories should exist, so warn if this fails
	*pdir = &node->u.dir;
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

struct NodeLatch
{
	size_t i;
	std::vector<const TNode*> v;

	static bool ci_less(const TNode* n1, const TNode* n2)
	{
		return stricmp(n1->exact_name, n2->exact_name) < 0;
	}

	NodeLatch(TChildren& c)
	{
		i = 0;

		v.reserve(c.size());
		std::copy(c.begin(), c.end(), std::back_inserter(v));
		std::sort(v.begin(), v.end(), ci_less);
	}

	bool empty() const
	{
		return i == v.size();
	}

	const TNode* get_next()
	{
		assert(!empty());
		return v[i++];
	}
};


// must not be held across rebuild! refcount to make sure
int tree_open_dir(const char* path_slash, void** latch)
{
	TDir* dir;
	CHECK_ERR(tree_lookup_dir(path_slash, &dir));
	*latch = new NodeLatch(dir->children);
	return 0;
}


int tree_next_dirent(void* latch_, const char* filter, vfsDirEnt* dirent)
{
	bool want_dir = true;
	if(filter)
	{
		if(filter[0] == '/')
		{
			if(filter[1] == '|')
				filter += 2;
		}
		else
			want_dir = false;
	}

	// loop until a TNode matches what is requested, or end of list.
	NodeLatch* latch = (NodeLatch*)latch_;
	const TNode* node;
	for(;;)
	{
		if(latch->empty())
			return ERR_DIR_END;
		node = latch->get_next();

		if(node->type == N_DIR)
		{
			if(want_dir)
			{
				dirent->size = -1;
				dirent->mtime = 0;	// not currently supported for dirs
				break;
			}
		}
		else if(node->type == N_FILE)
		{
			// (note: filter = 0 matches anything)
			if(match_wildcard(node->exact_name, filter))
			{
				dirent->size  = node->u.file.size;
				dirent->mtime = node->u.file.mtime;
				break;
			}
		}
#ifndef NDEBUG
		else
			debug_warn("invalid TNode type");
#endif
	}

	// success; set shared fields
	dirent->name = node->exact_name;
	return 0;
}


int tree_close_dir(void* latch_)
{
	NodeLatch* latch = (NodeLatch*)latch_;
	delete latch;
	return 0;
}
