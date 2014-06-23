/* Copyright (C) 2013 Wildfire Games.
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

#include "Simulation2.h"

#include "scriptinterface/ScriptInterface.h"

#include "simulation2/MessageTypes.h"
#include "simulation2/system/ComponentManager.h"
#include "simulation2/system/ParamNode.h"
#include "simulation2/system/SimContext.h"
#include "simulation2/components/ICmpAIManager.h"
#include "simulation2/components/ICmpCommandQueue.h"
#include "simulation2/components/ICmpTemplateManager.h"

#include "graphics/MapReader.h"
#include "graphics/Terrain.h"
#include "lib/timer.h"
#include "lib/file/vfs/vfs_util.h"
#include "maths/MathUtil.h"
#include "ps/CLogger.h"
#include "ps/ConfigDB.h"
#include "ps/Filesystem.h"
#include "ps/Loader.h"
#include "ps/Profile.h"
#include "ps/Pyrogenesis.h"
#include "ps/XML/Xeromyces.h"

#include <iomanip>

#if MSC_VERSION
#include <process.h>
#define getpid _getpid // use the non-deprecated function name
#endif

static std::string Hexify(const std::string& s) // TODO: shouldn't duplicate this function in so many places
{
	std::stringstream str;
	str << std::hex;
	for (size_t i = 0; i < s.size(); ++i)
		str << std::setfill('0') << std::setw(2) << (int)(unsigned char)s[i];
	return str.str();
}

class CSimulation2Impl
{
public:
	CSimulation2Impl(CUnitManager* unitManager, shared_ptr<ScriptRuntime> rt, CTerrain* terrain) :
		m_SimContext(), m_ComponentManager(m_SimContext, rt),
		m_EnableOOSLog(false), m_EnableSerializationTest(false)
	{
		m_SimContext.m_UnitManager = unitManager;
		m_SimContext.m_Terrain = terrain;
		m_ComponentManager.LoadComponentTypes();

		RegisterFileReloadFunc(ReloadChangedFileCB, this);

		// Tests won't have config initialised
		if (CConfigDB::IsInitialised())
		{
			CFG_GET_VAL("ooslog", Bool, m_EnableOOSLog);
			CFG_GET_VAL("serializationtest", Bool, m_EnableSerializationTest);
		}
	}

	~CSimulation2Impl()
	{
		UnregisterFileReloadFunc(ReloadChangedFileCB, this);
	}

	void ResetState(bool skipScriptedComponents, bool skipAI)
	{
		m_DeltaTime = 0.0;
		m_LastFrameOffset = 0.0f;
		m_TurnNumber = 0;
		ResetComponentState(m_ComponentManager, skipScriptedComponents, skipAI);
	}

	static void ResetComponentState(CComponentManager& componentManager, bool skipScriptedComponents, bool skipAI)
	{
		componentManager.ResetState();
		componentManager.InitSystemEntity();
		componentManager.AddSystemComponents(skipScriptedComponents, skipAI);
	}

	static bool LoadDefaultScripts(CComponentManager& componentManager, std::set<VfsPath>* loadedScripts);
	static bool LoadScripts(CComponentManager& componentManager, std::set<VfsPath>* loadedScripts, const VfsPath& path);
	Status ReloadChangedFile(const VfsPath& path);

	static Status ReloadChangedFileCB(void* param, const VfsPath& path)
	{
		return static_cast<CSimulation2Impl*>(param)->ReloadChangedFile(path);
	}

	int ProgressiveLoad();
	void Update(int turnLength, const std::vector<SimulationCommand>& commands);
	static void UpdateComponents(CSimContext& simContext, fixed turnLengthFixed, const std::vector<SimulationCommand>& commands);
	void Interpolate(float simFrameLength, float frameOffset, float realFrameLength);

	void DumpState();

	CSimContext m_SimContext;
	CComponentManager m_ComponentManager;
	double m_DeltaTime;
	float m_LastFrameOffset;

	std::string m_StartupScript;
	CScriptValRooted m_InitAttributes;
	CScriptValRooted m_MapSettings;

	std::set<VfsPath> m_LoadedScripts;

	uint32_t m_TurnNumber;

	bool m_EnableOOSLog;


	// Functions and data for the serialization test mode: (see Update() for relevant comments)

	bool m_EnableSerializationTest;

	struct SerializationTestState
	{
		std::stringstream state;
		std::stringstream debug;
		std::string hash;
	};

	void DumpSerializationTestState(SerializationTestState& state, const OsPath& path, const OsPath::String& suffix);

	void ReportSerializationFailure(
		SerializationTestState* primaryStateBefore, SerializationTestState* primaryStateAfter,
		SerializationTestState* secondaryStateBefore, SerializationTestState* secondaryStateAfter);

	static std::vector<SimulationCommand> CloneCommandsFromOtherContext(ScriptInterface& oldScript, ScriptInterface& newScript,
		const std::vector<SimulationCommand>& commands)
	{
		std::vector<SimulationCommand> newCommands = commands;
		for (size_t i = 0; i < newCommands.size(); ++i)
		{
			newCommands[i].data = CScriptValRooted(newScript.GetContext(),
				newScript.CloneValueFromOtherContext(oldScript, newCommands[i].data.get()));
		}
		return newCommands;
	}
};

bool CSimulation2Impl::LoadDefaultScripts(CComponentManager& componentManager, std::set<VfsPath>* loadedScripts)
{
	return (
		LoadScripts(componentManager, loadedScripts, L"simulation/components/interfaces/") &&
		LoadScripts(componentManager, loadedScripts, L"simulation/helpers/") &&
		LoadScripts(componentManager, loadedScripts, L"simulation/components/")
	);
}

bool CSimulation2Impl::LoadScripts(CComponentManager& componentManager, std::set<VfsPath>* loadedScripts, const VfsPath& path)
{
	VfsPaths pathnames;
	if (vfs::GetPathnames(g_VFS, path, L"*.js", pathnames) < 0)
		return false;

	bool ok = true;
	for (VfsPaths::iterator it = pathnames.begin(); it != pathnames.end(); ++it)
	{
		VfsPath filename = *it;
		if (loadedScripts)
			loadedScripts->insert(filename);
		LOGMESSAGE(L"Loading simulation script '%ls'", filename.string().c_str());
		if (! componentManager.LoadScript(filename))
			ok = false;
	}
	return ok;
}

Status CSimulation2Impl::ReloadChangedFile(const VfsPath& path)
{
	// Ignore if this file wasn't loaded as a script
	// (TODO: Maybe we ought to load in any new .js files that are created in the right directories)
	if (m_LoadedScripts.find(path) == m_LoadedScripts.end())
		return INFO::OK;

	// If the file doesn't exist (e.g. it was deleted), don't bother loading it since that'll give an error message.
	// (Also don't bother trying to 'unload' it from the component manager, because that's not possible)
	if (!VfsFileExists(path))
		return INFO::OK;

	LOGMESSAGE(L"Reloading simulation script '%ls'", path.string().c_str());
	if (!m_ComponentManager.LoadScript(path, true))
		return ERR::FAIL;

	return INFO::OK;
}

int CSimulation2Impl::ProgressiveLoad()
{
	// yield after this time is reached. balances increased progress bar
	// smoothness vs. slowing down loading.
	const double end_time = timer_Time() + 200e-3;

	int ret;

	do
	{
		bool progressed = false;
		int total = 0;
		int progress = 0;

		CMessageProgressiveLoad msg(&progressed, &total, &progress);

		m_ComponentManager.BroadcastMessage(msg);

		if (!progressed || total == 0)
			return 0; // we have nothing left to load

		ret = Clamp(100*progress / total, 1, 100);
	}
	while (timer_Time() < end_time);

	return ret;
}

void CSimulation2Impl::DumpSerializationTestState(SerializationTestState& state, const OsPath& path, const OsPath::String& suffix)
{
	if (!state.hash.empty())
	{
		std::ofstream file (OsString(path / (L"hash." + suffix)).c_str(), std::ofstream::out | std::ofstream::trunc);
		file << Hexify(state.hash);
	}

	if (!state.debug.str().empty())
	{
		std::ofstream file (OsString(path / (L"debug." + suffix)).c_str(), std::ofstream::out | std::ofstream::trunc);
		file << state.debug.str();
	}

	if (!state.state.str().empty())
	{
		std::ofstream file (OsString(path / (L"state." + suffix)).c_str(), std::ofstream::out | std::ofstream::trunc | std::ofstream::binary);
		file << state.state.str();
	}
}

void CSimulation2Impl::ReportSerializationFailure(
	SerializationTestState* primaryStateBefore, SerializationTestState* primaryStateAfter,
	SerializationTestState* secondaryStateBefore, SerializationTestState* secondaryStateAfter)
{
	OsPath path = psLogDir() / "oos_log";
	CreateDirectories(path, 0700);

	// Clean up obsolete files from previous runs
	wunlink(path / "hash.before.a");
	wunlink(path / "hash.before.b");
	wunlink(path / "debug.before.a");
	wunlink(path / "debug.before.b");
	wunlink(path / "state.before.a");
	wunlink(path / "state.before.b");
	wunlink(path / "hash.after.a");
	wunlink(path / "hash.after.b");
	wunlink(path / "debug.after.a");
	wunlink(path / "debug.after.b");
	wunlink(path / "state.after.a");
	wunlink(path / "state.after.b");

	if (primaryStateBefore)
		DumpSerializationTestState(*primaryStateBefore, path, L"before.a");
	if (primaryStateAfter)
		DumpSerializationTestState(*primaryStateAfter, path, L"after.a");
	if (secondaryStateBefore)
		DumpSerializationTestState(*secondaryStateBefore, path, L"before.b");
	if (secondaryStateAfter)
		DumpSerializationTestState(*secondaryStateAfter, path, L"after.b");

	debug_warn(L"Serialization test failure");
}

void CSimulation2Impl::Update(int turnLength, const std::vector<SimulationCommand>& commands)
{
	PROFILE3("sim update");
	PROFILE2_ATTR("turn %d", (int)m_TurnNumber);

	fixed turnLengthFixed = fixed::FromInt(turnLength) / 1000;

	/*
	 * In serialization test mode, we save the original (primary) simulation state before each turn update.
	 * We run the update, then load the saved state into a secondary context.
	 * We serialize that again and compare to the original serialization (to check that
	 * serialize->deserialize->serialize is equivalent to serialize).
	 * Then we run the update on the secondary context, and check that its new serialized
	 * state matches the primary context after the update (to check that the simulation doesn't depend
	 * on anything that's not serialized).
	 */

	const bool serializationTestDebugDump = false; // set true to save human-readable state dumps before an error is detected, for debugging (but slow)
	const bool serializationTestHash = true; // set true to save and compare hash of state

	SerializationTestState primaryStateBefore;
	if (m_EnableSerializationTest)
	{
		ENSURE(m_ComponentManager.SerializeState(primaryStateBefore.state));
		if (serializationTestDebugDump)
			ENSURE(m_ComponentManager.DumpDebugState(primaryStateBefore.debug, false));
		if (serializationTestHash)
			ENSURE(m_ComponentManager.ComputeStateHash(primaryStateBefore.hash, false));
	}


	UpdateComponents(m_SimContext, turnLengthFixed, commands);


	if (m_EnableSerializationTest)
	{
		// Initialise the secondary simulation
		CTerrain secondaryTerrain;
		CSimContext secondaryContext;
		secondaryContext.m_Terrain = &secondaryTerrain;
		CComponentManager secondaryComponentManager(secondaryContext, m_ComponentManager.GetScriptInterface().GetRuntime());
		secondaryComponentManager.LoadComponentTypes();
		ENSURE(LoadDefaultScripts(secondaryComponentManager, NULL));
		ResetComponentState(secondaryComponentManager, false, false);

		// Load the map into the secondary simulation

		LDR_BeginRegistering();
		CMapReader* mapReader = new CMapReader; // automatically deletes itself

		// TODO: this duplicates CWorld::RegisterInit and could probably be cleaned up a bit
		std::string mapType;
		m_ComponentManager.GetScriptInterface().GetProperty(m_InitAttributes.get(), "mapType", mapType);
		if (mapType == "random")
		{
			// TODO: support random map scripts
			debug_warn(L"Serialization test mode only supports scenarios");
		}
		else
		{
			std::wstring mapFile;
			m_ComponentManager.GetScriptInterface().GetProperty(m_InitAttributes.get(), "map", mapFile);

			VfsPath mapfilename = VfsPath(mapFile).ChangeExtension(L".pmp");
			mapReader->LoadMap(mapfilename, CScriptValRooted(), &secondaryTerrain, NULL, NULL, NULL, NULL, NULL, NULL,
				NULL, NULL, &secondaryContext, INVALID_PLAYER, true); // throws exception on failure
		}
		LDR_EndRegistering();
		ENSURE(LDR_NonprogressiveLoad() == INFO::OK);

		ENSURE(secondaryComponentManager.DeserializeState(primaryStateBefore.state));

		SerializationTestState secondaryStateBefore;
		ENSURE(secondaryComponentManager.SerializeState(secondaryStateBefore.state));
		if (serializationTestDebugDump)
			ENSURE(secondaryComponentManager.DumpDebugState(secondaryStateBefore.debug, false));
		if (serializationTestHash)
			ENSURE(secondaryComponentManager.ComputeStateHash(secondaryStateBefore.hash, false));

		if (primaryStateBefore.state.str() != secondaryStateBefore.state.str() ||
			primaryStateBefore.hash != secondaryStateBefore.hash)
		{
			ReportSerializationFailure(&primaryStateBefore, NULL, &secondaryStateBefore, NULL);
		}

		SerializationTestState primaryStateAfter;
		ENSURE(m_ComponentManager.SerializeState(primaryStateAfter.state));
		if (serializationTestHash)
			ENSURE(m_ComponentManager.ComputeStateHash(primaryStateAfter.hash, false));

		UpdateComponents(secondaryContext, turnLengthFixed,
			CloneCommandsFromOtherContext(m_ComponentManager.GetScriptInterface(), secondaryComponentManager.GetScriptInterface(), commands));

		SerializationTestState secondaryStateAfter;
		ENSURE(secondaryComponentManager.SerializeState(secondaryStateAfter.state));
		if (serializationTestHash)
			ENSURE(secondaryComponentManager.ComputeStateHash(secondaryStateAfter.hash, false));

		if (primaryStateAfter.state.str() != secondaryStateAfter.state.str() ||
			primaryStateAfter.hash != secondaryStateAfter.hash)
		{
			// Only do the (slow) dumping now we know we're going to need to report it
			ENSURE(m_ComponentManager.DumpDebugState(primaryStateAfter.debug, false));
			ENSURE(secondaryComponentManager.DumpDebugState(secondaryStateAfter.debug, false));

			ReportSerializationFailure(&primaryStateBefore, &primaryStateAfter, &secondaryStateBefore, &secondaryStateAfter);
		}
	}

