#ifndef _Player_H
#define _Player_H

#include "CStr.h"
#include "scripting/ScriptableObject.h"
#include "scripting/ScriptCustomTypes.h" 
#include "EntityHandles.h"

class CNetMessage;

typedef SColour SPlayerColour;

class CPlayer : public CJSObject<CPlayer>
{
	// FIXME: These shouldn't be public (need load-from-attributes method in game.cpp)
public:

	CStrW m_Name;
	uint m_PlayerID;
	SPlayerColour m_Colour;
	
public:
	CPlayer( uint playerID );

	bool ValidateCommand(CNetMessage *pMsg);

	inline const CStrW &GetName() const
	{	return m_Name; }

	// Caller frees...
	std::vector<HEntity>* GetControlledEntities();

// Scripting functions
	
	jsval ToString( JSContext* context, uintN argc, jsval* argv );	
	jsval GetControlledEntities_JS();

	static void ScriptingInit();
};

#endif
