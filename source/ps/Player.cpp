#include "precompiled.h"

#include "Player.h"
#include "Network/NetMessage.h"

CPlayer::CPlayer(uint playerID):
	m_PlayerID(playerID)
{}

bool CPlayer::ValidateCommand(CNetMessage *pMsg)
{
	return true;
}
