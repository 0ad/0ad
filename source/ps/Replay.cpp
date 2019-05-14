/* Copyright (C) 2019 Wildfire Games.
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
#include "lib/file/file_system.h"
#include "lib/res/h_mgr.h"
#include "lib/tex/tex.h"
#include "ps/Game.h"
#include "ps/CLogger.h"
#include "ps/Loader.h"
#include "ps/Mod.h"
#include "ps/Profile.h"
#include "ps/ProfileViewer.h"
#include "ps/Pyrogenesis.h"
#include "ps/Mod.h"
#include "ps/Util.h"
#include "ps/VisualReplay.h"
#include "scriptinterface/ScriptInterface.h"
#include "scriptinterface/ScriptRuntime.h"
#include "scriptinterface/ScriptStats.h"
#include "simulation2/Simulation2.h"
#include "simulation2/helpers/SimulationCommand.h"

#include <ctime>
#include <fstream>

/**
 * Number of turns between two saved profiler snapshots.
 * Keep in sync with source/tools/replayprofile/graph.js
 */
static const int PROFILE_TURN_INTERVAL = 20;

CReplayLogger::CReplayLogger(const ScriptInterface& scriptInterface) :
	m_ScriptInterface(scriptInterface), m_Stream(NULL)
{
}

CReplayLogger::~CReplayLogger()
{
	delete m_Stream;
}

void CReplayLogger::StartGame(JS::MutableHandleValue attribs)
{
	JSContext* cx = m_ScriptInterface.GetContext();
	JSAutoRequest rq(cx);

	// Add timestamp, since the file-modification-date can change
	m_ScriptInterface.SetProperty(attribs, "timestamp", (double)std::time(nullptr));

	// Add engine version and currently loaded mods for sanity checks when replaying
	m_ScriptInterface.SetProperty(attribs, "engine_version", CStr(engine_version));
	JS::RootedValue mods(cx, Mod::GetLoadedModsWithVersions(m_ScriptInterface));
	m_ScriptInterface.SetProperty(attribs, "mods", mods);

	m_Directory = createDateIndexSubdirectory(VisualReplay::GetDirectoryPath());
	debug_printf("Writing replay to %s\n", m_Directory.string8().c_str());

	m_Stream = new std::ofstream(OsString(m_Directory / L"commands.txt").c_str(), std::ofstream::out | std::ofstream::trunc);
	*m_Stream << "start " << m_ScriptInterface.StringifyJSON(attribs, false) << "\n";
}

