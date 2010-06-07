/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef INCLUDED_GAMEATTRIBUTES
#define INCLUDED_GAMEATTRIBUTES

#include "Player.h"

#include "scripting/SynchedJSObject.h"
#include "simulation/LOSManager.h"

//class CNetServerSession;
class CNetSession;
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
	size_t m_SlotID;
	EPlayerSlotAssignment m_Assignment;

	//CNetServerSession *m_pSession;
	CNetSession *m_pSession;
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
	//void SetAssignment(EPlayerSlotAssignment, CNetServerSession *pSession, int sessionID);
	void SetAssignment(EPlayerSlotAssignment, CNetSession *pSession, int sessionID);
	
protected:
	friend class CGameAttributes;
	inline void SetSlotID(size_t slotID)
	{	m_SlotID=slotID; }

public:
	CPlayerSlot(size_t slotID, CPlayer *pPlayer);
	~CPlayerSlot();
	
	inline CPlayer *GetPlayer()
	{	return m_pPlayer; }
	inline int GetSessionID()
	{	return m_SessionID; }
	inline size_t GetSlotID()
	{	return m_SlotID; }
	
	// Only applicable on the server host, and may return NULL if the slot
	// is not assigned to a server session.
	//inline CNetServerSession *GetSession()
	//{	return m_pSession; }
	inline CNetSession *GetSession()
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
	//void AssignToSession(CNetServerSession *pSession);
	void AssignToSession(CNetSession *pSession);

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
	public CSynchedJSObject<CGameAttributes>
{
public:
	typedef void (UpdateCallback)(const CStrW& name, const CStrW& newValue, void *data);

	CStrW m_MapFile;
	CStrW m_ResourceLevel;
	CStrW m_StartingPhase;
	CStrW m_GameMode;
	int m_LOSSetting; // ELOSSetting
	bool m_FogOfWar;
	bool m_ScreenshotMode;

	// Note: we must use the un-internationalized name of the resource level and starting phase

private:
	friend JSBool PlayerSlotArray_JS::GetProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp );

	size_t m_NumSlots;
	
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
	jsval JSI_GetUsedSlotsAmount(JSContext* cx);

public:
	CGameAttributes();
	virtual ~CGameAttributes();
	
	void SetValue(const CStrW& name, const CStrW& value);
	
	inline void SetUpdateCallback(UpdateCallback *cb, void *userdata)
	{
		m_UpdateCB=cb;
		m_UpdateCBData=userdata;
	}
	
	inline size_t GetSlotCount()
	{	return m_NumSlots; }

	inline CStrW GetGameMode()
	{ return m_GameMode; }

	// Remove all slots that are either opened or closed, so that all slots have
	// an assignment and a player. Player IDs will be assigned in the same order
	// as the slot indexes, but without holes in the numbering.
	void FinalizeSlots();
	
	// Get the player object for the passed Player ID
	CPlayer *GetPlayer(size_t id);
	// Get the slot object with the specified index
	CPlayerSlot *GetSlot(size_t index);
	
	void SetPlayerUpdateCallback(CPlayer::UpdateCallback *cb, void *userdata);
	void SetPlayerSlotAssignmentCallback(PlayerSlotAssignmentCB *cb, void *userdata);

	static void ScriptingInit();
};

extern CGameAttributes *g_GameAttributes;

#endif
