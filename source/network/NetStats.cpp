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

#include "NetStats.h"

#include "lib/external_libraries/enet.h"

enum
{
	Row_InData,
	Row_OutData,
	Row_LastSendTime,
	Row_LastRecvTime,
	Row_NextTimeout,
	Row_PacketsSent,
	Row_PacketsLost,
	Row_LastRTT,
	Row_RTT,
	Row_MTU,
	Row_ReliableInTransit,
	NumberRows
};

CNetStatsTable::CNetStatsTable(const ENetPeer* peer)
	: m_Host(NULL), m_Peer(peer)
{
}

CNetStatsTable::CNetStatsTable(const ENetHost* host)
	: m_Host(host), m_Peer(NULL)
{
}

CStr CNetStatsTable::GetName()
{
	return "net";
}

CStr CNetStatsTable::GetTitle()
{
	if (m_Host)
		return "Network host statistics";
	else
		return "Network client statistics";
}

size_t CNetStatsTable::GetNumberRows()
{
	return NumberRows;
}

const std::vector<ProfileColumn>& CNetStatsTable::GetColumns()
{
	m_ColumnDescriptions.clear();
	m_ColumnDescriptions.push_back(ProfileColumn("Name", 200));
	if (m_Host)
	{
		for (size_t i = 0; i < m_Host->peerCount; ++i)
			m_ColumnDescriptions.push_back(ProfileColumn("Peer "+CStr(i), 80));
	}
	else if (m_Peer)
	{
		m_ColumnDescriptions.push_back(ProfileColumn("Value", 80));
	}
	return m_ColumnDescriptions;
}

CStr CNetStatsTable::GetCellText(size_t row, size_t col)
{
#define ROW(id, title, member) \
	case id: \
		if (col == 0) return title; \
		if (m_Peer) return CStr(m_Peer->member); \
		if (m_Host && col <= m_Host->peerCount) return CStr(m_Host->peers[col-1].member); \
		return "???"

	switch(row)
	{
	ROW(Row_InData, "incoming bytes", incomingDataTotal);
	ROW(Row_OutData, "outgoing bytes", outgoingDataTotal);
	ROW(Row_LastSendTime, "last send time", lastSendTime);
	ROW(Row_LastRecvTime, "last receive time", lastReceiveTime);
	ROW(Row_NextTimeout, "next timeout", nextTimeout);
	ROW(Row_PacketsSent, "packets sent", packetsSent);
	ROW(Row_PacketsLost, "packets lost", packetsLost);
	ROW(Row_LastRTT, "last RTT", lastRoundTripTime);
	ROW(Row_RTT, "mean RTT", roundTripTime);
	ROW(Row_MTU, "MTU", mtu);
	ROW(Row_ReliableInTransit, "reliable data in transit", reliableDataInTransit);

	default:
		return "???";
	}
}

AbstractProfileTable* CNetStatsTable::GetChild(size_t UNUSED(row))
{
	return 0;
}