//	if (m_TurnNumber == 0)
//		m_ComponentManager.GetScriptInterface().DumpHeap();

	// Run the GC occasionally
	// (TODO: we ought to schedule this for a frame where we're not
	// running the sim update, to spread the load)
	if (m_TurnNumber % 1 == 0)
		m_ComponentManager.GetScriptInterface().MaybeIncrementalRuntimeGC();

	if (m_EnableOOSLog)
		DumpState();

	// Start computing AI for the next turn
	CmpPtr<ICmpAIManager> cmpAIManager(m_SimContext, SYSTEM_ENTITY);
	if (cmpAIManager)
		cmpAIManager->StartComputation();

	++m_TurnNumber;
}

void CSimulation2Impl::UpdateComponents(CSimContext& simContext, fixed turnLengthFixed, const std::vector<SimulationCommand>& commands)
{
	// TODO: the update process is pretty ugly, with lots of messages and dependencies
	// between different components. Ought to work out a nicer way to do this.

	CComponentManager& componentManager = simContext.GetComponentManager();

	CMessageTurnStart msgTurnStart;
	componentManager.BroadcastMessage(msgTurnStart);

	CmpPtr<ICmpPathfinder> cmpPathfinder(simContext, SYSTEM_ENTITY);
	if (cmpPathfinder)
		cmpPathfinder->FinishAsyncRequests();

	// Push AI commands onto the queue before we use them
	CmpPtr<ICmpAIManager> cmpAIManager(simContext, SYSTEM_ENTITY);
	if (cmpAIManager)
		cmpAIManager->PushCommands();

	CmpPtr<ICmpCommandQueue> cmpCommandQueue(simContext, SYSTEM_ENTITY);
	if (cmpCommandQueue)
		cmpCommandQueue->FlushTurn(commands);

	// Process newly generated move commands so the UI feels snappy
	if (cmpPathfinder)
		cmpPathfinder->ProcessSameTurnMoves();

	// Send all the update phases
	{
		CMessageUpdate msgUpdate(turnLengthFixed);
		componentManager.BroadcastMessage(msgUpdate);
	}
	{
		CMessageUpdate_MotionFormation msgUpdate(turnLengthFixed);
		componentManager.BroadcastMessage(msgUpdate);
	}

	// Process move commands for formations (group proxy)
	if (cmpPathfinder)
		cmpPathfinder->ProcessSameTurnMoves();

	{
		CMessageUpdate_MotionUnit msgUpdate(turnLengthFixed);
		componentManager.BroadcastMessage(msgUpdate);
	}
	{
		CMessageUpdate_Final msgUpdate(turnLengthFixed);
		componentManager.BroadcastMessage(msgUpdate);
	}

	// Process moves resulting from group proxy movement (unit needs to catch up or realign) and any others
	if (cmpPathfinder)
		cmpPathfinder->ProcessSameTurnMoves();

	// Clean up any entities destroyed during the simulation update
	componentManager.FlushDestroyedComponents();
}

