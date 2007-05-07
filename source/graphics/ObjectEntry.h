#ifndef INCLUDED_OBJECTENTRY
#define INCLUDED_OBJECTENTRY

class CModel;
class CSkeletonAnim;
class CObjectBase;
class CObjectManager;
struct SPropPoint;

#include <map>
#include <set>
#include <vector>

#include "ps/CStr.h"
#include "ps/Overlay.h"

class CObjectEntry
{
public:
	CObjectEntry(CObjectBase* base);
	~CObjectEntry();

	// Construct this actor, using the specified variation selections
	bool BuildVariation(const std::vector<std::set<CStr> >& selections,
		const std::vector<u8>& variationKey, CObjectManager& objectManager);

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

private:
	typedef std::multimap<CStr, CSkeletonAnim*> SkeletonAnimMap;
	SkeletonAnimMap m_Animations;
		// TODO: something more memory-efficient than storing loads of similar strings for each unit?
};


#endif
