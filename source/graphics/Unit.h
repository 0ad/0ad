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
	// constructor - unit invalid without a model and object
	CUnit(CObjectEntry* object, CModel* model)
		: m_Object(object), m_Model(model), m_Entity(NULL), m_ID(-1)
	{
		debug_assert(object && model);
	}
	CUnit(CObjectEntry* object, CModel* model, CEntity* entity)
		: m_Object(object), m_Model(model), m_Entity(entity), m_ID(-1)
	{
		debug_assert(object && model);
	}

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
	bool SetRandomAnimation(const CStr8& name, bool once = false);

	// Returns the animation a random one matching 'name'. If none is found,
	// returns idle instead.
	CSkeletonAnim* GetRandomAnimation(const CStr8& name);

	// Returns whether the currently active animation is one of the ones
	// matchin 'name'.
	bool IsPlayingAnimation(const CStr8& name);

	int GetID() const { return m_ID; }
	void SetID(int id) { m_ID = id; }

private:
	// object from which unit was created
	CObjectEntry* m_Object;
	// object model representation
	CModel* m_Model;
	// the entity that this actor represents, if any
	CEntity* m_Entity;
	// unique (per CGame) ID number for units created in the editor, as a
	// permanent way of referencing them. -1 for non-editor units.
	int m_ID;
};

#endif
