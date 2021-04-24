/* Copyright (C) 2021 Wildfire Games.
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

#include "precompiled.h"

#include <algorithm>
#include <queue>

#include "ObjectBase.h"

#include "ObjectManager.h"
#include "ps/XML/Xeromyces.h"
#include "ps/Filesystem.h"
#include "ps/CLogger.h"
#include "lib/timer.h"
#include "maths/MathUtil.h"

#include <boost/random/uniform_int_distribution.hpp>

namespace
{
/**
 * The maximal quality for an actor.
 */
static constexpr int MAX_QUALITY = 255;

/**
 * How many quality levels a given actor can have.
 */
static constexpr int MAX_LEVELS_PER_ACTOR_DEF = 5;

int GetQuality(const CStr& value)
{
	if (value == "low")
		return 100;
	else if (value == "medium")
		return 150;
	else if (value == "high")
		return 200;
	else
		return value.ToInt();
}
} // anonymous namespace

CObjectBase::CObjectBase(CObjectManager& objectManager, CActorDef& actorDef, u8 qualityLevel)
: m_ObjectManager(objectManager), m_ActorDef(actorDef)
{
	m_QualityLevel = qualityLevel;
	m_Properties.m_CastShadows = false;
	m_Properties.m_FloatOnWater = false;

	// Remove leading art/actors/ & include quality level.
	m_Identifier = m_ActorDef.m_Pathname.string8().substr(11) + CStr::FromInt(m_QualityLevel);
}

std::unique_ptr<CObjectBase> CObjectBase::CopyWithQuality(u8 newQualityLevel) const
{
	std::unique_ptr<CObjectBase> ret = std::make_unique<CObjectBase>(m_ObjectManager, m_ActorDef, newQualityLevel);
	// No need to actually change any quality-related stuff here, we assume that this is a copy for props.
	ret->m_VariantGroups = m_VariantGroups;
	ret->m_Material = m_Material;
	ret->m_Properties = m_Properties;
	return ret;
}

void CObjectBase::Load(const CXeromyces& XeroFile, const XMBElement& root)
{
	// Define all the elements used in the XML file
#define EL(x) int el_##x = XeroFile.GetElementID(#x)
#define AT(x) int at_##x = XeroFile.GetAttributeID(#x)
	EL(castshadow);
	EL(float);
	EL(group);
	EL(material);
	AT(maxquality);
	AT(minquality);
#undef AT
#undef EL


	// Set up the group vector to avoid reallocation and copying later.
	{
		int groups = 0;
		XERO_ITER_EL(root, child)
		{
			if (child.GetNodeName() == el_group)
				++groups;
		}

		m_VariantGroups.reserve(groups);
	}

	// (This XML-reading code is rather worryingly verbose...)

	auto shouldSkip = [&](XMBElement& node) {
		XERO_ITER_ATTR(node, attr)
		{
			if (attr.Name == at_minquality && GetQuality(attr.Value) > m_QualityLevel)
				return true;
			else if (attr.Name == at_maxquality && GetQuality(attr.Value) <= m_QualityLevel)
				return true;
		}
		return false;
	};

	XERO_ITER_EL(root, child)
	{
		int child_name = child.GetNodeName();

		if (shouldSkip(child))
			continue;

		if (child_name == el_group)
		{
			std::vector<Variant>& currentGroup = m_VariantGroups.emplace_back();
			currentGroup.reserve(child.GetChildNodes().size());
			XERO_ITER_EL(child, variant)
			{
				if (shouldSkip(variant))
					continue;

				LoadVariant(XeroFile, variant, currentGroup.emplace_back());
			}

			if (currentGroup.size() == 0)
				LOGERROR("Actor group has zero variants ('%s')", m_Identifier);
		}
		else if (child_name == el_castshadow)
			m_Properties.m_CastShadows = true;
		else if (child_name == el_float)
			m_Properties.m_FloatOnWater = true;
		else if (child_name == el_material)
			m_Material = VfsPath("art/materials") / child.GetText().FromUTF8();
	}

	if (m_Material.empty())
		m_Material = VfsPath("art/materials/default.xml");
}

