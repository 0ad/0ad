#ifndef INCLUDED_GAMEATTRIBUTES
#define INCLUDED_GAMEATTRIBUTES

#include "Player.h"

#include "scripting/SynchedJSObject.h"

class CNetServerSession;
class CGameAttributes;
class CPlayerSlot;

enum EPlayerSlotAssignment
{
	SLOT_CLOSED,
	SLOT_OPEN,
	SLOT_SESSION,
	SLOT_AI
};

typedef void (PlayerSlotAssignmentCB)(void *data, CPlayerSlot *);

class CPlayerSlot: public CJSObject<CPlayerSlot>
{
	int m_SlotID;
	EPlayerSlotAssignment m_Assignment;

	CNetServerSession *m_pSession;
	int m_SessionID;
	CPlayer *m_pPlayer;
	
	PlayerSlotAssignmentCB *m_Callback;
	void *m_CallbackData;
	
	bool JSI_AssignClosed(JSContext *cx, uintN argc, jsval *argv);
	
	// Assign to a session, takes one argument (a NetSession object)
	bool JSI_AssignToSession(JSContext *cx, uintN argc, jsval *argv);
	// Assign to the local player in SP or Server Player in MP
	bool JSI_AssignLocal(JSContext *cx, uintN argc, jsval *argv);
	
	bool JSI_AssignOpen(JSContext *cx, uintN argc, jsval *argv);

// TODO This will wait until there actually is AI to set up
//	bool JSI_AssignAI(JSContext *cx, uintN argc, jsval *argv);

	jsval JSI_GetSession(JSContext* cx);
	jsval JSI_GetAssignment(JSContext* cx);

	void CallCallback();
	void SetAssignment(EPlayerSlotAssignment, CNetServerSession *pSession, int sessionID);
	
protected:
	friend class CGameAttributes;
	inline void SetSlotID(int slotID)
	{	m_SlotID=slotID; }

public:
	CPlayerSlot(int slotID, CPlayer *pPlayer);
	~CPlayerSlot();
	
	inline CPlayer *GetPlayer()
	{	return m_pPlayer; }
	inline int GetSessionID()
	{	return m_SessionID; }
	inline int GetSlotID()
	{	return m_SlotID; }
	
	// Only applicable on the server host, and may return NULL if the slot
	// is not assigned to a server session.
	inline CNetServerSession *GetSession()
	{	return m_pSession; }
	
	
	inline void SetCallback(PlayerSlotAssignmentCB *callback, void *data)
	{
		m_Callback=callback;
		m_CallbackData=data;
	}
	
	inline EPlayerSlotAssignment GetAssignment()
	{	return m_Assignment; }

	// Reset any assignment the slot might have had before and mark the slot as
	// closed.
	void AssignClosed();

	// [Server] Assign the slot to a connected session
	void AssignToSession(CNetServerSession *pSession);

	// [Client] The slot has been assigned by the server to a session ID, mirror
	// the assignment
	void AssignToSessionID(int sessionID);

	// Reset any assignment the slot might have before and mark the slot as free
	void AssignOpen();

	// Assign to the local player in SP or Server Player in MP
	void AssignLocal();

// TODO This will wait until there actually is AI to set up
//	void AssignAI();

	static void ScriptingInit();
};

namespace PlayerSlotArray_JS
{
	JSBool GetProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp );	
}

class CGameAttributes:
	public CSynchedJSObject<CGameAttributes>,
	public Singleton<CGameAttributes>
{
public:
	typedef void (UpdateCallback)(const CStrW& name, const CStrW& newValue, void *data);

	CStrW m_MapFile;
	CStrW m_ResourceLevel;
	CStrW m_StartingPhase;
	CStrW m_GameMode;
	uint m_LOSSetting;
	bool m_FogOfWar;
	bool m_ScreenshotMode;

	// Note: we must use the un-internationalized name of the resource level and starting phase

private:
	friend JSBool PlayerSlotArray_JS::GetProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp );

	uint m_NumSlots;
	
	// All players in the game. m_Players[0] is the Gaia Player, like in CGame.
	// m_Players[1..n] have a corresponding player slot in m_PlayerSlots[0..n-1]
	std::vector <CPlayer *> m_Players;

	std::vector <CPlayerSlot *> m_PlayerSlots;
	JSObject *m_PlayerSlotArrayJS;

	UpdateCallback *m_UpdateCB;
	void *m_UpdateCBData;
	
	CPlayer::UpdateCallback *m_PlayerUpdateCB;
	void *m_PlayerUpdateCBData;
	
	PlayerSlotAssignmentCB *m_PlayerSlotAssignmentCB;
	void *m_PlayerSlotAssignmentCBData;
	
	virtual void Update(const CStrW& name, ISynchedJSProperty *attrib);	
	static void OnNumSlotsUpdate(CSynchedJSObjectBase *owner);

	jsval JSI_GetPlayerSlots(JSContext* cx);
	jsval_t JSI_GetOpenSlot(JSContext *cx, uintN argc, jsval *argv);

public:
	CGameAttributes();
	virtual ~CGameAttributes();
	
	void SetValue(const CStrW& name, const CStrW& value);
	
	inline void SetUpdateCallback(UpdateCallback *cb, void *userdata)
	{
		m_UpdateCB=cb;
		m_UpdateCBData=userdata;
	}
	
	inline uint GetSlotCount()
	{	return m_NumSlots; }

	inline CStrW GetGameMode()
	{ return m_GameMode; }

	// Remove all slots that are either opened or closed, so that all slots have
	// an assignment and a player. Player IDs will be assigned in the same order
	// as the slot indexes, but without holes in the numbering.
	void FinalizeSlots();
	
	// Get the player object for the passed Player ID
	CPlayer *GetPlayer(int id);
	// Get the slot object with the specified index
	CPlayerSlot *GetSlot(int index);
	
	void SetPlayerUpdateCallback(CPlayer::UpdateCallback *cb, void *userdata);
	void SetPlayerSlotAssignmentCallback(PlayerSlotAssignmentCB *cb, void *userdata);

	static void ScriptingInit();
};
#define g_GameAttributes CGameAttributes::GetSingleton()

#endif
