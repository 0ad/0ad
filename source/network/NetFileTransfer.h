/* Copyright (C) 2019 Wildfire Games.
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

#ifndef NETFILETRANSFER_H
#define NETFILETRANSFER_H

#include <map>

class CNetMessage;
class CFileTransferResponseMessage;
class CFileTransferDataMessage;
class CFileTransferAckMessage;
class CNetClientSession;
class CNetServerSession;
class INetSession;

// Assume this is sufficiently less than MTU that packets won't get
// fragmented or dropped.
static const size_t DEFAULT_FILE_TRANSFER_PACKET_SIZE = 1024;

// To improve performance without flooding ENet's internal buffers,
// maintain a small number of in-flight packets.
// Pick numbers so that with e.g. 200ms round-trip latency
// we can hopefully get windowSize*packetSize*1000/200 = 160KB/s bandwidth
static const size_t DEFAULT_FILE_TRANSFER_WINDOW_SIZE = 32;

// Some arbitrary limit to make it slightly harder to use up all of someone's RAM
static const size_t MAX_FILE_TRANSFER_SIZE = 8*MiB;

/**
 * Asynchronous file-receiving task.
 * Other code should subclass this, implement OnComplete(),
 * then pass it to CNetFileTransferer::StartTask.
 */
class CNetFileReceiveTask
{
public:
	CNetFileReceiveTask() : m_RequestID(0), m_Length(0) { }
	virtual ~CNetFileReceiveTask() {}

	/**
	 * Called when m_Buffer contains the full received data.
	 */
	virtual void OnComplete() = 0;

	// TODO: Ought to have an OnFailure, e.g. when the session drops or there's another error

	/**
	 * Uniquely identifies the request within the scope of its CNetFileTransferer.
	 * Set automatically by StartTask.
	 */
	u32 m_RequestID;

	size_t m_Length;

	std::string m_Buffer;
};

/**
 * Handles transferring files between clients and servers.
 */
class CNetFileTransferer
{
public:
	CNetFileTransferer(INetSession* session)
		: m_Session(session), m_NextRequestID(1), m_LastProgressReportTime(0)
	{
	}

	/**
	 * Should be called when a message is received from the network.
	 * Returns INFO::SKIPPED if the message is not one that this class handles.
	 * Returns INFO::OK if the message is handled successfully,
	 * or ERR::FAIL if handled unsuccessfully.
	 */
	Status HandleMessageReceive(const CNetMessage& message);

	/**
	 * Registers a file-receiving task.
	 */
	void StartTask(const shared_ptr<CNetFileReceiveTask>& task);

	/**
	 * Registers data to be sent in response to a request.
	 * (Callers are expected to have their own mechanism for receiving
	 * requests and deciding what to respond with.)
	 */
	void StartResponse(u32 requestID, const std::string& data);

	/**
	 * Call frequently (e.g. once per frame) to trigger any necessary
	 * packet processing.
	 */
	void Poll();

private:
	Status OnFileTransferResponse(const CFileTransferResponseMessage& message);
	Status OnFileTransferData(const CFileTransferDataMessage& message);
	Status OnFileTransferAck(const CFileTransferAckMessage& message);

	/**
	 * Asynchronous file-sending task.
	 */
	struct CNetFileSendTask
	{
		u32 requestID;
		std::string buffer;
		size_t offset;
		size_t maxWindowSize;
		size_t packetsInFlight;
	};

	INetSession* m_Session;

	u32 m_NextRequestID;

	using FileReceiveTasksMap = std::map<u32, shared_ptr<CNetFileReceiveTask> >;
	FileReceiveTasksMap m_FileReceiveTasks;

	using FileSendTasksMap = std::map<u32, CNetFileSendTask>;
	FileSendTasksMap m_FileSendTasks;

	double m_LastProgressReportTime;
};

#endif // NETFILETRANSFER_H