void CObjectBase::LoadVariant(const CXeromyces& XeroFile, const XMBElement& variant, Variant& currentVariant)
{
	#define EL(x) int el_##x = XeroFile.GetElementID(#x)
	#define AT(x) int at_##x = XeroFile.GetAttributeID(#x)
	EL(animation);
	EL(animations);
	EL(color);
	EL(decal);
	EL(mesh);
	EL(particles);
	EL(prop);
	EL(props);
	EL(texture);
	EL(textures);
	EL(variant);
	AT(actor);
	AT(angle);
	AT(attachpoint);
	AT(depth);
	AT(event);
	AT(file);
	AT(frequency);
	AT(id);
	AT(load);
	AT(maxheight);
	AT(minheight);
	AT(name);
	AT(offsetx);
	AT(offsetz);
	AT(selectable);
	AT(sound);
	AT(speed);
	AT(width);
	#undef AT
	#undef EL

	if (variant.GetNodeName() != el_variant)
	{
		LOGERROR("Invalid variant format (unrecognised root element '%s')", XeroFile.GetElementString(variant.GetNodeName()).c_str());
		return;
	}

	// Load variants first, so that they can be overriden if necessary.
	XERO_ITER_ATTR(variant, attr)
	{
		if (attr.Name == at_file)
		{
			// Open up an external file to load.
			// Don't crash hard when failures happen, but log them and continue
			m_ActorDef.m_UsedFiles.insert(attr.Value);
			CXeromyces XeroVariant;
			if (XeroVariant.Load(g_VFS, "art/variants/" + attr.Value) == PSRETURN_OK)
			{
				XMBElement variantRoot = XeroVariant.GetRoot();
				LoadVariant(XeroVariant, variantRoot, currentVariant);
			}
			else
				LOGERROR("Could not open path %s", attr.Value);
			// Continue loading extra definitions in this variant to allow nested files
		}
	}

	XERO_ITER_ATTR(variant, attr)
	{
		if (attr.Name == at_name)
			currentVariant.m_VariantName = attr.Value.LowerCase();
		else if (attr.Name == at_frequency)
			currentVariant.m_Frequency = attr.Value.ToInt();
	}

	XERO_ITER_EL(variant, option)
	{
		int option_name = option.GetNodeName();

		if (option_name == el_mesh)
		{
			currentVariant.m_ModelFilename = VfsPath("art/meshes") / option.GetText().FromUTF8();
		}
		else if (option_name == el_textures)
		{
			XERO_ITER_EL(option, textures_element)
			{
				ENSURE(textures_element.GetNodeName() == el_texture);

				Samp samp;
				XERO_ITER_ATTR(textures_element, se)
				{
					if (se.Name == at_file)
						samp.m_SamplerFile = VfsPath("art/textures/skins") / se.Value.FromUTF8();
					else if (se.Name == at_name)
						samp.m_SamplerName = CStrIntern(se.Value);
				}
				currentVariant.m_Samplers.push_back(samp);
			}
		}
		else if (option_name == el_decal)
		{
			XMBAttributeList attrs = option.GetAttributes();
			Decal decal;
			decal.m_SizeX = attrs.GetNamedItem(at_width).ToFloat();
			decal.m_SizeZ = attrs.GetNamedItem(at_depth).ToFloat();
			decal.m_Angle = DEGTORAD(attrs.GetNamedItem(at_angle).ToFloat());
			decal.m_OffsetX = attrs.GetNamedItem(at_offsetx).ToFloat();
			decal.m_OffsetZ = attrs.GetNamedItem(at_offsetz).ToFloat();
			currentVariant.m_Decal = decal;
		}
		else if (option_name == el_particles)
		{
			XMBAttributeList attrs = option.GetAttributes();
			VfsPath file = VfsPath("art/particles") / attrs.GetNamedItem(at_file).FromUTF8();
			currentVariant.m_Particles = file;

			// For particle hotloading, it's easiest to reload the entire actor,
			// so remember the relevant particle file as a dependency for this actor
			m_ActorDef.m_UsedFiles.insert(file);
		}
		else if (option_name == el_color)
		{
			currentVariant.m_Color = option.GetText();
		}
		else if (option_name == el_animations)
		{
			XERO_ITER_EL(option, anim_element)
			{
				ENSURE(anim_element.GetNodeName() == el_animation);

				Anim anim;
				XERO_ITER_ATTR(anim_element, ae)
				{
					if (ae.Name == at_name)
						anim.m_AnimName = ae.Value;
					else if (ae.Name == at_id)
						anim.m_ID = ae.Value;
					else if (ae.Name == at_frequency)
						anim.m_Frequency = ae.Value.ToInt();
					else if (ae.Name == at_file)
						anim.m_FileName = VfsPath("art/animation") / ae.Value.FromUTF8();
					else if (ae.Name == at_speed)
						anim.m_Speed = ae.Value.ToInt() > 0 ? ae.Value.ToInt() / 100.f : 1.f;
					else if (ae.Name == at_event)
						anim.m_ActionPos = Clamp(ae.Value.ToFloat(), 0.f, 1.f);
					else if (ae.Name == at_load)
						anim.m_ActionPos2 = Clamp(ae.Value.ToFloat(), 0.f, 1.f);
					else if (ae.Name == at_sound)
						anim.m_SoundPos = Clamp(ae.Value.ToFloat(), 0.f, 1.f);
				}
				currentVariant.m_Anims.push_back(anim);
			}
		}
		else if (option_name == el_props)
		{
			XERO_ITER_EL(option, prop_element)
			{
				ENSURE(prop_element.GetNodeName() == el_prop);

				Prop prop;
				XERO_ITER_ATTR(prop_element, pe)
				{
					if (pe.Name == at_attachpoint)
						prop.m_PropPointName = pe.Value;
					else if (pe.Name == at_actor)
						prop.m_ModelName = pe.Value.FromUTF8();
					else if (pe.Name == at_minheight)
						prop.m_minHeight = pe.Value.ToFloat();
					else if (pe.Name == at_maxheight)
						prop.m_maxHeight = pe.Value.ToFloat();
					else if (pe.Name == at_selectable)
						prop.m_selectable = pe.Value != "false";
				}
				currentVariant.m_Props.push_back(prop);
			}
		}
	}
}

