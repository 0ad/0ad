#ifndef INCLUDED_GAMERECORD
#define INCLUDED_GAMERECORD

#include "CStr.h"

class CNetMessage;
class CTurnManager;

class CGameRecord
{
	bool m_IsRecording;
	CTurnManager *m_pTurnManager;

public:
	void Load(const CStr& filename);
	void Record(const CStr& filename);

	bool IsRecording();

	/*
		NOTE: The message will not be deleted by this method. Ownership remains
		the caller's.
	*/
	void WriteMessage(CNetMessage *pMsg);

	CTurnManager *GetPlaybackTurnManager();
};

#endif
