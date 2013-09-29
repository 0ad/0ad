/* Copyright (C) 2011 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * GPG3-style hierarchical profiler
 */

#include "precompiled.h"

#include "Profile.h"
#include "ProfileViewer.h"
#include "lib/timer.h"

#if OS_WIN && !defined(NDEBUG)
# define USE_CRT_SET_ALLOC_HOOK
#endif

#if defined(__GLIBC__) && !defined(NDEBUG)
//# define USE_GLIBC_MALLOC_HOOK
# define USE_GLIBC_MALLOC_OVERRIDE
# include <dlfcn.h>
# include <malloc.h>
# include "lib/sysdep/cpu.h"
#endif

#include <numeric>

///////////////////////////////////////////////////////////////////////////////////////////////
// CProfileNodeTable



/**
 * Class CProfileNodeTable: Implement ProfileViewer's AbstractProfileTable
 * interface in order to display profiling data in-game.
 */
class CProfileNodeTable : public AbstractProfileTable
{
public:
	CProfileNodeTable(CProfileNode* n);
	virtual ~CProfileNodeTable();

	// Implementation of AbstractProfileTable interface
	virtual CStr GetName();
	virtual CStr GetTitle();
	virtual size_t GetNumberRows();
	virtual const std::vector<ProfileColumn>& GetColumns();
	
	virtual CStr GetCellText(size_t row, size_t col);
	virtual AbstractProfileTable* GetChild(size_t row);
	virtual bool IsHighlightRow(size_t row);
	
private:
	/**
	 * struct ColumnDescription: The only purpose of this helper structure
	 * is to provide the global constructor that sets up the column
	 * description.
	 */
	struct ColumnDescription
	{
		std::vector<ProfileColumn> columns;
		
		ColumnDescription()
		{
			columns.push_back(ProfileColumn("Name", 230));
			columns.push_back(ProfileColumn("calls/frame", 80));
			columns.push_back(ProfileColumn("msec/frame", 80));
			columns.push_back(ProfileColumn("mallocs/frame", 120));
			columns.push_back(ProfileColumn("calls/turn", 80));
			columns.push_back(ProfileColumn("msec/turn", 80));
			columns.push_back(ProfileColumn("mallocs/turn", 80));
		}
	};
	
	/// The node represented by this table
	CProfileNode* node;
	
	/// Columns description (shared by all instances)
	static ColumnDescription columnDescription;
};

CProfileNodeTable::ColumnDescription CProfileNodeTable::columnDescription;


// Constructor/Destructor
CProfileNodeTable::CProfileNodeTable(CProfileNode* n)
{
	node = n;
}

CProfileNodeTable::~CProfileNodeTable()
{
}

// Short name (= name of profile node)
CStr CProfileNodeTable::GetName()
{
	return node->GetName();
}

// Title (= explanatory text plus time totals)
CStr CProfileNodeTable::GetTitle()
{
	char buf[512];
	sprintf_s(buf, ARRAY_SIZE(buf), "Profiling Information for: %s (Time in node: %.3f msec/frame)", node->GetName(), node->GetFrameTime() * 1000.0f );
	return buf;
}

// Total number of children
size_t CProfileNodeTable::GetNumberRows()
{
	return node->GetChildren()->size() + node->GetScriptChildren()->size() + 1;
}

// Column description
const std::vector<ProfileColumn>& CProfileNodeTable::GetColumns()
{
	return columnDescription.columns;
}