std::vector<u8> CObjectBase::CalculateVariationKey(const std::vector<const std::set<CStr>*>& selections) const
{
	// (TODO: see CObjectManager::FindObjectVariation for an opportunity to
	// call this function a bit less frequently)

	// Calculate a complete list of choices, one per group, based on the
	// supposedly-complete selections (i.e. not making random choices at this
	// stage).
	// In each group, if one of the variants has a name matching a string in the
	// first 'selections', set use that one.
	// Otherwise, try with the next (lower priority) selections set, and repeat.
	// Otherwise, choose the first variant (arbitrarily).

	std::vector<u8> choices;

	std::multimap<CStr, CStrW> chosenProps;

	for (std::vector<std::vector<CObjectBase::Variant> >::const_iterator grp = m_VariantGroups.begin();
		grp != m_VariantGroups.end();
		++grp)
	{
		// Ignore groups with nothing inside. (A warning will have been
		// emitted by the loading code.)
		if (grp->size() == 0)
			continue;

		int match = -1; // -1 => none found yet

		// If there's only a single variant, choose that one
		if (grp->size() == 1)
		{
			match = 0;
		}
		else
		{
			// Determine the first variant that matches the provided strings,
			// starting with the highest priority selections set:

			for (const std::set<CStr>* selset : selections)
			{
				ENSURE(grp->size() < 256); // else they won't fit in 'choices'

				for (size_t i = 0; i < grp->size(); ++i)
				{
					if (selset->count((*grp)[i].m_VariantName))
					{
						match = (u8)i;
						break;
					}
				}

				// Stop after finding the first match
				if (match != -1)
					break;
			}

			// If no match, just choose the first
			if (match == -1)
				match = 0;
		}

		choices.push_back(match);
		// Remember which props were chosen, so we can call CalculateVariationKey on them
		// at the end.
		// Erase all existing props which are overridden by this variant:
		const Variant& var((*grp)[match]);

		for (const Prop& prop : var.m_Props)
			chosenProps.erase(prop.m_PropPointName);
		// and then insert the new ones:
		for (const Prop& prop : var.m_Props)
			if (!prop.m_ModelName.empty())
				chosenProps.insert(make_pair(prop.m_PropPointName, prop.m_ModelName));
	}

	// Load each prop, and add their CalculateVariationKey to our key:
	for (std::multimap<CStr, CStrW>::iterator it = chosenProps.begin(); it != chosenProps.end(); ++it)
	{
		if (auto [success, prop] = m_ObjectManager.FindActorDef(it->second); success)
		{
			std::vector<u8> propChoices = prop.GetBase(m_QualityLevel)->CalculateVariationKey(selections);
			choices.insert(choices.end(), propChoices.begin(), propChoices.end());
		}
	}

	return choices;
}

