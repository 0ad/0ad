///////////////////////////////////////////////////////////////////////////////
//
// Name:		SkeletonAnimManager.h
// Author:		Rich Cross
// Contact:		rich@wildfiregames.com
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _SKELETONANIMMANAGER_H
#define _SKELETONANIMMANAGER_H

#include <map>
#include <set>
#include "SkeletonAnimDef.h"
#include "Singleton.h"


// access to sole CSkeletonAnimManager object
#define g_SkelAnimMan CSkeletonAnimManager::GetSingleton()

///////////////////////////////////////////////////////////////////////////////
// CSkeletonAnimManager : owner class of all skeleton anims - manages creation, 
// loading and destruction of animation data
class CSkeletonAnimManager : public Singleton<CSkeletonAnimManager>
{
public:
	// constructor, destructor
	CSkeletonAnimManager();
	~CSkeletonAnimManager();
	
	// return a given animation by filename; return null if filename doesn't
	// refer to valid animation file
	CSkeletonAnimDef* GetAnimation(const char* filename);

private:
	CSkeletonAnimDef* LoadAnimation(const char* filename);

	// map of all known animations
	std::map<CStr,CSkeletonAnimDef*> m_Animations;
	// set of bad animation names - prevents multiple reloads of bad files
	std::set<CStr> m_BadAnimationFiles;
};

#endif