// Retrieve cell text
CStr CProfileNodeTable::GetCellText(size_t row, size_t col)
{
	CProfileNode* child;
	size_t nrchildren = node->GetChildren()->size();
	size_t nrscriptchildren = node->GetScriptChildren()->size();
	char buf[256] = "?";
	
	if (row < nrchildren)
		child = (*node->GetChildren())[row];
	else if (row < nrchildren + nrscriptchildren)
		child = (*node->GetScriptChildren())[row - nrchildren];
	else if (row > nrchildren + nrscriptchildren)
		return "!bad row!";
	else
	{
		// "unlogged" row
		if (col == 0)
			return "unlogged";
		else if (col == 1)
			return "";
		else if (col == 4)
			return "";
		
		double unlogged_time_frame = node->GetFrameTime();
		double unlogged_time_turn = node->GetTurnTime();
		double unlogged_mallocs_frame = node->GetFrameMallocs();
		double unlogged_mallocs_turn = node->GetTurnMallocs();
		CProfileNode::const_profile_iterator it;

		for (it = node->GetChildren()->begin(); it != node->GetChildren()->end(); ++it)
		{
			unlogged_time_frame -= (*it)->GetFrameTime();
			unlogged_time_turn -= (*it)->GetTurnTime();
			unlogged_mallocs_frame -= (*it)->GetFrameMallocs();
			unlogged_mallocs_turn -= (*it)->GetTurnMallocs();
		}
		for (it = node->GetScriptChildren()->begin(); it != node->GetScriptChildren()->end(); ++it)
		{
			unlogged_time_frame -= (*it)->GetFrameTime();
			unlogged_time_turn -= (*it)->GetTurnTime();
			unlogged_mallocs_frame -= (*it)->GetFrameMallocs();
			unlogged_mallocs_turn -= (*it)->GetTurnMallocs();
		}
		
		// The root node can't easily count per-turn values (since Turn isn't called until
		// halfway though a frame), so just reset them the zero to prevent weird displays
		if (!node->GetParent())
		{
			unlogged_time_turn = 0.0;
			unlogged_mallocs_turn = 0.0;
		}

		if (col == 2)
			sprintf_s(buf, ARRAY_SIZE(buf), "%.3f", unlogged_time_frame * 1000.0f);
		else if (col == 3)
			sprintf_s(buf, ARRAY_SIZE(buf), "%.1f", unlogged_mallocs_frame);
		else if (col == 5)
			sprintf_s(buf, ARRAY_SIZE(buf), "%.3f", unlogged_time_turn * 1000.f);
		else if (col == 6)
			sprintf_s(buf, ARRAY_SIZE(buf), "%.1f", unlogged_mallocs_turn);
		
		return CStr(buf);
	}
	
	switch(col)
	{
	default:
	case 0:
		return child->GetName();
		
	case 1:
		sprintf_s(buf, ARRAY_SIZE(buf), "%.1f", child->GetFrameCalls());
		break;
	case 2:
		sprintf_s(buf, ARRAY_SIZE(buf), "%.3f", child->GetFrameTime() * 1000.0f);
		break;
	case 3:
		sprintf_s(buf, ARRAY_SIZE(buf), "%.1f", child->GetFrameMallocs());
		break;
	case 4:
		sprintf_s(buf, ARRAY_SIZE(buf), "%.1f", child->GetTurnCalls());
		break;
	case 5:
		sprintf_s(buf, ARRAY_SIZE(buf), "%.3f", child->GetTurnTime() * 1000.0f);
		break;
	case 6:
		sprintf_s(buf, ARRAY_SIZE(buf), "%.1f", child->GetTurnMallocs());
		break;
	}
	return CStr(buf);
}

// Return a pointer to the child table if the child node is expandable
AbstractProfileTable* CProfileNodeTable::GetChild(size_t row)
{
	CProfileNode* child;
	size_t nrchildren = node->GetChildren()->size();
	size_t nrscriptchildren = node->GetScriptChildren()->size();
	
	if (row < nrchildren)
		child = (*node->GetChildren())[row];
	else if (row < nrchildren + nrscriptchildren)
		child = (*node->GetScriptChildren())[row - nrchildren];
	else
		return 0;
	
	if (child->CanExpand())
		return child->display_table;
	
	return 0;
}

// Highlight all script nodes
bool CProfileNodeTable::IsHighlightRow(size_t row)
{
	size_t nrchildren = node->GetChildren()->size();
	size_t nrscriptchildren = node->GetScriptChildren()->size();
	
	return (row >= nrchildren && row < (nrchildren + nrscriptchildren));
}

///////////////////////////////////////////////////////////////////////////////////////////////
// CProfileNode implementation


// Note: As with the GPG profiler, name is assumed to be a pointer to a constant string; only pointer equality is checked.
CProfileNode::CProfileNode( const char* _name, CProfileNode* _parent )
{
	name = _name;
	recursion = 0;

	Reset();

	parent = _parent;

	display_table = new CProfileNodeTable(this);
}

