/* Copyright (C) 2022 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "lib/self_test.h"

#include "network/StunClient.h"

#include "lib/external_libraries/enet.h"
#include "ps/ConfigDB.h"
#include "ps/CStr.h"

class TestStunClient : public CxxTest::TestSuite
{
public:
	void setUp()
	{
		// Sets networking up in a cross-platform manner.
		enet_initialize();
	}

	void tearDown()
	{
		enet_deinitialize();
	}

	void test_local_ip()
	{
		CStr ip;
		TS_ASSERT(StunClient::FindLocalIP(ip));
		// Quick validation that this looks like a valid IP address.
		if (ip.size() < 7 || ip.size() > 15)
		{
			TS_FAIL("StunClient::FindLocalIP did not return a valid IPV4 address: wrong size");
			return;
		}
		int dots = 0;
		for (char c : ip)
		{
			if (c == '.')
				++dots;
			else if (c < '0' && c > '9')
			{
				TS_FAIL("StunClient::FindLocalIP did not return a valid IPV4 address: wrong character");
				return;
			}
		}
		if (dots != 3)
			TS_FAIL("StunClient::FindLocalIP did not return a valid IPV4 address: wrong separators");
	}

	void test_stun_DISABLED()
	{
		// Disabled test -> should return your external IP by connecting to our STUN server.
		CConfigDB::Initialise();
		CStr ip;
		u16 port;
		g_ConfigDB.SetValueString(CFG_COMMAND, "lobby.stun.server", "lobby.wildfiregames.com");
		g_ConfigDB.SetValueString(CFG_COMMAND, "lobby.stun.port", "3478");
		ENetAddress addr { ENET_HOST_ANY, ENET_PORT_ANY };
		ENetHost* host = enet_host_create(&addr, 1, 1, 0, 0);
		StunClient::FindPublicIP(*host, ip, port);
		LOGWARNING("%s %i", ip.c_str(), port);
		CConfigDB::Shutdown();
	}
};
