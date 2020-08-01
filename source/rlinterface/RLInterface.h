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
#ifndef INCLUDED_RLINTERFACE
#define INCLUDED_RLINTERFACE

#include "simulation2/helpers/Player.h"

#include <condition_variable>
#include <mutex>
#include <vector>

struct ScenarioConfig {
	bool saveReplay;
	player_id_t playerID;
	std::string content;
};
struct Command {
	int playerID;
	std::string json_cmd;
};

enum GameMessageType { Reset, Commands };
struct GameMessage {
	GameMessageType type;
	std::vector<Command> commands;
};

extern void EndGame();

struct mg_context;
const static std::string EMPTY_STATE;

class RLInterface
{

	public:

		std::string Step(const std::vector<Command> commands);
		std::string Reset(const ScenarioConfig* scenario);
		std::vector<std::string> GetTemplates(const std::vector<std::string> names) const;

		void EnableHTTP(const char* server_address);
		std::string SendGameMessage(const GameMessage msg);
		bool TryGetGameMessage(GameMessage& msg);
		void TryApplyMessage();
		std::string GetGameState();
		bool IsGameRunning();

	private:
		mg_context* m_MgContext = nullptr;
		const GameMessage* m_GameMessage = nullptr;
		std::string m_GameState;
		bool m_NeedsGameState = false;
		mutable std::mutex m_lock;
		std::mutex m_msgLock;
		std::condition_variable m_msgApplied;
		ScenarioConfig m_ScenarioConfig;
};

extern RLInterface* g_RLInterface;

#endif // INCLUDED_RLINTERFACE
