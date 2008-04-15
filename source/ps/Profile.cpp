/**
 * =========================================================================
 * File        : Profile.cpp
 * Project     : Pyrogeneses
 * Description : GPG3-style hierarchical profiler
 * =========================================================================
 */

#include "precompiled.h"

#include "Profile.h"
#include "ProfileViewer.h"
#include "lib/timer.h"

#if OS_WIN
#include "lib/sysdep/win/wdbg_heap.h"
#endif
#if defined(__GLIBC__) && !defined(NDEBUG)
# define GLIBC_MALLOC_HOOK
# include <malloc.h>
# include "ThreadUtil.h"
#endif

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
			columns.push_back(ProfileColumn("calls/frame", 100));
			columns.push_back(ProfileColumn("msec/frame", 100));
			columns.push_back(ProfileColumn("%/frame", 100));
			columns.push_back(ProfileColumn("%/parent", 100));
			columns.push_back(ProfileColumn("mem allocs", 100));
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
	
	snprintf(buf, sizeof(buf), "Profiling Information for: %s (Time in node: %.3f msec/frame)", node->GetName(), node->GetFrameTime() * 1000.0f );
	buf[sizeof(buf)-1] = '\0';
	
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
		
		float unlogged = node->GetFrameTime();
		long unlogged_mallocs = node->GetFrameMallocs();
		CProfileNode::const_profile_iterator it;

		for (it = node->GetChildren()->begin(); it != node->GetChildren()->end(); ++it)
		{
			unlogged -= (*it)->GetFrameTime();
			unlogged_mallocs -= (*it)->GetFrameMallocs();
		}
		for (it = node->GetScriptChildren()->begin(); it != node->GetScriptChildren()->end(); ++it)
		{
			unlogged -= (*it)->GetFrameTime();
			unlogged_mallocs -= (*it)->GetFrameMallocs();
		}
		
		if (col == 2)
			snprintf(buf, sizeof(buf), "%.3f", unlogged * 1000.0f);
		else if (col == 3)
			snprintf(buf, sizeof(buf), "%.1f", unlogged / g_Profiler.GetRoot()->GetFrameTime());
		else if (col == 4)
			snprintf(buf, sizeof(buf), "%.1f", unlogged * 100.0f / g_Profiler.GetRoot()->GetFrameTime());
		else if (col == 5)
			snprintf(buf, sizeof(buf), "%ld", unlogged_mallocs);
		buf[sizeof(buf)-1] = '\0';
		
		return CStr(buf);
	}
	
	switch(col)
	{
	default:
	case 0:
		return child->GetName();
		
	case 1:
#ifdef PROFILE_AMORTIZE
		snprintf(buf, sizeof(buf), "%.3f", child->GetFrameCalls());
#else
		snprintf(buf, sizeof(buf), "%d", child->GetFrameCalls());
#endif
		break;
	case 2:
		snprintf(buf, sizeof(buf), "%.3f", child->GetFrameTime() * 1000.0f);
		break;
	case 3:
		snprintf(buf, sizeof(buf), "%.1f", child->GetFrameTime() * 100.0 / g_Profiler.GetRoot()->GetFrameTime());
		break;
	case 4:
		snprintf(buf, sizeof(buf), "%.1f", child->GetFrameTime() * 100.0 / node->GetFrameTime());
		break;
	case 5:
		snprintf(buf, sizeof(buf), "%ld", child->GetFrameMallocs());
		break;
	}
	buf[sizeof(buf)-1] = '\0';
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

	Reset();
}

CProfileNode::~CProfileNode()
{
	profile_iterator it;
	for( it = children.begin(); it != children.end(); it++ )
		delete( *it );	
	for( it = script_children.begin(); it != script_children.end(); it++ )
		delete( *it );
	
	delete display_table;
}

const CProfileNode* CProfileNode::GetChild( const char* childName ) const
{
	const_profile_iterator it;
	for( it = children.begin(); it != children.end(); it++ )
		if( (*it)->name == childName )
			return( *it );

	return( NULL );
}

const CProfileNode* CProfileNode::GetScriptChild( const char* childName ) const
{
	const_profile_iterator it;
	for( it = script_children.begin(); it != script_children.end(); it++ )
		if( (*it)->name == childName )
			return( *it );

	return( NULL );
}

CProfileNode* CProfileNode::GetChild( const char* childName )
{
	profile_iterator it;
	for( it = children.begin(); it != children.end(); it++ )
		if( (*it)->name == childName )
			return( *it );
	
	CProfileNode* newNode = new CProfileNode( childName, this );
	children.push_back( newNode );
	return( newNode );
}

CProfileNode* CProfileNode::GetScriptChild( const char* childName )
{
	profile_iterator it;
	for( it = script_children.begin(); it != script_children.end(); it++ )
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
	calls_total = 0;
	calls_frame_current = 0;
#ifdef PROFILE_AMORTIZE
	int i;
	for( i = 0; i < PROFILE_AMORTIZE_FRAMES; i++ )
	{
		calls_frame_buffer[i] = 0;
		time_frame_buffer[i] = 0.0;
	}
	calls_frame_last = calls_frame_buffer;
	calls_frame_amortized = 0.0f;
#else
	calls_frame_last = 0;
#endif

	time_total = 0.0;
	time_frame_current = 0.0;
#ifdef PROFILE_AMORTIZE
	time_frame_last = time_frame_buffer;
	time_frame_amortized = 0.0;
#else
	time_frame_last = 0.0;
#endif

	mallocs_total = 0;
	mallocs_frame_current = 0;
	mallocs_frame_last = 0;

	profile_iterator it;
	for( it = children.begin(); it != children.end(); it++ )
		(*it)->Reset();
	for( it = script_children.begin(); it != script_children.end(); it++ )
		(*it)->Reset();
}