void CSimulation2Impl::Interpolate(float simFrameLength, float frameOffset, float realFrameLength)
{
	PROFILE3("sim interpolate");

	m_LastFrameOffset = frameOffset;

	CMessageInterpolate msg(simFrameLength, frameOffset, realFrameLength);
	m_ComponentManager.BroadcastMessage(msg);

	// Clean up any entities destroyed during interpolate (e.g. local corpses)
	m_ComponentManager.FlushDestroyedComponents();
}

void CSimulation2Impl::DumpState()
{
	PROFILE("DumpState");

	std::stringstream pid;
	pid << getpid();
	std::stringstream name;\
	name << std::setw(5) << std::setfill('0') << m_TurnNumber << ".txt";
	OsPath path = psLogDir() / "sim_log" / pid.str() / name.str();
	CreateDirectories(path.Parent(), 0700);
	std::ofstream file (OsString(path).c_str(), std::ofstream::out | std::ofstream::trunc);

	file << "State hash: " << std::hex;
	std::string hashRaw;
	m_ComponentManager.ComputeStateHash(hashRaw, false);
	for (size_t i = 0; i < hashRaw.size(); ++i)
		file << std::setfill('0') << std::setw(2) << (int)(unsigned char)hashRaw[i];
	file << std::dec << "\n";

	file << "\n";

	m_ComponentManager.DumpDebugState(file, true);

	std::ofstream binfile (OsString(path.ChangeExtension(L".dat")).c_str(), std::ofstream::out | std::ofstream::trunc | std::ofstream::binary);
	m_ComponentManager.SerializeState(binfile);
}