const CObjectBase::Variation CObjectBase::BuildVariation(const std::vector<u8>& variationKey) const
{
	Variation variation;

	// variationKey should correspond with m_Variants, giving the id of the
	// chosen variant from each group. (Except variationKey has some bits stuck
	// on the end for props, but we don't care about those in here.)

	std::vector<std::vector<CObjectBase::Variant> >::const_iterator grp = m_VariantGroups.begin();
	std::vector<u8>::const_iterator match = variationKey.begin();
	for ( ;
		grp != m_VariantGroups.end() && match != variationKey.end();
		++grp, ++match)
	{
		// Ignore groups with nothing inside. (A warning will have been
		// emitted by the loading code.)
		if (grp->size() == 0)
			continue;

		size_t id = *match;
		if (id >= grp->size())
		{
			// This should be impossible
			debug_warn(L"BuildVariation: invalid variant id");
			continue;
		}

		// Get the matched variant
		const CObjectBase::Variant& var ((*grp)[id]);

		// Apply its data:

		if (! var.m_ModelFilename.empty())
			variation.model = var.m_ModelFilename;

		if (var.m_Decal.m_SizeX && var.m_Decal.m_SizeZ)
			variation.decal = var.m_Decal;

		if (! var.m_Particles.empty())
			variation.particles = var.m_Particles;

		if (! var.m_Color.empty())
			variation.color = var.m_Color;

		// If one variant defines one prop attached to e.g. "root", and this
		// variant defines two different props with the same attachpoint, the one
		// original should be erased, and replaced by the two new ones.
		//
		// So, erase all existing props which are overridden by this variant:
		for (std::vector<CObjectBase::Prop>::const_iterator it = var.m_Props.begin(); it != var.m_Props.end(); ++it)
			variation.props.erase(it->m_PropPointName);
		// and then insert the new ones:
		for (std::vector<CObjectBase::Prop>::const_iterator it = var.m_Props.begin(); it != var.m_Props.end(); ++it)
			if (! it->m_ModelName.empty()) // if the name is empty then the overridden prop is just deleted
				variation.props.insert(make_pair(it->m_PropPointName, *it));

		// Same idea applies for animations.
		// So, erase all existing animations which are overridden by this variant:
		for (std::vector<CObjectBase::Anim>::const_iterator it = var.m_Anims.begin(); it != var.m_Anims.end(); ++it)
			variation.anims.erase(it->m_AnimName);
		// and then insert the new ones:
		for (std::vector<CObjectBase::Anim>::const_iterator it = var.m_Anims.begin(); it != var.m_Anims.end(); ++it)
			variation.anims.insert(make_pair(it->m_AnimName, *it));

		// Same for samplers, though perhaps not strictly necessary:
		for (std::vector<CObjectBase::Samp>::const_iterator it = var.m_Samplers.begin(); it != var.m_Samplers.end(); ++it)
			variation.samplers.erase(it->m_SamplerName.string());
		for (std::vector<CObjectBase::Samp>::const_iterator it = var.m_Samplers.begin(); it != var.m_Samplers.end(); ++it)
			variation.samplers.insert(make_pair(it->m_SamplerName.string(), *it));
	}

	return variation;
}

