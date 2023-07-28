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

#include "precompiled.h"

#include "Replay.h"

#include "graphics/TerrainTextureManager.h"
#include "lib/timer.h"
#include "lib/file/file_system.h"
#include "lib/tex/tex.h"
#include "ps/CLogger.h"
#include "ps/Game.h"
#include "ps/GameSetup/GameSetup.h"
#include "ps/GameSetup/CmdLineArgs.h"
#include "ps/GameSetup/Paths.h"
#include "ps/Loader.h"
#include "ps/Mod.h"
#include "ps/Profile.h"
#include "ps/ProfileViewer.h"
#include "ps/Pyrogenesis.h"
#include "ps/Mod.h"
#include "ps/Util.h"
#include "ps/VisualReplay.h"
#include "scriptinterface/FunctionWrapper.h"
#include "scriptinterface/Object.h"
#include "scriptinterface/ScriptContext.h"
#include "scriptinterface/ScriptInterface.h"
#include "scriptinterface/ScriptRequest.h"
#include "scriptinterface/ScriptStats.h"
#include "scriptinterface/JSON.h"
#include "simulation2/components/ICmpGuiInterface.h"
#include "simulation2/helpers/Player.h"
#include "simulation2/helpers/SimulationCommand.h"
#include "simulation2/Simulation2.h"
#include "simulation2/system/CmpPtr.h"

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
	ScriptRequest rq(m_ScriptInterface);

	// Add timestamp, since the file-modification-date can change
	Script::SetProperty(rq, attribs, "timestamp", (double)std::time(nullptr));

	// Add engine version and currently loaded mods for sanity checks when replaying
	Script::SetProperty(rq, attribs, "engine_version", engine_version);
	JS::RootedValue mods(rq.cx);
	Script::ToJSVal(rq, &mods, g_Mods.GetEnabledModsData());
	Script::SetProperty(rq, attribs, "mods", mods);

	m_Directory = createDateIndexSubdirectory(VisualReplay::GetDirectoryPath());
	debug_printf("FILES| Replay written to '%s'\n", m_Directory.string8().c_str());

	m_Stream = new std::ofstream(OsString(m_Directory / L"commands.txt").c_str(), std::ofstream::out | std::ofstream::trunc);
	*m_Stream << "start " << Script::StringifyJSON(rq, attribs, false) << "\n";
}

