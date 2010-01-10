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
	 */
	bool LoadScripts(const VfsPath& path);

	/**
	 * Reload any scripts that were loaded from the given filename.
	 * (This is used to implement hotloading.)
	 */
	LibError ReloadChangedFile(const VfsPath& path);

	/**
	 * Initialise (or re-initialise) the complete simulation state.
	 * Must be called after LoadScripts, and must be called
	 * before any methods that depend on the simulation state.
	 * @param skipGui don't load the GUI interface components (this is intended for use by test cases)
	 */
	void ResetState(bool skipGui = false);

	void Update(float frameTime);
	void Interpolate(float frameTime);

	entity_id_t AddEntity(const std::wstring& templateName);
	entity_id_t AddEntity(const std::wstring& templateName, entity_id_t preferredId);
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

	entity_id_t AllocateNewEntity();

private:
	CSimulation2Impl* m;

	NONCOPYABLE(CSimulation2);
};

#endif // INCLUDED_SIMULATION2