CProfileNode::~CProfileNode()
{
	profile_iterator it;
	for( it = children.begin(); it != children.end(); ++it )
		delete( *it );	
	for( it = script_children.begin(); it != script_children.end(); ++it )
		delete( *it );
	
	delete display_table;
}

template<typename T>
static double average(const T& collection)
{
	if (collection.empty())
		return 0.0;
	return std::accumulate(collection.begin(), collection.end(), 0.0) / collection.size();
}

double CProfileNode::GetFrameCalls() const
{
	return average(calls_per_frame);
}

double CProfileNode::GetFrameTime() const
{
	return average(time_per_frame);
}

double CProfileNode::GetTurnCalls() const
{
	return average(calls_per_turn);
}

double CProfileNode::GetTurnTime() const
{
	return average(time_per_turn);
}

double CProfileNode::GetFrameMallocs() const
{
	return average(mallocs_per_frame);
}

double CProfileNode::GetTurnMallocs() const
{
	return average(mallocs_per_turn);
}

const CProfileNode* CProfileNode::GetChild( const char* childName ) const
{
	const_profile_iterator it;
	for( it = children.begin(); it != children.end(); ++it )
		if( (*it)->name == childName )
			return( *it );

	return( NULL );
}

const CProfileNode* CProfileNode::GetScriptChild( const char* childName ) const
{
	const_profile_iterator it;
	for( it = script_children.begin(); it != script_children.end(); ++it )
		if( (*it)->name == childName )
			return( *it );

	return( NULL );
}

CProfileNode* CProfileNode::GetChild( const char* childName )
{
	profile_iterator it;
	for( it = children.begin(); it != children.end(); ++it )
		if( (*it)->name == childName )
			return( *it );
	
	CProfileNode* newNode = new CProfileNode( childName, this );
	children.push_back( newNode );
	return( newNode );
}

CProfileNode* CProfileNode::GetScriptChild( const char* childName )
{
	profile_iterator it;
	for( it = script_children.begin(); it != script_children.end(); ++it )
		if( (*it)->name == childName )
			return( *it );
	
	CProfileNode* newNode = new CProfileNode( childName, this );
	script_children.push_back( newNode );
	return( newNode );
}

bool CProfileNode::CanExpand()
{
	return( !( children.empty() && script_children.empty() ) );
}

void CProfileNode::Reset()
{
	calls_per_frame.clear();
	calls_per_turn.clear();
	calls_frame_current = 0;
	calls_turn_current = 0;

	time_per_frame.clear();
	time_per_turn.clear();
	time_frame_current = 0.0;
	time_turn_current = 0.0;

	mallocs_per_frame.clear();
	mallocs_per_turn.clear();
	mallocs_frame_current = 0;
	mallocs_turn_current = 0;

	profile_iterator it;
	for (it = children.begin(); it != children.end(); ++it)
		(*it)->Reset();
	for (it = script_children.begin(); it != script_children.end(); ++it)
		(*it)->Reset();
}

void CProfileNode::Frame()
{
	calls_per_frame.push_back(calls_frame_current);
	time_per_frame.push_back(time_frame_current);
	mallocs_per_frame.push_back(mallocs_frame_current);

	calls_frame_current = 0;
	time_frame_current = 0.0;
	mallocs_frame_current = 0;
	
	profile_iterator it;
	for (it = children.begin(); it != children.end(); ++it)
		(*it)->Frame();
	for (it = script_children.begin(); it != script_children.end(); ++it)
		(*it)->Frame();
}

void CProfileNode::Turn()
{
	calls_per_turn.push_back(calls_turn_current);
	time_per_turn.push_back(time_turn_current);
	mallocs_per_turn.push_back(mallocs_turn_current);

	calls_turn_current = 0;
	time_turn_current = 0.0;
	mallocs_turn_current = 0;

	profile_iterator it;
	for (it = children.begin(); it != children.end(); ++it)
		(*it)->Turn();
	for (it = script_children.begin(); it != script_children.end(); ++it)
		(*it)->Turn();
}

// TODO: these should probably only count allocations that occur in the thread being profiled
#if defined(USE_CRT_SET_ALLOC_HOOK)

static long malloc_count = 0;
static _CRT_ALLOC_HOOK prev_hook;

