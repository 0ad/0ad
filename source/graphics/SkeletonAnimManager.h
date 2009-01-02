/**
 * =========================================================================
 * File        : SkeletonAnimManager.h
 * Project     : 0 A.D.
 * Description : Owner of all skeleton animations
 * =========================================================================
 */

#ifndef INCLUDED_SKELETONANIMMANAGER
#define INCLUDED_SKELETONANIMMANAGER

#include <map>
#include <set>

class CColladaManager;
class CSkeletonAnimDef;
class CStr8;

///////////////////////////////////////////////////////////////////////////////
// CSkeletonAnimManager : owner class of all skeleton anims - manages creation, 
// loading and destruction of animation data
class CSkeletonAnimManager : noncopyable
{
public:
	// constructor, destructor
	CSkeletonAnimManager(CColladaManager& colladaManager);
	~CSkeletonAnimManager();
	
	// return a given animation by filename; return null if filename doesn't
	// refer to valid animation file
	CSkeletonAnimDef* GetAnimation(const CStr8& filename);

private:
	CSkeletonAnimDef* LoadAnimation(const char* filename);

	// map of all known animations. Value is NULL if it failed to load.
	std::map<CStr8, CSkeletonAnimDef*> m_Animations;

	CColladaManager& m_ColladaManager;
};

#endif
