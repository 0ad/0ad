/*
Entity.h

Entities and associated classes
Mark Thompson (markt@0ad.wildfiregames.com)

Last modified: 19 January 2004 (Mark Thompson)

--Overview--

Contains the class definitions for entities, netsafe entity handles, handle manager

--Usage--

TODO

--Examples--

TODO

--More info--

TDD at http://forums.wildfiregames.com/0ad

*/

#ifndef ENTITY_INCLUDED
#define ENTITY_INCLUDED

#include "assert.h"
#include "stdio.h"
#include "ps/singleton.h"

// Handle defines

#define MAX_HANDLES 8192
#define BAD_HANDLE MAX_HANDLES
#define NULL 0
typedef unsigned long DWORD;

// Forward-declarations

class CHandlePool;
class CEntity;
class HEntity;
class HEntityWeak;

// CEntity: Entity class

class CEntity
{
	friend CHandlePool;
private:
	HEntityWeak Handle;		// Reference to self, to include in messages, etc.
							// Weak handle isn't refcounted, but is as soon
							// as it gets passed out of this class.
public:
	CEntity();				
	~CEntity();				
	void Dispatch( CEvent* ev ); 
}

// CHandle: Entity handles, used by manager class

class CHandle
{
	friend CHandlePool;
private:
	CEntity* ptr;
	unsigned short refcount;
	unsigned short nextfree; //Linked list for free handles
	CHandle();
	~CHandle();
};

// CHandlePool: Tracks entity handles

#define g_HandlePool CHandlePool::GetSingleton()

class CHandlePool : public Singleton<CHandlePool>
{
	friend HEntity;
private:
	unsigned short FreeHandle;
	CHandle Pool[MAX_HANDLES];
public:
	CHandlePool();
	~CHandlePool();
	HEntity Assign( CEntity* ptr );
	HEntity Create() { return Assign( new CEntity ); };
	void Destroy( unsigned short index );
};

// HEntity: Entity smart pointer

class HEntity
{
	friend CHandlePool;
private:
	unsigned short index;
	HEntity( unsigned short index );
	void AddRef();
	void DecRef();
public:
	CEntity& operator*();
	CEntity* operator->();
	HEntity();
	HEntity( const HEntity& copy );
	HEntity( const HEntityWeak& copy );
	void operator=( const HEntity& copy );
	void operator=( const HEntityWeak& copy );
	~HEntity();
};

// HEntityWeak: Weak (non-refcounted) smart pointer

class HEntityWeak
{
	friend CHandlePool;
private:
	unsigned short index;
	HEntityWeak( unsigned short index );
public:
	CEntity& operator*();
	CEntity* operator->();
};

#endif // !defined( ENTITY_INCLUDED )

	