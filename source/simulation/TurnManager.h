#ifndef _simulation_TurnManager_H
#define _simulation_TurnManager_H

//#include "Network/NetMessage.h"
//#include "Network/Network.h"
#include <vector>

#include "lib/types.h"

class CGameRecord;
class CNetMessage;
class IMessagePipeEnd;

class CTurnManager
{
public:
	// Used with IterateBatch() to iterate a command batch and set the sync mask
	// for each message. If the iterating function doesn't wish to change the
	// mask, it should return oldMask unchanged.
	typedef uint (BatchIteratorFunc)(CNetMessage *pMsg, uint oldMask, void *userdata);

	// FIXME Should be in CNetServer instead
	struct SClientTimingData
	{
		// The maximum latency observed from this client
		uint m_MaxLatency;
		// Approximate current round-trip time in milliseconds
		uint m_Latency;
		// Frames per Second - won't be used unless 1000/m_FPS > m_Latency (i.e.
		// framerate is too slow to handle [m_Latency] as the turn length)
		uint m_FPS;
	};

private:
	struct SMessageSyncEntry
	{
		// A bitmask telling which clients to sync this message to
		uint m_ClientMask;
		// The message pointer
		CNetMessage *m_pMessage;

		inline SMessageSyncEntry(CNetMessage *pMsg):
			m_ClientMask(0),
			m_pMessage(pMsg)
		{}
	};

	struct SBatch
	{
		std::vector <SMessageSyncEntry> m_Messages;
		uint m_TurnLength;

		void Swap(SBatch &other);
	};

	struct SClient
	{
		// FIXME Move to CNetServer
		SClientTimingData m_TimingData;
		IMessagePipeEnd *m_Pipe;
	};

	std::vector <SClient> m_Clients;
	SBatch m_Batches[3];

	CGameRecord *m_pRecord;

protected:	
	// Rotate the three batches: {0, 1, 2} => {1, 2, 0}
	void RotateBatches();

	// Go through each message in the specified batch and send the message to
	// each of the clients whose bit in the message's mask is set
	void SendBatch(uint batch);
	void ClearBatch(uint batch);

	void SetClientPipe(uint client, IMessagePipeEnd *pipe);
	// FIXME Should be in CNetServer instead [and implemented]
	// void UpdateTimingData(uint client, uint fps, uint currentLatency);
	void SetTurnLength(uint batch, uint turnLength);

	void SendMessage(CNetMessage *pMsg, uint clientMask);

	// Add the message to the specified batch. The message is assumed to be
	// validated before passed here, and will be blindly trusted.
	void QueueMessage(uint batch, CNetMessage *pMsg);

	// Send the specified batch to CGameRecord for recording
	void RecordBatch(uint batch);
	static BatchIteratorFunc RecordIterator;

public:
	CTurnManager();

	void Initialize(size_t numClients);

	// Return the millisecond delay between the last frame and the next.
	// CSimulation will use this to determine when to perform the deterministic
	// update and call NewTurn()
	uint GetTurnLength();

	// Called by CSimulation when the current turn time has passed.
	virtual void NewTurn()=0;

	void IterateBatch(uint batch, BatchIteratorFunc *func, void *userdata);

	// Queue a command originating from the local player.
	virtual void QueueLocalCommand(CNetMessage *pMsg)=0;
};

class CSinglePlayerTurnManager: public CTurnManager
{
public:
	CSinglePlayerTurnManager();

	virtual void NewTurn();
	virtual void QueueLocalCommand(CNetMessage *pMsg);
};

extern CSinglePlayerTurnManager *g_SinglePlayerTurnManager;

#endif
