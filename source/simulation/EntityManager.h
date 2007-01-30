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

#include <set>

#include "EntityHandles.h"
#include "ps/Game.h"
#include "ps/World.h"

class CEntityTemplate;
class CPlayer;
class CStrW;
class CVector3D;

#define MAX_HANDLES 4096

// collision patch size, in graphics units, not tiles (1 tile = 4 units)
#define COLLISION_PATCH_SIZE 8

#define g_EntityManager g_Game->GetWorld()->GetEntityManager()

class CEntityManager
{
friend class CEntity;
friend class HEntity;
friend class CHandle;
	CHandle m_entities[MAX_HANDLES];
	std::vector<bool> m_refd;
	std::vector<CEntity*> m_reaper;
	std::vector<CEntity*>* m_collisionPatches;
	int m_nextalloc;
	static bool m_extant;
	bool m_death;
	int m_collisionPatchesPerSide;
	
	//Optimized data for triggers. key = playerID, nested key = entity class, value = frequency
	std::map<size_t, std::map<CStrW, int> > m_entityClassData;

	void destroy( u16 handle );
	void deleteAllHelper();
	
	inline bool isEntityRefd( u16 index )
	{
		return m_refd[index];
		//return m_entities[index].m_refcount && !m_entities[index].m_entity->entf_get(ENTF_DESTROYED);
	}
	
public:
	bool m_screenshotMode;

	CEntityManager();
	~CEntityManager();

	HEntity create( CEntityTemplate* base, CVector3D position, float orientation, 
		const std::set<CStr>& actorSelections, const CStrW* building = 0 );

	HEntity create( const CStrW& templateName, CPlayer* player, CVector3D position, 
		float orientation, const CStrW* building = 0 );

	HEntity createFoundation( const CStrW& templateName, CPlayer* player, CVector3D position, 
		float orientation );

	HEntity* getByHandle( u16 index );
	CHandle *getHandle( int index );
	
	inline int getPlayerUnitCount( size_t player, const CStrW& name )
	{
		return m_entityClassData[player][name];
	}
	void AddEntityClassData(const HEntity& handle);

	void updateAll( size_t timestep );
	void interpolateAll( float relativeoffset );
	void InitializeAll();
// 	void TickAll();
	void renderAll();
	void conformAll();
	void invalidateAll();

	void deleteAll();
	
	bool GetDeath() { return m_death; }
	void SetDeath(bool set) { m_death=set; }
	
	// Predicate functions
	typedef bool (*EntityPredicate)( CEntity* target, void* userdata );

	template<EntityPredicate left, EntityPredicate right> static bool EntityPredicateLogicalOr( CEntity* target, void* userdata )
	{	return( left( target, userdata ) || right( target, userdata ) ); }
	template<EntityPredicate left, EntityPredicate right> static bool EntityPredicateLogicalAnd( CEntity* target, void* userdata )
	{	return( left( target, userdata ) && right( target, userdata ) ); }
	template<EntityPredicate operand> static bool EntityPredicateLogicalNot( CEntity* target, void* userdata )
	{	return( !operand( target, userdata ) );	}

	std::vector<HEntity>* matches( EntityPredicate predicate, void* userdata = NULL );
	std::vector<HEntity>* getExtant();
	void GetExtant( std::vector<CEntity*>& results ); // TODO: Switch most/all uses of getExtant() to this.
	static inline bool extant()	// True if the singleton is actively maintaining handles. When false, system is shutting down, handles are quietly dumped.
	{
		return( m_extant );
	}
	
	void GetInRange( float x, float z, float radius, std::vector<CEntity*>& results );
	void GetInLOS( CEntity* entity, std::vector<CEntity*>& results );

	std::vector<CEntity*>* getCollisionPatch( CEntity* e );
};

#endif
