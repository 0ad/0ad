#ifndef INCLUDED_AURA
#define INCLUDED_AURA

#include "EntityHandles.h"
#include "maths/Vector4D.h"
#include "scripting/SpiderMonkey.h"
#include "ps/CStr.h"

class CEntity;

class CAura
{
public:
	JSContext* m_cx;
	CEntity* m_source;
	CStrW m_name;
	CVector4D m_color;
	float m_radius;			// In graphics units
	int m_tickRate;		// In milliseconds
	JSObject* m_handler;
	std::vector<HEntity> m_influenced;
	int m_tickCyclePos;	// Add time to this until it's time to tick again

	CAura( JSContext* cx, CEntity* source, CStrW& name, float radius, int tickRate, const CVector4D& color, JSObject* handler );
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
	void Update( int timestep );
};

#endif