void CProfileNode::Frame()
{
	calls_total += calls_frame_current;
	time_total += time_frame_current;
	mallocs_total += mallocs_frame_current;

#ifdef PROFILE_AMORTIZE
	calls_frame_amortized -= *calls_frame_last;
	*calls_frame_last = calls_frame_current;
	calls_frame_amortized += calls_frame_current;
	time_frame_amortized -= *time_frame_last;
	*time_frame_last = time_frame_current;
	time_frame_amortized += time_frame_current;
	if( ++calls_frame_last == ( calls_frame_buffer + PROFILE_AMORTIZE_FRAMES ) )
		calls_frame_last = calls_frame_buffer;
	if( ++time_frame_last == ( time_frame_buffer + PROFILE_AMORTIZE_FRAMES ) )
		time_frame_last = time_frame_buffer;
#else
	calls_frame_last = calls_frame_current;
	time_frame_last = time_frame_current;
#endif
	mallocs_frame_last = mallocs_frame_current;

	calls_frame_current = 0;
	time_frame_current = 0.0;
	mallocs_frame_current = 0;
	
	profile_iterator it;
	for( it = children.begin(); it != children.end(); it++ )
		(*it)->Frame();
	for( it = script_children.begin(); it != script_children.end(); it++ )
		(*it)->Frame();
}

// TODO: these should probably only count allocations that occur in the thread being profiled
#if OS_WIN
static void alloc_hook_initialize()
{
}
static long get_memory_alloc_count()
{
	return (long)wdbg_heap_NumberOfAllocations();
}
#elif defined(GLIBC_MALLOC_HOOK)
// Set up malloc hooks to count allocations - see
// http://www.gnu.org/software/libc/manual/html_node/Hooks-for-Malloc.html
static int malloc_count = 0;
static void *(*old_malloc_hook) (size_t, const void*);
static CMutex malloc_mutex;
static void *malloc_hook(size_t size, const void* UNUSED(caller))
{
	// TODO: this is totally not nicely thread-safe - glibc's hook system seems to
	// not make threading easy, and I don't want to think too hard, so this is
	// just all put inside a lock.
	CScopeLock lock(malloc_mutex);
	
	++malloc_count;
	__malloc_hook = old_malloc_hook;
	void* result = malloc(size);
	old_malloc_hook = __malloc_hook;
	__malloc_hook = malloc_hook;
	return result;
}
static void alloc_hook_initialize()
{
	CScopeLock lock(malloc_mutex);
	
	old_malloc_hook = __malloc_hook;
	__malloc_hook = malloc_hook;
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
	if( recursion++ == 0 )
	{
		start = timer_Time();
		start_mallocs = get_memory_alloc_count();
	}
}

bool CProfileNode::Return()
{
	if( !parent ) return( false );

	if( ( --recursion == 0 ) && ( calls_frame_current != 0 ) )
	{
		time_frame_current += ( timer_Time() - start );
		mallocs_frame_current += ( get_memory_alloc_count() - start_mallocs );
	}
	return( recursion == 0 );
}

void CProfileNode::ScriptingInit()
{
	AddProperty( L"name", (IJSObject::GetFn)&CProfileNode::JS_GetName );
	/*
	AddReadOnlyClassProperty( L"callsTotal", &CProfileNode::calls_total );
	AddReadOnlyClassProperty( L"callsPerFrame", &CProfileNode::calls_frame_last );
	AddReadOnlyClassProperty( L"timeTotal", &CProfileNode::time_total );
	AddReadOnlyClassProperty( L"timePerFrame", &CProfileNode::time_frame_last );
	*/
	CJSObject<CProfileNode, true>::ScriptingInit( "ProfilerNode" );
}

CProfileManager::CProfileManager()
{
	root = new CProfileNode( "root", NULL );
	current = root;
	frame_start = 0.0;
	frame_start_mallocs = 0;
	g_ProfileViewer.AddRootTable(root->display_table);
}

CProfileManager::~CProfileManager()
{
	std::map<CStr8, const char*>::iterator it;
	for( it = m_internedStrings.begin(); it != m_internedStrings.end(); it++ )
		delete[]( it->second );

	delete( root );
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

const char* CProfileManager::InternString( const CStr8& intern )
{
	std::map<CStr8, const char*>::iterator it = m_internedStrings.find( intern );
	if( it != m_internedStrings.end() )
		return( it->second );
	
	size_t length = intern.length();
	char* data = new char[length + 1];
	strcpy( data, intern.c_str() );
	data[length] = 0;
	m_internedStrings.insert( std::pair<CStr8, const char*>( intern, data ) );	
	return( data );
}

void CProfileManager::Stop()
{
	if( current->Return() )
		current = current->GetParent();
}

void CProfileManager::Reset()
{
	root->Reset();
	start = frame_start = timer_Time();
	start_mallocs = frame_start_mallocs = get_memory_alloc_count();
}

void CProfileManager::Frame()
{
	ONCE(alloc_hook_initialize());
	
	root->time_frame_current = ( timer_Time() - frame_start );
	root->mallocs_frame_current = ( get_memory_alloc_count() - frame_start_mallocs );
	root->Frame();
	
	frame_start = timer_Time();
	frame_start_mallocs = get_memory_alloc_count();
}

void CProfileManager::StructuralReset()
{
	delete( root );
	root = new CProfileNode( "root", NULL );
	current = root;
	g_ProfileViewer.AddRootTable(root->display_table);
}
