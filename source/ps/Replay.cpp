/* Copyright (C) 2011 Wildfire Games.
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

#include "graphics/TerrainTextureManager.h"
#include "lib/timer.h"
#include "lib/path_util.h"
#include "lib/utf8.h"
#include "lib/file/file_system.h"
#include "lib/tex/tex.h"
#include "ps/Game.h"
#include "ps/Loader.h"
#include "ps/Profile.h"
#include "ps/ProfileViewer.h"
#include "scriptinterface/ScriptInterface.h"
#include "scriptinterface/ScriptStats.h"
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
	// Construct the directory name based on the PID, to be relatively unique.
	// Append "-1", "-2" etc if we run multiple matches in a single session,
	// to avoid accidentally overwriting earlier logs.

	std::wstringstream name;
	name << L"sim_log/" << getpid();

	static int run = -1;
	if (++run)
		name << "-" << run;

	name << L"/commands.txt";

	NativePath path = Path::Join(psLogDir(), NativePath(name.str()));
	CreateDirectories(Path::Path(path), 0700);
	m_Stream = new std::ofstream(StringFromNativePath(path).c_str(), std::ofstream::out | std::ofstream::trunc);
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

void CReplayLogger::Hash(const std::string& hash, bool quick)
{
	if (quick)
		*m_Stream << "hash-quick " << Hexify(hash) << "\n";
	else
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

void CReplayPlayer::Load(const std::string& path)
{
	debug_assert(!m_Stream);

	m_Stream = new std::ifstream(path.c_str());
	debug_assert(m_Stream->good());
}

void CReplayPlayer::Replay()
{
	debug_assert(m_Stream);

	new CProfileViewer;
	new CProfileManager;
	g_ScriptStatsTable = new CScriptStatsTable;
	g_ProfileViewer.AddRootTable(g_ScriptStatsTable);

	CGame game(true);
	g_Game = &game;

	// Need some stuff for terrain movement costs:
	// (TODO: this ought to be independent of any graphics code)
	tex_codec_register_all();
	new CTerrainTextureManager;
	g_TexMan.LoadTerrainTextures();


	std::vector<SimulationCommand> commands;
	u32 turn = 0;
	u32 turnLength = 0;

	std::string type;
	while ((*m_Stream >> type).good())
	{
//		if (turn >= 1400) break;

		if (type == "start")
		{
			std::string line;
			std::getline(*m_Stream, line);
			CScriptValRooted attribs = game.GetSimulation2()->GetScriptInterface().ParseJSON(line);

			game.StartGame(attribs);
			LDR_NonprogressiveLoad();
			PSRETURN ret = game.ReallyStartGame();
			debug_assert(ret == PSRETURN_OK);
		}
		else if (type == "turn")
		{
			*m_Stream >> turn >> turnLength;
			debug_printf(L"Turn %d (%d)... ", turn, turnLength);
		}
		else if (type == "cmd")
		{
			u32 player;
			*m_Stream >> player;

			std::string line;
			std::getline(*m_Stream, line);
			CScriptValRooted data = game.GetSimulation2()->GetScriptInterface().ParseJSON(line);

			SimulationCommand cmd = { player, data };
			commands.push_back(cmd);
		}
		else if (type == "hash" || type == "hash-quick")
		{
			std::string replayHash;
			*m_Stream >> replayHash;

			bool quick = (type == "hash-quick");

//			if (turn >= 1300)
//			if (turn >= 0)
			if (turn % 100 == 0)
			{
				std::string hash;
				bool ok = game.GetSimulation2()->ComputeStateHash(hash, quick);
				debug_assert(ok);
				std::string hexHash = Hexify(hash);
				if (hexHash == replayHash)
					debug_printf(L"hash ok (%hs)", hexHash.c_str());
				else
					debug_printf(L"HASH MISMATCH (%hs != %hs)", hexHash.c_str(), replayHash.c_str());
			}
		}
		else if (type == "end")
		{
			game.GetSimulation2()->Update(turnLength, commands);
			commands.clear();

//			std::string hash;
//			bool ok = game.GetSimulation2()->ComputeStateHash(hash, true);
//			debug_assert(ok);
//			debug_printf(L"%hs", Hexify(hash).c_str());

			debug_printf(L"\n");

			g_Profiler.Frame();

//			if (turn % 1000 == 0)
//				JS_GC(game.GetSimulation2()->GetScriptInterface().GetContext());

			if (turn % 20 == 0)
				g_ProfileViewer.SaveToFile();
		}
		else
		{
			debug_printf(L"Unrecognised replay token %hs\n", type.c_str());
		}
	}

	std::string hash;
	bool ok = game.GetSimulation2()->ComputeStateHash(hash, false);
	debug_assert(ok);
	debug_printf(L"# Final state: %hs\n", Hexify(hash).c_str());

	timer_DisplayClientTotals();

	// Clean up
	delete &g_TexMan;
	tex_codec_unregister_all();

	delete &g_Profiler;
	delete &g_ProfileViewer;
}
