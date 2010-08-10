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

#include "Replay.h"

#include "lib/utf8.h"
#include "lib/file/file_system.h"
#include "ps/Game.h"
#include "ps/Loader.h"
#include "scriptinterface/ScriptInterface.h"
#include "simulation2/Simulation2.h"
#include "simulation2/helpers/SimulationCommand.h"

#include <sstream>
#include <fstream>
#include <iomanip>

#if MSC_VERSION
#include <process.h>
#define getpid _getpid // use the non-deprecated function name
#endif

static std::string Hexify(const std::string& s)
{
	std::stringstream str;
	str << std::hex;
	for (size_t i = 0; i < s.size(); ++i)
		str << std::setfill('0') << std::setw(2) << (int)(unsigned char)s[i];
	return str.str();
}

CReplayLogger::CReplayLogger(ScriptInterface& scriptInterface) :
	m_ScriptInterface(scriptInterface)
{
	std::wstringstream name;
	name << L"sim_log/" << getpid() << L"/commands.txt";
	fs::wpath path (psLogDir()/name.str());
	CreateDirectories(path.branch_path(), 0700);
	m_Stream = new std::ofstream(path.external_file_string().c_str(), std::ofstream::out | std::ofstream::trunc);
}

CReplayLogger::~CReplayLogger()
{
	delete m_Stream;
}

void CReplayLogger::StartGame(const CScriptValRooted& attribs)
{
	*m_Stream << "start " << m_ScriptInterface.StringifyJSON(attribs.get(), false) << "\n";
}

void CReplayLogger::Turn(u32 n, u32 turnLength, const std::vector<SimulationCommand>& commands)
{
	*m_Stream << "turn " << n << " " << turnLength << "\n";
	for (size_t i = 0; i < commands.size(); ++i)
	{
		*m_Stream << "cmd " << commands[i].player << " " << m_ScriptInterface.StringifyJSON(commands[i].data.get(), false) << "\n";
	}
	*m_Stream << "end\n";
	m_Stream->flush();
}

void CReplayLogger::Hash(const std::string& hash)
{
	*m_Stream << "hash " << Hexify(hash) << "\n";
}

////////////////////////////////////////////////////////////////

CReplayPlayer::CReplayPlayer() :
	m_Stream(NULL)
{
}

CReplayPlayer::~CReplayPlayer()
{
	delete m_Stream;
}

void CReplayPlayer::Load(const fs::path& path)
{
	debug_assert(!m_Stream);

	m_Stream = new std::ifstream(path.external_file_string().c_str());
	debug_assert(m_Stream->good());
}

void CReplayPlayer::Replay()
{
	debug_assert(m_Stream);

	CGame game(true);
	g_Game = &game;

	std::vector<SimulationCommand> commands;
	u32 turnLength = 0;

	std::string type;
	while ((*m_Stream >> type).good())
	{
		if (type == "start")
		{
			std::string line;
			std::getline(*m_Stream, line);
			std::wstring linew = wstring_from_utf8(line);
			utf16string line16(linew.begin(), linew.end());
			CScriptValRooted attribs = game.GetSimulation2()->GetScriptInterface().ParseJSON(line16);

			game.StartGame(attribs);
			LDR_NonprogressiveLoad();
			PSRETURN ret = game.ReallyStartGame();
			debug_assert(ret == PSRETURN_OK);
		}
		else if (type == "turn")
		{
			u32 turn;
			*m_Stream >> turn >> turnLength;
			printf("Turn %d (%d)... ", turn, turnLength);
		}
		else if (type == "cmd")
		{
			u32 player;
			*m_Stream >> player;

			std::string line;
			std::getline(*m_Stream, line);
			std::wstring linew = wstring_from_utf8(line);
			utf16string line16(linew.begin(), linew.end());
			CScriptValRooted data = game.GetSimulation2()->GetScriptInterface().ParseJSON(line16);

			SimulationCommand cmd = { player, data };
			commands.push_back(cmd);
		}
		else if (type == "end")
		{
			game.GetSimulation2()->Update(turnLength, commands);
			commands.clear();

			std::string hash;
			bool ok = game.GetSimulation2()->ComputeStateHash(hash);
			debug_assert(ok);

			printf("%s\n", Hexify(hash).c_str());
		}
		else
		{
			printf("Unrecognised replay token %s\n", type.c_str());
		}
	}
}
