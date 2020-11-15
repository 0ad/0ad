/* Copyright (C) 2020 Wildfire Games.
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

// Pull in the headers from the default precompiled header,
// even if rlinterface doesn't use precompiled headers.
#include "lib/precompiled.h"

#include "rlinterface/RLInterface.h"

#include "gui/GUIManager.h"
#include "ps/Game.h"
#include "ps/GameSetup/GameSetup.h"
#include "ps/Loader.h"
#include "ps/CLogger.h"
#include "simulation2/components/ICmpAIInterface.h"
#include "simulation2/components/ICmpTemplateManager.h"
#include "simulation2/Simulation2.h"
#include "simulation2/system/LocalTurnManager.h"
#include "third_party/mongoose/mongoose.h"

#include <queue>
#include <tuple>
#include <sstream>

// Globally accessible pointer to the RL Interface.
RLInterface* g_RLInterface = nullptr;

// Interactions with the game engine (g_Game) must be done in the main
// thread as there are specific checks for this. We will pass our commands
// to the main thread to be applied
std::string RLInterface::SendGameMessage(const GameMessage msg)
{
	std::unique_lock<std::mutex> msgLock(m_msgLock);
	m_GameMessage = &msg;
	m_msgApplied.wait(msgLock);
	return m_GameState;
}

std::string RLInterface::Step(const std::vector<Command> commands)
{
	std::lock_guard<std::mutex> lock(m_lock);
	GameMessage msg = { GameMessageType::Commands, commands };
	return SendGameMessage(msg);
}

std::string RLInterface::Reset(const ScenarioConfig* scenario)
{
	std::lock_guard<std::mutex> lock(m_lock);
	m_ScenarioConfig = *scenario;
	struct GameMessage msg = { GameMessageType::Reset };
	return SendGameMessage(msg);
}

std::vector<std::string> RLInterface::GetTemplates(const std::vector<std::string> names) const
{
	std::lock_guard<std::mutex> lock(m_lock);
	CSimulation2& simulation = *g_Game->GetSimulation2();
	CmpPtr<ICmpTemplateManager> cmpTemplateManager(simulation.GetSimContext().GetSystemEntity());

	std::vector<std::string> templates;
	for (const std::string& templateName : names)
	{
		const CParamNode* node = cmpTemplateManager->GetTemplate(templateName);

		if (node != nullptr)
		{
			std::string content = utf8_from_wstring(node->ToXML());
			templates.push_back(content);
		}
	}

	return templates;
}

static void* RLMgCallback(mg_event event, struct mg_connection *conn, const struct mg_request_info *request_info)
{
	RLInterface* interface = (RLInterface*)request_info->user_data;
	ENSURE(interface);

	void* handled = (void*)""; // arbitrary non-NULL pointer to indicate successful handling

	const char* header200 =
		"HTTP/1.1 200 OK\r\n"
		"Access-Control-Allow-Origin: *\r\n"
		"Content-Type: text/plain; charset=utf-8\r\n\r\n";

	const char* header404 =
		"HTTP/1.1 404 Not Found\r\n"
		"Content-Type: text/plain; charset=utf-8\r\n\r\n"
		"Unrecognised URI";

	const char* noPostData =
		"HTTP/1.1 400 Bad Request\r\n"
		"Content-Type: text/plain; charset=utf-8\r\n\r\n"
		"No POST data found.";

	const char* notRunningResponse =
		"HTTP/1.1 400 Bad Request\r\n"
		"Content-Type: text/plain; charset=utf-8\r\n\r\n"
		"Game not running. Please create a scenario first.";

	switch (event)
	{
	case MG_NEW_REQUEST:
	{
		std::stringstream stream;

		std::string uri = request_info->uri;

		if (uri == "/reset")
		{
			const char* val = mg_get_header(conn, "Content-Length");
			if (!val)
			{
				mg_printf(conn, "%s", noPostData);
				return handled;
			}
			ScenarioConfig scenario;
			std::string qs(request_info->query_string);
			scenario.saveReplay = qs.find("saveReplay") != std::string::npos;

			scenario.playerID = 1;
			char playerID[1];
			int len = mg_get_var(request_info->query_string, qs.length(), "playerID", playerID, 1);
			if (len != -1)
				scenario.playerID = std::stoi(playerID);

			int bufSize = std::atoi(val);
			std::unique_ptr<char> buf = std::unique_ptr<char>(new char[bufSize]);
			mg_read(conn, buf.get(), bufSize);
			std::string content(buf.get(), bufSize);
			scenario.content = content;

			std::string gameState = interface->Reset(&scenario);

			stream << gameState.c_str();
		}
		else if (uri == "/step")
		{
			if (!interface->IsGameRunning())
			{
				mg_printf(conn, "%s", notRunningResponse);
				return handled;
			}

			const char* val = mg_get_header(conn, "Content-Length");
			if (!val)
			{
				mg_printf(conn, "%s", noPostData);
				return handled;
			}
			int bufSize = std::atoi(val);
			std::unique_ptr<char> buf = std::unique_ptr<char>(new char[bufSize]);
			mg_read(conn, buf.get(), bufSize);
			std::string postData(buf.get(), bufSize);
			std::stringstream postStream(postData);
			std::string line;
			std::vector<Command> commands;

			while (std::getline(postStream, line, '\n'))
			{
				Command cmd;
				const std::size_t splitPos = line.find(";");
				if (splitPos != std::string::npos)
				{
					cmd.playerID = std::stoi(line.substr(0, splitPos));
					cmd.json_cmd = line.substr(splitPos + 1);
					commands.push_back(cmd);
				}
			}
			std::string gameState = interface->Step(commands);
			if (gameState.empty())
			{
				mg_printf(conn, "%s", notRunningResponse);
				return handled;
			}
			else
				stream << gameState.c_str();
		}
		else if (uri == "/templates")
		{
			if (!interface->IsGameRunning()) {
				mg_printf(conn, "%s", notRunningResponse);
				return handled;
			}
			const char* val = mg_get_header(conn, "Content-Length");
			if (!val)
			{
				mg_printf(conn, "%s", noPostData);
				return handled;
			}
			int bufSize = std::atoi(val);
			std::unique_ptr<char> buf = std::unique_ptr<char>(new char[bufSize]);
			mg_read(conn, buf.get(), bufSize);
			std::string postData(buf.get(), bufSize);
			std::stringstream postStream(postData);
			std::string line;
			std::vector<std::string> templateNames;
			while (std::getline(postStream, line, '\n'))
				templateNames.push_back(line);

			for (std::string templateStr : interface->GetTemplates(templateNames))
				stream << templateStr.c_str() << "\n";

		}
		else
		{
			mg_printf(conn, "%s", header404);
			return handled;
		}

		mg_printf(conn, "%s", header200);
		std::string str = stream.str();
		mg_write(conn, str.c_str(), str.length());
		return handled;
	}

	case MG_HTTP_ERROR:
		return nullptr;

	case MG_EVENT_LOG:
		// Called by Mongoose's cry()
		LOGERROR("Mongoose error: %s", request_info->log_message);
		return nullptr;

	case MG_INIT_SSL:
		return nullptr;

	default:
		debug_warn(L"Invalid Mongoose event type");
		return nullptr;
	}
};

void RLInterface::EnableHTTP(const char* server_address)
{
	LOGMESSAGERENDER("Starting RL interface HTTP server");

	// Ignore multiple enablings
	if (m_MgContext)
		return;

	const char *options[] = {
		"listening_ports", server_address,
		"num_threads", "6", // enough for the browser's parallel connection limit
		nullptr
	};
	m_MgContext = mg_start(RLMgCallback, this, options);
	ENSURE(m_MgContext);
}

bool RLInterface::TryGetGameMessage(GameMessage& msg)
{
	if (m_GameMessage != nullptr) {
		msg = *m_GameMessage;
		m_GameMessage = nullptr;
		return true;
	}
	return false;
}

void RLInterface::TryApplyMessage()
{
	const bool nonVisual = !g_GUI;
	const bool isGameStarted = g_Game && g_Game->IsGameStarted();
	if (m_NeedsGameState && isGameStarted)
	{
		m_GameState = GetGameState();
		m_msgApplied.notify_one();
		m_msgLock.unlock();
		m_NeedsGameState = false;
	}

	if (m_msgLock.try_lock())
	{
		GameMessage msg;
		if (TryGetGameMessage(msg)) {
			switch (msg.type)
			{
				case GameMessageType::Reset:
				{
					if (isGameStarted)
						EndGame();

					g_Game = new CGame(m_ScenarioConfig.saveReplay);
					ScriptInterface& scriptInterface = g_Game->GetSimulation2()->GetScriptInterface();
					ScriptRequest rq(scriptInterface);

					JS::RootedValue attrs(rq.cx);
					scriptInterface.ParseJSON(m_ScenarioConfig.content, &attrs);

					g_Game->SetPlayerID(m_ScenarioConfig.playerID);
					g_Game->StartGame(&attrs, "");

					if (nonVisual)
					{
						LDR_NonprogressiveLoad();
						ENSURE(g_Game->ReallyStartGame() == PSRETURN_OK);
						m_GameState = GetGameState();
						m_msgApplied.notify_one();
						m_msgLock.unlock();
					}
					else
					{
						JS::RootedValue initData(rq.cx);
						scriptInterface.CreateObject(rq, &initData);
						scriptInterface.SetProperty(initData, "attribs", attrs);

						JS::RootedValue playerAssignments(rq.cx);
						scriptInterface.CreateObject(rq, &playerAssignments);
						scriptInterface.SetProperty(initData, "playerAssignments", playerAssignments);

						g_GUI->SwitchPage(L"page_loading.xml", &scriptInterface, initData);
						m_NeedsGameState = true;
					}
					break;
				}

				case GameMessageType::Commands:
				{
					if (!g_Game)
					{
						m_GameState = EMPTY_STATE;
						m_msgApplied.notify_one();
						m_msgLock.unlock();
						return;
					}

					CLocalTurnManager* turnMgr = static_cast<CLocalTurnManager*>(g_Game->GetTurnManager());

					const ScriptInterface& scriptInterface = g_Game->GetSimulation2()->GetScriptInterface();
					ScriptRequest rq(scriptInterface);
					for (Command command : msg.commands)
					{
						JS::RootedValue commandJSON(rq.cx);
						scriptInterface.ParseJSON(command.json_cmd, &commandJSON);
						turnMgr->PostCommand(command.playerID, commandJSON);
					}

					const double deltaRealTime = DEFAULT_TURN_LENGTH_SP;
					if (nonVisual)
					{
						const double deltaSimTime = deltaRealTime * g_Game->GetSimRate();
						size_t maxTurns = static_cast<size_t>(g_Game->GetSimRate());
						g_Game->GetTurnManager()->Update(deltaSimTime, maxTurns);
					}
					else
						g_Game->Update(deltaRealTime);

					m_GameState = GetGameState();
					m_msgApplied.notify_one();
					m_msgLock.unlock();
					break;
				}
			}
		}
		else
			m_msgLock.unlock();
	}
}

std::string RLInterface::GetGameState()
{
	const ScriptInterface& scriptInterface = g_Game->GetSimulation2()->GetScriptInterface();
	ScriptRequest rq(scriptInterface);

	const CSimContext simContext = g_Game->GetSimulation2()->GetSimContext();
	CmpPtr<ICmpAIInterface> cmpAIInterface(simContext.GetSystemEntity());
	JS::RootedValue state(rq.cx);
	cmpAIInterface->GetFullRepresentation(&state, true);
	return scriptInterface.StringifyJSON(&state, false);
}

bool RLInterface::IsGameRunning()
{
	return !!g_Game;
}
