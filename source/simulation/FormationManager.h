//Andrew aka pyrolink
//ajdecker1022@msn.com
//This keeps track of all the formations exisisting in the game.

#include "ps/Singleton.h"
#include "BaseFormationCollection.h"
#include "scripting/DOMEvent.h"

#define g_FormationManager CFormationManager::GetSingleton()

class CEntity;
class CStr;
class CBaseFormation;
class CVector2D;
class CEntityFormation;

struct CEntityList;

class CFormationManager : public Singleton<CFormationManager>
{
#define FormIterator std::vector<CEntityFormation*>::iterator

public:
	CFormationManager() {}
	~CFormationManager();
	void CreateFormation( CStrW& name, CEntityList& entities );
	//entity is any unit in the formation
	void DestroyFormation( size_t form );
	inline bool IsValidFormation( int index )
	{
		return ((size_t)index < m_formations.size() && index >= 0);
	}
	bool AddUnit( CEntity*& entity, int& form );
	CEntityList AddUnitList( CEntityList& entities, int form );
	
	//Returns false if the formation is destroyed
	bool RemoveUnit( CEntity*& entity );
	CEntityFormation* GetFormation(int form);
	void UpdateIndexes( size_t update );

private:
	std::vector<CEntityFormation*> m_formations;
};