#ifndef ENTITY_MANAGER_INCLUDED
#define ENTITY_MANAGER_INCLUDED

#include "Singleton.h"
#include "Entity.h"
#include "EntityHandles.h"
#include "EntityMessage.h"

#define MAX_HANDLES 4096

#define g_EntityManager CEntityManager::GetSingleton()

class CEntityManager : public Singleton<CEntityManager>
{
friend HEntity;
friend CHandle;
	CHandle m_entities[MAX_HANDLES];
	int m_nextalloc;
	static bool m_extant;
public:
	typedef bool (*EntityPredicate)( CEntity* target );
	CEntityManager();
	~CEntityManager();
	HEntity create( CBaseEntity* base, CVector3D position, float orientation );
	HEntity create( CStr templatename, CVector3D position, float orientation );
	void updateAll( float timestep );
	void dispatchAll( CMessage* msg );
	std::vector<HEntity>* matches( EntityPredicate predicate );
	static inline bool extant()	// True if the singleton is actively maintaining handles. When false, system is shutting down, handles are quietly dumped.
	{
		return( m_extant );
	}
};

#endif