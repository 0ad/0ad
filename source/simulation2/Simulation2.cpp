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

#include "Simulation2.h"

#include "simulation2/MessageTypes.h"
#include "simulation2/system/ComponentManager.h"
#include "simulation2/system/ParamNode.h"
#include "simulation2/system/SimContext.h"
#include "simulation2/components/ICmpTemplateManager.h"
#include "simulation2/components/ICmpCommandQueue.h"

#include "lib/file/file_system_util.h"
#include "lib/utf8.h"
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

		// (can't call ResetState here since the scripts haven't been loaded yet)
	}

	~CSimulation2Impl()
	{
		UnregisterFileReloadFunc(ReloadChangedFileCB, this);
	}

	CParamNode LoadXML(const std::wstring& name)
	{
		CParamNode ret;

		VfsPath path = VfsPath(L"simulation/templates/") / name;
		CXeromyces xero;
		PSRETURN ok = xero.Load(g_VFS, path);
		if (ok != PSRETURN_OK)
			return ret; // (Xeromyces already logged an error)

		CParamNode::LoadXML(ret, xero);
		return ret;
	}

	void ResetState(bool skipScriptedComponents)
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
		m_ComponentManager.AddComponent(SYSTEM_ENTITY, CID_Pathfinder, LoadXML(L"special/pathfinder.xml").GetChild("Pathfinder"));
		m_ComponentManager.AddComponent(SYSTEM_ENTITY, CID_ProjectileManager, noParam);
		m_ComponentManager.AddComponent(SYSTEM_ENTITY, CID_RangeManager, noParam);
		m_ComponentManager.AddComponent(SYSTEM_ENTITY, CID_SoundManager, noParam);
		m_ComponentManager.AddComponent(SYSTEM_ENTITY, CID_Terrain, noParam);
		m_ComponentManager.AddComponent(SYSTEM_ENTITY, CID_WaterManager, noParam);

		// Add scripted system components:
		if (!skipScriptedComponents)
		{
#define LOAD_SCRIPTED_COMPONENT(name) \
			cid = m_ComponentManager.LookupCID(name); \
			if (cid == CID__Invalid) \
				LOGERROR(L"Can't find component type " L##name); \
			m_ComponentManager.AddComponent(SYSTEM_ENTITY, cid, noParam)

			LOAD_SCRIPTED_COMPONENT("GuiInterface");
			LOAD_SCRIPTED_COMPONENT("PlayerManager");
			LOAD_SCRIPTED_COMPONENT("Timer");

#undef LOAD_SCRIPTED_COMPONENT
		}
	}

	bool LoadScripts(const VfsPath& path);
	LibError ReloadChangedFile(const VfsPath& path);

	static LibError ReloadChangedFileCB(void* param, const VfsPath& path)
	{
		return static_cast<CSimulation2Impl*>(param)->ReloadChangedFile(path);
	}

	bool Update(int turnLength, const std::vector<SimulationCommand>& commands);
	void Interpolate(float frameLength, float frameOffset);

	void DumpState();

	CSimContext m_SimContext;
	CComponentManager m_ComponentManager;
	double m_DeltaTime;
	float m_LastFrameOffset;

	std::wstring m_StartupScript;
	CScriptValRooted m_MapSettings;

	std::set<std::wstring> m_LoadedScripts;

	uint32_t m_TurnNumber;

	bool m_EnableOOSLog;
};

bool CSimulation2Impl::LoadScripts(const VfsPath& path)
{
	VfsPaths pathnames;
	if (fs_util::GetPathnames(g_VFS, path, L"*.js", pathnames) < 0)
		return false;

	bool ok = true;
	for (VfsPaths::iterator it = pathnames.begin(); it != pathnames.end(); ++it)
	{
		std::wstring filename = it->string();
		m_LoadedScripts.insert(filename);
		LOGMESSAGE(L"Loading simulation script '%ls'", filename.c_str());
		if (! m_ComponentManager.LoadScript(filename))
			ok = false;
	}
	return ok;
}