static int crt_alloc_hook(int allocType, void* userData, size_t size, int blockType,
	long requestNumber, const unsigned char* filename, int lineNumber)
{
	if (allocType == _HOOK_ALLOC && ThreadUtil::IsMainThread())
		++malloc_count;

	if (prev_hook)
		return prev_hook(allocType, userData, size, blockType, requestNumber, filename, lineNumber);
	else
		return 1;
}

static void alloc_hook_initialize()
{
	prev_hook = _CrtSetAllocHook(crt_alloc_hook);
}

static long get_memory_alloc_count()
{
	return malloc_count;
}

#elif defined(USE_GLIBC_MALLOC_HOOK)

// Set up malloc hooks to count allocations - see
// http://www.gnu.org/software/libc/manual/html_node/Hooks-for-Malloc.html
static intptr_t malloc_count = 0;
static void *(*old_malloc_hook) (size_t, const void*);
static pthread_mutex_t alloc_hook_mutex = PTHREAD_MUTEX_INITIALIZER;
static void *malloc_hook(size_t size, const void* UNUSED(caller))
{
	// This doesn't really work across threads. The hooks are global variables, and
	// we have to temporarily unhook in order to call the real malloc, and during that
	// time period another thread may perform an unhooked (hence uncounted) allocation
	// which we will miss.

	// Two threads may execute the hook simultaneously, so we need to do the
	// temporary unhooking in a thread-safe way, so for simplicity we just use a mutex.
	pthread_mutex_lock(&alloc_hook_mutex);
	++malloc_count;
	__malloc_hook = old_malloc_hook;
	void* result = malloc(size);
	old_malloc_hook = __malloc_hook;
	__malloc_hook = malloc_hook;
	pthread_mutex_unlock(&alloc_hook_mutex);
	return result;
}

static void alloc_hook_initialize()
{
	pthread_mutex_lock(&alloc_hook_mutex);
	old_malloc_hook = __malloc_hook;
	__malloc_hook = malloc_hook;
	// (we don't want to bother hooking realloc and memalign, because if they allocate
	// new memory then they'll be caught by the malloc hook anyway)
	pthread_mutex_unlock(&alloc_hook_mutex);
}
/*
It would be nice to do:
    __attribute__ ((visibility ("default"))) void (*__malloc_initialize_hook)() = malloc_initialize_hook;
except that doesn't seem to work in practice, since something (?) resets the
hook to NULL some time while loading the game, after we've set it here - so
we just call malloc_initialize_hook once inside CProfileManager::Frame instead
and hope nobody deletes our hook after that.
*/
static long get_memory_alloc_count()
{
	return malloc_count;
}

#elif defined(USE_GLIBC_MALLOC_OVERRIDE)

static intptr_t alloc_count = 0;

// We override the malloc/realloc/calloc/free functions and then use dlsym to
// defer the actual allocation to the real libc implementation.
// The dlsym call will (in glibc 2.9/2.10) call calloc once (to allocate an error
// message structure), so we have a bootstrapping problem when trying to
// get the first called function via dlsym. So we kludge it by returning a statically-allocated
// buffer for the very first call to calloc after we've called dlsym.
// This is quite hacky but it seems to just about work in practice...
static bool alloc_bootstrapped = false;
static char alloc_bootstrap_buffer[32]; // sufficient for x86_64
static bool alloc_has_called_dlsym = false;
// (We'll only be running a single thread at this point so no need for locking these variables)

//#define ALLOC_DEBUG

void* malloc(size_t sz)
{
	cpu_AtomicAdd(&alloc_count, 1);

	static void *(*libc_malloc)(size_t);
	if (libc_malloc == NULL)
	{
		alloc_has_called_dlsym = true;
		libc_malloc = (void *(*)(size_t)) dlsym(RTLD_NEXT, "malloc");
	}
	void* ret = libc_malloc(sz);
#ifdef ALLOC_DEBUG
	printf("### malloc(%d) = %p\n", sz, ret);
#endif
	return ret;
}

void* realloc(void* ptr, size_t sz)
{
	cpu_AtomicAdd(&alloc_count, 1);

	static void *(*libc_realloc)(void*, size_t);
	if (libc_realloc == NULL)
	{
		alloc_has_called_dlsym = true;
		libc_realloc = (void *(*)(void*, size_t)) dlsym(RTLD_NEXT, "realloc");
	}
	void* ret = libc_realloc(ptr, sz);
#ifdef ALLOC_DEBUG
	printf("### realloc(%p, %d) = %p\n", ptr, sz, ret);
#endif
	return ret;
}

