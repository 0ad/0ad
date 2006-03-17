#ifndef _UNIT_H
#define _UNIT_H



class CModel;
class CObjectEntry;
class CEntity;
class CSkeletonAnim;
class CStr8;

/////////////////////////////////////////////////////////////////////////////////////////////
// CUnit: simple "actor" definition - defines a sole object within the world
class CUnit
{
public:
	CUnit(CObjectEntry* object, CEntity* entity, const std::set<CStrW>& actorSelections);

	// destructor
	~CUnit();


	// get unit's template object; never NULL
	CObjectEntry* GetObject() { return m_Object; }
	// get unit's model data; never NULL
	CModel* GetModel() { return m_Model; }
	// get actor's entity; can be NULL
	CEntity* GetEntity() { return m_Entity; }

	// Put here as it conveniently references both the model and the ObjectEntry
	void ShowAmmunition();
	void HideAmmunition();

	// Sets the animation a random one matching 'name'. If none is found,
	// sets to idle instead.
	bool SetRandomAnimation(const CStr8& name, bool once = false, float speed = 0.0f);

	// Returns a random animation matching 'name'. If none is found,
	// returns idle instead.
	CSkeletonAnim* GetRandomAnimation(const CStr8& name);

	// Sets the entity-selection, and updates the unit to use the new
	// actor variation.
	void SetEntitySelection(const CStrW& selection);

	// Returns whether the currently active animation is one of the ones
	// matching 'name'.
	bool IsPlayingAnimation(const CStr8& name);

	// Set player ID of this unit
	void SetPlayerID(int id);

	int GetID() const { return m_ID; }
	void SetID(int id) { m_ID = id; }

private:
	// object from which unit was created
	CObjectEntry* m_Object;
	// object model representation
	CModel* m_Model;
	// the entity that this actor represents, if any
	CEntity* m_Entity;
	// player id of this unit (only used for graphical effects)
	int m_PlayerID;

	// unique (per map) ID number for units created in the editor, as a
	// permanent way of referencing them. -1 for non-editor units.
	int m_ID;

	// actor-level selections for this unit
	std::set<CStrW> m_ActorSelections;
	// entity-level selections for this unit
	std::set<CStrW> m_EntitySelections;

	void ReloadObject();
};

#endif