std::set<CStr> CObjectBase::CalculateRandomRemainingSelections(uint32_t seed, const std::vector<std::set<CStr>>& initialSelections) const
{
	rng_t rng;
	rng.seed(seed);

	std::set<CStr> remainingSelections = CalculateRandomRemainingSelections(rng, initialSelections);
	for (const std::set<CStr>& sel : initialSelections)
		remainingSelections.insert(sel.begin(), sel.end());

	return remainingSelections; // now actually a complete set of selections
}

std::set<CStr> CObjectBase::CalculateRandomRemainingSelections(rng_t& rng, const std::vector<std::set<CStr>>& initialSelections) const
{
	std::set<CStr> remainingSelections;
	std::multimap<CStr, CStrW> chosenProps;

	// Calculate a complete list of selections, so there is at least one
	// (and in most cases only one) per group.
	// In each group, if one of the variants has a name matching a string in
	// 'selections', use that one.
	// If more than one matches, choose randomly from those matching ones.
	// If none match, choose randomly from all variants.
	//
	// When choosing randomly, make use of each variant's frequency. If all
	// variants have frequency 0, treat them as if they were 1.

	for (std::vector<std::vector<Variant> >::const_iterator grp = m_VariantGroups.begin();
		grp != m_VariantGroups.end();
		++grp)
	{
		// Ignore groups with nothing inside. (A warning will have been
		// emitted by the loading code.)
		if (grp->size() == 0)
			continue;

		int match = -1; // -1 => none found yet

		// If there's only a single variant, choose that one
		if (grp->size() == 1)
		{
			match = 0;
		}
		else
		{
			// See if a variant (or several, but we only care about the first)
			// is already matched by the selections we've made, keeping their
			// priority order into account

			for (size_t s = 0; s < initialSelections.size(); ++s)
			{
				for (size_t i = 0; i < grp->size(); ++i)
				{
					if (initialSelections[s].count((*grp)[i].m_VariantName))
					{
						match = (int)i;
						break;
					}
				}

				if (match >= 0)
					break;
			}

			// If there was one, we don't need to do anything now because there's
			// already something to choose. Otherwise, choose randomly from the others.
			if (match == -1)
			{
				// Sum the frequencies
				int totalFreq = 0;
				for (size_t i = 0; i < grp->size(); ++i)
					totalFreq += (*grp)[i].m_Frequency;

				// Someone might be silly and set all variants to have freq==0, in
				// which case we just pretend they're all 1
				bool allZero = (totalFreq == 0);
				if (allZero) totalFreq = (int)grp->size();

				// Choose a random number in the interval [0..totalFreq)
				int randNum = boost::random::uniform_int_distribution<int>(0, totalFreq-1)(rng);

				// and use that to choose one of the variants
				for (size_t i = 0; i < grp->size(); ++i)
				{
					randNum -= (allZero ? 1 : (*grp)[i].m_Frequency);
					if (randNum < 0)
					{
						remainingSelections.insert((*grp)[i].m_VariantName);
						// (If this change to 'remainingSelections' interferes with earlier choices, then
						// we'll get some non-fatal inconsistencies that just break the randomness. But that
						// shouldn't happen, much.)
						// (As an example, suppose you have a group with variants "a" and "b", and another
						// with variants "a" and "c"; now if random selection choses "b" for the first
						// and "a" for the second, then the selection of "a" from the second group will
						// cause "a" to be used in the first instead of the "b").
						match = (int)i;
						break;
					}
				}
				ENSURE(randNum < 0);
				// This should always succeed; otherwise it
				// wouldn't have chosen any of the variants.
			}
		}

		// Remember which props were chosen, so we can call CalculateRandomVariation on them
		// at the end.
		const Variant& var ((*grp)[match]);
		// Erase all existing props which are overridden by this variant:
		for (const Prop& prop : var.m_Props)
			chosenProps.erase(prop.m_PropPointName);
		// and then insert the new ones:
		for (const Prop& prop : var.m_Props)
			if (!prop.m_ModelName.empty())
				chosenProps.insert(make_pair(prop.m_PropPointName, prop.m_ModelName));
	}

	// Load each prop, and add their required selections to ours:
	for (std::multimap<CStr, CStrW>::iterator it = chosenProps.begin(); it != chosenProps.end(); ++it)
	{
		if (auto [success, prop] = m_ObjectManager.FindActorDef(it->second); success)
		{
			std::vector<std::set<CStr> > propInitialSelections = initialSelections;
			if (!remainingSelections.empty())
				propInitialSelections.push_back(remainingSelections);

			std::set<CStr> propRemainingSelections = prop.GetBase(m_QualityLevel)->CalculateRandomRemainingSelections(rng, propInitialSelections);
			remainingSelections.insert(propRemainingSelections.begin(), propRemainingSelections.end());

			// Add the prop's used files to our own (recursively) so we can hotload
			// when any prop is changed
			m_ActorDef.m_UsedFiles.insert(prop.m_UsedFiles.begin(), prop.m_UsedFiles.end());
		}
	}

	return remainingSelections;
}

