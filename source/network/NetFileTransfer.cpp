#include "precompiled.h"

#include "NetFileTransfer.h"

#include "lib/timer.h"
#include "network/NetMessage.h"
#include "network/NetSession.h"
#include "ps/CLogger.h"

Status CNetFileTransferer::HandleMessageReceive(const CNetMessage* message)
{
	if (message->GetType() == NMT_FILE_TRANSFER_RESPONSE)
	{
		CFileTransferResponseMessage* respMessage = (CFileTransferResponseMessage*)message;

		if (m_FileReceiveTasks.find(respMessage->m_RequestID) == m_FileReceiveTasks.end())
		{
			LOGERROR(L"Net transfer: Unsolicited file transfer response (id=%d)", (int)respMessage->m_RequestID);
			return ERR::FAIL;
		}

		if (respMessage->m_Length == 0 || respMessage->m_Length > MAX_FILE_TRANSFER_SIZE)
		{
			LOGERROR(L"Net transfer: Invalid size for file transfer response (length=%d)", (int)respMessage->m_Length);
			return ERR::FAIL;
		}

		shared_ptr<CNetFileReceiveTask> task = m_FileReceiveTasks[respMessage->m_RequestID];

		task->m_Length = respMessage->m_Length;
		task->m_Buffer.reserve(respMessage->m_Length);

		LOGMESSAGERENDER(L"Downloading data over network (%d KB) - please wait...", task->m_Length/1024);
		m_LastProgressReportTime = timer_Time();

		return INFO::OK;
	}
	else if (message->GetType() == NMT_FILE_TRANSFER_DATA)
	{
		CFileTransferDataMessage* dataMessage = (CFileTransferDataMessage*)message;

		if (m_FileReceiveTasks.find(dataMessage->m_RequestID) == m_FileReceiveTasks.end())
		{
			LOGERROR(L"Net transfer: Unsolicited file transfer data (id=%d)", (int)dataMessage->m_RequestID);
			return ERR::FAIL;
		}

		shared_ptr<CNetFileReceiveTask> task = m_FileReceiveTasks[dataMessage->m_RequestID];

		task->m_Buffer += dataMessage->m_Data;

		if (task->m_Buffer.size() > task->m_Length)
		{
			LOGERROR(L"Net transfer: Invalid size for file transfer data (length=%d actual=%d)", (int)task->m_Length, (int)task->m_Buffer.size());
			return ERR::FAIL;
		}

		CFileTransferAckMessage ackMessage;
		ackMessage.m_RequestID = task->m_RequestID;
		ackMessage.m_NumPackets = 1; // TODO: would be nice to send a single ack for multiple packets at once
		m_Session->SendMessage(&ackMessage);

		if (task->m_Buffer.size() == task->m_Length)
		{
			LOGMESSAGERENDER(L"Download completed");

			task->OnComplete();
			m_FileReceiveTasks.erase(dataMessage->m_RequestID);
			return INFO::OK;
		}

		// TODO: should report progress using proper GUI

		// Report the download status occassionally
		double t = timer_Time();
		if (t > m_LastProgressReportTime + 0.5)
		{
			LOGMESSAGERENDER(L"Downloading data: %.1f%% of %d KB", 100.f*task->m_Buffer.size()/task->m_Length, task->m_Length/1024);
			m_LastProgressReportTime = t;
		}

		return INFO::OK;
	}
	else if (message->GetType() == NMT_FILE_TRANSFER_ACK)
	{
		CFileTransferAckMessage* ackMessage = (CFileTransferAckMessage*)message;

		if (m_FileSendTasks.find(ackMessage->m_RequestID) == m_FileSendTasks.end())
		{
			LOGERROR(L"Net transfer: Unsolicited file transfer ack (id=%d)", (int)ackMessage->m_RequestID);
			return ERR::FAIL;
		}

		CNetFileSendTask& task = m_FileSendTasks[ackMessage->m_RequestID];

		if (ackMessage->m_NumPackets > task.packetsInFlight)
		{
			LOGERROR(L"Net transfer: Invalid num packets for file transfer ack (num=%d inflight=%d)",
				(int)ackMessage->m_NumPackets, (int)task.packetsInFlight);
			return ERR::FAIL;
		}

		task.packetsInFlight -= ackMessage->m_NumPackets;

		return INFO::OK;
	}

	return INFO::SKIPPED;
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
	for (FileSendTasksMap::iterator it = m_FileSendTasks.begin(); it != m_FileSendTasks.end(); ++it)
	{
		while (it->second.packetsInFlight < it->second.maxWindowSize && it->second.offset < it->second.buffer.size())
		{
			CFileTransferDataMessage dataMessage;
			dataMessage.m_RequestID = it->second.requestID;
			ssize_t packetSize = std::min(DEFAULT_FILE_TRANSFER_PACKET_SIZE, it->second.buffer.size() - it->second.offset);
			dataMessage.m_Data = it->second.buffer.substr(it->second.offset, packetSize);
			it->second.offset += packetSize;
			it->second.packetsInFlight++;
			m_Session->SendMessage(&dataMessage);
		}
	}

	// TODO: need to garbage-collect finished tasks
}