////////////////////////////////////////////////////////////////

CSimulation2::CSimulation2(CUnitManager* unitManager, shared_ptr<ScriptRuntime> rt, CTerrain* terrain) :
	m(new CSimulation2Impl(unitManager, rt, terrain))
{
}

CSimulation2::~CSimulation2()
{
	delete m;
}

// Forward all method calls to the appropriate CSimulation2Impl/CComponentManager methods:

void CSimulation2::EnableOOSLog()
{
	m->m_EnableOOSLog = true;
}

void CSimulation2::EnableSerializationTest()
{
	m->m_EnableSerializationTest = true;
}

entity_id_t CSimulation2::AddEntity(const std::wstring& templateName)
{
	return m->m_ComponentManager.AddEntity(templateName, m->m_ComponentManager.AllocateNewEntity());
}

entity_id_t CSimulation2::AddEntity(const std::wstring& templateName, entity_id_t preferredId)
{
	return m->m_ComponentManager.AddEntity(templateName, m->m_ComponentManager.AllocateNewEntity(preferredId));
}

entity_id_t CSimulation2::AddLocalEntity(const std::wstring& templateName)
{
	return m->m_ComponentManager.AddEntity(templateName, m->m_ComponentManager.AllocateNewLocalEntity());
}

void CSimulation2::DestroyEntity(entity_id_t ent)
{
	m->m_ComponentManager.DestroyComponentsSoon(ent);
}

