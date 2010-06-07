/* Copyright (C) 2010 Wildfire Games.
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
#include "NetSession.h"

static const uint INVALID_SESSION = 0;

//-----------------------------------------------------------------------------
// Name: CNetSession()
// Desc: Constructor
//-----------------------------------------------------------------------------
CNetSession::CNetSession(CNetHost* pHost, ENetPeer* pPeer)
{
	m_Host = pHost;
	m_Peer = pPeer;
	m_ID = INVALID_SESSION;
	m_PlayerSlot = NULL;
}

//-----------------------------------------------------------------------------
// Name: ~CNetSession()
// Desc: Destructor
//-----------------------------------------------------------------------------
CNetSession::~CNetSession()
{
	m_Peer = NULL;
}

//-----------------------------------------------------------------------------
// Name: SetName()
// Desc: Set a new name for the session
//-----------------------------------------------------------------------------
void CNetSession::SetName(const CStr& name)
{
	m_Name = name;
}

//-----------------------------------------------------------------------------
// Name: SetID()
// Desc: Set new ID for this session
//-----------------------------------------------------------------------------
void CNetSession::SetID(uint ID)
{
	m_ID = ID;
}

//-----------------------------------------------------------------------------
// Name: SetPlayerSlot()
// Desc: Set the player slot for this session
//-----------------------------------------------------------------------------
void CNetSession::SetPlayerSlot(CPlayerSlot* pPlayerSlot)
{
	m_PlayerSlot = pPlayerSlot;
}

//-----------------------------------------------------------------------------
// Name: ScriptingInit()
// Desc:
//-----------------------------------------------------------------------------
void CNetSession::ScriptingInit()
{
	AddProperty(L"id", &CNetSession::m_ID);
	AddProperty(L"name", &CNetSession::m_Name);

	CJSObject<CNetSession>::ScriptingInit("NetSession");
}
