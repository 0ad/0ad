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
	u16 m_refcount;
	CHandle();
};

class HEntity
{
	friend CEntityManager;
	u16 m_handle;
	void addRef();
	void decRef();
	HEntity( u16 index );
public:
	CEntity& operator*();
	CEntity* operator->();
	HEntity();
	HEntity( const HEntity& copy );
	void operator=( const HEntity& copy );
	~HEntity();
};

#endif