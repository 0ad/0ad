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

#include "MeshManager.h"

#include "graphics/ColladaManager.h"
#include "graphics/ModelDef.h"
#include "ps/CLogger.h"
#include "ps/FileIo.h" // to get access to its CError
#include "ps/Profile.h"

#define LOG_CATEGORY "mesh"

// TODO: should this cache models while they're not actively in the game?
// (Currently they'll probably be deleted when the reference count drops to 0,
// even if it's quite possible that they'll get reloaded very soon.)

CMeshManager::CMeshManager(CColladaManager& colladaManager)
: m_ColladaManager(colladaManager)
{
}

CMeshManager::~CMeshManager()
{
}

CModelDefPtr CMeshManager::GetMesh(const CStr& filename)
{
	// Strip a three-letter file extension (if there is one) from the filename
	CStr name;
	if (filename.length() > 4 && filename[filename.length()-4] == '.')
		name = filename.substr(0, filename.length()-4);
	else
		name = filename;

	// Find the mesh if it's already been loaded and cached
	mesh_map::iterator iter = m_MeshMap.find(name);
	if (iter != m_MeshMap.end() && !iter->second.expired())
		return CModelDefPtr(iter->second);

	PROFILE( "load mesh" );

	VfsPath pmdFilename = m_ColladaManager.GetLoadableFilename(name, CColladaManager::PMD);

	if (pmdFilename.empty())
	{
		LOG(CLogger::Error, LOG_CATEGORY, "Could not load mesh '%s'", filename.c_str());
		return CModelDefPtr();
	}

	try
	{
		CModelDefPtr model (CModelDef::Load(pmdFilename, name));
		m_MeshMap[name] = model;
		return model;
	}
	catch (PSERROR_File&)
	{
		LOG(CLogger::Error, LOG_CATEGORY, "Could not load mesh '%s'", filename.c_str());
		return CModelDefPtr();
	}
}
