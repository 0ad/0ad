/* Copyright (C) 2010 Wildfire Games.
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

#ifndef NETSESSION_H
#define NETSESSION_H

#include "fsm.h"
#include "scripting/ScriptableObject.h"

class CPlayerSlot;
class CNetHost;
class CNetSession;
typedef struct _ENetPeer ENetPeer;

struct FsmActionCtx
{
	CNetHost* pHost;
	CNetSession* pSession;
};

/**
 * CNetSession is a wrapper class around the ENet peer concept
 * which represents a peer from a network connection. A
 * network session is spawned by CNetServer each time a
 * client connects and destroyed when it disconnects. When a
 * new message is received from a client, its representing
 * session object's message handler is called for processing
 * that message.
 * CNetSession is also a state machine. All client requests
 * are delegated to the current state. The current
 * CNetSessionState object's methods will change the current
 * state as appropriate.
 */

class CNetSession : public CFsm,
					public CJSObject<CNetSession>
{
	NONCOPYABLE(CNetSession);

	friend class CNetHost;

public:

	virtual ~CNetSession();

	/**
	 * Retrieves the name of the session
	 *
	 * @return							Session name
	 */
	const CStrW& GetName() const { return m_Name; }

	/**
	 * Set the new name for the session
	 *
	 * @param name						The session new name
	 */
	void SetName(const CStr& name);

	/**
	 * Retrieves the ID of the session
	 *
	 * @return							Session ID
	 */
	uint GetID() const { return m_ID; }

	/**
	 * Set the ID for this session
	 *
	 * @param							New session ID
	 */
	void SetID(uint ID);

	FsmActionCtx* GetFsmActionCtx() { return &m_FsmActionCtx; }

	void SetPlayerSlot(CPlayerSlot* pPlayerSlot);
	CPlayerSlot* GetPlayerSlot() { return m_PlayerSlot; }
	static void ScriptingInit();

private:

	// Only the hosts can create sessions
	CNetSession(CNetHost* pHost, ENetPeer* pPeer);

	CNetHost*		  m_Host;			 // The associated local host
	ENetPeer*		  m_Peer;			 // Represents the peer host
	uint			  m_ID;				 // Session ID
	CStrW			  m_Name;			 // Session name
	CPlayerSlot*	  m_PlayerSlot;
	FsmActionCtx	  m_FsmActionCtx;
};

#endif	// NETSESSION_H
