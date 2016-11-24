/* Copyright (C) 2012 Wildfire Games.
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

/*
 * Owner of all skeleton animations
 */

#include "precompiled.h"

#include "SkeletonAnimManager.h"

#include "graphics/ColladaManager.h"
#include "graphics/Model.h"
#include "graphics/SkeletonAnimDef.h"
#include "ps/CLogger.h"
#include "ps/FileIo.h"


///////////////////////////////////////////////////////////////////////////////
// CSkeletonAnimManager constructor
CSkeletonAnimManager::CSkeletonAnimManager(CColladaManager& colladaManager)
: m_ColladaManager(colladaManager)
{
}

///////////////////////////////////////////////////////////////////////////////
// CSkeletonAnimManager destructor
CSkeletonAnimManager::~CSkeletonAnimManager()
{
	typedef boost::unordered_map<VfsPath,CSkeletonAnimDef*>::iterator Iter;
	for (Iter i = m_Animations.begin(); i != m_Animations.end(); ++i)
		delete i->second;
}

///////////////////////////////////////////////////////////////////////////////
// GetAnimation: return a given animation by filename; return null if filename
// doesn't refer to valid animation file
CSkeletonAnimDef* CSkeletonAnimManager::GetAnimation(const VfsPath& pathname)
{
	VfsPath name = pathname.ChangeExtension(L"");

	// Find if it's already been loaded
	boost::unordered_map<VfsPath, CSkeletonAnimDef*>::iterator iter = m_Animations.find(name);
	if (iter != m_Animations.end())
		return iter->second;

	CSkeletonAnimDef* def = NULL;

	// Find the file to load
	VfsPath psaFilename = m_ColladaManager.GetLoadablePath(name, CColladaManager::PSA);

	if (psaFilename.empty())
	{
		LOGERROR("Could not load animation '%s'", pathname.string8());
		def = NULL;
	}
	else
	{
		try
		{
			def = CSkeletonAnimDef::Load(psaFilename);
		}
		catch (PSERROR_File&)
		{
			LOGERROR("Could not load animation '%s'", psaFilename.string8());
		}
	}

	if (def)
		LOGMESSAGE("CSkeletonAnimManager::GetAnimation(%s): Loaded successfully", pathname.string8());
	else
		LOGERROR("CSkeletonAnimManager::GetAnimation(%s): Failed loading, marked file as bad", pathname.string8());

	// Add to map
	m_Animations[name] = def; // NULL if failed to load - we won't try loading it again
	return def;
}
