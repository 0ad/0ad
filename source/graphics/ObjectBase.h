#ifndef _OBJECTBASE_H
#define _OBJECTBASE_H

class CModel;
class CSkeletonAnim;

#include <vector>
#include "CStr.h"

class CObjectBase
{
public:

	struct Anim {
		// constructor
		Anim() : m_Speed(1), m_ActionPos( 0.0 ), m_AnimData(0) {}

		// name of the animation - "Idle", "Run", etc
		CStr m_AnimName;
		// filename of the animation - manidle.psa, manrun.psa, etc
		CStr m_FileName;
		// animation speed, as specified in XML actor file
		float m_Speed;
		// fraction of the way through the animation that the interesting bit
		// happens (this is converted to an absolute time when the animation
		// data is loaded)
		double m_ActionPos;
		// the animation data, specific to the this model
		CSkeletonAnim* m_AnimData;
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
		CStr m_VariantName;
		int m_Frequency;
		CStr m_ModelFilename;
		CStr m_TextureFilename;
		CStr m_Color;

		std::vector<Anim> m_Anims;
		std::vector<Prop> m_Props;
	};

	CObjectBase();

	std::vector< std::vector<Variant> > m_Variants;

	typedef std::vector<u8> variation_key;

	void CalculateVariation(std::set<CStr>& strings, variation_key& variation);

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
};


#endif