void CSimulation2::FlushDestroyedEntities()
{
	m->m_ComponentManager.FlushDestroyedComponents();
}

IComponent* CSimulation2::QueryInterface(entity_id_t ent, int iid) const
{
	return m->m_ComponentManager.QueryInterface(ent, iid);
}

void CSimulation2::PostMessage(entity_id_t ent, const CMessage& msg) const
{
	m->m_ComponentManager.PostMessage(ent, msg);
}

void CSimulation2::BroadcastMessage(const CMessage& msg) const
{
	m->m_ComponentManager.BroadcastMessage(msg);
}

CSimulation2::InterfaceList CSimulation2::GetEntitiesWithInterface(int iid)
{
	return m->m_ComponentManager.GetEntitiesWithInterface(iid);
}

const CSimulation2::InterfaceListUnordered& CSimulation2::GetEntitiesWithInterfaceUnordered(int iid)
{
	return m->m_ComponentManager.GetEntitiesWithInterfaceUnordered(iid);
}

const CSimContext& CSimulation2::GetSimContext() const
{
	return m->m_SimContext;
}

ScriptInterface& CSimulation2::GetScriptInterface() const
{
	return m->m_ComponentManager.GetScriptInterface();
}

void CSimulation2::ReplaceSkirmishGlobals()
{
	GetScriptInterface().CallFunctionVoid(GetScriptInterface().GetGlobalObject(), "ReplaceSkirmishGlobals");
}

