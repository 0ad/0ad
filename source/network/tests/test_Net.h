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

#include "lib/external_libraries/enet.h"
#include "lib/external_libraries/sdl.h"
#include "network/NetServer.h"
#include "network/NetClient.h"
#include "network/NetTurnManager.h"
#include "ps/CLogger.h"
#include "ps/Game.h"
#include "ps/Filesystem.h"
#include "ps/Loader.h"
#include "ps/XML/Xeromyces.h"
#include "scriptinterface/ScriptInterface.h"
#include "simulation2/Simulation2.h"

class TestNetComms : public CxxTest::TestSuite
{
public:
	void setUp()
	{
		g_VFS = CreateVfs(20 * MiB);
		TS_ASSERT_OK(g_VFS->Mount(L"", DataDir()/L"mods/public", VFS_MOUNT_MUST_EXIST));
		TS_ASSERT_OK(g_VFS->Mount(L"cache", DataDir()/L"_testcache"));
		CXeromyces::Startup();

		enet_initialize();
	}

	void tearDown()
	{
		enet_deinitialize();

		CXeromyces::Terminate();
		g_VFS.reset();
		DeleteDirectory(DataDir()/L"_testcache");
	}

	bool clients_are_all(const std::vector<CNetClient*>& clients, uint state)
	{
		for (size_t j = 0; j < clients.size(); ++j)
			if (clients[j]->GetCurrState() != state)
				return false;
		return true;
	}

	void connect(CNetServer& server, const std::vector<CNetClient*>& clients)
	{
		TS_ASSERT(server.SetupConnection());
		for (size_t j = 0; j < clients.size(); ++j)
			TS_ASSERT(clients[j]->SetupConnection("127.0.0.1"));

		for (size_t i = 0; ; ++i)
		{
//			debug_printf(L".");
			server.Poll();
			for (size_t j = 0; j < clients.size(); ++j)
				clients[j]->Poll();

			if (server.GetState() == SERVER_STATE_PREGAME && clients_are_all(clients, NCS_PREGAME))
				break;

			if (i > 20)
			{
				TS_FAIL("connection timeout");
				break;
			}

			SDL_Delay(100);
		}
	}

	void disconnect(CNetServer& server, const std::vector<CNetClient*>& clients)
	{
		for (size_t i = 0; ; ++i)
		{
//			debug_printf(L".");
			server.Poll();
			for (size_t j = 0; j < clients.size(); ++j)
				clients[j]->Poll();

			if (server.GetState() == SERVER_STATE_UNCONNECTED && clients_are_all(clients, NCS_UNCONNECTED))
				break;

			if (i > 20)
			{
				TS_FAIL("disconnection timeout");
				break;
			}

			SDL_Delay(100);
		}
	}

	void wait(CNetServer& server, const std::vector<CNetClient*>& clients, size_t msecs)
	{
		for (size_t i = 0; i < msecs/10; ++i)
		{
			server.Poll();
			for (size_t j = 0; j < clients.size(); ++j)
				clients[j]->Poll();

			SDL_Delay(10);
		}
	}

	void test_basic_DISABLED()
	{
		// This doesn't actually test much, it just runs a very quick multiplayer game
		// and prints a load of debug output so you can see if anything funny's going on

		TestStdoutLogger logger;

		std::vector<CNetClient*> clients;

		CGame client1Game(true);
		CGame client2Game(true);
		CGame client3Game(true);

		CNetServer server;

		CScriptValRooted attrs;
		server.GetScriptInterface().Eval("({map:'_default',thing:'example'})", attrs);
		server.UpdateGameAttributes(attrs);

		CNetClient client1(&client1Game);
		CNetClient client2(&client2Game);
		CNetClient client3(&client3Game);

		clients.push_back(&client1);
		clients.push_back(&client2);
		clients.push_back(&client3);

		connect(server, clients);
		debug_printf(L"%ls", client1.TestReadGuiMessages().c_str());

		server.StartGame();
		server.Poll();
		SDL_Delay(100);
		for (size_t j = 0; j < clients.size(); ++j)
		{
			clients[j]->Poll();
			TS_ASSERT_OK(LDR_NonprogressiveLoad());
			clients[j]->LoadFinished();
		}

		wait(server, clients, 100);

		{
			CScriptValRooted cmd;
			client1.GetScriptInterface().Eval("({type:'debug-print', message:'[>>> client1 test sim command]\\n'})", cmd);
			client1Game.GetTurnManager()->PostCommand(cmd);
		}

		{
			CScriptValRooted cmd;
			client2.GetScriptInterface().Eval("({type:'debug-print', message:'[>>> client2 test sim command]\\n'})", cmd);
			client2Game.GetTurnManager()->PostCommand(cmd);
		}

		wait(server, clients, 100);
		client1Game.GetTurnManager()->Update(1.0f);
		client2Game.GetTurnManager()->Update(1.0f);
		client3Game.GetTurnManager()->Update(1.0f);
		wait(server, clients, 100);
		client1Game.GetTurnManager()->Update(1.0f);
		client2Game.GetTurnManager()->Update(1.0f);
		client3Game.GetTurnManager()->Update(1.0f);
		wait(server, clients, 100);
	}
};
