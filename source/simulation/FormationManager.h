//This keeps track of all the formations exisisting in the game.

#ifndef INCLUDED_FORMATIONMANAGER
#define INCLUDED_FORMATIONMANAGER

#include "ps/Singleton.h"
#include "scripting/DOMEvent.h"

#define g_FormationManager CFormationManager::GetSingleton()

class CEntity;
class CStr;
class CFormation;
class CVector2D;
class CEntityFormation;

struct CEntityList;

class CFormationManager : public Singleton<CFormationManager>
{
#define FormIterator std::vector<CEntityFormation*>::iterator

public:
	CFormationManager() {}
	~CFormationManager();
	void CreateFormation( CEntityList& entities, CStrW& name );
	//entity is any unit in the formation
	void DestroyFormation( size_t form );
	inline bool IsValidFormation( size_t index )
	{
		return ((size_t)index < m_formations.size() && index >= 0);
	}
	bool AddUnit( CEntity* entity, size_t& form );
	CEntityList AddUnitList( CEntityList& entities, size_t form );

	//Returns false if the formation is destroyed
	bool RemoveUnit( CEntity* entity );
	bool RemoveUnitList( CEntityList& entities );
	CEntityFormation* GetFormation(size_t form);
	void UpdateIndexes( size_t update );

private:
	std::vector<CEntityFormation*> m_formations;
};

#endif // INCLUDED_FORMATIONMANAGER
