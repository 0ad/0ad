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

#include "precompiled.h"

#include "NetFileTransfer.h"

#include "lib/timer.h"
#include "network/NetMessage.h"
#include "network/NetSession.h"
#include "ps/CLogger.h"

Status CNetFileTransferer::HandleMessageReceive(const CNetMessage& message)
{
	switch (message.GetType())
	{
	case NMT_FILE_TRANSFER_RESPONSE:
		return OnFileTransferResponse(static_cast<const CFileTransferResponseMessage&>(message));

	case NMT_FILE_TRANSFER_DATA:
		return OnFileTransferData(static_cast<const CFileTransferDataMessage&>(message));

	case NMT_FILE_TRANSFER_ACK:
		return OnFileTransferAck(static_cast<const CFileTransferAckMessage&>(message));

	default:
		return INFO::SKIPPED;
	}
}

Status CNetFileTransferer::OnFileTransferResponse(const CFileTransferResponseMessage& message)
{
	const FileReceiveTasksMap::iterator it = m_FileReceiveTasks.find(message.m_RequestID);
	if (it == m_FileReceiveTasks.end())
	{
		LOGERROR("Net transfer: Unsolicited file transfer response (id=%lu)", message.m_RequestID);
		return ERR::FAIL;
	}

	if (message.m_Length == 0 || message.m_Length > MAX_FILE_TRANSFER_SIZE)
	{
		LOGERROR("Net transfer: Invalid size for file transfer response (length=%lu)", message.m_Length);
		return ERR::FAIL;
	}

	CNetFileReceiveTask& task = *it->second;

	task.m_Length = message.m_Length;
	task.m_Buffer.reserve(message.m_Length);

	LOGMESSAGERENDER("Downloading data over network (%lu KB) - please wait...", task.m_Length / 1024);
	m_LastProgressReportTime = timer_Time();

	return INFO::OK;
}

Status CNetFileTransferer::OnFileTransferData(const CFileTransferDataMessage& message)
{
	FileReceiveTasksMap::iterator it = m_FileReceiveTasks.find(message.m_RequestID);
	if (it == m_FileReceiveTasks.end())
	{
		LOGERROR("Net transfer: Unsolicited file transfer data (id=%lu)", message.m_RequestID);
		return ERR::FAIL;
	}

	CNetFileReceiveTask& task = *it->second;

	task.m_Buffer += message.m_Data;

	if (task.m_Buffer.size() > task.m_Length)
	{
		LOGERROR("Net transfer: Invalid size for file transfer data (length=%lu actual=%zu)", task.m_Length, task.m_Buffer.size());
		return ERR::FAIL;
	}

	CFileTransferAckMessage ackMessage;
	ackMessage.m_RequestID = task.m_RequestID;
	ackMessage.m_NumPackets = 1; // TODO: would be nice to send a single ack for multiple packets at once
	m_Session->SendMessage(&ackMessage);

	if (task.m_Buffer.size() == task.m_Length)
	{
		LOGMESSAGERENDER("Download completed");

		task.OnComplete();
		m_FileReceiveTasks.erase(message.m_RequestID);
		return INFO::OK;
	}

	// TODO: should report progress using proper GUI

	// Report the download status occassionally
	double t = timer_Time();
	if (t > m_LastProgressReportTime + 0.5)
	{
		LOGMESSAGERENDER("Downloading data: %.1f%% of %lu KB", 100.f * task.m_Buffer.size() / task.m_Length, task.m_Length / 1024);
		m_LastProgressReportTime = t;
	}

	return INFO::OK;
}

Status CNetFileTransferer::OnFileTransferAck(const CFileTransferAckMessage& message)
{
	FileSendTasksMap::iterator it = m_FileSendTasks.find(message.m_RequestID);
	if (it == m_FileSendTasks.end())
	{
		LOGERROR("Net transfer: Unsolicited file transfer ack (id=%lu)", message.m_RequestID);
		return ERR::FAIL;
	}

	CNetFileSendTask& task = it->second;

	if (message.m_NumPackets > task.packetsInFlight)
	{
		LOGERROR("Net transfer: Invalid num packets for file transfer ack (num=%lu inflight=%lu)",
			message.m_NumPackets, task.packetsInFlight);
		return ERR::FAIL;
	}

	task.packetsInFlight -= message.m_NumPackets;

	return INFO::OK;

}

void CNetFileTransferer::StartTask(const shared_ptr<CNetFileReceiveTask>& task)
{
	u32 requestID = m_NextRequestID++;

	task->m_RequestID = requestID;
	m_FileReceiveTasks[requestID] = task;

	CFileTransferRequestMessage request;
	request.m_RequestID = requestID;
	m_Session->SendMessage(&request);
}

void CNetFileTransferer::StartResponse(u32 requestID, const std::string& data)
{
	CNetFileSendTask task;
	task.requestID = requestID;
	task.buffer = data;
	task.offset = 0;
	task.packetsInFlight = 0;
	task.maxWindowSize = DEFAULT_FILE_TRANSFER_WINDOW_SIZE;

	m_FileSendTasks[task.requestID] = task;
	CFileTransferResponseMessage respMessage;
	respMessage.m_RequestID = requestID;
	respMessage.m_Length = task.buffer.size();
	m_Session->SendMessage(&respMessage);
}

void CNetFileTransferer::Poll()
{
	// Find tasks which have fewer packets in flight than their window size,
	// and send more packets
	for (std::pair<const u32, CNetFileSendTask>& p : m_FileSendTasks)
	{
		CNetFileSendTask& task = p.second;

		while (task.packetsInFlight < task.maxWindowSize && task.offset < task.buffer.size())
		{
			CFileTransferDataMessage dataMessage;
			dataMessage.m_RequestID = task.requestID;
			ssize_t packetSize = std::min(DEFAULT_FILE_TRANSFER_PACKET_SIZE, task.buffer.size() - task.offset);
			dataMessage.m_Data = task.buffer.substr(task.offset, packetSize);
			task.offset += packetSize;
			++task.packetsInFlight;
			m_Session->SendMessage(&dataMessage);
		}
	}

	// TODO: need to garbage-collect finished tasks
}
