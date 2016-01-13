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

#include "precompiled.h"

#include "ObjectEntry.h"

#include "graphics/Decal.h"
#include "graphics/Material.h"
#include "graphics/MaterialManager.h"
#include "graphics/MeshManager.h"
#include "graphics/Model.h"
#include "graphics/ModelDef.h"
#include "graphics/ObjectBase.h"
#include "graphics/ObjectManager.h"
#include "graphics/ParticleManager.h"
#include "graphics/SkeletonAnim.h"
#include "graphics/TextureManager.h"
#include "lib/rand.h"
#include "ps/CLogger.h"
#include "ps/Game.h"
#include "ps/World.h"
#include "renderer/Renderer.h"
#include "simulation2/Simulation2.h"

#include <sstream>

CObjectEntry::CObjectEntry(CObjectBase* base, CSimulation2& simulation) :
	m_Base(base), m_Color(1.0f, 1.0f, 1.0f, 1.0f), m_Model(NULL), m_Outdated(false), m_Simulation(simulation)
{
}

CObjectEntry::~CObjectEntry()
{
	for (const std::pair<CStr, CSkeletonAnim*>& anim : m_Animations)
		delete anim.second;

	delete m_Model;
}


bool CObjectEntry::BuildVariation(const std::vector<std::set<CStr> >& selections,
								  const std::vector<u8>& variationKey,
								  CObjectManager& objectManager)
{
	CObjectBase::Variation variation = m_Base->BuildVariation(variationKey);

	// Copy the chosen data onto this model:

	for (std::multimap<CStr, CObjectBase::Samp>::iterator it = variation.samplers.begin(); it != variation.samplers.end(); ++it)
		m_Samplers.push_back(it->second);
	
	m_ModelName = variation.model;

	if (! variation.color.empty())
	{
		std::stringstream str;
		str << variation.color;
		int r, g, b;
		if (! (str >> r >> g >> b)) // Any trailing data is ignored
			LOGERROR("Actor '%s' has invalid RGB color '%s'", utf8_from_wstring(m_Base->m_ShortName), variation.color);
		else
			m_Color = CColor(r/255.0f, g/255.0f, b/255.0f, 1.0f);
	}

	if (variation.decal.m_SizeX && variation.decal.m_SizeZ)
	{
		CMaterial material = g_Renderer.GetMaterialManager().LoadMaterial(m_Base->m_Material);
		
		for (const CObjectBase::Samp& samp : m_Samplers)
		{
			CTextureProperties textureProps(samp.m_SamplerFile);
			textureProps.SetWrap(GL_CLAMP_TO_BORDER);
			CTexturePtr texture = g_Renderer.GetTextureManager().CreateTexture(textureProps);
			// TODO: Should check which renderpath is selected and only preload the necessary textures.
			texture->Prefetch(); 
			material.AddSampler(CMaterial::TextureSampler(samp.m_SamplerName, texture));
		}

		SDecal decal(material,
			variation.decal.m_SizeX, variation.decal.m_SizeZ,
			variation.decal.m_Angle, variation.decal.m_OffsetX, variation.decal.m_OffsetZ,
			m_Base->m_Properties.m_FloatOnWater);
		m_Model = new CModelDecal(objectManager.GetTerrain(), decal);

		return true;
	}

	if (!variation.particles.empty())
	{
		m_Model = new CModelParticleEmitter(g_Renderer.GetParticleManager().LoadEmitterType(variation.particles));
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
		LOGERROR("CObjectEntry::BuildVariation(): Model %s failed to load", m_ModelName.string8());
		return false;
	}

	// delete old model, create new 
	CModel* model = new CModel(objectManager.GetSkeletonAnimManager(), m_Simulation);
	delete m_Model;
	m_Model = model;
	model->SetMaterial(g_Renderer.GetMaterialManager().LoadMaterial(m_Base->m_Material));
	model->GetMaterial().AddStaticUniform("objectColor", CVector4D(m_Color.r, m_Color.g, m_Color.b, m_Color.a));
	model->InitModel(modeldef);
	
	if (m_Samplers.empty())
		LOGERROR("Actor '%s' has no textures.", utf8_from_wstring(m_Base->m_ShortName));
	
	for (const CObjectBase::Samp& samp : m_Samplers)
	{
		CTextureProperties textureProps(samp.m_SamplerFile);
		textureProps.SetWrap(GL_CLAMP_TO_EDGE);
		CTexturePtr texture = g_Renderer.GetTextureManager().CreateTexture(textureProps);
		// if we've loaded this model we're probably going to render it soon, so prefetch its texture. 
		// All textures are prefetched even in the fixed pipeline, including the normal maps etc.
		// TODO: Should check which renderpath is selected and only preload the necessary textures.
		texture->Prefetch(); 
		model->GetMaterial().AddSampler(CMaterial::TextureSampler(samp.m_SamplerName, texture));
	}

	for (const CStrIntern& requSampName : model->GetMaterial().GetRequiredSampler())
	{
		if (std::find_if(m_Samplers.begin(), m_Samplers.end(),
		                 [&](const CObjectBase::Samp& sampler) { return sampler.m_SamplerName == requSampName; }) == m_Samplers.end())
			LOGERROR("Actor %s: required texture sampler %s not found (material %s)", utf8_from_wstring(m_Base->m_ShortName), requSampName.string().c_str(), m_Base->m_Material.string8().c_str());
	}
	
	// calculate initial object space bounds, based on vertex positions
	model->CalcStaticObjectBounds();

	// load the animations
	for (std::multimap<CStr, CObjectBase::Anim>::iterator it = variation.anims.begin(); it != variation.anims.end(); ++it)
	{
		CStr name = it->first.LowerCase();

		if (!it->second.m_FileName.empty())
		{
			CSkeletonAnim* anim = model->BuildAnimation(it->second.m_FileName, name, it->second.m_Speed, it->second.m_ActionPos, it->second.m_ActionPos2, it->second.m_SoundPos);
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
		anim->m_SoundPos = 0.f;
		m_Animations.insert(std::make_pair("idle", anim));

		// Ignore errors, since they're probably saying this is a non-animated model
		model->SetAnimation(anim);
	}
	else
	{
		// start up idling
		if (!model->SetAnimation(GetRandomAnimation("idle")))
			LOGERROR("Failed to set idle animation in model \"%s\"", m_ModelName.string8());
	}

	// build props - TODO, RC - need to fix up bounds here
	// TODO: Make sure random variations get handled correctly when a prop fails
	for (const CObjectBase::Prop& prop : props)
	{
		// Pluck out the special attachpoint 'projectile'
		if (prop.m_PropPointName == "projectile")
		{
			m_ProjectileModelName = prop.m_ModelName;
			continue;
		}

		CObjectEntry* oe = objectManager.FindObjectVariation(prop.m_ModelName.c_str(), selections);
		if (!oe)
		{
			LOGERROR("Failed to build prop model \"%s\" on actor \"%s\"", utf8_from_wstring(prop.m_ModelName), utf8_from_wstring(m_Base->m_ShortName));
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
				model->AddProp(proppoint, propmodel, oe, prop.m_minHeight, prop.m_maxHeight, prop.m_selectable);
			if (propmodel->ToCModel())
				propmodel->ToCModel()->SetAnimation(oe->GetRandomAnimation("idle"));
		}
		else
			LOGERROR("Failed to find matching prop point called \"%s\" in model \"%s\" for actor \"%s\"", ppn, m_ModelName.string8(), utf8_from_wstring(m_Base->m_ShortName));
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
