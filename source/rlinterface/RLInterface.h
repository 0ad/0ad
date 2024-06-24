/* Copyright (C) 2024 Wildfire Games.
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

#ifndef INCLUDED_RLINTERFACE
#define INCLUDED_RLINTERFACE

#include "simulation2/helpers/Player.h"
#include "third_party/mongoose/mongoose.h"

#include <condition_variable>
#include <mutex>
#include <vector>

namespace RL
{
struct ScenarioConfig
{
	bool saveReplay;
	player_id_t playerID;
	std::string content;
};

struct GameCommand
{
	int playerID;
	std::string json_cmd;
};

enum class GameMessageType
{
	None,
	Reset,
	Commands,
	Evaluate,
};

/**
 * Holds messages from the RL client to the game.
 */
struct GameMessage
{
	GameMessageType type;
	std::vector<GameCommand> commands;
};

/**
 * Implements an interface providing fundamental capabilities required for reinforcement
 * learning (over HTTP).
 *
 * This consists of enabling an external script to configure the scenario (via Reset) and
 * then step the game engine manually and apply player actions (via Step). The interface
 * also supports querying unit templates to provide information about max health and other
 * potentially relevant game state information.
 *
 * See source/tools/rlclient/ for the external client code.
 *
 * The HTTP server is threaded.
 * Flow of data (with the interface active):
 *  0. The game/main thread calls TryApplyMessage()
 *    - If no messages are pending, GOTO 0 (the simulation is not advanced).
 *  1. TryApplyMessage locks m_MsgLock, pulls the message, processes it, advances the simulation, and sets m_ReturnValue.
 *  2. TryApplyMessage notifies the RL thread that it can carry on and unlocks m_MsgLock. The main thread carries on frame rendering and goes back to 0.
 *  3. The RL thread locks m_MsgLock, reads m_ReturnValue, unlocks m_MsgLock, and sends the gamestate as HTTP Response to the RL client.
 *	4. The client processes the response and ultimately sends a new HTTP message to the RL Interface.
 *  5. The RL thread locks m_MsgLock, pushes the message, and starts waiting on the game/main thread to notify it (step 2).
 *   - GOTO 0.
 */
class Interface
{
	NONCOPYABLE(Interface);
public:
	Interface(const char* server_address);
	~Interface();

	/**
	 * Non-blocking call to process any pending messages from the RL client.
	 * Updates m_ReturnValue to the gamestate after messages have been processed.
	 */
	void TryApplyMessage();

private:
	static void* MgCallback(mg_event event, struct mg_connection *conn, const struct mg_request_info *request_info);
	static std::string GetRequestContent(struct mg_connection *conn);

	/**
	 * Process commands, update the simulation by one turn.
	 * @return the gamestate after processing commands.
	 */
	std::string Step(std::vector<GameCommand>&& commands);

	/**
	 * Reset the game state according to scenario, cleaning up existing games if required.
	 * @return the gamestate after resetting.
	 */
	std::string Reset(ScenarioConfig&& scenario);

	/**
	 * Evaluate JS code in the engine such as applying arbitrary modifiers.
	 * @return the gamestate after script evaluation.
	 */
	std::string Evaluate(std::string&& code);

	/**
	 * @return template data for all templates of @param names.
	 */
	std::vector<std::string> GetTemplates(const std::vector<std::string>& names) const;

	/**
	 * @return true if a game is currently running.
	 */
	bool IsGameRunning() const;

	/**
	 * Internal helper. Move @param msg into m_GameMessage, wait until it has been processed by the main thread,
	 * and @return the gamestate after that message is processed.
	 * It is invalid to call this if m_GameMessage is not currently empty.
	 */
	std::string SendGameMessage(GameMessage&& msg);

	/**
	 * Internal helper.
	 * @return true if m_GameMessage is not empty, and updates @param msg, false otherwise (msg is then unchanged).
	 */
	bool TryGetGameMessage(GameMessage& msg);

	/**
	 * Process any pending messages from the RL client.
	 * Updates m_ReturnValue to the gamestate after messages have been processed.
	 */
	void ApplyMessage(const GameMessage& msg);

	/**
	 * @return the full gamestate as a JSON strong.
	 * This uses the AI representation since it is readily available in the JS Engine.
	 */
	std::string GetGameState() const;

private:
	GameMessage m_GameMessage{GameMessageType::None};
	ScenarioConfig m_ScenarioConfig;
	std::string m_ReturnValue;
	bool m_NeedsGameState = false;

	mutable std::mutex m_Lock;
	std::mutex m_MsgLock;
	std::condition_variable m_MsgApplied;
	std::string m_Code;

	mg_context* m_Context;
};

}

#endif // INCLUDED_RLINTERFACE
