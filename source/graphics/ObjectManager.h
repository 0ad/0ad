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

class CEntityTemplate;
class CMatrix3D;
class CMeshManager;
class CObjectBase;
class CObjectEntry;
class CSkeletonAnimManager;

///////////////////////////////////////////////////////////////////////////////////////////
// CObjectManager: manager class for all possible actor types
class CObjectManager
{
	NONCOPYABLE(CObjectManager);
public:
	struct ObjectKey
	{
		ObjectKey(const CStr& name, const std::vector<u8>& var)
			: ActorName(name), ActorVariation(var) {}

		CStr ActorName;
		std::vector<u8> ActorVariation;

		bool operator< (const CObjectManager::ObjectKey& a) const;
	};

public:

	// constructor, destructor
	CObjectManager(CMeshManager& meshManager, CSkeletonAnimManager& skeletonAnimManager);
	~CObjectManager();

	// Provide access to the manager classes for meshes and animations - they're
	// needed when objects are being created and so this seems like a convenient
	// place to centralise access.
	CMeshManager& GetMeshManager() const { return m_MeshManager; }
	CSkeletonAnimManager& GetSkeletonAnimManager() const { return m_SkeletonAnimManager; }

	void UnloadObjects();

	CObjectEntry* FindObject(const wchar_t* objname);
	void DeleteObject(CObjectEntry* entry);
	
	CObjectBase* FindObjectBase(const wchar_t* objname);

	CObjectEntry* FindObjectVariation(const wchar_t* objname, const std::vector<std::set<CStr> >& selections);
	CObjectEntry* FindObjectVariation(CObjectBase* base, const std::vector<std::set<CStr> >& selections);

	// Get all names, quite slowly. (Intended only for Atlas.)
	static void GetAllObjectNames(std::vector<CStr>& names);
	static void GetPropObjectNames(std::vector<CStr>& names);

private:
	CMeshManager& m_MeshManager;
	CSkeletonAnimManager& m_SkeletonAnimManager;

	std::map<ObjectKey, CObjectEntry*> m_Objects;
	std::map<CStrW, CObjectBase*> m_ObjectBases;
};

#endif