void CReplayLogger::Turn(u32 n, u32 turnLength, std::vector<SimulationCommand>& commands)
{
	JSContext* cx = m_ScriptInterface.GetContext();
	JSAutoRequest rq(cx);

	*m_Stream << "turn " << n << " " << turnLength << "\n";

	for (SimulationCommand& command : commands)
		*m_Stream << "cmd " << command.player << " " << m_ScriptInterface.StringifyJSON(&command.data, false) << "\n";

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

OsPath CReplayLogger::GetDirectory() const
{
	return m_Directory;
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

void CReplayPlayer::Load(const OsPath& path)
{
	ENSURE(!m_Stream);

	m_Stream = new std::ifstream(OsString(path).c_str());
	ENSURE(m_Stream->good());
}

CStr CReplayPlayer::ModListToString(const std::vector<std::vector<CStr>>& list) const
{
	CStr text;
	for (const std::vector<CStr>& mod : list)
		text += mod[0] + " (" + mod[1] + ")\n";
	return text;
}

void CReplayPlayer::CheckReplayMods(const ScriptInterface& scriptInterface, JS::HandleValue attribs) const
{
	JSContext* cx = scriptInterface.GetContext();
	JSAutoRequest rq(cx);

	std::vector<std::vector<CStr>> replayMods;
	scriptInterface.GetProperty(attribs, "mods", replayMods);

	std::vector<std::vector<CStr>> enabledMods;
	JS::RootedValue enabledModsJS(cx, Mod::GetLoadedModsWithVersions(scriptInterface));
	scriptInterface.FromJSVal(cx, enabledModsJS, enabledMods);

	CStr warn;
	if (replayMods.size() != enabledMods.size())
		warn = "The number of enabled mods does not match the mods of the replay.";
	else
		for (size_t i = 0; i < replayMods.size(); ++i)
		{
			if (replayMods[i][0] != enabledMods[i][0])
			{
				warn = "The enabled mods don't match the mods of the replay.";
				break;
			}
			else if (replayMods[i][1] != enabledMods[i][1])
			{
				warn = "The mod '" + replayMods[i][0] + "' with version '" + replayMods[i][1] + "' is required by the replay file, but version '" + enabledMods[i][1] + "' is present!";
				break;
			}
		}

	if (!warn.empty())
		LOGWARNING("%s\nThe mods of the replay are:\n%s\nThese mods are enabled:\n%s", warn, ModListToString(replayMods), ModListToString(enabledMods));
}

void CReplayPlayer::Replay(const bool serializationtest, const int rejointestturn, const bool ooslog, const bool testHashFull, const bool testHashQuick)
{
	ENSURE(m_Stream);

	new CProfileViewer;
	new CProfileManager;
	g_ScriptStatsTable = new CScriptStatsTable;
	g_ProfileViewer.AddRootTable(g_ScriptStatsTable);

	const int runtimeSize = 384 * 1024 * 1024;
	const int heapGrowthBytesGCTrigger = 20 * 1024 * 1024;
	g_ScriptRuntime = ScriptInterface::CreateRuntime(shared_ptr<ScriptRuntime>(), runtimeSize, heapGrowthBytesGCTrigger);

	Mod::CacheEnabledModVersions(g_ScriptRuntime);

	g_Game = new CGame(true, false);
	if (serializationtest)
		g_Game->GetSimulation2()->EnableSerializationTest();
	if (rejointestturn > 0)
		g_Game->GetSimulation2()->EnableRejoinTest(rejointestturn);
	if (ooslog)
		g_Game->GetSimulation2()->EnableOOSLog();

	// Need some stuff for terrain movement costs:
	// (TODO: this ought to be independent of any graphics code)
	new CTerrainTextureManager;
	g_TexMan.LoadTerrainTextures();

	// Initialise h_mgr so it doesn't crash when emitting sounds
	h_mgr_init();

	std::vector<SimulationCommand> commands;
	u32 turn = 0;
	u32 turnLength = 0;

	{
	JSContext* cx = g_Game->GetSimulation2()->GetScriptInterface().GetContext();
	JSAutoRequest rq(cx);
	std::string type;

	while ((*m_Stream >> type).good())
	{
		if (type == "start")
		{
			std::string line;
			std::getline(*m_Stream, line);
			JS::RootedValue attribs(cx);
			ENSURE(g_Game->GetSimulation2()->GetScriptInterface().ParseJSON(line, &attribs));

			CheckReplayMods(g_Game->GetSimulation2()->GetScriptInterface(), attribs);

			g_Game->StartGame(&attribs, "");

			// TODO: Non progressive load can fail - need a decent way to handle this
			LDR_NonprogressiveLoad();

			PSRETURN ret = g_Game->ReallyStartGame();
			ENSURE(ret == PSRETURN_OK);
		}
		else if (type == "turn")
		{
			*m_Stream >> turn >> turnLength;
			debug_printf("Turn %u (%u)...\n", turn, turnLength);
		}
		else if (type == "cmd")
		{
			player_id_t player;
			*m_Stream >> player;

			std::string line;
			std::getline(*m_Stream, line);
			JS::RootedValue data(cx);
			g_Game->GetSimulation2()->GetScriptInterface().ParseJSON(line, &data);
			g_Game->GetSimulation2()->GetScriptInterface().FreezeObject(data, true);
			commands.emplace_back(SimulationCommand(player, cx, data));
		}
		else if (type == "hash" || type == "hash-quick")
		{
			std::string replayHash;
			*m_Stream >> replayHash;
			TestHash(type, replayHash, testHashFull, testHashQuick);
		}
		else if (type == "end")
		{
			{
				g_Profiler2.RecordFrameStart();
				PROFILE2("frame");
				g_Profiler2.IncrementFrameNumber();
				PROFILE2_ATTR("%d", g_Profiler2.GetFrameNumber());

				g_Game->GetSimulation2()->Update(turnLength, commands);
				commands.clear();
			}

			g_Profiler.Frame();

			if (turn % PROFILE_TURN_INTERVAL == 0)
				g_ProfileViewer.SaveToFile();
		}
		else
			debug_printf("Unrecognised replay token %s\n", type.c_str());
	}
	}

	SAFE_DELETE(m_Stream);

	g_Profiler2.SaveToFile();

	std::string hash;
	bool ok = g_Game->GetSimulation2()->ComputeStateHash(hash, false);
	ENSURE(ok);
	debug_printf("# Final state: %s\n", Hexify(hash).c_str());
	timer_DisplayClientTotals();

	SAFE_DELETE(g_Game);

	// Must be explicitly destructed here to avoid callbacks from the JSAPI trying to use g_Profiler2 when
	// it's already destructed.
	g_ScriptRuntime.reset();

	// Clean up
	delete &g_TexMan;

	delete &g_Profiler;
	delete &g_ProfileViewer;
	SAFE_DELETE(g_ScriptStatsTable);
}

void CReplayPlayer::TestHash(const std::string& hashType, const std::string& replayHash, const bool testHashFull, const bool testHashQuick)
{
	bool quick = (hashType == "hash-quick");
	if ((quick && !testHashQuick) || (!quick && !testHashFull))
		return;

	std::string hash;
	ENSURE(g_Game->GetSimulation2()->ComputeStateHash(hash, quick));

	std::string hexHash = Hexify(hash);

	if (hexHash == replayHash)
		debug_printf("%s ok (%s)\n", hashType.c_str(), hexHash.c_str());
	else
		debug_printf("%s MISMATCH (%s != %s)\n", hashType.c_str(), hexHash.c_str(), replayHash.c_str());
}
