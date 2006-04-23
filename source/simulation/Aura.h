#include "Entity.h"
#include "EntityHandles.h"

#ifndef __AURA_H__
#define __AURA_H__

class CAura
{
public:
	JSContext* m_cx;
	CEntity* m_source;
	CStrW m_name;
	float m_radius;
	JSObject* m_handler;
	std::vector<HEntity> m_influenced;

	CAura( JSContext* cx, CEntity* source, CStrW& name, float radius, JSObject* handler );
	~CAura();
	
	// Remove all entities from under our influence; this isn't done in the destructor since
	// the destructor needs to be called at the end of the program when some CEntities
	// have been deleted despite our keeping handles to them, in addition to just when an
	// entity dies. RemoveAll will only be called in the second case.
	void RemoveAll();

	// Forcefully removes an entity from the aura. Useful so that a unit that is killed can
	// notify its auras to remove it before it dies (so they can still access its data).
	void Remove( CEntity* ent );

	void Update( size_t timestep );
};

#endif
