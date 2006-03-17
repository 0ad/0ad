#ifndef _OBJECTENTRY_H
#define _OBJECTENTRY_H

class CModel;
class CSkeletonAnim;
struct SPropPoint;

#include <map>
#include <vector>
#include <map>
#include "CStr.h"
#include "ObjectBase.h"
#include "Overlay.h"

class CObjectEntry
{
public:
	CObjectEntry(int type, CObjectBase* base);
	~CObjectEntry();

	bool BuildVariation(const std::vector<std::set<CStrW> >& selections);

	// Base actor. Contains all the things that don't change between
	// different variations of the actor.
	CObjectBase* m_Base;

	// texture name
	CStr m_TextureName;
	// model name
	CStr m_ModelName;
	// colour (used when doing alpha-channel colouring, but not doing player-colour)
	CColor m_Color;
		// (probable TODO: make colour a per-model thing, rather than per-objectEntry,
		// so we can have lots of colour variations without wasting memory on
		// lots of objectEntries)

	CModel* m_ProjectileModel;
	CModel* m_AmmunitionModel;
	SPropPoint* m_AmmunitionPoint;

	// Returns a randomly-chosen animation matching the given name.
	// If none is found, returns NULL.
	CSkeletonAnim* GetRandomAnimation(const CStr& animationName);

	// corresponding model
	CModel* m_Model;
	// type of object; index into object managers types array
	int m_Type;

private:
	typedef std::multimap<CStr, CSkeletonAnim*> SkeletonAnimMap;
	SkeletonAnimMap m_Animations;
		// TODO: something more memory-efficient than storing loads of similar strings for each unit?
};


#endif
