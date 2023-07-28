/* Copyright (C) 2022 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Owner of all skeleton animations
 */

#include "precompiled.h"

#include "SkeletonAnimManager.h"

#include "graphics/ColladaManager.h"
#include "graphics/Model.h"
#include "graphics/SkeletonAnim.h"
#include "graphics/SkeletonAnimDef.h"
#include "ps/CLogger.h"
#include "ps/CStr.h"
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
}

///////////////////////////////////////////////////////////////////////////////
// GetAnimation: return a given animation by filename; return null if filename
// doesn't refer to valid animation file
CSkeletonAnimDef* CSkeletonAnimManager::GetAnimation(const VfsPath& pathname)
{
	VfsPath name = pathname.ChangeExtension(L"");

	// Find if it's already been loaded
	std::unordered_map<VfsPath, std::unique_ptr<CSkeletonAnimDef>>::iterator iter = m_Animations.find(name);
	if (iter != m_Animations.end())
		return iter->second.get();

	std::unique_ptr<CSkeletonAnimDef> def;

	// Find the file to load
	VfsPath psaFilename = m_ColladaManager.GetLoadablePath(name, CColladaManager::PSA);

	if (psaFilename.empty())
		LOGERROR("Could not load animation '%s'", pathname.string8());
	else
		try
		{
			def = CSkeletonAnimDef::Load(psaFilename);
		}
		catch (PSERROR_File&)
		{
			LOGERROR("Could not load animation '%s'", psaFilename.string8());
		}

	if (def)
		LOGMESSAGE("CSkeletonAnimManager::GetAnimation(%s): Loaded successfully", pathname.string8());
	else
		LOGERROR("CSkeletonAnimManager::GetAnimation(%s): Failed loading, marked file as bad", pathname.string8());

	// Add to map, NULL if failed to load - we won't try loading it again
	return m_Animations.insert_or_assign(name, std::move(def)).first->second.get();
}

/**
 * BuildAnimation: load raw animation frame animation from given file, and build a
 * animation specific to this model
 */
std::unique_ptr<CSkeletonAnim> CSkeletonAnimManager::BuildAnimation(const VfsPath& pathname, const CStr8& name, const CStr8& ID, int frequency, float speed, float actionpos, float actionpos2, float soundpos)
{
	CSkeletonAnimDef* def = GetAnimation(pathname);
	if (!def)
		return nullptr;

	std::unique_ptr<CSkeletonAnim> anim = std::make_unique<CSkeletonAnim>();
	anim->m_Name = name;
	anim->m_ID = ID;
	anim->m_Frequency = frequency;
	anim->m_AnimDef = def;
	anim->m_Speed = speed;

	if (actionpos == -1.f)
		anim->m_ActionPos = -1.f;
	else
		anim->m_ActionPos = actionpos * anim->m_AnimDef->GetDuration();

	if (actionpos2 == -1.f)
		anim->m_ActionPos2 = -1.f;
	else
		anim->m_ActionPos2 = actionpos2 * anim->m_AnimDef->GetDuration();

	if (soundpos == -1.f)
		anim->m_SoundPos = -1.f;
	else
		anim->m_SoundPos = soundpos * anim->m_AnimDef->GetDuration();

	anim->m_ObjectBounds.SetEmpty();

	return anim;
}
