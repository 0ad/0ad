/* Copyright (C) 2011 Wildfire Games.
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

#include "ObjectEntry.h"

#include "graphics/Decal.h"
#include "graphics/MaterialManager.h"
#include "graphics/MeshManager.h"
#include "graphics/Model.h"
#include "graphics/ModelDef.h"
#include "graphics/ObjectBase.h"
#include "graphics/ObjectManager.h"
#include "graphics/SkeletonAnim.h"
#include "graphics/TextureManager.h"
#include "lib/rand.h"
#include "ps/CLogger.h"
#include "ps/Game.h"
#include "ps/World.h"
#include "renderer/Renderer.h"

#include <sstream>

CObjectEntry::CObjectEntry(CObjectBase* base) :
	m_Base(base), m_Color(1.0f, 1.0f, 1.0f, 1.0f), m_Model(NULL)
{
}

template<typename T, typename S> static void delete_pair_2nd(std::pair<T,S> v) { delete v.second; }

CObjectEntry::~CObjectEntry()
{
	std::for_each(m_Animations.begin(), m_Animations.end(), delete_pair_2nd<CStr, CSkeletonAnim*>);

	delete m_Model;
}


bool CObjectEntry::BuildVariation(const std::vector<std::set<CStr> >& selections,
								  const std::vector<u8>& variationKey,
								  CObjectManager& objectManager)
{
	CObjectBase::Variation variation = m_Base->BuildVariation(variationKey);

	// Copy the chosen data onto this model:

	m_TextureName = variation.texture;
	m_ModelName = variation.model;

	if (! variation.color.empty())
	{
		std::stringstream str;
		str << variation.color;
		int r, g, b;
		if (! (str >> r >> g >> b)) // Any trailing data is ignored
			LOGERROR(L"Actor '%ls' has invalid RGB colour '%hs'", m_Base->m_ShortName.c_str(), variation.color.c_str());
		else
			m_Color = CColor(r/255.0f, g/255.0f, b/255.0f, 1.0f);
	}

	if (variation.decal.m_SizeX && variation.decal.m_SizeZ)
	{
		CTextureProperties textureProps(m_TextureName);

		// Decals should be transparent, so clamp to the border (default 0,0,0,0)
		textureProps.SetWrap(GL_CLAMP_TO_BORDER);

		CTexturePtr texture = g_Renderer.GetTextureManager().CreateTexture(textureProps);
		texture->Prefetch(); // if we've loaded this model we're probably going to render it soon, so prefetch its texture

		SDecal decal(texture,
			variation.decal.m_SizeX, variation.decal.m_SizeZ,
			variation.decal.m_Angle, variation.decal.m_OffsetX, variation.decal.m_OffsetZ,
			m_Base->m_Properties.m_FloatOnWater);
		m_Model = new CModelDecal(g_Game->GetWorld()->GetTerrain(), decal);

		return true;
	}

	std::vector<CObjectBase::Prop> props;

	for (std::multimap<CStr, CObjectBase::Prop>::iterator it = variation.props.begin(); it != variation.props.end(); ++it)
		props.push_back(it->second);

	// Build the model:

	// try and create a model
	CModelDefPtr modeldef (objectManager.GetMeshManager().GetMesh(m_ModelName));
	if (!modeldef)
	{
		LOGERROR(L"CObjectEntry::BuildVariation(): Model %ls failed to load", m_ModelName.c_str());
		return false;
	}

	// delete old model, create new 
	CModel* model = new CModel(objectManager.GetSkeletonAnimManager());
	delete m_Model;
	m_Model = model;
	model->SetMaterial(g_MaterialManager.LoadMaterial(m_Base->m_Material));
	model->GetMaterial().SetTextureColor(m_Color);
	model->InitModel(modeldef);

	CTextureProperties textureProps(m_TextureName);
	textureProps.SetWrap(GL_CLAMP_TO_EDGE);
	CTexturePtr texture = g_Renderer.GetTextureManager().CreateTexture(textureProps);
	texture->Prefetch(); // if we've loaded this model we're probably going to render it soon, so prefetch its texture
	model->SetTexture(texture);

	// calculate initial object space bounds, based on vertex positions
	model->CalcObjectBounds();

	// load the animations
	for (std::multimap<CStr, CObjectBase::Anim>::iterator it = variation.anims.begin(); it != variation.anims.end(); ++it)
	{
		CStr name = it->first.LowerCase();

		// TODO: Use consistent names everywhere, then remove this translation section.
		// (It's just mapping the names used in actors onto the names used by code.)
		if (name == "attack") name = "melee";
		else if (name == "chop") name = "gather";
		else if (name == "decay") name = "corpse";

		if (! it->second.m_FileName.empty())
		{
			CSkeletonAnim* anim = model->BuildAnimation(it->second.m_FileName, name, it->second.m_Speed, it->second.m_ActionPos, it->second.m_ActionPos2);
			if (anim)
				m_Animations.insert(std::make_pair(name, anim));
		}
	}

	// ensure there's always an idle animation
	if (m_Animations.find("idle") == m_Animations.end())
	{
		CSkeletonAnim* anim = new CSkeletonAnim();
		anim->m_Name = "idle";
		anim->m_AnimDef = NULL;
		anim->m_Speed = 0.f;
		anim->m_ActionPos = 0.f;
		anim->m_ActionPos2 = 0.f;
		m_Animations.insert(std::make_pair("idle", anim));

		// Ignore errors, since they're probably saying this is a non-animated model
		model->SetAnimation(anim);
	}
	else
	{
		// start up idling
		if (!model->SetAnimation(GetRandomAnimation("idle")))
			LOGERROR(L"Failed to set idle animation in model \"%ls\"", m_ModelName.c_str());
	}

	// build props - TODO, RC - need to fix up bounds here
	// TODO: Make sure random variations get handled correctly when a prop fails
	for (size_t p = 0; p < props.size(); p++)
	{
		const CObjectBase::Prop& prop = props[p];

		// Pluck out the special attachpoint 'projectile'
		if (prop.m_PropPointName == "projectile")
		{
			m_ProjectileModelName = prop.m_ModelName;
			continue;
		}

		CObjectEntry* oe = objectManager.FindObjectVariation(prop.m_ModelName.c_str(), selections);
		if (!oe)
		{
			LOGERROR(L"Failed to build prop model \"%ls\" on actor \"%ls\"", prop.m_ModelName.c_str(), m_Base->m_ShortName.c_str());
			continue;
		}

		// If we don't have a projectile but this prop does (e.g. it's our rider), then
		// use that as our projectile too
		if (m_ProjectileModelName.empty() && !oe->m_ProjectileModelName.empty())
			m_ProjectileModelName = oe->m_ProjectileModelName;

		CStr ppn = prop.m_PropPointName;
		bool isAmmo = false;

		// Handle the special attachpoint 'loaded-<proppoint>'
		if (ppn.Find("loaded-") == 0)
		{
			ppn = prop.m_PropPointName.substr(7);
			isAmmo = true;
		}

		const SPropPoint* proppoint = modeldef->FindPropPoint(ppn.c_str());
		if (proppoint)
		{
			CModelAbstract* propmodel = oe->m_Model->Clone();
			if (isAmmo)
				model->AddAmmoProp(proppoint, propmodel, oe);
			else
				model->AddProp(proppoint, propmodel, oe);
			if (propmodel->ToCModel())
				propmodel->ToCModel()->SetAnimation(oe->GetRandomAnimation("idle"));
		}
		else
			LOGERROR(L"Failed to find matching prop point called \"%hs\" in model \"%ls\" for actor \"%ls\"", ppn.c_str(), m_ModelName.c_str(), m_Base->m_ShortName.c_str());
	}

	// setup flags
	if (m_Base->m_Properties.m_CastShadows)
	{
		model->SetFlags(model->GetFlags()|MODELFLAG_CASTSHADOWS);
	}

	return true;
}

CSkeletonAnim* CObjectEntry::GetRandomAnimation(const CStr& animationName) const
{
	SkeletonAnimMap::const_iterator lower = m_Animations.lower_bound(animationName);
	SkeletonAnimMap::const_iterator upper = m_Animations.upper_bound(animationName);
	size_t count = std::distance(lower, upper);
	if (count == 0)
		return NULL;

	size_t id = rand(0, count);
	std::advance(lower, id);
	return lower->second;
}

std::vector<CSkeletonAnim*> CObjectEntry::GetAnimations(const CStr& animationName) const
{
	std::vector<CSkeletonAnim*> anims;

	SkeletonAnimMap::const_iterator lower = m_Animations.lower_bound(animationName);
	SkeletonAnimMap::const_iterator upper = m_Animations.upper_bound(animationName);
	for (SkeletonAnimMap::const_iterator it = lower; it != upper; ++it)
		anims.push_back(it->second);
	return anims;
}