LibError CSimulation2Impl::ReloadChangedFile(const VfsPath& path)
{
	const std::wstring& filename = path.string();

	// Ignore if this file wasn't loaded as a script
	// (TODO: Maybe we ought to load in any new .js files that are created in the right directories)
	if (m_LoadedScripts.find(filename) == m_LoadedScripts.end())
		return INFO::OK;

	// If the file doesn't exist (e.g. it was deleted), don't bother loading it since that'll give an error message.
	// (Also don't bother trying to 'unload' it from the component manager, because that's not possible)
	if (!FileExists(path))
		return INFO::OK;

	LOGMESSAGE(L"Reloading simulation script '%ls'", filename.c_str());
	if (!m_ComponentManager.LoadScript(filename, true))
		return ERR::FAIL;

	return INFO::OK;
}

bool CSimulation2Impl::Update(int turnLength, const std::vector<SimulationCommand>& commands)
{
	fixed turnLengthFixed = fixed::FromInt(turnLength) / 1000;

	// TODO: the update process is pretty ugly, with lots of messages and dependencies
	// between different components. Ought to work out a nicer way to do this.

	CMessageTurnStart msgTurnStart;
	m_ComponentManager.BroadcastMessage(msgTurnStart);

	if (m_TurnNumber == 0)
	{
		ScriptInterface& scriptInterface = m_ComponentManager.GetScriptInterface();
		CScriptVal ret;
		scriptInterface.CallFunction(scriptInterface.GetGlobalObject(), "LoadMapSettings", m_MapSettings, ret);

		if (!m_StartupScript.empty())
			m_ComponentManager.GetScriptInterface().LoadScript(L"map startup script", m_StartupScript);
	}

	CmpPtr<ICmpPathfinder> cmpPathfinder(m_SimContext, SYSTEM_ENTITY);
	if (!cmpPathfinder.null())
		cmpPathfinder->FinishAsyncRequests();

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

	if (m_EnableOOSLog)
		DumpState();

	++m_TurnNumber;

	return true; // TODO: don't bother with bool return
}

void CSimulation2Impl::Interpolate(float frameLength, float frameOffset)
{
	m_LastFrameOffset = frameOffset;

	CMessageInterpolate msg(frameLength, frameOffset);
	m_ComponentManager.BroadcastMessage(msg);
}

void CSimulation2Impl::DumpState()
{
	PROFILE("DumpState");

	std::wstringstream name;
	name << L"sim_log/" << getpid() << L"/" << std::setw(5) << std::setfill(L'0') << m_TurnNumber << L".txt";
	fs::wpath path (psLogDir()/name.str());
	CreateDirectories(path.branch_path(), 0700);
	std::ofstream file (path.external_file_string().c_str(), std::ofstream::out | std::ofstream::trunc);

	file << "State hash: " << std::hex;
	std::string hashRaw;
	m_ComponentManager.ComputeStateHash(hashRaw);
	for (size_t i = 0; i < hashRaw.size(); ++i)
		file << std::setfill('0') << std::setw(2) << (int)(unsigned char)hashRaw[i];
	file << std::dec << "\n";

	file << "\n";

	m_ComponentManager.DumpDebugState(file);

	std::ofstream binfile (change_extension(path, L".dat").external_file_string().c_str(), std::ofstream::out | std::ofstream::trunc | std::ofstream::binary);
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

const CSimulation2::InterfaceList& CSimulation2::GetEntitiesWithInterface(int iid)
{
	return m->m_ComponentManager.GetEntitiesWithInterface(iid);
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

void CSimulation2::SetMapSettings(const utf16string& settings)
{
	m->m_MapSettings = m->m_ComponentManager.GetScriptInterface().ParseJSON(settings);
}

std::string CSimulation2::GetMapSettings()
{
	return m->m_ComponentManager.GetScriptInterface().StringifyJSON(m->m_MapSettings.get());
}

LibError CSimulation2::ReloadChangedFile(const VfsPath& path)
{
	return m->ReloadChangedFile(path);
}

void CSimulation2::ResetState(bool skipGui)
{
	m->ResetState(skipGui);
}

bool CSimulation2::ComputeStateHash(std::string& outHash)
{
	return m->m_ComponentManager.ComputeStateHash(outHash);
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
