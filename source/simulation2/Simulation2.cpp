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
#include "maths/MathUtil.h"
#include "ps/CLogger.h"
#include "ps/Filesystem.h"

bool g_UseSimulation2 = false;

class CSimulation2Impl
{
public:
	CSimulation2Impl(CUnitManager* unitManager, CTerrain* terrain) :
		m_SimContext(), m_ComponentManager(m_SimContext)
	{
		m_SimContext.m_ComponentManager = &m_ComponentManager;
		m_SimContext.m_UnitManager = unitManager;
		m_SimContext.m_Terrain = terrain;
		m_ComponentManager.LoadComponentTypes();

		// (can't call ResetState here since the scripts haven't been loaded yet)
	}

	void ResetState(bool skipGui)
	{
		m_ComponentManager.ResetState();

		m_DeltaTime = 0.0;

		CParamNode noParam;
		CComponentManager::ComponentTypeId cid;

		// Add native system components:
		m_ComponentManager.AddComponent(SYSTEM_ENTITY, CID_TemplateManager, noParam);
		m_ComponentManager.AddComponent(SYSTEM_ENTITY, CID_Terrain, noParam);
		m_ComponentManager.AddComponent(SYSTEM_ENTITY, CID_CommandQueue, noParam);

		// Add scripted system components:
		if (!skipGui)
		{
			cid = m_ComponentManager.LookupCID("GuiInterface");
			if (cid == CID__Invalid)
				LOGERROR(L"Can't find component type GuiInterface");
			m_ComponentManager.AddComponent(SYSTEM_ENTITY, cid, noParam);
		}
	}

	bool LoadScripts(const VfsPath& path);
	LibError ReloadChangedFile(const VfsPath& path);

	void AddComponent(entity_id_t ent, EComponentTypeId cid, const CParamNode& paramNode);

	entity_id_t AddEntity(const std::wstring& templateName, entity_id_t ent);

	void Update(float frameTime);
	void Interpolate(float frameTime);

	CSimContext m_SimContext;
	CComponentManager m_ComponentManager;
	double m_DeltaTime;

	std::set<std::wstring> m_LoadedScripts;

	static const int TURN_LENGTH = 300; // TODO: Use CTurnManager
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

void CSimulation2Impl::AddComponent(entity_id_t ent, EComponentTypeId cid, const CParamNode& paramNode)
{
	m_ComponentManager.AddComponent(ent, cid, paramNode);
}

entity_id_t CSimulation2Impl::AddEntity(const std::wstring& templateName, entity_id_t ent)
{
	CmpPtr<ICmpTemplateManager> tempMan(m_SimContext, SYSTEM_ENTITY);
	debug_assert(!tempMan.null());

	// TODO: should assert that ent doesn't exist

	const CParamNode* tmpl = tempMan->LoadTemplate(ent, templateName, -1);
	if (!tmpl)
		return INVALID_ENTITY; // LoadTemplate will have reported the error

	// Construct a component for each child of the root element
	const CParamNode::ChildrenMap& tmplChilds = tmpl->GetChildren();
	for (CParamNode::ChildrenMap::const_iterator it = tmplChilds.begin(); it != tmplChilds.end(); ++it)
	{
		// Ignore attributes on the root element
		if (it->first.length() && it->first[0] == '@')
			continue;

		CComponentManager::ComponentTypeId cid = m_ComponentManager.LookupCID(it->first);
		if (cid == CID__Invalid)
		{
			LOGERROR(L"Unrecognised component type name '%hs' in entity template '%ls'", it->first.c_str(), templateName.c_str());
			return INVALID_ENTITY;
		}

		if (!m_ComponentManager.AddComponent(ent, cid, it->second))
		{
			LOGERROR(L"Failed to construct component type name '%hs' in entity template '%ls'", it->first.c_str(), templateName.c_str());
			return INVALID_ENTITY;
		}

		// TODO: maybe we should delete already-constructed components if one of them fails?
	}

	return ent;
}

void CSimulation2Impl::Update(float frameTime)
{
	// TODO: Use CTurnManager
	m_DeltaTime += frameTime;
	if (m_DeltaTime >= 0.0)
	{
		double turnLength = TURN_LENGTH / 1000.0;
		CFixed_23_8 turnLengthFixed = CFixed_23_8::FromInt(TURN_LENGTH) / 1000;
		m_DeltaTime -= turnLength;

		m_ComponentManager.BroadcastMessage(CMessageTurnStart());

		CmpPtr<ICmpCommandQueue> cmpCommandQueue(m_SimContext, SYSTEM_ENTITY);
		if (!cmpCommandQueue.null())
			cmpCommandQueue->ProcessCommands();

		m_ComponentManager.BroadcastMessage(CMessageUpdate(turnLengthFixed));
	}
}

void CSimulation2Impl::Interpolate(float UNUSED(frameTime))
{
	// TODO: Use CTurnManager
	double turnLength = TURN_LENGTH / 1000.0;
	float offset = clamp(m_DeltaTime / turnLength + 1.0, 0.0, 1.0);
	m_ComponentManager.BroadcastMessage(CMessageInterpolate(offset));
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

entity_id_t CSimulation2::AddEntity(const std::wstring& templateName)
{
	return m->AddEntity(templateName, m->m_ComponentManager.AllocateNewEntity());
}

entity_id_t CSimulation2::AddEntity(const std::wstring& templateName, entity_id_t preferredId)
{
	return m->AddEntity(templateName, m->m_ComponentManager.AllocateNewEntity(preferredId));
}

entity_id_t CSimulation2::AddLocalEntity(const std::wstring& templateName)
{
	return m->AddEntity(templateName, m->m_ComponentManager.AllocateNewLocalEntity());
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

void CSimulation2::Update(float frameTime)
{
	m->Update(frameTime);
}

void CSimulation2::Interpolate(float frameTime)
{
	m->Interpolate(frameTime);
}

bool CSimulation2::LoadScripts(const VfsPath& path)
{
	return m->LoadScripts(path);
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
