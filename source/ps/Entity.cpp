// Last modified: 19 January 2004 (Mark Thompson)

#include "Entity.h"

//--------------------------------------------------------
// CEntity: Entity class
//--------------------------------------------------------

//--------------------------------------------------------
// CEntity::CEntity()
//--------------------------------------------------------

CEntity::CEntity()
{
}

//--------------------------------------------------------
// CEntity::~CEntity()
//--------------------------------------------------------

CEntity::~CEntity()
{
}

//--------------------------------------------------------
// CEntity::Dispatch( CEvent* ev )
//
// Handles the event ev, sent by the scheduler
//--------------------------------------------------------

void CEntity::Dispatch( CEvent* ev )
{
}

//--------------------------------------------------------
// CHandle: Stores entity refcounts, etc.
//--------------------------------------------------------

//--------------------------------------------------------
// CHandle::CHandle()
//--------------------------------------------------------

CHandle::CHandle()
{
	ptr = NULL;
	refcount = 0;
}

//--------------------------------------------------------
// CHandle::~CHandle()
//--------------------------------------------------------

CHandle::~CHandle()
{
	if( refcount )
	{
		// WARNING: RELEASING LIVE HANDLE!
		// TODO: Link this into the new logfile system.
	}
	delete ptr;
}

//--------------------------------------------------------
// CHandlePool: Tracks entity handles.
//--------------------------------------------------------

//--------------------------------------------------------
// CHandlePool::CHandlePool()
//--------------------------------------------------------

CHandlePool::CHandlePool()
{
  for( unsigned short t = 0; t < MAX_HANDLES; t++ )
  {
	  Pool[t].nextfree = t + 1;
  }
  FreeHandle = 0;
}

//--------------------------------------------------------
// CHandlePool::~CHandlePool()
//--------------------------------------------------------

CHandlePool::~CHandlePool()
{
	//Terminating, clearing remaining handles...
}

//--------------------------------------------------------
// CHandlePool::Assign( CEntity* ptr )
//--------------------------------------------------------

HEntity CHandlePool::Assign( CEntity* ptr )
{
	assert( FreeHandle != BAD_HANDLE );
	unsigned short i = FreeHandle;
	ptr->Handle = HEntityWeak( i );
	FreeHandle = Pool[i].nextfree;
	Pool[i].ptr = ptr;
	return HEntity( i );
}

//--------------------------------------------------------
// CHandlePool::Destroy( unsigned short index )
//--------------------------------------------------------

void CHandlePool::Destroy( unsigned short index )
{
	//Garbage collection
	delete Pool[index].ptr;
	Pool[index].nextfree = FreeHandle;
	Pool[index].refcount = 0;
	FreeHandle = index;
}

//--------------------------------------------------------
// HEntity: Refcounted handle to entities.
//--------------------------------------------------------

//--------------------------------------------------------
// HEntity::HEntity()
//--------------------------------------------------------

HEntity::HEntity()
{
	index = BAD_HANDLE;
}

//--------------------------------------------------------
// HEntity::HEntity( unsigned short index )
//--------------------------------------------------------

HEntity::HEntity( unsigned short _index )
{
	index = _index;
	AddRef();
}

//--------------------------------------------------------
// HEntity::HEntity( const HEntity& copy )
//--------------------------------------------------------

HEntity::HEntity( const HEntity& copy )
{
	index = copy.index;
	AddRef();
}

//--------------------------------------------------------
// HEntity::HEntity( const HEntityWeak& copy )
//--------------------------------------------------------

HEntity::HEntity( const HEntityWeak& copy )
{
	index = copy.index;
	AddRef();
}

//--------------------------------------------------------
// HEntity::AddRef()
//
// Increments reference count
//--------------------------------------------------------

void HEntity::AddRef()
{
	if( index == BAD_HANDLE ) return;
	g_HandlePool.Pool[index].refcount++;
}

//--------------------------------------------------------
// HEntity::DecRef()
//
// Decrements reference count
//--------------------------------------------------------

void HEntity::DecRef() 
{
	if( index == BAD_HANDLE ) return;
	if( !g_HandlePool.Pool[index].refcount ) return;
	if( --g_HandlePool.Pool[index].refcount == 0 )
		g_HandlePool.Destroy( index );
}

//--------------------------------------------------------
// HEntity::operator=( const HEntity& copy )
//--------------------------------------------------------

void HEntity::operator=( const HEntity& copy )
{
	DecRef();
	index = copy.index;
	AddRef();
}

//--------------------------------------------------------
// HEntity::operator=( const HEntityWeak& copy )
//--------------------------------------------------------

void HEntity::operator=( const HEntityWeak& copy )
{
	DecRef();
	index = copy.index;
	AddRef();
}

//--------------------------------------------------------
// HEntity::operator*()
//--------------------------------------------------------

CEntity& HEntity::operator*()
{
	assert( index != BAD_HANDLE );
	assert( g_HandlePool.Pool[index].ptr );
	return( *g_HandlePool.Pool[index].ptr );
}

//--------------------------------------------------------
// HEntity: operator->()
//--------------------------------------------------------

CEntity* HEntity::operator->()
{
	assert( index != BAD_HANDLE );
	assert( g_HandlePool.Pool[index].ptr );
	return( g_HandlePool.Pool[index].ptr );
}

//--------------------------------------------------------
// HEntity::~HEntity()
//--------------------------------------------------------

HEntity::~HEntity()
{
	DecRef();
}

//--------------------------------------------------------
// HEntityWeak: Like HEntity, but it isn't refcounted.
//				used mainly to hold references-to-self.
//				It's slightly faster, and automatically
//				converts to a safe HEntity when needed.
//--------------------------------------------------------

//--------------------------------------------------------
// HEntityWeak::HEntityWeak()
//--------------------------------------------------------

HEntityWeak::HEntityWeak()
{
	index = BAD_HANDLE;
}

//--------------------------------------------------------
// HEntityWeak::HEntityWeak( unsigned short _index )
//--------------------------------------------------------

HEntityWeak::HEntityWeak( unsigned short _index )
{
	index = _index;
}

//--------------------------------------------------------
// HEntityWeak::operator*()
//--------------------------------------------------------

CEntity& HEntityWeak::operator*()
{
	assert( index != BAD_HANDLE );
	assert( g_HandlePool.Pool[index].ptr );
	return( *g_HandlePool.Pool[index].ptr );
}

//--------------------------------------------------------
// HEntityWeak::operator->()
//--------------------------------------------------------

CEntity* HEntityWeak::operator->()
{
	assert( index != BAD_HANDLE );
	assert( g_HandlePool.Pool[index].ptr );
	return( g_HandlePool.Pool[index].ptr );
}

//--------------------------------------------------------
// HEntityWeak::operator HEntity()
//--------------------------------------------------------

HEntityWeak::operator HEntity()
{
	return HEntity( index );
}
