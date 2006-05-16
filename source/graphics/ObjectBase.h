#ifndef _OBJECTBASE_H
#define _OBJECTBASE_H

class CModel;
class CSkeletonAnim;

#include <vector>
#include <set>
#include <map>
#include "CStr.h"

class CObjectBase
{
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
		double m_ActionPos;
		double m_ActionPos2;
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

	CObjectBase();

	std::vector<u8> CalculateVariationKey(const std::vector<std::set<CStrW> >& selections);

	const Variation BuildVariation(const std::vector<u8>& variationKey);

	std::set<CStrW> CalculateRandomVariation(const std::set<CStrW>& initialSelections);

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
	} m_Properties;

	// the material file
	CStr m_Material;

private:
	std::vector< std::vector<Variant> > m_VariantGroups;
};


#endif
