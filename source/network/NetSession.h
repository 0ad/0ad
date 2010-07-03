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

#include "network/fsm.h"
#include "network/NetHost.h"
#include "ps/CStr.h"
#include "scriptinterface/ScriptVal.h"

class CNetClient;
class CNetServer;

class CNetServerSessionLocal; // forward declaration, needed because of circular references

class CNetStatsTable;

/**
 * @file
 * Network client/server sessions.
 *
 * Each session has two classes: CNetClientSession runs on the client,
 * and CNetServerSession runs on the server.
 * A client runs one session at once; a server typically runs many.
 *
 * There are two variants of each session: Remote (the normal ENet-based
 * network session) and Local (a shortcut when the client and server are
 * running inside the same process and don't need the network to communicate).
 */

/**
 * The client end of a network session.
 * Provides an abstraction of the network interface, allowing communication with the server.
 */
class CNetClientSession
{
	NONCOPYABLE(CNetClientSession);

public:
	CNetClientSession(CNetClient& client);
	virtual ~CNetClientSession();

	virtual void Poll() = 0;
	virtual void Disconnect() = 0;
	virtual bool SendMessage(const CNetMessage* message) = 0;

	CNetClient& GetClient() { return m_Client; }

private:
	CNetClient& m_Client;
};

/**
 * ENet-based implementation of CNetClientSession.
 */
class CNetClientSessionRemote : public CNetClientSession
{
	NONCOPYABLE(CNetClientSessionRemote);

public:
	CNetClientSessionRemote(CNetClient& client);
	~CNetClientSessionRemote();

	bool Connect(u16 port, const CStr& server);

	virtual void Poll();
	virtual void Disconnect();
	virtual bool SendMessage(const CNetMessage* message);

	ENetPacket* CreatePacket(const CNetMessage* message);

private:
	ENetHost* m_Host;
	ENetPeer* m_Server;
	CNetStatsTable* m_Stats;
};

/**
 * Local implementation of CNetClientSession, for use with servers
 * running in the same process.
 */
class CNetClientSessionLocal : public CNetClientSession
{
	NONCOPYABLE(CNetClientSessionLocal);

public:
	CNetClientSessionLocal(CNetClient& client, CNetServer& server);

	void SetServerSession(CNetServerSessionLocal* session) { m_ServerSession = session; }
	CNetServerSessionLocal* GetServerSession() { return m_ServerSession; }

	virtual void Poll();
	virtual void Disconnect();
	virtual bool SendMessage(const CNetMessage* message);

	void AddLocalMessage(const CNetMessage* message);

private:
	CNetServer& m_Server;
	CNetServerSessionLocal* m_ServerSession;

	std::vector<CNetMessage*> m_LocalMessageQueue;
};


/**
 * The server's end of a network session.
 * Represents an abstraction of the state of the client, storing all the per-client data
 * needed by the server.
 */
class CNetServerSession : public CFsm
{
	NONCOPYABLE(CNetServerSession);

public:
	CNetServerSession(CNetServer& server);
	virtual ~CNetServerSession();

	CNetServer& GetServer() { return m_Server; }

	const CStr& GetGUID() const { return m_GUID; }
	void SetGUID(const CStr& guid) { m_GUID = guid; }

	const CStrW& GetUserName() const { return m_UserName; }
	void SetUserName(const CStrW& name) { m_UserName = name; }

	u32 GetHostID() const { return m_HostID; }
	void SetHostID(u32 id) { m_HostID = id; }

	virtual void Disconnect() = 0;
	virtual bool SendMessage(const CNetMessage* message) = 0;

private:
	CNetServer& m_Server;

	CStr m_GUID;
	CStrW m_UserName;
	u32 m_HostID;
};

/**
 * ENet-based implementation of CNetServerSession.
 */
class CNetServerSessionRemote : public CNetServerSession
{
	NONCOPYABLE(CNetServerSessionRemote);

public:
	CNetServerSessionRemote(CNetServer& server, ENetPeer* peer);

	virtual void Disconnect();
	virtual bool SendMessage(const CNetMessage* message);

private:
	ENetPeer* m_Peer;
};

/**
 * Local implementation of CNetServerSession, for use with clients
 * running in the same process.
 */
class CNetServerSessionLocal : public CNetServerSession
{
	NONCOPYABLE(CNetServerSessionLocal);

public:
	CNetServerSessionLocal(CNetServer& server, CNetClientSessionLocal& clientSession);

	virtual void Disconnect();
	virtual bool SendMessage(const CNetMessage* message);

private:
	CNetClientSessionLocal& m_ClientSession;
};

#endif	// NETSESSION_H