void CSimulation2::InitGame(const CScriptVal& data)
{
	GetScriptInterface().CallFunctionVoid(GetScriptInterface().GetGlobalObject(), "InitGame", data);
}

void CSimulation2::Update(int turnLength)
{
	std::vector<SimulationCommand> commands;
	m->Update(turnLength, commands);
}

void CSimulation2::Update(int turnLength, const std::vector<SimulationCommand>& commands)
{
	m->Update(turnLength, commands);
}

void CSimulation2::Interpolate(float simFrameLength, float frameOffset, float realFrameLength)
{
	m->Interpolate(simFrameLength, frameOffset, realFrameLength);
}

void CSimulation2::RenderSubmit(SceneCollector& collector, const CFrustum& frustum, bool culling)
{
	PROFILE3("sim submit");

	CMessageRenderSubmit msg(collector, frustum, culling);
	m->m_ComponentManager.BroadcastMessage(msg);
}

float CSimulation2::GetLastFrameOffset() const
{
	return m->m_LastFrameOffset;
}

bool CSimulation2::LoadScripts(const VfsPath& path)
{
	return m->LoadScripts(m->m_ComponentManager, &m->m_LoadedScripts, path);
}

bool CSimulation2::LoadDefaultScripts()
{
	return m->LoadDefaultScripts(m->m_ComponentManager, &m->m_LoadedScripts);
}