void CReplayLogger::Turn(u32 n, u32 turnLength, std::vector<SimulationCommand>& commands)
{
	ScriptRequest rq(m_ScriptInterface);

	*m_Stream << "turn " << n << " " << turnLength << "\n";

	for (SimulationCommand& command : commands)
		*m_Stream << "cmd " << command.player << " " << Script::StringifyJSON(rq, &command.data, false) << "\n";

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

void CReplayLogger::SaveMetadata(const CSimulation2& simulation)
{
	CmpPtr<ICmpGuiInterface> cmpGuiInterface(simulation, SYSTEM_ENTITY);
	if (!cmpGuiInterface)
	{
		LOGERROR("Could not save replay metadata!");
		return;
	}

	ScriptInterface& scriptInterface = simulation.GetScriptInterface();
	ScriptRequest rq(scriptInterface);

	JS::RootedValue arg(rq.cx);
	JS::RootedValue metadata(rq.cx);
	cmpGuiInterface->ScriptCall(INVALID_PLAYER, L"GetReplayMetadata", arg, &metadata);

	const OsPath fileName = g_Game->GetReplayLogger().GetDirectory() / L"metadata.json";
	CreateDirectories(fileName.Parent(), 0700);

	std::ofstream stream (OsString(fileName).c_str(), std::ofstream::out | std::ofstream::trunc);
	if (stream)
	{
		stream << Script::StringifyJSON(rq, &metadata, false);
		stream.close();
		debug_printf("FILES| Replay metadata written to '%s'\n", fileName.string8().c_str());
	}
	else
		debug_printf("FILES| Failed to write replay metadata to '%s'\n", fileName.string8().c_str());

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

namespace
{
CStr ModListToString(const std::vector<const Mod::ModData*>& list)
{
	CStr text;
	for (const Mod::ModData* data : list)
		text += data->m_Pathname + " (" + data->m_Version + ")\n";
	return text;
}

void CheckReplayMods(const std::vector<Mod::ModData>& replayMods)
{
	std::vector<const Mod::ModData*> replayData;
	replayData.reserve(replayMods.size());
	for (const Mod::ModData& data : replayMods)
		replayData.push_back(&data);
	if (!Mod::AreModsPlayCompatible(g_Mods.GetEnabledModsData(), replayData))
		LOGWARNING("Incompatible replay mods detected.\nThe mods of the replay are:\n%s\nThese mods are enabled:\n%s",
			ModListToString(replayData), ModListToString(g_Mods.GetEnabledModsData()));
}
} // anonymous namespace

void CReplayPlayer::Replay(const bool serializationtest, const int rejointestturn, const bool ooslog, const bool testHashFull, const bool testHashQuick)
{
	ENSURE(m_Stream);

	new CProfileViewer;
	new CProfileManager;
	g_ScriptStatsTable = new CScriptStatsTable;
	g_ProfileViewer.AddRootTable(g_ScriptStatsTable);

	const int contextSize = 384 * 1024 * 1024;
	const int heapGrowthBytesGCTrigger = 20 * 1024 * 1024;
	g_ScriptContext = ScriptContext::CreateContext(contextSize, heapGrowthBytesGCTrigger);

	std::vector<SimulationCommand> commands;
	u32 turn = 0;
	u32 turnLength = 0;

	{
	std::string type;

	while ((*m_Stream >> type).good())
	{
		if (type == "start")
		{
			std::string attribsStr;
			{
				ScriptInterface scriptInterface("Engine", "Replay", g_ScriptContext);
				ScriptRequest rq(scriptInterface);
				std::getline(*m_Stream, attribsStr);
				JS::RootedValue attribs(rq.cx);
				if (!Script::ParseJSON(rq, attribsStr, &attribs))
				{
					LOGERROR("Error parsing JSON attributes: %s", attribsStr);
					// TODO: do something cleverer than crashing.
					ENSURE(false);
				}

				// Load the mods specified in the replay.
				std::vector<Mod::ModData> replayMods;
				if (!Script::GetProperty(rq, attribs, "mods", replayMods))
				{
					LOGERROR("Could not get replay mod information.");
					// TODO: do something cleverer than crashing.
					ENSURE(false);
				}

				std::vector<CStr> mods;
				for (const Mod::ModData& data : replayMods)
					mods.emplace_back(data.m_Pathname);

				// Ignore the return value, we check below.
				g_Mods.UpdateAvailableMods(scriptInterface);
				g_Mods.EnableMods(mods, false);
				CheckReplayMods(replayMods);

				MountMods(Paths(g_CmdLineArgs), g_Mods.GetEnabledMods());
			}

			g_Game = new CGame(false);
			if (serializationtest)
				g_Game->GetSimulation2()->EnableSerializationTest();
			if (rejointestturn >= 0)
				g_Game->GetSimulation2()->EnableRejoinTest(rejointestturn);
			if (ooslog)
				g_Game->GetSimulation2()->EnableOOSLog();

			ScriptRequest rq(g_Game->GetSimulation2()->GetScriptInterface());
			JS::RootedValue attribs(rq.cx);
			ENSURE(Script::ParseJSON(rq, attribsStr, &attribs));
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
			ScriptRequest rq(g_Game->GetSimulation2()->GetScriptInterface());
			JS::RootedValue data(rq.cx);
			Script::ParseJSON(rq, line, &data);
			Script::FreezeObject(rq, data, true);
			commands.emplace_back(SimulationCommand(player, rq.cx, data));
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
	g_ScriptContext.reset();

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
