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

#include "precompiled.h"

#include "ObjectManager.h"

#include "graphics/ObjectBase.h"
#include "graphics/ObjectEntry.h"
#include "ps/CLogger.h"
#include "ps/Profile.h"
#include "ps/Filesystem.h"

#define LOG_CATEGORY L"graphics"

template<typename T, typename S>
static void delete_pair_2nd(std::pair<T,S> v)
{
	delete v.second;
}

template<typename T>
struct second_equals
{
	T x;
	second_equals(const T& x) : x(x) {}
	template<typename S> bool operator()(const S& v) { return v.second == x; }
};

bool CObjectManager::ObjectKey::operator< (const CObjectManager::ObjectKey& a) const
{
	if (ActorName < a.ActorName)
		return true;
	else if (ActorName > a.ActorName)
		return false;
	else
		return ActorVariation < a.ActorVariation;
}

CObjectManager::CObjectManager(CMeshManager& meshManager, CSkeletonAnimManager& skeletonAnimManager)
: m_MeshManager(meshManager), m_SkeletonAnimManager(skeletonAnimManager)
{
}

CObjectManager::~CObjectManager()
{
	UnloadObjects();
}


CObjectBase* CObjectManager::FindObjectBase(const wchar_t* objectname)
{
	debug_assert(objectname[0] != '\0');

	// See if the base type has been loaded yet:

	std::map<CStrW, CObjectBase*>::iterator it = m_ObjectBases.find(objectname);
	if (it != m_ObjectBases.end())
		return it->second;

	// Not already loaded, so try to load it:

	CObjectBase* obj = new CObjectBase(*this);

	if (obj->Load(objectname))
	{
		m_ObjectBases[objectname] = obj;
		return obj;
	}
	else
		delete obj;

	LOG(CLogger::Error, LOG_CATEGORY, L"CObjectManager::FindObjectBase(): Cannot find object '%ls'", objectname);

	return 0;
}

CObjectEntry* CObjectManager::FindObject(const wchar_t* objname)
{
	std::vector<std::set<CStr> > selections; // TODO - should this really be empty?
	return FindObjectVariation(objname, selections);
}

CObjectEntry* CObjectManager::FindObjectVariation(const wchar_t* objname, const std::vector<std::set<CStr> >& selections)
{
	CObjectBase* base = FindObjectBase(objname);

	if (! base)
		return NULL;

	return FindObjectVariation(base, selections);
}

CObjectEntry* CObjectManager::FindObjectVariation(CObjectBase* base, const std::vector<std::set<CStr> >& selections)
{
	PROFILE( "object variation loading" );

	// Look to see whether this particular variation has already been loaded

	std::vector<u8> choices = base->CalculateVariationKey(selections);
	ObjectKey key (base->m_Name, choices);

	std::map<ObjectKey, CObjectEntry*>::iterator it = m_Objects.find(key);
	if (it != m_Objects.end())
		return it->second;

	// If it hasn't been loaded, load it now

	CObjectEntry* obj = new CObjectEntry(base); // TODO: type ?

	// TODO (for some efficiency): use the pre-calculated choices for this object,
	// which has already worked out what to do for props, instead of passing the
	// selections into BuildVariation and having it recalculate the props' choices.

	if (! obj->BuildVariation(selections, choices, *this))
	{
		DeleteObject(obj);
		return NULL;
	}

	m_Objects[key] = obj;

	return obj;
}


void CObjectManager::DeleteObject(CObjectEntry* entry)
{
	std::map<ObjectKey, CObjectEntry*>::iterator it;
	while (m_Objects.end() != (it = find_if(m_Objects.begin(), m_Objects.end(), second_equals<CObjectEntry*>(entry))))
		m_Objects.erase(it);

	delete entry;
}


void CObjectManager::UnloadObjects()
{
	std::for_each(
		m_Objects.begin(),
		m_Objects.end(),
		delete_pair_2nd<ObjectKey, CObjectEntry*>
	);
	m_Objects.clear();

	std::for_each(
		m_ObjectBases.begin(),
		m_ObjectBases.end(),
		delete_pair_2nd<CStr, CObjectBase*>
	);
	m_ObjectBases.clear();
}



static LibError GetObjectName_ThunkCb(const VfsPath& pathname, const FileInfo& UNUSED(fileInfo), uintptr_t cbData)
{
	std::vector<CStr>* names = (std::vector<CStr>*)cbData;
	CStr name(pathname.string());
	names->push_back(name.AfterFirst("actors/"));
	return INFO::CB_CONTINUE;
}

void CObjectManager::GetAllObjectNames(std::vector<CStr>& names)
{
	(void)fs_util::ForEachFile(g_VFS, L"art/actors/", GetObjectName_ThunkCb, (uintptr_t)&names, L"*.xml", fs_util::DIR_RECURSIVE);
}

void CObjectManager::GetPropObjectNames(std::vector<CStr>& names)
{
	(void)fs_util::ForEachFile(g_VFS, L"art/actors/props/", GetObjectName_ThunkCb, (uintptr_t)&names, L"*.xml", fs_util::DIR_RECURSIVE);
}
