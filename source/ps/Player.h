#ifndef _Player_H
#define _Player_H

#include "CStr.h"

class CNetMessage;

class CPlayer
{
	CStrW m_Name;
	uint m_PlayerID;

public:
	CPlayer(uint playerID);

	bool ValidateCommand(CNetMessage *pMsg);

	inline const CStrW &GetName() const
	{	return m_Name; }
};

#endif
