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

#include "lib/self_test.h"

#include "lib/external_libraries/sdl.h"
#include "network/NetServer.h"
#include "network/NetClient.h"
#include "ps/ConfigDB.h"
#include "ps/Filesystem.h"
#include "ps/GameAttributes.h"
#include "ps/XML/Xeromyces.h"
#include "scriptinterface/ScriptInterface.h"

class TestNetServer : public CNetServer
{
public:
	TestNetServer(ScriptInterface& scriptInterface, CGameAttributes& gameAttributes) :
		CNetServer(scriptInterface, NULL, &gameAttributes), m_HasConnection(false)
	{
	}

	bool m_HasConnection;

protected:
	virtual void OnPlayerJoin(CNetSession* pSession)
	{
		debug_printf(L"# player joined\n");

		for (size_t slot = 0; slot < m_GameAttributes->GetSlotCount(); ++slot)
		{
			if (m_GameAttributes->GetSlot(slot)->GetAssignment() == SLOT_OPEN)
			{
				debug_printf(L"# assigning slot %d\n", slot);
				m_GameAttributes->GetSlot(slot)->AssignToSession(pSession);
				m_HasConnection = true;
				break;
			}
		}
	}

	virtual void OnPlayerLeave(CNetSession* UNUSED(pSession))
	{
		debug_printf(L"# player left\n");
	}
};

class TestNetClient : public CNetClient
{
public:
	TestNetClient(ScriptInterface& scriptInterface, CGameAttributes& gameAttributes) :
		CNetClient(scriptInterface, NULL, &gameAttributes), m_HasConnection(false)
	{
	}

	bool m_HasConnection;

protected:
	virtual void OnConnectComplete()
	{
		debug_printf(L"# connect complete\n");
		m_HasConnection = true;
	}

	virtual void OnStartGame()
	{
		debug_printf(L"# start game\n");
	}
};

class TestNetComms : public CxxTest::TestSuite
{
public:
	void setUp()
	{
		g_VFS = CreateVfs(20 * MiB);
		TS_ASSERT_OK(g_VFS->Mount(L"", DataDir()/L"mods/public", VFS_MOUNT_MUST_EXIST));
		TS_ASSERT_OK(g_VFS->Mount(L"cache", DataDir()/L"_testcache"));
		CXeromyces::Startup();

		new ScriptingHost;
		CGameAttributes::ScriptingInit();
		CNetServer::ScriptingInit();
		CNetClient::ScriptingInit();

		new CConfigDB;
	}

	void tearDown()
	{
		delete &g_ConfigDB;

		CGameAttributes::ScriptingShutdown();
		CNetServer::ScriptingShutdown();
		CNetClient::ScriptingShutdown();
		delete &g_ScriptingHost;

		CXeromyces::Terminate();
		g_VFS.reset();
		DeleteDirectory(DataDir()/L"_testcache");
	}

	void connect(TestNetServer& server, TestNetClient& client)
	{
		TS_ASSERT(server.Start(NULL, 0, NULL));
		TS_ASSERT(client.Create());
		TS_ASSERT(client.ConnectAsync("127.0.0.1", DEFAULT_HOST_PORT));
		for (size_t i = 0; ; ++i)
		{
			debug_printf(L".");
			server.Poll();
			client.Poll();

			if (server.m_HasConnection && client.m_HasConnection)
				break;

			if (i > 20)
			{
				TS_FAIL("connection timeout");
				break;
			}

			SDL_Delay(100);
		}
	}

	void test_basic_DISABLED()
	{
		ScriptInterface scriptInterface("Engine");

		CGameAttributes gameAttributesServer;
		CGameAttributes gameAttributesClient;
		TestNetServer server(scriptInterface, gameAttributesServer);
		TestNetClient client(scriptInterface, gameAttributesClient);
		connect(server, client);
		client.CNetHost::Shutdown();
		server.CNetHost::Shutdown();
	}

	void TODO_test_destructor()
	{
		ScriptInterface scriptInterface("Engine");

		CGameAttributes gameAttributesServer;
		CGameAttributes gameAttributesClient;
		TestNetServer server(scriptInterface, gameAttributesServer);
		TestNetClient client(scriptInterface, gameAttributesClient);
		connect(server, client);
		// run in Valgrind; this shouldn't leak
	}
};