void* calloc(size_t nm, size_t sz)
{
	cpu_AtomicAdd(&alloc_count, 1);

	static void *(*libc_calloc)(size_t, size_t);
	if (libc_calloc == NULL)
	{
		if (alloc_has_called_dlsym && !alloc_bootstrapped)
		{
			ENSURE(nm*sz <= ARRAY_SIZE(alloc_bootstrap_buffer));
#ifdef ALLOC_DEBUG
			printf("### calloc-bs(%d, %d) = %p\n", nm, sz, alloc_bootstrap_buffer);
#endif
			alloc_bootstrapped = true;
			return alloc_bootstrap_buffer;
		}
		alloc_has_called_dlsym = true;
		libc_calloc = (void *(*)(size_t, size_t)) dlsym(RTLD_NEXT, "calloc");
	}
	void* ret = libc_calloc(nm, sz);
#ifdef ALLOC_DEBUG
	printf("### calloc(%d, %d) = %p\n", nm, sz, ret);
#endif
	return ret;
}

void free(void* ptr)
{
	static void (*libc_free)(void*);
	if (libc_free == NULL)
	{
		alloc_has_called_dlsym = true;
		libc_free = (void (*)(void*)) dlsym(RTLD_NEXT, "free");
	}

	libc_free(ptr);
#ifdef ALLOC_DEBUG
	printf("### free(%p)\n", ptr);
#endif
}

static void alloc_hook_initialize()
{
}

static long get_memory_alloc_count()
{
	return alloc_count;
}

#else

static void alloc_hook_initialize()
{
}
static long get_memory_alloc_count()
{
	// TODO: don't show this column of data when we don't have sensible values
	// to display.
	return 0;
}
#endif

void CProfileNode::Call()
{
	calls_frame_current++;
	calls_turn_current++;
	if (recursion++ == 0)
	{
		start = timer_Time();
		start_mallocs = get_memory_alloc_count();
	}
}

bool CProfileNode::Return()
{
	if (--recursion != 0)
		return false;

	double now = timer_Time();
	long allocs = get_memory_alloc_count();
	time_frame_current += (now - start);
	time_turn_current += (now - start);
	mallocs_frame_current += (allocs - start_mallocs);
	mallocs_turn_current += (allocs - start_mallocs);
	return true;
}

CProfileManager::CProfileManager() :
	root(NULL), current(NULL), needs_structural_reset(false)
{
	PerformStructuralReset();
}

CProfileManager::~CProfileManager()
{
	delete root;
}

void CProfileManager::Start( const char* name )
{
	if( name != current->GetName() )
		current = current->GetChild( name );
	current->Call();
}

void CProfileManager::StartScript( const char* name )
{
	if( name != current->GetName() )
		current = current->GetScriptChild( name );
	current->Call();
}

void CProfileManager::Stop()
{
	if (current->Return())
		current = current->GetParent();
}

void CProfileManager::Reset()
{
	root->Reset();
}

void CProfileManager::Frame()
{
	ONCE(alloc_hook_initialize());

	root->time_frame_current += (timer_Time() - root->start);
	root->mallocs_frame_current += (get_memory_alloc_count() - root->start_mallocs);

	root->Frame();

	if (needs_structural_reset)
	{
		PerformStructuralReset();
		needs_structural_reset = false;
	}

	root->start = timer_Time();
	root->start_mallocs = get_memory_alloc_count();
}

void CProfileManager::Turn()
{
	root->Turn();
}

void CProfileManager::StructuralReset()
{
	// We can't immediately perform the reset, because we're probably already
	// nested inside the profile tree and it will get very confused if we delete
	// the tree when we're not currently at the root.
	// So just set a flag to perform the reset at the end of the frame.

	needs_structural_reset = true;
}

void CProfileManager::PerformStructuralReset()
{
	delete root;
	root = new CProfileNode("root", NULL);
	root->Call();
	current = root;
	g_ProfileViewer.AddRootTable(root->display_table, true);
}
