#ifndef INCLUDED_TURNMANAGER
#define INCLUDED_TURNMANAGER

//#include "Network/NetMessage.h"
//#include "Network/Network.h"
#include <vector>

class CGameRecord;
class CNetMessage;
class IMessagePipeEnd;

class CTurnManager
{
public:
	// Default turn length
	static const int DEFAULT_TURN_LENGTH = 300;
	
	// Used with IterateBatch() to iterate a command batch and set the sync mask
	// for each message. If the iterating function doesn't wish to change the
	// mask, it should return oldMask unchanged.
	typedef uintptr_t (BatchIteratorFunc)(CNetMessage *pMsg, uintptr_t oldMask, void *userdata);

	// FIXME Should be in CNetServer instead
	struct SClientTimingData
	{
		// The maximum latency observed from this client
		int m_MaxLatency;
		// Approximate current round-trip time in milliseconds
		int m_Latency;
		// Frames per Second - won't be used unless 1000/m_FPS > m_Latency (i.e.
		// framerate is too slow to handle [m_Latency] as the turn length)
		int m_FPS;
	};

private:
	struct SMessageSyncEntry
	{
		// A bitmask telling which clients to sync this message to
		uintptr_t m_ClientMask;
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
		int m_TurnLength;

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
	void SendBatch(uintptr_t batch);
	void ClearBatch(uintptr_t batch);

	void SetClientPipe(size_t client, IMessagePipeEnd *pipe);
	// FIXME Should be in CNetServer instead [and implemented]
	// void UpdateTimingData(size_t client, int fps, int currentLatency);
	void SetTurnLength(uintptr_t batch, int turnLength);

	void SendMessage(CNetMessage *pMsg, uintptr_t clientMask);

	// Add the message to the specified batch. The message is assumed to be
	// validated before passed here, and will be blindly trusted.
	void QueueMessage(uintptr_t batch, CNetMessage *pMsg);

	// Send the specified batch to CGameRecord for recording
	void RecordBatch(uintptr_t batch);
	static BatchIteratorFunc RecordIterator;

public:
	CTurnManager();
	virtual ~CTurnManager() { }

	void Initialize(size_t numClients);

	// Return the millisecond delay between the last frame and the next.
	// CSimulation will use this to determine when to perform the deterministic
	// update and call NewTurn()
	int GetTurnLength();
	
	// Called by CSimulation when the current turn time has passed.
	virtual void NewTurn() = 0;
	
	// Used by CSimulation to ask whether it can call NewTurn.
	virtual bool NewTurnReady() = 0;

	// Apply a function to all messages in a given batch.
	void IterateBatch(uintptr_t batch, BatchIteratorFunc *func, void *userdata);

	// Queue a command originating from the local player.
	virtual void QueueLocalCommand(CNetMessage *pMsg) = 0;
};

class CSinglePlayerTurnManager: public CTurnManager
{
public:
	CSinglePlayerTurnManager();

	virtual void NewTurn();
	virtual void QueueLocalCommand(CNetMessage *pMsg);
	virtual bool NewTurnReady();
};

extern CSinglePlayerTurnManager *g_SinglePlayerTurnManager;

#endif
