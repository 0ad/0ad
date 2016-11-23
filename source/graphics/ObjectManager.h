/* Copyright (C) 2009 Wildfire Games.
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

#ifndef INCLUDED_OBJECTMANAGER
#define INCLUDED_OBJECTMANAGER

#include <vector>
#include <map>
#include <set>

#include "ps/CStr.h"
#include "lib/file/vfs/vfs_path.h"

class CMeshManager;
class CObjectBase;
class CObjectEntry;
class CSkeletonAnimManager;
class CSimulation2;
class CTerrain;

///////////////////////////////////////////////////////////////////////////////////////////
// CObjectManager: manager class for all possible actor types
class CObjectManager
{
	NONCOPYABLE(CObjectManager);
public:
	// Unique identifier of an actor variation
	struct ObjectKey
	{
		ObjectKey(const CStrW& name, const std::vector<u8>& var)
			: ActorName(name), ActorVariation(var) {}

		bool operator< (const CObjectManager::ObjectKey& a) const;

	private:
		CStrW ActorName;
		std::vector<u8> ActorVariation;
	};

public:

	// constructor, destructor
	CObjectManager(CMeshManager& meshManager, CSkeletonAnimManager& skeletonAnimManager, CSimulation2& simulation);
	~CObjectManager();

	// Provide access to the manager classes for meshes and animations - they're
	// needed when objects are being created and so this seems like a convenient
	// place to centralise access.
	CMeshManager& GetMeshManager() const { return m_MeshManager; }
	CSkeletonAnimManager& GetSkeletonAnimManager() const { return m_SkeletonAnimManager; }

	void UnloadObjects();

	CObjectEntry* FindObject(const CStrW& objname);
	void DeleteObject(CObjectEntry* entry);

	CObjectBase* FindObjectBase(const CStrW& objname);

	CObjectEntry* FindObjectVariation(const CStrW& objname, const std::vector<std::set<CStr> >& selections);
	CObjectEntry* FindObjectVariation(CObjectBase* base, const std::vector<std::set<CStr> >& selections);

	/**
	 * Get the terrain object that actors managed by this manager should be linked
	 * with (primarily for the purpose of decals)
	 */
	CTerrain* GetTerrain();

	/**
	 * Reload any scripts that were loaded from the given filename.
	 * (This is used to implement hotloading.)
	 */
	Status ReloadChangedFile(const VfsPath& path);

private:
	CMeshManager& m_MeshManager;
	CSkeletonAnimManager& m_SkeletonAnimManager;
	CSimulation2& m_Simulation;

	std::map<ObjectKey, CObjectEntry*> m_Objects;
	std::map<CStrW, CObjectBase*> m_ObjectBases;
};

#endif
