///////////////////////////////////////////////////////////////////////////////
//
// Name:		SkeletonAnimManager.cpp
// Author:		Rich Cross
// Contact:		rich@wildfiregames.com
//
///////////////////////////////////////////////////////////////////////////////

#include "res/res.h"
#include "Model.h"
#include "CLogger.h"
#include "SkeletonAnimManager.h"
#include <algorithm>

///////////////////////////////////////////////////////////////////////////////
// CSkeletonAnimManager constructor
CSkeletonAnimManager::CSkeletonAnimManager()
{
}

///////////////////////////////////////////////////////////////////////////////
// CSkeletonAnimManager destructor
CSkeletonAnimManager::~CSkeletonAnimManager()
{
	typedef std::map<CStr,CSkeletonAnimDef*>::iterator Iter;
	for (Iter i=m_Animations.begin();i!=m_Animations.end();++i) {
		delete i->second;
	}
}

///////////////////////////////////////////////////////////////////////////////
// GetAnimation: return a given animation by filename; return null if filename 
// doesn't refer to valid animation file
CSkeletonAnimDef* CSkeletonAnimManager::GetAnimation(const char* filename)
{
	// already loaded?
	CStr fname(filename);
	std::map<CStr,CSkeletonAnimDef*>::iterator iter=m_Animations.find(fname);
	if (iter!=m_Animations.end()) {
		// yes - return it
		return iter->second;
	}

	// already failed to load?
	std::set<CStr>::iterator setiter=m_BadAnimationFiles.find(fname);
	if (setiter!=m_BadAnimationFiles.end()) {
		// yes - return null
		return 0;
	}

	// try and load it now
	CSkeletonAnimDef* def;
	try {
		def=CSkeletonAnimDef::Load(filename);
	} catch (...) {
		def=0;
	}

	if (!def) {
		LOG(ERROR, "CSkeletonAnimManager::GetAnimation(%s): Failed loading, marked file as bad\n", filename);
		// add this file as bad
		m_BadAnimationFiles.insert(fname);
		return 0;
	} else {
		LOG(NORMAL, "CSkeletonAnimManager::GetAnimation(%s): Loaded successfully\n", filename);
		// add mapping for this file
		m_Animations[fname]=def;
		return def;
	}
}
