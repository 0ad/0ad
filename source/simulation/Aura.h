#ifndef __AURA_H__
#define __AURA_H__

#include "EntityHandles.h"
#include "maths/Vector4D.h"

class CEntity;

class CAura
{
public:
	JSContext* m_cx;
	CEntity* m_source;
	CStrW m_name;
	CVector4D m_color;
	float m_radius;			// In graphics units
	size_t m_tickRate;		// In milliseconds
	JSObject* m_handler;
	std::vector<HEntity> m_influenced;
	size_t m_tickCyclePos;	// Add time to this until it's time to tick again

	CAura( JSContext* cx, CEntity* source, CStrW& name, float radius, size_t tickRate, const CVector4D& color, JSObject* handler );
	~CAura();
	
	// Remove all entities from under our influence; this isn't done in the destructor since
	// the destructor needs to be called at the end of the program when some CEntities
	// have been deleted despite our keeping handles to them, in addition to just when an
	// entity dies. RemoveAll will only be called in the second case.
	void RemoveAll();

	// Forcefully removes an entity from the aura. Useful so that a unit that is killed can
	// notify its auras to remove it before it dies (so they can still access its data).
	void Remove( CEntity* ent );

	// Called to update the aura each simulation frame.
	void Update( size_t timestep );
};

#endif
