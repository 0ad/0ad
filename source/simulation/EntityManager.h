// EntityManager.h
//
// Mark Thompson mot20@cam.ac.uk / mark@wildfiregames.com
// 
// Maintains entity id->object mappings. Does most of the work involved in creating an entity.
//
// Usage: Do not attempt to directly instantiate an entity class.
//        HEntity bob = g_EntityManager.create( unit_class_name, position, orientation );
//	   or HEntity jim = g_EntityManager.create( pointer_to_unit_class, position, orientation );
//
//        Perform updates on all world entities by g_EntityManager.updateAll( timestep )
//		  Dispatch an identical message to all world entities by g_EntityManager.dispatchAll( message_pointer )
//		  Get an STL vector container of all entities with a certain property with g_EntityManager.matches( predicate )
//        or just get all entities with g_EntityManager.getExtant().
//        
//        Those last two functions - caller has responsibility for deleting the collection when you're done with it.


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
friend class HEntity;
friend class CHandle;
	CHandle m_entities[MAX_HANDLES];
	std::vector<CEntity*> m_reaper;
	int m_nextalloc;
	static bool m_extant;
	void destroy( u16 handle );
public:
	typedef bool (*EntityPredicate)( CEntity* target );
	CEntityManager();
	~CEntityManager();
	HEntity create( CBaseEntity* base, CVector3D position, float orientation );
	HEntity create( CStr templatename, CVector3D position, float orientation );
	HEntity* getByHandle( u16 index );
	void updateAll( size_t timestep );
	void interpolateAll( float relativeoffset );
	void dispatchAll( CMessage* msg );
	void renderAll();
	std::vector<HEntity>* matches( EntityPredicate predicate );
	std::vector<HEntity>* matches( EntityPredicate predicate1, EntityPredicate predicate2 );
	std::vector<HEntity>* getExtant();
	static inline bool extant()	// True if the singleton is actively maintaining handles. When false, system is shutting down, handles are quietly dumped.
	{
		return( m_extant );
	}
};

#endif