void CSimulation2::SetStartupScript(const std::string& code)
{
	m->m_StartupScript = code;
}

const std::string& CSimulation2::GetStartupScript()
{
	return m->m_StartupScript;
}

void CSimulation2::SetInitAttributes(const CScriptValRooted& attribs)
{
	m->m_InitAttributes = attribs;
}

CScriptValRooted CSimulation2::GetInitAttributes()
{
	return m->m_InitAttributes;
}

void CSimulation2::SetMapSettings(const std::string& settings)
{
	m->m_MapSettings = m->m_ComponentManager.GetScriptInterface().ParseJSON(settings);
}

void CSimulation2::SetMapSettings(const CScriptValRooted& settings)
{
	m->m_MapSettings = settings;
}

std::string CSimulation2::GetMapSettingsString()
{
	return m->m_ComponentManager.GetScriptInterface().StringifyJSON(m->m_MapSettings.get());
}

CScriptVal CSimulation2::GetMapSettings()
{
	return m->m_MapSettings.get();
}

void CSimulation2::LoadPlayerSettings(bool newPlayers)
{
	GetScriptInterface().CallFunctionVoid(GetScriptInterface().GetGlobalObject(), "LoadPlayerSettings", m->m_MapSettings, newPlayers);
}

void CSimulation2::LoadMapSettings()
{
	// Initialize here instead of in Update()
	GetScriptInterface().CallFunctionVoid(GetScriptInterface().GetGlobalObject(), "LoadMapSettings", m->m_MapSettings);

	if (!m->m_StartupScript.empty())
		GetScriptInterface().LoadScript(L"map startup script", m->m_StartupScript);
	
	// Load the trigger script after we have loaded the simulation and the map.
	if (GetScriptInterface().HasProperty(m->m_MapSettings.get(), "TriggerScripts"))
	{
		std::vector<std::string> scriptNames;
		GetScriptInterface().GetProperty(m->m_MapSettings.get(), "TriggerScripts", scriptNames);
		for (u32 i = 0; i < scriptNames.size(); i++)
		{
			std::string scriptName = "maps/" + scriptNames[i];
			LOGMESSAGE(L"Loading trigger script '%hs'", scriptName.c_str());
			m->m_ComponentManager.LoadScript(scriptName.data());
		}
	}
}

int CSimulation2::ProgressiveLoad()
{
	return m->ProgressiveLoad();
}

Status CSimulation2::ReloadChangedFile(const VfsPath& path)
{
	return m->ReloadChangedFile(path);
}

void CSimulation2::ResetState(bool skipScriptedComponents, bool skipAI)
{
	m->ResetState(skipScriptedComponents, skipAI);
}

bool CSimulation2::ComputeStateHash(std::string& outHash, bool quick)
{
	return m->m_ComponentManager.ComputeStateHash(outHash, quick);
}

bool CSimulation2::DumpDebugState(std::ostream& stream)
{
	return m->m_ComponentManager.DumpDebugState(stream, true);
}

bool CSimulation2::SerializeState(std::ostream& stream)
{
	return m->m_ComponentManager.SerializeState(stream);
}

