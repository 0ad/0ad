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

#include "Simulation2.h"

#include "simulation2/MessageTypes.h"
#include "simulation2/system/ComponentManager.h"
#include "simulation2/system/ParamNode.h"
#include "simulation2/system/SimContext.h"
#include "simulation2/components/ICmpAIManager.h"
#include "simulation2/components/ICmpCommandQueue.h"
#include "simulation2/components/ICmpTemplateManager.h"

#include "lib/timer.h"
#include "lib/file/vfs/vfs_util.h"
#include "maths/MathUtil.h"
#include "ps/CLogger.h"
#include "ps/Filesystem.h"
#include "ps/Profile.h"
#include "ps/Pyrogenesis.h"
#include "ps/XML/Xeromyces.h"

#include <iomanip>

#if MSC_VERSION
#include <process.h>
#define getpid _getpid // use the non-deprecated function name
#endif

class CSimulation2Impl
{
public:
	CSimulation2Impl(CUnitManager* unitManager, CTerrain* terrain) :
		m_SimContext(), m_ComponentManager(m_SimContext), m_EnableOOSLog(false)
	{
		m_SimContext.m_UnitManager = unitManager;
		m_SimContext.m_Terrain = terrain;
		m_ComponentManager.LoadComponentTypes();

		RegisterFileReloadFunc(ReloadChangedFileCB, this);

//		m_EnableOOSLog = true; // TODO: this should be a command-line flag or similar
	}

	~CSimulation2Impl()
	{
		UnregisterFileReloadFunc(ReloadChangedFileCB, this);
	}

