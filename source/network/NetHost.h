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

#ifndef NETHOST_H
#define NETHOST_H

#include "fsm.h"

#include "ps/CStr.h"

#include <vector>

typedef struct _ENetPeer ENetPeer;
typedef struct _ENetHost ENetHost;
class CNetSession;
class CNetHost;
class CNetMessage;
class ScriptInterface;

struct PeerSession
{
	ENetPeer* pPeer;
	CNetSession* pSession;
};

typedef std::vector<PeerSession> PeerSessionList;

/**
 * Wrapper around ENet host concept
 */
class CNetHost
{
	NONCOPYABLE(CNetHost);

public:

	CNetHost(ScriptInterface& scriptInterface);
	virtual ~CNetHost();

	bool Create();
	bool Create(uint port, uint maxPeers);
	void Shutdown();

	/**
	 * Indicates whether the host is currently a server
	 *
	 * @return					Boolean indicating whether the host is a server
	 */
	virtual bool IsServer() const { return false; }

	/**
	 * Returns the number of sessions for the host
	 *
	 * @return					The number of sessions
	 */
	uint GetSessionCount() const;

	/**
	 * Returns the session object for the specified index
	 *
	 * @param index				Index for session
	 * @return					Session object for index or	NULL if not found
	 */
	CNetSession* GetSession(uint index);

	/**
	 * Connects to foreign host synchronously
	 *
	 * @param host						Foreign host name
	 * @param port						Port on which the foreign host listens
	 * @return							true on success, false on failure
	 */
	bool Connect(const CStr& host, uint port);

	/**
	 * Connects to foreign host asynchronously (i.e. without waiting for the connection
	 * to succeed or to time out)
	 *
	 * @param host						Foreign host name
	 * @param port						Port on which the foreign host listens
	 * @return							true on success, false on failure
	 */
	bool ConnectAsync(const CStr& host, uint port);

	/**
	 * Disconnects session from host
	 *
	 * @param pSession					Session representing peer
	 * @return							true on success, false otherwise
	 */
	bool Disconnect(CNetSession* pSession);

	/**
	 * Listens for incoming connections and dispatches host events
	 */
	void Poll();

	/**
	 * Broadcast the specified message to connected clients
	 *
	 * @param pMessage			Message to broadcast
	 */
	void Broadcast(const CNetMessage* pMessage);

	/**
	 * Send the specified message to client
	 *
	 * @param pMessage					The message to send
	 */
	bool SendMessage(const CNetSession* pSession, const CNetMessage* pMessage);

protected:

	// Allow application to handle new client connect
	virtual bool SetupSession(CNetSession* pSession) = 0;
	virtual bool HandleConnect(CNetSession* pSession) = 0;
	virtual bool HandleDisconnect(CNetSession* pSession) = 0;

private:
	/**
	 * Receive a message from client if available
	 */
	CNetMessage* ReceiveMessage(const CNetSession* pSession);

	bool HandleMessageReceive(CNetMessage* pMessage, CNetSession* pSession);

	ScriptInterface& m_ScriptInterface;

	ENetHost* m_Host; // Represents this host
	PeerSessionList m_PeerSessions; // Session list of connected peers
};

#endif	// NETHOST_H
