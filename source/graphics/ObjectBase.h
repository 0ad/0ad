/* Copyright (C) 2021 Wildfire Games.
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

#ifndef INCLUDED_OBJECTBASE
#define INCLUDED_OBJECTBASE

#include "lib/file/vfs/vfs_path.h"
#include "ps/CStr.h"
#include "ps/CStrIntern.h"

class CActorDef;
class CObjectEntry;
class CObjectManager;
class CXeromyces;
class XMBElement;

#include <boost/random/mersenne_twister.hpp>
#include <map>
#include <memory>
#include <set>
#include <unordered_set>
#include <vector>

/**
 * Maintains the tree of possible objects from a specific actor definition at a given quality level.
 * An Object Base is made of:
 *  - a material
 *  - a few properties (float on water / casts shadow / ...)
 *  - a number of variant groups.
 * Any actual object in game will pick a variant from each group (see ObjectEntry).
 */
class CObjectBase
{
	friend CActorDef;

	// See CopyWithQuality() below.
	NONCOPYABLE(CObjectBase);
public:
	struct Anim
	{
		// constructor
		Anim() : m_Frequency(0), m_Speed(1.f), m_ActionPos(-1.f), m_ActionPos2(-1.f), m_SoundPos(-1.f) {}
		// name of the animation - "Idle", "Run", etc
		CStr m_AnimName;
		// ID of the animation: if not empty, something specific to sync with props.
		CStr m_ID = "";
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

	CObjectBase(CObjectManager& objectManager, CActorDef& actorDef, u8 QualityLevel);

	// Returns a set of selection such that, added to initialSelections, CalculateVariationKey can proceed.
	std::set<CStr> CalculateRandomRemainingSelections(uint32_t seed, const std::vector<std::set<CStr>>& initialSelections) const;

	// Get the variation key (indices of chosen variants from each group)
	// based on the selection strings.
	// Should not have to make a random choice: the selections should be complete.
	std::vector<u8> CalculateVariationKey(const std::vector<const std::set<CStr>*>& selections) const;

	// Get the final actor data, combining all selected variants
	const Variation BuildVariation(const std::vector<u8>& variationKey) const;

	// Get a list of variant groups for this object, plus for all possible
	// props. Duplicated groups are removed, if several props share the same
	// variant names.
	std::vector<std::vector<CStr> > GetVariantGroups() const;

	// Return a string identifying this actor uniquely (includes quality level information);
	const CStr& GetIdentifier() const;

	/**
	 * Returns whether this object (including any possible props)
	 * uses the given file. (This is used for hotloading.)
	 */
	bool UsesFile(const VfsPath& pathname) const;


	struct {
		// cast shadows from this object
		bool m_CastShadows;
		// float on top of water
		bool m_FloatOnWater;
	} m_Properties;

	// the material file
	VfsPath m_Material;

	// Quality level - part of the data resource path.
	u8 m_QualityLevel;

private:
	// Private interface for CActorDef/ObjectEntry

	/**
	 * Acts as an explicit copy constructor, for a new quality level.
	 * Note that this does not reload the actor, so this setting will only change props.
	 */
	std::unique_ptr<CObjectBase> CopyWithQuality(u8 newQualityLevel) const;

	// A low-quality RNG like rand48 causes visible non-random patterns (particularly
	// in large grids of the same actor with consecutive seeds, e.g. forests),
	// so use a better one that appears to avoid those patterns
	using rng_t = boost::mt19937;
	std::set<CStr> CalculateRandomRemainingSelections(rng_t& rng, const std::vector<std::set<CStr>>& initialSelections) const;

	/**
	 * Get all quality levels at which this object changes (includes props).
	 * Intended to be called by CActorFef.
	 * @param splits - a sorted vector of unique quality splits.
	 */
	void GetQualitySplits(std::vector<u8>& splits) const;

	[[nodiscard]] bool Load(const CXeromyces& XeroFile, const XMBElement& base);
	[[nodiscard]] bool LoadVariant(const CXeromyces& XeroFile, const XMBElement& variant, Variant& currentVariant);

private:
	// Backref to the owning actor.
	CActorDef& m_ActorDef;

	// Used to identify this actor uniquely in the ObjectManager (and for debug).
	CStr m_Identifier;

	std::vector< std::vector<Variant> > m_VariantGroups;
	CObjectManager& m_ObjectManager;
};

/**
 * Represents an actor file. Actors can contain various quality levels.
 * An ActorDef maintains a CObjectBase for each specified quality level, and provides access to it.
 */
class CActorDef
{
	// Friend these three so they can use GetBase.
	friend class CObjectManager;
	friend class CObjectBase;
	friend class CObjectEntry;

	NONCOPYABLE(CActorDef);
public:

	CActorDef(CObjectManager& objectManager);

	std::vector<u8> QualityLevels() const;

	VfsPath GetPathname() const { return m_Pathname; }

	/**
	 * Return a list of selections specifying a particular variant in all groups, based on the seed.
	 */
	std::set<CStr> PickSelectionsAtRandom(uint32_t seed) const;

// Interface accessible from CObjectManager / CObjectBase
protected:
	/**
	 * Return the Object base matching the given quality level.
	 */
	const std::shared_ptr<CObjectBase>& GetBase(u8 QualityLevel) const;

	/**
	 * Initialise this object by loading from the given file.
	 * Returns false on error.
	 */
	bool Load(const VfsPath& pathname);

	/**
	 * Initialise this object with a default placeholder actor,
	 * pretending to be the actor at pathname.
	 */
	void LoadErrorPlaceholder(const VfsPath& pathname);

	/**
	 * Returns whether this actor (including any possible props)
	 * uses the given file. (This is used for hotloading.)
	 */
	bool UsesFile(const VfsPath& pathname) const;

	// filename that this was loaded from
	VfsPath m_Pathname;

private:
	CObjectManager& m_ObjectManager;

	// std::shared_ptr to avoid issues during hotloading.
	std::vector<std::shared_ptr<CObjectBase>> m_ObjectBases;

	std::unordered_set<VfsPath> m_UsedFiles;
};

#endif
