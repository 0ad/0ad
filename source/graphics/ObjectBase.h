/* Copyright (C) 2016 Wildfire Games.
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

#ifndef INCLUDED_OBJECTBASE
#define INCLUDED_OBJECTBASE

class CModel;
class CSkeletonAnim;
class CObjectManager;
class CXeromyces;
class XMBElement;

#include <vector>
#include <set>
#include <map>
#include <boost/unordered_set.hpp>
#include "lib/file/vfs/vfs_path.h"
#include "ps/CStr.h"
#include "ps/CStrIntern.h"

#include <boost/random/mersenne_twister.hpp>

class CObjectBase
{
	NONCOPYABLE(CObjectBase);
public:

	struct Anim
	{
		// constructor
		Anim() : m_Frequency(0), m_Speed(1.f), m_ActionPos(-1.f), m_ActionPos2(-1.f), m_SoundPos(-1.f) {}
		// name of the animation - "Idle", "Run", etc
		CStr m_AnimName;
		int m_Frequency;
		// filename of the animation - manidle.psa, manrun.psa, etc
		VfsPath m_FileName;
		// animation speed, as specified in XML actor file
		float m_Speed;
		// fraction [0.0, 1.0] of the way through the animation that the interesting bit(s)
		// happens, or -1.0 if unspecified
		float m_ActionPos;
		float m_ActionPos2;
		float m_SoundPos;
	};

	struct Prop
	{
		// constructor
		Prop() : m_minHeight(0.f), m_maxHeight(0.f), m_selectable(true) {}
		// name of the prop point to attach to - "Prop01", "Prop02", "Head", "LeftHand", etc ..
		CStr m_PropPointName;
		// name of the model file - art/actors/props/sword.xml or whatever
		CStrW m_ModelName;
		// allow the prop to ajust the height from minHeight to maxHeight relative to the main model
		float m_minHeight;
		float m_maxHeight;
		bool m_selectable;
	};

	struct Samp
	{
		// identifier name of sampler in GLSL shaders
		CStrIntern m_SamplerName;
		// path to load from
		VfsPath m_SamplerFile;
	};

	struct Decal
	{
		Decal() : m_SizeX(0.f), m_SizeZ(0.f), m_Angle(0.f), m_OffsetX(0.f), m_OffsetZ(0.f) {}

		float m_SizeX;
		float m_SizeZ;
		float m_Angle;
		float m_OffsetX;
		float m_OffsetZ;
	};

	struct Variant
	{
		Variant() : m_Frequency(0) {}

		CStr m_VariantName; // lowercase name
		int m_Frequency;
		VfsPath m_ModelFilename;
		Decal m_Decal;
		VfsPath m_Particles;
		CStr m_Color;

		std::vector<Anim> m_Anims;
		std::vector<Prop> m_Props;
		std::vector<Samp> m_Samplers;
	};

	struct Variation
	{
		VfsPath model;
		Decal decal;
		VfsPath particles;
		CStr color;
		std::multimap<CStr, Prop> props;
		std::multimap<CStr, Anim> anims;
		std::multimap<CStr, Samp> samplers;
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
	std::set<CStr> CalculateRandomVariation(uint32_t seed, const std::set<CStr>& initialSelections);

	// Given a prioritized vector of selection string sets that partially specify
	// a variation, calculates a remaining set of selection strings such that the resulting
	// set merged with the initial selections fully specifies an exact variation of
	// the actor. The resulting selections are selected randomly, but only where a choice
	// is necessary (i.e. where there are multiple variants but the initial selections,
	// applied in priority order, fail to select one).
	std::set<CStr> CalculateRandomRemainingSelections(uint32_t seed, const std::vector<std::set<CStr> >& initialSelections);

	// Get a list of variant groups for this object, plus for all possible
	// props. Duplicated groups are removed, if several props share the same
	// variant names.
	std::vector<std::vector<CStr> > GetVariantGroups() const;

	/**
	 * Initialise this object by loading from the given file.
	 * Returns false on error.
	 */
	bool Load(const VfsPath& pathname);

	/**
	 * Reload this object from the file that it was previously loaded from.
	 * Returns false on error.
	 */
	bool Reload();

	/**
	 * Returns whether this object (including any possible props)
	 * uses the given file. (This is used for hotloading.)
	 */
	bool UsesFile(const VfsPath& pathname);

	// filename that this was loaded from
	VfsPath m_Pathname;

	// short human-readable name
	CStrW m_ShortName;

	struct {
		// cast shadows from this object
		bool m_CastShadows;
		// float on top of water
		bool m_FloatOnWater;
	} m_Properties;

	// the material file
	VfsPath m_Material;

private:
	// A low-quality RNG like rand48 causes visible non-random patterns (particularly
	// in large grids of the same actor with consecutive seeds, e.g. forests),
	// so use a better one that appears to avoid those patterns
	typedef boost::mt19937 rng_t;

	std::set<CStr> CalculateRandomRemainingSelections(rng_t& rng, const std::vector<std::set<CStr> >& initialSelections);

	std::vector< std::vector<Variant> > m_VariantGroups;
	CObjectManager& m_ObjectManager;

	boost::unordered_set<VfsPath> m_UsedFiles;

	void LoadVariant(const CXeromyces& XeroFile, const XMBElement& variant, Variant& currentVariant);
};

#endif
