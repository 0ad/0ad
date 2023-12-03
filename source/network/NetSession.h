/* Copyright (C) 2022 Wildfire Games.
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

#include "lib/external_libraries/enet.h"
#include "network/FSM.h"
#include "network/NetFileTransfer.h"
#include "network/NetHost.h"
#include "ps/CStr.h"

#include <boost/lockfree/queue.hpp>

#include <atomic>

/**
 * Report the peer if we didn't receive a packet after this time (milliseconds).
 */
inline constexpr u32 NETWORK_WARNING_TIMEOUT = 2000;

class CNetClient;
class CNetServerWorker;

class CNetStatsTable;

typedef struct _ENetHost ENetHost;

/**
 * @file
 * Network client/server sessions.
 *
 * Each session has two classes: CNetClientSession runs on the client,
 * and CNetServerSession runs on the server.
 * A client runs one session at once; a server typically runs many.
 */

/**
 * Interface for sessions to which messages can be sent.
 */
class INetSession
{
public:
	virtual ~INetSession() {}
	virtual bool SendMessage(const CNetMessage* message) = 0;
};

/**
 * The client end of a network session.
 * Provides an abstraction of the network interface, allowing communication with the server.
 * The NetClientSession is threaded, so all calls to the public interface must be thread-safe.
 */
class CNetClientSession : public INetSession
{
	NONCOPYABLE(CNetClientSession);

public:
	CNetClientSession(CNetClient& client);
	~CNetClientSession();

	bool Connect(const CStr& server, const u16 port, ENetHost* enetClient);

	/**
	 * The client NetSession is threaded to avoid getting timeouts if the main thread hangs.
	 * Call Connect() before starting this loop.
	 */
	static void RunNetLoop(CNetClientSession* session);

	/**
	 * Shut down the net session.
	 */
	void Shutdown();

	/**
	 * Processes pending messages.
	 */
	void ProcessPolledMessages();

	/**
	 * Queue up a message to send to the server on the next Loop() call.
	 */
	virtual bool SendMessage(const CNetMessage* message) override;

	/**
	 * Number of milliseconds since the most recent packet of the server was received.
	 */
	u32 GetLastReceivedTime() const;

	/**
	 * Average round trip time to the server.
	 */
	u32 GetMeanRTT() const;

	CNetFileTransferer& GetFileTransferer() { return m_FileTransferer; }
private:
	/**
	 * Process queued incoming messages.
	 */
	void Poll();

	/**
	 * Flush queued outgoing network messages.
	 */
	void Flush();

	CNetClient& m_Client;

	CNetFileTransferer m_FileTransferer;

	// Net messages received and waiting for fetching.
	boost::lockfree::queue<ENetEvent> m_IncomingMessages;
	// Net messages to send on the next flush() call.
	boost::lockfree::queue<ENetPacket*> m_OutgoingMessages;

	// Last know state. If false, flushing errors are silenced.
	bool m_Connected = false;

	// Wrapper around enet stats - those are atomic as the code is lock-free.
	std::atomic<u32> m_LastReceivedTime;
	std::atomic<u32> m_MeanRTT;

	// If this is true, calling Connect() or deleting the session is an error.
	std::atomic<bool> m_LoopRunning;
	std::atomic<bool> m_ShouldShutdown;

	ENetHost* m_Host;
	ENetPeer* m_Server;
	CNetStatsTable* m_Stats;
};


/**
 * The server's end of a network session.
 * Represents an abstraction of the state of the client, storing all the per-client data
 * needed by the server.
 *
 * Thread-safety:
 * - This is constructed and used by CNetServerWorker in the network server thread.
 */
class CNetServerSession : public CFsm, public INetSession
{
	NONCOPYABLE(CNetServerSession);

public:
	CNetServerSession(CNetServerWorker& server, ENetPeer* peer);

	CNetServerWorker& GetServer() { return m_Server; }

	const CStr& GetGUID() const { return m_GUID; }
	void SetGUID(const CStr& guid) { m_GUID = guid; }

	const CStrW& GetUserName() const { return m_UserName; }
	void SetUserName(const CStrW& name) { m_UserName = name; }

	u32 GetHostID() const { return m_HostID; }
	void SetHostID(u32 id) { m_HostID = id; }

	u32 GetIPAddress() const;

	/**
	 * Number of milliseconds since the latest packet of that client was received.
	 */
	u32 GetLastReceivedTime() const;

	/**
	 * Average round trip time to the client.
	 */
	u32 GetMeanRTT() const;

	/**
	 * Sends a disconnection notification to the client,
	 * and sends a NMT_CONNECTION_LOST message to the session FSM.
	 * The server will receive a disconnection notification after a while.
	 * The server will not receive any further messages sent via this session.
	 */
	void Disconnect(NetDisconnectReason reason);

	/**
	 * Sends an unreliable disconnection notification to the client.
	 * The server will not receive any disconnection notification.
	 * The server will not receive any further messages sent via this session.
	 */
	void DisconnectNow(NetDisconnectReason reason);

	/**
	 * Send a message to the client.
	 */
	virtual bool SendMessage(const CNetMessage* message);

	CNetFileTransferer& GetFileTransferer() { return m_FileTransferer; }

private:
	CNetServerWorker& m_Server;

	CNetFileTransferer m_FileTransferer;

	ENetPeer* m_Peer;

	CStr m_GUID;
	CStrW m_UserName;
	u32 m_HostID;
	CStr m_Password;
};

#endif	// NETSESSION_H