std::vector<std::vector<CStr> > CObjectBase::GetVariantGroups() const
{
	std::vector<std::vector<CStr> > groups;

	// Queue of objects (main actor plus props (recursively)) to be processed
	std::queue<const CObjectBase*> objectsQueue;
	objectsQueue.push(this);

	// Set of objects already processed, so we don't do them more than once
	std::set<const CObjectBase*> objectsProcessed;

	while (!objectsQueue.empty())
	{
		const CObjectBase* obj = objectsQueue.front();
		objectsQueue.pop();
		// Ignore repeated objects (likely to be props)
		if (objectsProcessed.find(obj) != objectsProcessed.end())
			continue;

		objectsProcessed.insert(obj);

		// Iterate through the list of groups
		for (size_t i = 0; i < obj->m_VariantGroups.size(); ++i)
		{
			// Copy the group's variant names into a new vector
			std::vector<CStr> group;
			group.reserve(obj->m_VariantGroups[i].size());
			for (size_t j = 0; j < obj->m_VariantGroups[i].size(); ++j)
				group.push_back(obj->m_VariantGroups[i][j].m_VariantName);

			// If this group is identical to one elsewhere, don't bother listing
			// it twice.
			// Linear search is theoretically not very efficient, but hopefully
			// we don't have enough props for that to matter...
			bool dupe = false;
			for (size_t j = 0; j < groups.size(); ++j)
			{
				if (groups[j] == group)
				{
					dupe = true;
					break;
				}
			}
			if (dupe)
				continue;

			// Add non-trivial groups (i.e. not just one entry) to the returned list
			if (obj->m_VariantGroups[i].size() > 1)
				groups.push_back(group);

			// Add all props onto the queue to be considered
			for (size_t j = 0; j < obj->m_VariantGroups[i].size(); ++j)
			{
				const std::vector<Prop>& props = obj->m_VariantGroups[i][j].m_Props;
				for (size_t k = 0; k < props.size(); ++k)
					if (!props[k].m_ModelName.empty())
						if (auto [success, prop] = m_ObjectManager.FindActorDef(props[k].m_ModelName.c_str()); success)
							objectsQueue.push(prop.GetBase(m_QualityLevel).get());
			}
		}
	}

	return groups;
}

void CObjectBase::GetQualitySplits(std::vector<u8>& splits) const
{
	std::vector<u8>::iterator it = std::find_if(splits.begin(), splits.end(), [this](u8 qualityLevel) { return qualityLevel >= m_QualityLevel; });
	if (it == splits.end() ||  *it != m_QualityLevel)
		splits.emplace(it, m_QualityLevel);

	for (const std::vector<Variant>& group : m_VariantGroups)
		for (const Variant& variant : group)
			for (const Prop& prop : variant.m_Props)
			{
				// TODO: we probably should clean those up after XML load.
				if (prop.m_ModelName.empty())
					continue;

				auto [success, propActor] = m_ObjectManager.FindActorDef(prop.m_ModelName.c_str());
				if (!success)
					continue;

				std::vector<u8> newSplits = propActor.QualityLevels();
				if (newSplits.size() <= 1)
					continue;

				// This is not entirely optimal since we might loop though redundant quality levels, but that shouldn't matter.
				// Custom implementation because this is inplace, std::set_union needs a 3rd vector.
				std::vector<u8>::iterator v1 = splits.begin();
				std::vector<u8>::iterator v2 = newSplits.begin();
				while (v2 != newSplits.end())
				{
					if (v1 == splits.end() || *v1 > *v2)
					{
						v1 = ++splits.insert(v1, *v2);
						++v2;
					}
					else if (*v1 == *v2)
					{
						++v1;
						++v2;
					}
					else
						++v1;
				}
			}
}

