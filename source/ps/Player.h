#ifndef INCLUDED_PLAYER
#define INCLUDED_PLAYER

#include "CStr.h"
#include "scripting/SynchedJSObject.h"
#include "scripting/ScriptableObject.h"
#include "scripting/ScriptCustomTypes.h" 
#include "ps/scripting/JSCollection.h" 
#include "Game.h"

class CNetMessage;
class HEntity;
class CTechnology;

typedef SColour SPlayerColour;

enum EDiplomaticStance 
{
	DIPLOMACY_ENEMY,
	DIPLOMACY_NEUTRAL,
	DIPLOMACY_ALLIED
};

const size_t invalidPlayerId = ~size_t(0);	// rationale: see Unit.h

class CPlayer : public CSynchedJSObject<CPlayer>
{
public:
	typedef void (UpdateCallback)(const CStrW& name, const CStrW& value, CPlayer *player, void *userdata);

private:
	CStrW m_Name;
	CStrW m_Civilization;	// Note: this must be the un-internationalized name of the civ
	size_t m_PlayerID;
	size_t m_LOSToken;
	SPlayerColour m_Colour;
	int /*EDiplomaticStance*/ m_DiplomaticStance[PS_MAX_PLAYERS+1];
	std::vector<CTechnology*> m_ActiveTechs;
	
	UpdateCallback *m_UpdateCB;
	void *m_UpdateCBData;

	virtual void Update(const CStrW& name, ISynchedJSProperty *prop);
	
public:
	CPlayer( size_t playerID );
	~CPlayer();

	bool ValidateCommand(CNetMessage *pMsg);

	inline size_t GetPlayerID() const
	{	return m_PlayerID; }
	inline void SetPlayerID(size_t id)
	{	m_PlayerID=id; }

	inline size_t GetLOSToken() const
	{	return m_LOSToken; }
	
	inline const CStrW& GetName() const
	{	return m_Name; }
	inline void SetName(const CStrW& name)
	{	m_Name = name; }
	
	inline const SPlayerColour &GetColour() const
	{	return m_Colour; }
	inline void SetColour(const SPlayerColour &colour)
	{	m_Colour = colour; }

	inline EDiplomaticStance GetDiplomaticStance(CPlayer* other) const
	{	return (EDiplomaticStance)m_DiplomaticStance[other->m_PlayerID]; }
	inline void SetDiplomaticStance(CPlayer* other, EDiplomaticStance stance)
	{	m_DiplomaticStance[other->m_PlayerID] = stance; }

	inline void SetUpdateCallback(UpdateCallback *cb, void *userdata)
	{
		m_UpdateCB=cb;
		m_UpdateCBData=userdata;
	}
	void SetValue(const CStrW& name, const CStrW& value);
	
	inline void AddActiveTech(CTechnology* tech)
	{
		m_ActiveTechs.push_back(tech);
	}

	inline const std::vector<CTechnology*>& GetActiveTechs()
	{
		return m_ActiveTechs;
	}

	void GetControlledEntities(std::vector<HEntity>& controlled_entities);

	// JS Interface Functions
	CStrW JSI_ToString( JSContext* context, uintN argc, jsval* argv );
	jsval JSI_GetControlledEntities( JSContext* context );
	void JSI_SetColour(JSContext *context, uintN argc, jsval *argv);
	jsval_t JSI_GetColour(JSContext *context, uintN argc, jsval *argv);
	void JSI_SetDiplomaticStance(JSContext *context, uintN argc, jsval *argv);
	jsval_t JSI_GetDiplomaticStance(JSContext *context, uintN argc, jsval *argv);
	CStrW JSI_GetName(JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv));

	static void ScriptingInit();
};

typedef CJSCollection<CPlayer*, &CPlayer::JSI_class> PlayerCollection;

#endif
