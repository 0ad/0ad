#ifndef _ps_GameRecord_H
#define _ps_GameRecord_H

#include "CStr.h"

class CNetMessage;
class CTurnManager;

class CGameRecord
{
	bool m_IsRecording;
	CTurnManager *m_pTurnManager;

public:
	void Load(CStr filename);
	void Record(CStr filename);

	bool IsRecording();

	void WriteMessage(CNetMessage *pMsg);

	CTurnManager *GetPlaybackTurnManager();
};

#endif
