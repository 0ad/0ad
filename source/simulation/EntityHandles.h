// EntityHandles.h
//
// Mark Thompson mot20@cam.ac.uk / mark@wildfiregames.com
// 
// Entity smart pointer definitions.
//
// Usage: Use in place of a standard CEntity pointer.
//	      Handles dereference and pointer-to-member as a standard smart pointer.
//		  Reference counted. Handles are freed and entity classes released when refcount hits 0.
//		  For this reason, be careful where and how long you store these.
//        
//		  Entities maintain their own handles via their HEntity me member.
//		  To release an entity, scramble this handle. The memory will be freed when the last reference to it expires.
//
//        Standard CEntity pointers must not be used for a) anything that could be sent over the network.
//														 b) anything that will be kept between simulation steps.
//		  CEntity pointers will be faster for passing to the graphics subsytem, etc. where they won't be stored.
//		  And yes, most of this /is/ obvious. But it should be said anyway.

#ifndef ENTITY_HANDLE_INCLUDED
#define ENTITY_HANDLE_INCLUDED

#include "types.h"

#define INVALID_HANDLE 65535

class CEntity;
class CEntityManager;

class CHandle
{
public:
	CEntity* m_entity;
	i16 m_refcount;
	CHandle();
};

class HEntity
{
	friend class CEntityManager;
	u16 m_handle;
private:
	void addRef();
	void decRef();
	HEntity( u16 index );
public:
	CEntity& operator*() const;
	CEntity* operator->() const;
	HEntity();
	HEntity( const HEntity& copy );
	void operator=( const HEntity& copy );
	bool operator==( const HEntity& test ) const;
	operator bool() const { return( m_handle != INVALID_HANDLE ); }
	operator CEntity*() const;
	~HEntity();
};

#endif
