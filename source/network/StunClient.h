/* Copyright (C) 2017 Wildfire Games.
 * Copyright (C) 2013-2016 SuperTuxKart-Team.
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

#ifndef STUNCLIENT_H
#define STUNCLIENT_H

#include "scriptinterface/ScriptInterface.h"

typedef struct _ENetHost ENetHost;

namespace StunClient
{

struct StunEndpoint {
	u32 ip;
	u16 port;
};

void SendStunRequest(ENetHost* transactionHost, u32 targetIp, u16 targetPort);

JS::Value FindStunEndpointHost(const ScriptInterface& scriptInterface, int port);

StunEndpoint* FindStunEndpointJoin(ENetHost* transactionHost);

void SendHolePunchingMessages(ENetHost* enetClient, const char* serverAddress, u16 serverPort);

}

#endif // STUNCLIENT_H
