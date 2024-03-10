/* Copyright (C) 2024 Wildfire Games.
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

#include "lib/self_test.h"

#include "network/NetFileTransfer.h"
#include "network/NetMessage.h"
#include "network/NetSession.h"

#include <utility>
#include <vector>

namespace
{
constexpr const char* MESSAGECONTENT{"Some example message content"};

class MessageQueues : public INetSession
{
public:
	~MessageQueues() final = default;
	bool SendMessage(const CNetMessage* message) final
	{
		switch (message->GetType())
		{
		case NMT_FILE_TRANSFER_REQUEST:
			requests.push_back(*static_cast<const CFileTransferRequestMessage*>(message));
			break;
		case NMT_FILE_TRANSFER_RESPONSE:
			responses.push_back(*static_cast<const CFileTransferResponseMessage*>(message));
			break;
		case NMT_FILE_TRANSFER_DATA:
			data.push_back(*static_cast<const CFileTransferDataMessage*>(message));
			break;
		case NMT_FILE_TRANSFER_ACK:
			acknowledgements.push_back(*static_cast<const CFileTransferAckMessage*>(message));
			break;
		default:
			TS_FAIL("Unhandeled message type");
		}

		return true;
	}

	std::vector<CFileTransferRequestMessage> requests;
	std::vector<CFileTransferResponseMessage> responses;
	std::vector<CFileTransferDataMessage> data;
	std::vector<CFileTransferAckMessage> acknowledgements;
};

void CheckSizes(MessageQueues& queues, size_t requestSize, size_t responseSize, size_t dataSize,
	size_t acknowledgementSize)
{
	TS_ASSERT_EQUALS(queues.requests.size(), requestSize);
	TS_ASSERT_EQUALS(queues.responses.size(), responseSize);
	TS_ASSERT_EQUALS(queues.data.size(), dataSize);
	TS_ASSERT_EQUALS(queues.acknowledgements.size(), acknowledgementSize);
}

struct Participant
{
	MessageQueues queues;
	CNetFileTransferer transferer{&queues};
};
}

class TestFileTransfer : public CxxTest::TestSuite
{
public:
	void test_transfer()
	{
		// The client requests some data from the server.

		Participant server;
		Participant client;

		bool complete{false};

		client.transferer.StartTask([&complete](std::string buffer)
			{
				// This callback is executed exactly once.
				const bool previousComplete{std::exchange(complete, true)};
				TS_ASSERT(!previousComplete);
				TS_ASSERT_STR_EQUALS(buffer, MESSAGECONTENT);
			});
		CheckSizes(client.queues, 1, 0, 0, 0);

		server.transferer.StartResponse(client.queues.requests.at(0).m_RequestID, MESSAGECONTENT);
		CheckSizes(server.queues, 0, 1, 0, 0);

		client.transferer.HandleMessageReceive(server.queues.responses.at(0));
		CheckSizes(client.queues, 1, 0, 0, 0);

		server.transferer.Poll();
		CheckSizes(server.queues, 0, 1, 1, 0);

		server.transferer.Poll();
		// If `MESSAGECONTENT` would be longer another message would be sent.
		CheckSizes(server.queues, 0, 1, 1, 0);

		TS_ASSERT(!complete);

		client.transferer.HandleMessageReceive(server.queues.data.at(0));
		CheckSizes(client.queues, 1, 0, 0, 1);

		TS_ASSERT(complete);

		server.transferer.HandleMessageReceive(client.queues.acknowledgements.at(0));
		CheckSizes(server.queues, 0, 1, 1, 0);
	}
};
