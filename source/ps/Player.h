#ifndef _Player_H
#define _Player_H

#include "CStr.h"
#include "scripting/SynchedJSObject.h"
#include "scripting/ScriptableObject.h"
#include "scripting/ScriptCustomTypes.h" 
#include "EntityHandles.h"

class CNetMessage;

typedef SColour SPlayerColour;

class CPlayer : public CSynchedJSObject<CPlayer>
{
public:
	typedef void (UpdateCallback)(CStrW name, CStrW value, CPlayer *player, void *userdata);

private:
	CStrW m_Name;
	uint m_PlayerID;
	SPlayerColour m_Colour;
	
	UpdateCallback *m_UpdateCB;
	void *m_UpdateCBData;
	
	virtual void Update(CStrW name, ISynchedJSProperty *prop);
	
public:
	CPlayer( uint playerID );

	bool ValidateCommand(CNetMessage *pMsg);

	inline uint GetPlayerID() const
	{	return m_PlayerID; }
	
	inline const CStrW &GetName() const
	{	return m_Name; }
	inline void SetName(const CStrW &name)
	{	m_Name = name; }
	
	inline const SPlayerColour &GetColour() const
	{	return m_Colour; }
	inline void SetColour(const SPlayerColour &colour)
	{	m_Colour = colour; }

	inline void SetUpdateCallback(UpdateCallback *cb, void *userdata)
	{
		m_UpdateCB=cb;
		m_UpdateCBData=userdata;
	}

	// Caller frees...
	std::vector<HEntity>* GetControlledEntities();

// Scripting functions
	
	jsval ToString( JSContext* context, uintN argc, jsval* argv );	
	jsval GetControlledEntities_JS();

	static void ScriptingInit();
};

#endif
