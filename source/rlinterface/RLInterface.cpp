/* Copyright (C) 2021 Wildfire Games.
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
#include "ps/CLogger.h"
#include "ps/Game.h"
#include "ps/GameSetup/GameSetup.h"
#include "ps/Loader.h"
#include "scriptinterface/Object.h"
#include "scriptinterface/ScriptInterface.h"
#include "scriptinterface/JSON.h"
#include "simulation2/Simulation2.h"
#include "simulation2/components/ICmpAIInterface.h"
#include "simulation2/components/ICmpTemplateManager.h"
#include "simulation2/system/LocalTurnManager.h"

#include <queue>
#include <sstream>
#include <tuple>

// Globally accessible pointer to the RL Interface.
std::unique_ptr<RL::Interface> g_RLInterface;

namespace RL
{
Interface::Interface(const char* server_address) : m_GameMessage({GameMessageType::None})
{
	LOGMESSAGERENDER("Starting RL interface HTTP server");

	const char *options[] = {
		"listening_ports", server_address,
		"num_threads", "1",
		nullptr
	};
	mg_context* mgContext = mg_start(MgCallback, this, options);
	ENSURE(mgContext);
}

// Interactions with the game engine (g_Game) must be done in the main
// thread as there are specific checks for this. We will pass messages
// to the main thread to be applied (ie, "GameMessage"s).
std::string Interface::SendGameMessage(GameMessage&& msg)
{
	std::unique_lock<std::mutex> msgLock(m_MsgLock);
	ENSURE(m_GameMessage.type == GameMessageType::None);
	m_GameMessage = std::move(msg);
	m_MsgApplied.wait(msgLock, [this]() { return m_GameMessage.type == GameMessageType::None; });
	return m_ReturnValue;
}

std::string Interface::Step(std::vector<GameCommand>&& commands)
{
	std::lock_guard<std::mutex> lock(m_Lock);
	return SendGameMessage({ GameMessageType::Commands, std::move(commands) });
}

std::string Interface::Reset(ScenarioConfig&& scenario)
{
	std::lock_guard<std::mutex> lock(m_Lock);
	m_ScenarioConfig = std::move(scenario);
	return SendGameMessage({ GameMessageType::Reset });
}

std::string Interface::Evaluate(std::string&& code)
{
	std::lock_guard<std::mutex> lock(m_Lock);
	m_Code = std::move(code);
	return SendGameMessage({ GameMessageType::Evaluate });
}

std::vector<std::string> Interface::GetTemplates(const std::vector<std::string>& names) const
{
	std::lock_guard<std::mutex> lock(m_Lock);
	CSimulation2& simulation = *g_Game->GetSimulation2();
	CmpPtr<ICmpTemplateManager> cmpTemplateManager(simulation.GetSimContext().GetSystemEntity());

	std::vector<std::string> templates;
	for (const std::string& templateName : names)
	{
		const CParamNode* node = cmpTemplateManager->GetTemplate(templateName);

		if (node != nullptr)
			templates.push_back(node->ToXMLString());
	}

	return templates;
}

void* Interface::MgCallback(mg_event event, struct mg_connection *conn, const struct mg_request_info *request_info)
{
	Interface* interface = (Interface*)request_info->user_data;
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

		const std::string uri = request_info->uri;

		if (uri == "/reset")
		{
			std::string data = GetRequestContent(conn);
			if (data.empty())
			{
				mg_printf(conn, "%s", noPostData);
				return handled;
			}
			ScenarioConfig scenario;
			const char *query_string = request_info->query_string;
			if (query_string != nullptr)
			{
				const std::string qs(query_string);
				scenario.saveReplay = qs.find("saveReplay") != std::string::npos;

				scenario.playerID = 1;
				char playerID[1];
				const int len = mg_get_var(query_string, qs.length(), "playerID", playerID, 1);
				if (len != -1)
					scenario.playerID = std::stoi(playerID);
			}

			scenario.content = std::move(data);

			const std::string gameState = interface->Reset(std::move(scenario));

			stream << gameState.c_str();
		}
		else if (uri == "/step")
		{
			if (!interface->IsGameRunning())
			{
				mg_printf(conn, "%s", notRunningResponse);
				return handled;
			}

			std::string data = GetRequestContent(conn);
			std::stringstream postStream(data);
			std::string line;
			std::vector<GameCommand> commands;

			while (std::getline(postStream, line, '\n'))
			{
				const std::size_t splitPos = line.find(";");
				if (splitPos == std::string::npos)
					continue;

				GameCommand cmd;
				cmd.playerID = std::stoi(line.substr(0, splitPos));
				cmd.json_cmd = line.substr(splitPos + 1);
				commands.push_back(std::move(cmd));
			}
			const std::string gameState = interface->Step(std::move(commands));
			if (gameState.empty())
			{
				mg_printf(conn, "%s", notRunningResponse);
				return handled;
			}
			else
				stream << gameState.c_str();
		}
		else if (uri == "/evaluate")
		{
			if (!interface->IsGameRunning())
			{
				mg_printf(conn, "%s", notRunningResponse);
				return handled;
			}

			std::string code = GetRequestContent(conn);
			if (code.empty())
			{
				mg_printf(conn, "%s", noPostData);
				return handled;
			}

			const std::string codeResult = interface->Evaluate(std::move(code));
			if (codeResult.empty())
			{
				mg_printf(conn, "%s", notRunningResponse);
				return handled;
			}
			else
				stream << codeResult.c_str();
		}
		else if (uri == "/templates")
		{
			if (!interface->IsGameRunning()) {
				mg_printf(conn, "%s", notRunningResponse);
				return handled;
			}
			const std::string data = GetRequestContent(conn);
			if (data.empty())
			{
				mg_printf(conn, "%s", noPostData);
				return handled;
			}
			std::stringstream postStream(data);
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
		const std::string str = stream.str();
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
}

std::string Interface::GetRequestContent(struct mg_connection *conn)
{
	const static std::string NO_CONTENT;
	const char* val = mg_get_header(conn, "Content-Length");
	if (!val)
	{
		return NO_CONTENT;
	}
	const int contentSize = std::atoi(val);

	std::string content(contentSize, ' ');
	mg_read(conn, content.data(), contentSize);
	return content;
}

bool Interface::TryGetGameMessage(GameMessage& msg)
{
	if (m_GameMessage.type != GameMessageType::None)
	{
		msg = m_GameMessage;
		m_GameMessage = {GameMessageType::None};
		return true;
	}
	return false;
}

void Interface::TryApplyMessage()
{
	const bool isGameStarted = g_Game && g_Game->IsGameStarted();
	if (m_NeedsGameState && isGameStarted)
	{
		m_ReturnValue = GetGameState();
		m_MsgApplied.notify_one();
		m_MsgLock.unlock();
		m_NeedsGameState = false;
	}

	if (!m_MsgLock.try_lock())
		return;

	GameMessage msg;
	if (!TryGetGameMessage(msg))
	{
		m_MsgLock.unlock();
		return;
	}

	ApplyMessage(msg);
}

void Interface::ApplyMessage(const GameMessage& msg)
{
	const static std::string EMPTY_STATE;
	const bool nonVisual = !g_GUI;
	const bool isGameStarted = g_Game && g_Game->IsGameStarted();
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
			Script::ParseJSON(rq, m_ScenarioConfig.content, &attrs);

			g_Game->SetPlayerID(m_ScenarioConfig.playerID);
			g_Game->StartGame(&attrs, "");

			if (nonVisual)
			{
				LDR_NonprogressiveLoad();
				ENSURE(g_Game->ReallyStartGame() == PSRETURN_OK);
				m_ReturnValue = GetGameState();
				m_MsgApplied.notify_one();
				m_MsgLock.unlock();
			}
			else
			{
				JS::RootedValue initData(rq.cx);
				Script::CreateObject(rq, &initData);
				Script::SetProperty(rq, initData, "attribs", attrs);

				JS::RootedValue playerAssignments(rq.cx);
				Script::CreateObject(rq, &playerAssignments);
				Script::SetProperty(rq, initData, "playerAssignments", playerAssignments);

				g_GUI->SwitchPage(L"page_loading.xml", &scriptInterface, initData);
				m_NeedsGameState = true;
			}
			break;
		}

		case GameMessageType::Commands:
		{
			if (!g_Game)
			{
				m_ReturnValue = EMPTY_STATE;
				m_MsgApplied.notify_one();
				m_MsgLock.unlock();
				return;
			}
			const ScriptInterface& scriptInterface = g_Game->GetSimulation2()->GetScriptInterface();
			CLocalTurnManager* turnMgr = static_cast<CLocalTurnManager*>(g_Game->GetTurnManager());

			for (const GameCommand& command : msg.commands)
			{
				ScriptRequest rq(scriptInterface);
				JS::RootedValue commandJSON(rq.cx);
				Script::ParseJSON(rq, command.json_cmd, &commandJSON);
				turnMgr->PostCommand(command.playerID, commandJSON);
			}

			const u32 deltaRealTime = DEFAULT_TURN_LENGTH;
			if (nonVisual)
			{
				const double deltaSimTime = deltaRealTime * g_Game->GetSimRate();
				const size_t maxTurns = static_cast<size_t>(g_Game->GetSimRate());
				g_Game->GetTurnManager()->Update(deltaSimTime, maxTurns);
			}
			else
				g_Game->Update(deltaRealTime);

			m_ReturnValue = GetGameState();
			m_MsgApplied.notify_one();
			m_MsgLock.unlock();
			break;
		}
		case GameMessageType::Evaluate:
		{
			if (!g_Game)
			{
				m_ReturnValue = EMPTY_STATE;
				m_MsgApplied.notify_one();
				m_MsgLock.unlock();
				return;
			}
			const ScriptInterface& scriptInterface = g_Game->GetSimulation2()->GetScriptInterface();
			ScriptRequest rq(scriptInterface);
			JS::RootedValue ret(rq.cx);
			scriptInterface.Eval(m_Code.c_str(), &ret);
			m_ReturnValue = Script::StringifyJSON(rq, &ret, false);
			m_MsgApplied.notify_one();
			m_MsgLock.unlock();
			break;
		}
		default:
		break;
	}
}

std::string Interface::GetGameState() const
{
	const ScriptInterface& scriptInterface = g_Game->GetSimulation2()->GetScriptInterface();
	const CSimContext simContext = g_Game->GetSimulation2()->GetSimContext();
	CmpPtr<ICmpAIInterface> cmpAIInterface(simContext.GetSystemEntity());
	ScriptRequest rq(scriptInterface);
	JS::RootedValue state(rq.cx);
	cmpAIInterface->GetFullRepresentation(&state, true);
	return Script::StringifyJSON(rq, &state, false);
}

bool Interface::IsGameRunning() const
{
	return g_Game != nullptr;
}
}
