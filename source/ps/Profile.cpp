#include "precompiled.h"

#include "Profile.h"

// Note: As with the GPG profiler, name is assumed to be a pointer to a constant string; only pointer equality is checked.
CProfileNode::CProfileNode( const char* _name, CProfileNode* _parent )
{
	name = _name;
	recursion = 0;
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
	parent = _parent;

	Reset();
}

CProfileNode::~CProfileNode()
{
	profile_iterator it;
	for( it = children.begin(); it != children.end(); it++ )
		delete( *it );	
	for( it = script_children.begin(); it != script_children.end(); it++ )
		delete( *it );
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

	calls_frame_current = 0;
	time_frame_current = 0.0;
	
	profile_iterator it;
	for( it = children.begin(); it != children.end(); it++ )
		(*it)->Frame();
	for( it = script_children.begin(); it != script_children.end(); it++ )
		(*it)->Frame();
}

void CProfileNode::Call()
{
	calls_frame_current++;
	if( recursion++ == 0 )
		start = get_time();
}

bool CProfileNode::Return()
{
	if( !parent ) return( false );

	if( ( --recursion == 0 ) && ( calls_frame_current != 0 ) )
		time_frame_current += ( get_time() - start );
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

const char* CProfileManager::InternString( CStr8 intern )
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
	start = get_time();
	frame_start = get_time();
}

void CProfileManager::Frame()
{
	root->time_frame_current = ( get_time() - frame_start );
	root->Frame();
	
	frame_start = get_time();
}

void CProfileManager::StructuralReset()
{
	delete( root );
	root = new CProfileNode( "root", NULL );
	current = root;
	ResetProfileViewer();
}

double CProfileManager::GetTime()
{
	return( get_time() - start );
}

double CProfileManager::GetFrameTime()
{
	return( get_time() - frame_start );
}

