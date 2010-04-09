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

#ifndef INCLUDED_SIMULATION2
#define INCLUDED_SIMULATION2

#include "simulation2/system/CmpPtr.h"
#include "simulation2/system/Components.h"

#include "lib/file/vfs/vfs_path.h"

#include <map>

class CSimulation2Impl;
class CSimContext;
class CUnitManager;
class CTerrain;
class IComponent;
class ScriptInterface;
class CMessage;
class CScriptVal;
class SceneCollector;
class CFrustum;

// Hopefully-temporary flag for transition to new simulation system
extern bool g_UseSimulation2;

/**
 * Public API for simulation system.
 */
class CSimulation2
{
public:
	// TODO: CUnitManager should probably be handled automatically by this
	// module, but for now we'll have it passed in externally instead
	CSimulation2(CUnitManager*, CTerrain*);
	~CSimulation2();

	/**
	 * Load all scripts in the specified directory (non-recursively),
	 * so they can register new component types and functions. This
	 * should be called immediately after constructing the CSimulation2 object.
	 * @return false on failure
	 */
	bool LoadScripts(const VfsPath& path);

	/**
	 * Call LoadScripts for each of the game's standard simulation script paths.
	 * @return false on failure
	 */
	bool LoadDefaultScripts();

	/**
	 * Reload any scripts that were loaded from the given filename.
	 * (This is used to implement hotloading.)
	 */
	LibError ReloadChangedFile(const VfsPath& path);

	/**
	 * Initialise (or re-initialise) the complete simulation state.
	 * Must be called after LoadScripts, and must be called
	 * before any methods that depend on the simulation state.
	 * @param skipScriptedComponents don't load the scripted system components
	 *   (this is intended for use by test cases that don't mount all of VFS)
	 */
	void ResetState(bool skipScriptedComponents = false);

	/**
	 * Initialise a new game, based on some script data.
	 * (This mustn't be used when e.g. loading saved games, only when starting new ones.)
	 * This calls the InitGame function defined in helpers/InitGame.js.
	 */
	void InitGame(const CScriptVal& data);

	void Update(float frameTime);
	void Interpolate(float frameTime);
	void RenderSubmit(SceneCollector& collector, const CFrustum& frustum, bool culling);

	/**
	 * Construct a new entity and add it to the world.
	 * @param templateName see ICmpTemplateManager for syntax
	 * @return the new entity ID, or INVALID_ENTITY on error
	 */
	entity_id_t AddEntity(const std::wstring& templateName);
	entity_id_t AddEntity(const std::wstring& templateName, entity_id_t preferredId);
	entity_id_t AddLocalEntity(const std::wstring& templateName);

	/**
	 * Destroys the specified entity, once FlushDestroyedEntities is called.
	 * Has no effect if the entity does not exist, or has already been added to the destruction queue.
	 */
	void DestroyEntity(entity_id_t ent);

	/**
	 * Does the actual destruction of entities from DestroyEntity.
	 * This is called automatically by Update, but should also be called at other
	 * times when an entity might have been deleted and should be removed from
	 * any further processing (e.g. after editor UI message processing)
	 */
	void FlushDestroyedEntities();

	IComponent* QueryInterface(entity_id_t ent, int iid) const;
	void PostMessage(entity_id_t ent, const CMessage& msg) const;
	void BroadcastMessage(const CMessage& msg) const;

	typedef std::map<entity_id_t, IComponent*> InterfaceList;
	const InterfaceList& GetEntitiesWithInterface(int iid);

	const CSimContext& GetSimContext() const;
	ScriptInterface& GetScriptInterface() const;

	bool ComputeStateHash(std::string& outHash);
	bool DumpDebugState(std::ostream& stream);
	bool SerializeState(std::ostream& stream);
	bool DeserializeState(std::istream& stream);

	std::string GenerateSchema();

private:
	CSimulation2Impl* m;

	NONCOPYABLE(CSimulation2);
};

#endif // INCLUDED_SIMULATION2
