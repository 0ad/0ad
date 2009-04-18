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

/**
 * =========================================================================
 * File        : SkeletonAnimManager.cpp
 * Project     : 0 A.D.
 * Description : Owner of all skeleton animations
 * =========================================================================
 */

#include "precompiled.h"

#include "SkeletonAnimManager.h"

#include "graphics/ColladaManager.h"
#include "graphics/Model.h"
#include "graphics/SkeletonAnimDef.h"
#include "ps/CLogger.h"
#include "ps/FileIo.h"

#define LOG_CATEGORY "graphics"

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
	typedef std::map<CStr,CSkeletonAnimDef*>::iterator Iter;
	for (Iter i = m_Animations.begin(); i != m_Animations.end(); ++i)
		delete i->second;
}

///////////////////////////////////////////////////////////////////////////////
// GetAnimation: return a given animation by filename; return null if filename 
// doesn't refer to valid animation file
CSkeletonAnimDef* CSkeletonAnimManager::GetAnimation(const CStr& filename)
{
	// Strip a three-letter file extension (if there is one) from the filename
	CStr name;
	if (filename.length() > 4 && filename[filename.length()-4] == '.')
		name = filename.substr(0, filename.length()-4);
	else
		name = filename;

	// Find if it's already been loaded
	std::map<CStr, CSkeletonAnimDef*>::iterator iter = m_Animations.find(name);
	if (iter != m_Animations.end())
		return iter->second;

	CSkeletonAnimDef* def = NULL;

	// Find the file to load
	VfsPath psaFilename = m_ColladaManager.GetLoadableFilename(name, CColladaManager::PSA);

	if (psaFilename.empty())
	{
		LOG(CLogger::Error, LOG_CATEGORY, "Could not load animation '%s'", filename.c_str());
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
			// ignore errors (they'll be logged elsewhere)
		}
	}

	if (def)
		LOG(CLogger::Normal,  LOG_CATEGORY, "CSkeletonAnimManager::GetAnimation(%s): Loaded successfully", filename.c_str());
	else
		LOG(CLogger::Error, LOG_CATEGORY, "CSkeletonAnimManager::GetAnimation(%s): Failed loading, marked file as bad", filename.c_str());

	// Add to map
	m_Animations[name] = def; // NULL if failed to load - we won't try loading it again
	return def;
}
