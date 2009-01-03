#ifndef INCLUDED_OBJECTBASE
#define INCLUDED_OBJECTBASE

class CModel;
class CSkeletonAnim;
class CObjectManager;

#include <vector>
#include <set>
#include <map>
#include "ps/CStr.h"

class CObjectBase
{
	NONCOPYABLE(CObjectBase);
public:

	struct Anim {
		// constructor
		Anim() : m_Speed(1), m_ActionPos(0.5), m_ActionPos2(0.0) {}

		// name of the animation - "Idle", "Run", etc
		CStr m_AnimName;
		// filename of the animation - manidle.psa, manrun.psa, etc
		CStr m_FileName;
		// animation speed, as specified in XML actor file
		float m_Speed;
		// fraction of the way through the animation that the interesting bit(s)
		// happens 
		float m_ActionPos;
		float m_ActionPos2;
	};

	struct Prop {
		// name of the prop point to attach to - "Prop01", "Prop02", "Head", "LeftHand", etc ..
		CStr m_PropPointName;
		// name of the model file - art/actors/props/sword.xml or whatever
		CStr m_ModelName;
	};

	struct Variant
	{
		Variant() : m_Frequency(0) {}
		CStr m_VariantName; // lowercase name
		int m_Frequency;
		CStr m_ModelFilename;
		CStr m_TextureFilename;
		CStr m_Color;

		std::vector<Anim> m_Anims;
		std::vector<Prop> m_Props;
	};

	struct Variation
	{
		CStr texture;
		CStr model;
		CStr color;
		std::map<CStr, CObjectBase::Prop> props;
		std::multimap<CStr, CObjectBase::Anim> anims;
	};

	CObjectBase(CObjectManager& objectManager);

	// Get the variation key (indices of chosen variants from each group)
	// based on the selection strings
	std::vector<u8> CalculateVariationKey(const std::vector<std::set<CStr> >& selections);

	// Get the final actor data, combining all selected variants
	const Variation BuildVariation(const std::vector<u8>& variationKey);

	// Get a set of selection strings that are complete enough to specify an
	// exact variation of the actor, using the initial selections wherever possible
	// and choosing randomly where a choice is necessary. 
	std::set<CStr> CalculateRandomVariation(const std::set<CStr>& initialSelections);

	// Get a list of variant groups for this object, plus for all possible
	// props. Duplicated groups are removed, if several props share the same
	// variant names.
	std::vector<std::vector<CStr> > GetVariantGroups() const;

	bool Load(const char* filename);

	// object name
	CStr m_Name;

	// short human-readable name
	CStr m_ShortName;

	struct {
		// automatically flatten terrain when applying object
		bool m_AutoFlatten;
		// cast shadows from this object
		bool m_CastShadows;
		// float on top of water
		bool m_FloatOnWater;
	} m_Properties;

	// the material file
	CStr m_Material;

private:
	std::vector< std::vector<Variant> > m_VariantGroups;
	CObjectManager& m_ObjectManager;
};


#endif
