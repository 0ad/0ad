// EntityManager.h
// 
// Maintains entity id->object mappings. Does most of the work involved in creating an entity.
//
// Usage: Do not attempt to directly instantiate an entity class.
//        HEntity bob = g_EntityManager.Create( unit_class_name, position, orientation );
//	   or HEntity jim = g_EntityManager.Create( pointer_to_unit_class, position, orientation );
//
//        Perform updates on all world entities by g_EntityManager.UpdateAll( timestep )
//		  Dispatch an identical message to all world entities by g_EntityManager.dispatchAll( message_pointer )
//		  Fill an STL vector container with all entities matching a certain predicate with g_EntityManager.GetMatchingAsHandles( container, predicate, data )
//        or just get all entities with g_EntityManager.GetExtant().
//        
//        Those last two functions - caller has responsibility for deleting the collection when you're done with it.

#ifndef INCLUDED_ENTITYMANAGER
#define INCLUDED_ENTITYMANAGER

#include <set>
#include <bitset>
#include <map>

#include "EntityHandles.h"
#include "ps/Game.h"
#include "ps/World.h"

class CEntityTemplate;
class CPlayer;
class CStrW;
class CStr8;
class CVector3D;

const size_t MAX_HANDLES = 16384;
cassert(MAX_HANDLES < 0x10000);

// collision patch size, in graphics units, not tiles (1 tile = 4 units)
#define COLLISION_PATCH_SIZE 8

#define g_EntityManager g_Game->GetWorld()->GetEntityManager()

class CEntityManager
{
friend class CEntity;
friend class HEntity;
friend class CHandle;
	CHandle m_entities[MAX_HANDLES];
	std::bitset<MAX_HANDLES> m_refd;
	std::vector<CEntity*> m_reaper;
	std::vector<CEntity*>* m_collisionPatches;
	int m_nextalloc;
	static bool m_extant;
	bool m_death;
	int m_collisionPatchesPerSide;
	
	//Optimized data for triggers. key = playerID, nested key = entity class, value = frequency
	std::map<size_t, std::map<CStrW, int> > m_entityClassData;

	void Destroy( size_t handle );
	void DeleteAllHelper();
	
	inline bool IsEntityRefd( size_t index )
	{
		return m_refd[index];
		//return m_entities[index].m_refcount && !m_entities[index].m_entity->entf_get(ENTF_DESTROYED);
	}
	
public:
	bool m_screenshotMode;

	CEntityManager();
	~CEntityManager();

	HEntity Create( CEntityTemplate* base, CVector3D position, float orientation, 
		const std::set<CStr8>& actorSelections, const CStrW* building = 0 );

	HEntity Create( const CStrW& templateName, CPlayer* player, CVector3D position, 
		float orientation, const CStrW* building = 0 );

	HEntity CreateFoundation( const CStrW& templateName, CPlayer* player, CVector3D position, 
		float orientation );

	HEntity* GetByHandle( size_t index );
	CHandle *GetHandle( size_t index );
	
	inline int GetPlayerUnitCount( size_t player, const CStrW& name )
	{
		if ( m_entityClassData[player].find(name) == m_entityClassData[player].end() )
			m_entityClassData[player][name] = 0;
		return m_entityClassData[player][name];
	}
	void RemoveUnitCount(CEntity* ent);	//Removes unit from population count
	void AddEntityClassData(const HEntity& handle);

	void UpdateAll( int timestep );
	void InterpolateAll( float relativeoffset );
	void InitializeAll();
// 	void TickAll();
	void RenderAll();
	void ConformAll();
	void InvalidateAll();

	void DeleteAll();
	
	bool GetDeath() { return m_death; }
	void SetDeath(bool set) { m_death=set; }


	//Kai: added function to update the triangulation when entities are created
	void updateObstacle(CEntity* tempHandle);
	
	// Predicate functions
	typedef bool (*EntityPredicate)( CEntity* target, void* userdata );

	template<EntityPredicate left, EntityPredicate right> static bool EntityPredicateLogicalOr( CEntity* target, void* userdata )
	{	return( left( target, userdata ) || right( target, userdata ) ); }
	template<EntityPredicate left, EntityPredicate right> static bool EntityPredicateLogicalAnd( CEntity* target, void* userdata )
	{	return( left( target, userdata ) && right( target, userdata ) ); }
	template<EntityPredicate operand> static bool EntityPredicateLogicalNot( CEntity* target, void* userdata )
	{	return( !operand( target, userdata ) );	}

	void GetMatchingAsHandles( std::vector<HEntity>& matchlist, EntityPredicate predicate, void* userdata = 0 );
	void GetExtantAsHandles( std::vector<HEntity>& results );
	void GetExtant( std::vector<CEntity*>& results );
	static inline bool IsExtant()	// True if the singleton is actively maintaining handles. When false, system is shutting down, handles are quietly dumped.
	{
		return( m_extant );
	}
	
	void GetInRange( float x, float z, float radius, std::vector<CEntity*>& results );
	void GetInLOS( CEntity* entity, std::vector<CEntity*>& results );

	std::vector<CEntity*>* GetCollisionPatch( CEntity* e );
};

#endif