	void ResetState(bool skipScriptedComponents, bool skipAI)
	{
		m_ComponentManager.ResetState();

		m_DeltaTime = 0.0;
		m_LastFrameOffset = 0.0f;
		m_TurnNumber = 0;

		CParamNode noParam;
		CComponentManager::ComponentTypeId cid;

		// Add native system components:
		m_ComponentManager.AddComponent(SYSTEM_ENTITY, CID_TemplateManager, noParam);

		m_ComponentManager.AddComponent(SYSTEM_ENTITY, CID_CommandQueue, noParam);
		m_ComponentManager.AddComponent(SYSTEM_ENTITY, CID_ObstructionManager, noParam);
		m_ComponentManager.AddComponent(SYSTEM_ENTITY, CID_Pathfinder, noParam);
		m_ComponentManager.AddComponent(SYSTEM_ENTITY, CID_ProjectileManager, noParam);
		m_ComponentManager.AddComponent(SYSTEM_ENTITY, CID_RangeManager, noParam);
		m_ComponentManager.AddComponent(SYSTEM_ENTITY, CID_SoundManager, noParam);
		m_ComponentManager.AddComponent(SYSTEM_ENTITY, CID_Terrain, noParam);
		m_ComponentManager.AddComponent(SYSTEM_ENTITY, CID_WaterManager, noParam);

		if (!skipAI)
		{
			m_ComponentManager.AddComponent(SYSTEM_ENTITY, CID_AIManager, noParam);
		}

		// Add scripted system components:
		if (!skipScriptedComponents)
		{
#define LOAD_SCRIPTED_COMPONENT(name) \
			cid = m_ComponentManager.LookupCID(name); \
			if (cid == CID__Invalid) \
				LOGERROR(L"Can't find component type " L##name); \
			m_ComponentManager.AddComponent(SYSTEM_ENTITY, cid, noParam)

			LOAD_SCRIPTED_COMPONENT("AIInterface");
			LOAD_SCRIPTED_COMPONENT("EndGameManager");
			LOAD_SCRIPTED_COMPONENT("GuiInterface");
			LOAD_SCRIPTED_COMPONENT("PlayerManager");
			LOAD_SCRIPTED_COMPONENT("Timer");

#undef LOAD_SCRIPTED_COMPONENT
		}
	}

	bool LoadScripts(const VfsPath& path);
	Status ReloadChangedFile(const VfsPath& path);

	static Status ReloadChangedFileCB(void* param, const VfsPath& path)
	{
		return static_cast<CSimulation2Impl*>(param)->ReloadChangedFile(path);
	}

	int ProgressiveLoad();
	bool Update(int turnLength, const std::vector<SimulationCommand>& commands);
	void Interpolate(float frameLength, float frameOffset);

	void DumpState();

	CSimContext m_SimContext;
	CComponentManager m_ComponentManager;
	double m_DeltaTime;
	float m_LastFrameOffset;

	std::wstring m_StartupScript;
	CScriptValRooted m_MapSettings;

	std::set<VfsPath> m_LoadedScripts;

	uint32_t m_TurnNumber;

	bool m_EnableOOSLog;
};

bool CSimulation2Impl::LoadScripts(const VfsPath& path)
{
	VfsPaths pathnames;
	if (vfs::GetPathnames(g_VFS, path, L"*.js", pathnames) < 0)
		return false;

	bool ok = true;
	for (VfsPaths::iterator it = pathnames.begin(); it != pathnames.end(); ++it)
	{
		VfsPath filename = *it;
		m_LoadedScripts.insert(filename);
		LOGMESSAGE(L"Loading simulation script '%ls'", filename.string().c_str());
		if (! m_ComponentManager.LoadScript(filename))
			ok = false;
	}
	return ok;
}

Status CSimulation2Impl::ReloadChangedFile(const VfsPath& path)
{
	const VfsPath& filename = path;

	// Ignore if this file wasn't loaded as a script
	// (TODO: Maybe we ought to load in any new .js files that are created in the right directories)
	if (m_LoadedScripts.find(filename) == m_LoadedScripts.end())
		return INFO::OK;

	// If the file doesn't exist (e.g. it was deleted), don't bother loading it since that'll give an error message.
	// (Also don't bother trying to 'unload' it from the component manager, because that's not possible)
	if (!VfsFileExists(path))
		return INFO::OK;

	LOGMESSAGE(L"Reloading simulation script '%ls'", filename.string().c_str());
	if (!m_ComponentManager.LoadScript(filename, true))
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

bool CSimulation2Impl::Update(int turnLength, const std::vector<SimulationCommand>& commands)
{
	fixed turnLengthFixed = fixed::FromInt(turnLength) / 1000;

	// TODO: the update process is pretty ugly, with lots of messages and dependencies
	// between different components. Ought to work out a nicer way to do this.

	CMessageTurnStart msgTurnStart;
	m_ComponentManager.BroadcastMessage(msgTurnStart);

	CmpPtr<ICmpPathfinder> cmpPathfinder(m_SimContext, SYSTEM_ENTITY);
	if (!cmpPathfinder.null())
		cmpPathfinder->FinishAsyncRequests();

	// Push AI commands onto the queue before we use them
	CmpPtr<ICmpAIManager> cmpAIManager(m_SimContext, SYSTEM_ENTITY);
	if (!cmpAIManager.null())
		cmpAIManager->PushCommands();

	CmpPtr<ICmpCommandQueue> cmpCommandQueue(m_SimContext, SYSTEM_ENTITY);
	if (!cmpCommandQueue.null())
		cmpCommandQueue->FlushTurn(commands);

	// Send all the update phases
	{
		CMessageUpdate msgUpdate(turnLengthFixed);
		m_ComponentManager.BroadcastMessage(msgUpdate);
	}
	{
		CMessageUpdate_MotionFormation msgUpdate(turnLengthFixed);
		m_ComponentManager.BroadcastMessage(msgUpdate);
	}
	{
		CMessageUpdate_MotionUnit msgUpdate(turnLengthFixed);
		m_ComponentManager.BroadcastMessage(msgUpdate);
	}
	{
		CMessageUpdate_Final msgUpdate(turnLengthFixed);
		m_ComponentManager.BroadcastMessage(msgUpdate);
	}

	// Clean up any entities destroyed during the simulation update
	m_ComponentManager.FlushDestroyedComponents();

//	if (m_TurnNumber == 0)
//		m_ComponentManager.GetScriptInterface().DumpHeap();

	// Run the GC occasionally
	// (TODO: we ought to schedule this for a frame where we're not
	// running the sim update, to spread the load)
	if (m_TurnNumber % 10 == 0)
		m_ComponentManager.GetScriptInterface().MaybeGC();

	if (m_EnableOOSLog)
		DumpState();

	// Start computing AI for the next turn
	if (!cmpAIManager.null())
		cmpAIManager->StartComputation();

	++m_TurnNumber;

	return true; // TODO: don't bother with bool return
}

void CSimulation2Impl::Interpolate(float frameLength, float frameOffset)
{
	m_LastFrameOffset = frameOffset;

	CMessageInterpolate msg(frameLength, frameOffset);
	m_ComponentManager.BroadcastMessage(msg);

	// Clean up any entities destroyed during interpolate (e.g. local corpses)
	m_ComponentManager.FlushDestroyedComponents();
}

void CSimulation2Impl::DumpState()
{
	PROFILE("DumpState");

	std::wstringstream name;
	name << L"sim_log/" << getpid() << L"/" << std::setw(5) << std::setfill(L'0') << m_TurnNumber << L".txt";
	OsPath path = psLogDir() / name.str();
	CreateDirectories(path.Parent(), 0700);
	std::ofstream file (OsString(path).c_str(), std::ofstream::out | std::ofstream::trunc);

	file << "State hash: " << std::hex;
	std::string hashRaw;
	m_ComponentManager.ComputeStateHash(hashRaw, false);
	for (size_t i = 0; i < hashRaw.size(); ++i)
		file << std::setfill('0') << std::setw(2) << (int)(unsigned char)hashRaw[i];
	file << std::dec << "\n";

	file << "\n";

	m_ComponentManager.DumpDebugState(file);

	std::ofstream binfile (OsString(path.ChangeExtension(L".dat")).c_str(), std::ofstream::out | std::ofstream::trunc | std::ofstream::binary);
	m_ComponentManager.SerializeState(binfile);
}

////////////////////////////////////////////////////////////////

CSimulation2::CSimulation2(CUnitManager* unitManager, CTerrain* terrain) :
	m(new CSimulation2Impl(unitManager, terrain))
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

void CSimulation2::InitGame(const CScriptVal& data)
{
	CScriptVal ret; // ignored
	GetScriptInterface().CallFunction(GetScriptInterface().GetGlobalObject(), "InitGame", data, ret);
}

bool CSimulation2::Update(int turnLength)
{
	std::vector<SimulationCommand> commands;
	return m->Update(turnLength, commands);
}

bool CSimulation2::Update(int turnLength, const std::vector<SimulationCommand>& commands)
{
	return m->Update(turnLength, commands);
}

void CSimulation2::Interpolate(float frameLength, float frameOffset)
{
	m->Interpolate(frameLength, frameOffset);
}

void CSimulation2::RenderSubmit(SceneCollector& collector, const CFrustum& frustum, bool culling)
{
	CMessageRenderSubmit msg(collector, frustum, culling);
	m->m_ComponentManager.BroadcastMessage(msg);
}

float CSimulation2::GetLastFrameOffset() const
{
	return m->m_LastFrameOffset;
}

bool CSimulation2::LoadScripts(const VfsPath& path)
{
	return m->LoadScripts(path);
}

bool CSimulation2::LoadDefaultScripts()
{
	return (
		m->LoadScripts(L"simulation/components/interfaces/") &&
		m->LoadScripts(L"simulation/helpers/") &&
		m->LoadScripts(L"simulation/components/")
	);
}

void CSimulation2::SetStartupScript(const std::wstring& code)
{
	m->m_StartupScript = code;
}

const std::wstring& CSimulation2::GetStartupScript()
{
	return m->m_StartupScript;
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

void CSimulation2::LoadPlayerSettings()
{
	GetScriptInterface().CallFunctionVoid(GetScriptInterface().GetGlobalObject(), "LoadPlayerSettings", m->m_MapSettings);
}

void CSimulation2::LoadMapSettings()
{
	// Initialize here instead of in Update()
	GetScriptInterface().CallFunctionVoid(GetScriptInterface().GetGlobalObject(), "LoadMapSettings", m->m_MapSettings);

	if (!m->m_StartupScript.empty())
		GetScriptInterface().LoadScript(L"map startup script", m->m_StartupScript);
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
	return m->m_ComponentManager.DumpDebugState(stream);
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
				data.push_back(std::string(file.GetBuffer(), file.GetBuffer() + file.GetBufferSize()));
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
				data.push_back(std::string(file.GetBuffer(), file.GetBuffer() + file.GetBufferSize()));
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
			data = std::string(file.GetBuffer(), file.GetBuffer() + file.GetBufferSize());
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
