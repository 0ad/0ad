#ifndef _Player_H
#define _Player_H

#include "CStr.h"
#include "scripting/SynchedJSObject.h"
#include "scripting/ScriptableObject.h"
#include "scripting/ScriptCustomTypes.h" 

class CNetMessage;
class HEntity;

typedef SColour SPlayerColour;

class CPlayer : public CSynchedJSObject<CPlayer>
{
public:
	typedef void (UpdateCallback)(CStrW name, CStrW value, CPlayer *player, void *userdata);

private:
	CStrW m_Name;
	CStrW m_Civilization;	// Note: this must be the un-internationalized name of the civ
	PS_uint m_PlayerID;
	PS_uint m_LOSToken;
	SPlayerColour m_Colour;
	
	UpdateCallback *m_UpdateCB;
	void *m_UpdateCBData;
	
	virtual void Update(CStrW name, ISynchedJSProperty *prop);
	
public:
	CPlayer( uint playerID );
	~CPlayer();

	bool ValidateCommand(CNetMessage *pMsg);

	inline PS_uint GetPlayerID() const
	{	return m_PlayerID; }
	inline void SetPlayerID(PS_uint id)
	{	m_PlayerID=id; }

	inline PS_uint GetLOSToken() const
	{	return m_LOSToken; }
	
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
	void SetValue(CStrW name, CStrW value);
	
	// Caller frees...
	std::vector<HEntity>* GetControlledEntities();

	// JS Interface Functions
	jsval JSI_ToString( JSContext* context, uintN argc, jsval* argv );
	jsval JSI_GetControlledEntities( JSContext* context );
	jsval JSI_SetColour(JSContext *context, uintN argc, jsval *argv);
	jsval JSI_GetColour(JSContext *context, uintN argc, jsval *argv);

	static void ScriptingInit();
};

#endif