bool CSimulation2::DeserializeState(std::istream& stream)
{
	// TODO: need to make sure the required SYSTEM_ENTITY components get constructed
	return m->m_ComponentManager.DeserializeState(stream);
}

std::string CSimulation2::GenerateSchema()
{
	return m->m_ComponentManager.GenerateSchema();
}

std::vector<std::string> CSimulation2::GetRMSData()
{
	VfsPath path(L"maps/random/");
	VfsPaths pathnames;

	std::vector<std::string> data;

	// Find all ../maps/random/*.json
	Status ret = vfs::GetPathnames(g_VFS, path, L"*.json", pathnames);
	if (ret == INFO::OK)
	{
		for (VfsPaths::iterator it = pathnames.begin(); it != pathnames.end(); ++it)
		{
			// Load JSON file
			CVFSFile file;
			PSRETURN ret = file.Load(g_VFS, *it);
			if (ret != PSRETURN_OK)
			{
				LOGERROR(L"Failed to load file '%ls': %hs", path.string().c_str(), GetErrorString(ret));
			}
			else
			{
				data.push_back(file.DecodeUTF8()); // assume it's UTF-8
			}
		}
	}
	else
	{
		// Some error reading directory
		wchar_t error[200];
		LOGERROR(L"Error reading directory '%ls': %ls", path.string().c_str(), StatusDescription(ret, error, ARRAY_SIZE(error)));
	}

	return data;
}

std::vector<std::string> CSimulation2::GetCivData()
{
	VfsPath path(L"civs/");
	VfsPaths pathnames;

	std::vector<std::string> data;

	// Load all JSON files in civs directory
	Status ret = vfs::GetPathnames(g_VFS, path, L"*.json", pathnames);
	if (ret == INFO::OK)
	{
		for (VfsPaths::iterator it = pathnames.begin(); it != pathnames.end(); ++it)
		{
			// Load JSON file
			CVFSFile file;
			PSRETURN ret = file.Load(g_VFS, *it);
			if (ret != PSRETURN_OK)
			{
				LOGERROR(L"CSimulation2::GetCivData: Failed to load file '%ls': %hs", path.string().c_str(), GetErrorString(ret));
			}
			else
			{
				data.push_back(file.DecodeUTF8()); // assume it's UTF-8
			}
		}
	}
	else
	{
		// Some error reading directory
		wchar_t error[200];
		LOGERROR(L"CSimulation2::GetCivData: Error reading directory '%ls': %ls", path.string().c_str(), StatusDescription(ret, error, ARRAY_SIZE(error)));
	}

	return data;
}

std::string CSimulation2::GetPlayerDefaults()
{
	return ReadJSON(L"simulation/data/player_defaults.json");
}

std::string CSimulation2::GetMapSizes()
{
	return ReadJSON(L"simulation/data/map_sizes.json");
}

std::string CSimulation2::ReadJSON(VfsPath path)
{
	std::string data;

	if (!VfsFileExists(path))
	{
		LOGERROR(L"File '%ls' does not exist", path.string().c_str());
	}
	else
	{
		// Load JSON file
		CVFSFile file;
		PSRETURN ret = file.Load(g_VFS, path);
		if (ret != PSRETURN_OK)
		{
			LOGERROR(L"Failed to load file '%ls': %hs", path.string().c_str(), GetErrorString(ret));
		}
		else
		{
			data = file.DecodeUTF8(); // assume it's UTF-8
		}
	}

	return data;
}

std::string CSimulation2::GetAIData()
{
	ScriptInterface& scriptInterface = GetScriptInterface();
	std::vector<CScriptValRooted> aiData = ICmpAIManager::GetAIs(scriptInterface);
	
	// Build single JSON string with array of AI data
	CScriptValRooted ais;
	if (!scriptInterface.Eval("({})", ais) || !scriptInterface.SetProperty(ais.get(), "AIData", aiData))
		return std::string();
	
	return scriptInterface.StringifyJSON(ais.get());
}