const CStr& CObjectBase::GetIdentifier() const
{
	return m_Identifier;
}

bool CObjectBase::UsesFile(const VfsPath& pathname) const
{
	return m_ActorDef.UsesFile(pathname);
}


CActorDef::CActorDef(CObjectManager& objectManager) : m_ObjectManager(objectManager)
{
}

std::vector<u8> CActorDef::QualityLevels() const
{
	std::vector<u8> splits;
	splits.reserve(m_ObjectBases.size());
	for (const std::shared_ptr<CObjectBase>& base : m_ObjectBases)
		splits.emplace_back(base->m_QualityLevel);
	return splits;
}

const std::shared_ptr<CObjectBase>& CActorDef::GetBase(u8 QualityLevel) const
{
	for (const std::shared_ptr<CObjectBase>& base : m_ObjectBases)
		if (base->m_QualityLevel >= QualityLevel)
			return base;
	// This code path ought to be impossible to take,
	// because by construction we must have at least one valid CObjectBase of quality MAX_QUALITY
	// (which necessarily fits the u8 comparison above).
	// However compilers will warn that we return a reference to a local temporary if I return nullptr,
	// so just return something sane instead.
	ENSURE(false);
	return m_ObjectBases.back();
}

bool CActorDef::Load(const VfsPath& pathname)
{
	m_UsedFiles.clear();
	m_UsedFiles.insert(pathname);

	m_ObjectBases.clear();

	CXeromyces XeroFile;
	if (XeroFile.Load(g_VFS, pathname, "actor") != PSRETURN_OK)
		return false;

	// Define all the elements used in the XML file
#define EL(x) int el_##x = XeroFile.GetElementID(#x)
#define AT(x) int at_##x = XeroFile.GetAttributeID(#x)
	EL(actor);
	EL(inline);
	EL(qualitylevels);
	AT(file);
	AT(inline);
	AT(quality);
	AT(version);
#undef AT
#undef EL

	XMBElement root = XeroFile.GetRoot();

	if (root.GetNodeName() != el_actor && root.GetNodeName() != el_qualitylevels)
	{
		LOGERROR("Invalid actor format (actor '%s', unrecognised root element '%s')",
				 pathname.string8().c_str(), XeroFile.GetElementString(root.GetNodeName()).c_str());
		return false;
	}

	m_Pathname = pathname;

	if (root.GetNodeName() == el_actor)
	{
		std::unique_ptr<CObjectBase> base = std::make_unique<CObjectBase>(m_ObjectManager, *this, MAX_QUALITY);
		base->Load(XeroFile, root);
		m_ObjectBases.emplace_back(std::move(base));
	}
	else
	{
		XERO_ITER_ATTR(root, attr)
		{
			if (attr.Name == at_version && attr.Value.ToInt() != 1)
			{
				LOGERROR("Invalid actor format (actor '%s', version %i is not supported)",
						 pathname.string8().c_str(), attr.Value.ToInt());
				return false;
			}
		}
		u8 quality = 0;
		XMBElement inlineActor;
		XERO_ITER_EL(root, child)
		{
			if (child.GetNodeName() == el_inline)
				inlineActor = child;
		}
		XERO_ITER_EL(root, actor)
		{
			if (actor.GetNodeName() != el_actor)
				continue;
			bool found_quality = false;
			bool use_inline = false;
			CStr file;
			XERO_ITER_ATTR(actor, attr)
			{
				if (attr.Name == at_quality)
				{
					int v = GetQuality(attr.Value);
					if (v > MAX_QUALITY)
					{
						LOGERROR("Qualitylevel to attribute must not be above %i (file %s)", MAX_QUALITY, pathname.string8());
						return false;
					}
					if (v <= quality)
					{
						LOGERROR("Elements must be in increasing quality order (file %s)", pathname.string8());
						return false;
					}
					quality = v;
					found_quality = true;
				}
				else if (attr.Name == at_file)
				{
					if (attr.Value.empty())
						LOGWARNING("Empty actor file specified (file %s)", pathname.string8());
					file = attr.Value;
				}
				else if (attr.Name == at_inline)
					use_inline = true;
			}
			if (!found_quality)
				quality = MAX_QUALITY;
			std::unique_ptr<CObjectBase> base = std::make_unique<CObjectBase>(m_ObjectManager, *this, quality);
			if (use_inline)
			{
				if (inlineActor.GetNodeName() == -1)
				{
					LOGERROR("Actor quality level refers to inline definition, but no inline definition found (file %s)", pathname.string8());
					return false;
				}
				base->Load(XeroFile, inlineActor);
			}
			else if (file.empty())
				base->Load(XeroFile, actor);
			else
			{
				if (actor.GetChildNodes().size() > 0)
					LOGWARNING("Actor definition refers to file but has children elements, they will be ignored (file %s)", pathname.string8());

				// Open up an external file to load.
				// Don't crash hard when failures happen, but log them and continue
				CXeromyces XeroActor;
				if (XeroActor.Load(g_VFS, "art/actors/" + file, "actor") == PSRETURN_OK)
				{
					const XMBElement& root = XeroActor.GetRoot();
					if (root.GetNodeName() != el_actor)
					{
						LOGERROR("Included actors cannot define quality levels (opening %s from file %s)", file, pathname.string8());
						return false;
					}
					base->Load(XeroActor, root);
				}
				else
				{
					LOGERROR("Could not open actor file at path %s (file %s)", file, pathname.string8());
					return false;
				}
				m_UsedFiles.insert(file);
			}
			m_ObjectBases.emplace_back(std::move(base));
		}
		if (quality != MAX_QUALITY)
		{
			LOGERROR("Quality levels must go up to %i (file %s)", MAX_QUALITY, pathname.string8().c_str());
			return false;
		}
	}

	// For each quality level, check if we need to further split (because of props).
	std::vector<u8> splits = QualityLevels();
	for (const std::shared_ptr<CObjectBase>& base : m_ObjectBases)
		base->GetQualitySplits(splits);
	ENSURE(splits.size() >= 1);
	if (splits.size() > MAX_LEVELS_PER_ACTOR_DEF)
	{
		LOGERROR("Too many quality levels (%i) for actor %s (max %i)", splits.size(), pathname.string8().c_str(), MAX_LEVELS_PER_ACTOR_DEF);
		return false;
	}

	std::vector<std::shared_ptr<CObjectBase>>::iterator it = m_ObjectBases.begin();
	std::vector<u8>::const_iterator qualityLevels = splits.begin();
	while (it != m_ObjectBases.end())
		if ((*it)->m_QualityLevel > *qualityLevels)
		{
			it = ++m_ObjectBases.emplace(it, (*it)->CopyWithQuality(*qualityLevels));
			++qualityLevels;
		}
		else if ((*it)->m_QualityLevel == *qualityLevels)
		{
			++it;
			++qualityLevels;
		}
		else
			++it;

	return true;
}

bool CActorDef::UsesFile(const VfsPath& pathname) const
{
	return m_UsedFiles.find(pathname) != m_UsedFiles.end();
}

void CActorDef::LoadErrorPlaceholder(const VfsPath& pathname)
{
	m_UsedFiles.clear();
	m_ObjectBases.clear();
	m_UsedFiles.emplace(pathname);
	m_Pathname = pathname;
	m_ObjectBases.emplace_back(std::make_shared<CObjectBase>(m_ObjectManager, *this, MAX_QUALITY));
}
