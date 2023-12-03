/* Copyright (C) 2023 Wildfire Games.
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
#include "graphics/ModelDummy.h"
#include "graphics/ObjectBase.h"
#include "graphics/ObjectManager.h"
#include "graphics/ParticleManager.h"
#include "graphics/SkeletonAnim.h"
#include "graphics/SkeletonAnimManager.h"
#include "graphics/TextureManager.h"
#include "lib/rand.h"
#include "ps/CLogger.h"
#include "ps/CStrInternStatic.h"
#include "ps/Game.h"
#include "ps/World.h"
#include "renderer/Renderer.h"
#include "renderer/SceneRenderer.h"
#include "simulation2/Simulation2.h"

#include <sstream>

CObjectEntry::CObjectEntry(const std::shared_ptr<CObjectBase>& base, const CSimulation2& simulation) :
	m_Base(base), m_Color(1.0f, 1.0f, 1.0f, 1.0f), m_Simulation(simulation)
{
}

CObjectEntry::~CObjectEntry() = default;

bool CObjectEntry::BuildVariation(const std::vector<const std::set<CStr>*>& completeSelections,
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
			LOGERROR("Actor '%s' has invalid RGB color '%s'", m_Base->GetIdentifier(), variation.color);
		else
			m_Color = CColor(r/255.0f, g/255.0f, b/255.0f, 1.0f);
	}

	if (variation.decal.m_SizeX && variation.decal.m_SizeZ)
	{
		CMaterial material = g_Renderer.GetSceneRenderer().GetMaterialManager().LoadMaterial(m_Base->m_Material);

		for (const CObjectBase::Samp& samp : m_Samplers)
		{
			CTextureProperties textureProps(samp.m_SamplerFile);
			// TODO: replace all samplers by CLAMP_TO_EDGE after decals
			// refactoring. Also we need to avoid custom border colors.
			textureProps.SetAddressMode(
				samp.m_SamplerName == str_baseTex
					? Renderer::Backend::Sampler::AddressMode::CLAMP_TO_BORDER
					: Renderer::Backend::Sampler::AddressMode::CLAMP_TO_EDGE);
			CTexturePtr texture = g_Renderer.GetTextureManager().CreateTexture(textureProps);
			// TODO: Should check which renderpath is selected and only preload the necessary textures.
			texture->Prefetch();
			material.AddSampler(CMaterial::TextureSampler(samp.m_SamplerName, texture));
		}

		SDecal decal(material,
			variation.decal.m_SizeX, variation.decal.m_SizeZ,
			variation.decal.m_Angle, variation.decal.m_OffsetX, variation.decal.m_OffsetZ,
			m_Base->m_Properties.m_FloatOnWater);
		m_Model = std::make_unique<CModelDecal>(objectManager.GetTerrain(), decal);

		return true;
	}

	if (!variation.particles.empty())
	{
		m_Model = std::make_unique<CModelParticleEmitter>(g_Renderer.GetSceneRenderer().GetParticleManager().LoadEmitterType(variation.particles));
		return true;
	}

	if (variation.model.empty())
	{
		m_Model = std::make_unique<CModelDummy>();
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
	CMaterial material = g_Renderer.GetSceneRenderer().GetMaterialManager().LoadMaterial(m_Base->m_Material);
	material.AddStaticUniform("objectColor", CVector4D(m_Color.r, m_Color.g, m_Color.b, m_Color.a));

	if (m_Samplers.empty())
		LOGERROR("Actor '%s' has no textures.", m_Base->GetIdentifier());

	for (const CObjectBase::Samp& samp : m_Samplers)
	{
		CTextureProperties textureProps(samp.m_SamplerFile);
		textureProps.SetAddressMode(Renderer::Backend::Sampler::AddressMode::CLAMP_TO_EDGE);
		CTexturePtr texture = g_Renderer.GetTextureManager().CreateTexture(textureProps);
		// if we've loaded this model we're probably going to render it soon, so prefetch its texture.
		// All textures are prefetched even in the fixed pipeline, including the normal maps etc.
		// TODO: Should check which renderpath is selected and only preload the necessary textures.
		texture->Prefetch();
		material.AddSampler(CMaterial::TextureSampler(samp.m_SamplerName, texture));
	}

	std::unique_ptr<CModel> newModel = std::make_unique<CModel>(m_Simulation, material, modeldef);
	CModel* model = newModel.get();
	m_Model = std::move(newModel);

	for (const CStrIntern& requSampName : model->GetMaterial().GetRequiredSampler())
	{
		if (std::find_if(m_Samplers.begin(), m_Samplers.end(),
		                 [&](const CObjectBase::Samp& sampler) { return sampler.m_SamplerName == requSampName; }) == m_Samplers.end())
			LOGERROR("Actor %s: required texture sampler %s not found (material %s)", m_Base->GetIdentifier(), requSampName.string().c_str(), m_Base->m_Material.string8().c_str());
	}

	// calculate initial object space bounds, based on vertex positions
	model->CalcStaticObjectBounds();

	// load the animations
	for (std::multimap<CStr, CObjectBase::Anim>::iterator it = variation.anims.begin(); it != variation.anims.end(); ++it)
	{
		CStr name = it->first.LowerCase();

		if (it->second.m_FileName.empty())
			continue;
		std::unique_ptr<CSkeletonAnim> anim = objectManager.GetSkeletonAnimManager().BuildAnimation(
			it->second.m_FileName,
			name,
			it->second.m_ID,
			it->second.m_Frequency,
			it->second.m_Speed,
			it->second.m_ActionPos,
			it->second.m_ActionPos2,
			it->second.m_SoundPos);
		if (anim)
			m_Animations.emplace(name, std::move(anim));
	}

	// ensure there's always an idle animation
	if (m_Animations.find("idle") == m_Animations.end())
	{
		std::unique_ptr<CSkeletonAnim> anim = std::make_unique<CSkeletonAnim>();
		anim->m_Name = "idle";
		anim->m_ID = "";
		anim->m_AnimDef = NULL;
		anim->m_Frequency = 0;
		anim->m_Speed = 0.f;
		anim->m_ActionPos = 0.f;
		anim->m_ActionPos2 = 0.f;
		anim->m_SoundPos = 0.f;
		SkeletonAnimMap::const_iterator it = m_Animations.emplace("idle", std::move(anim));

		// Ignore errors, since they're probably saying this is a non-animated model
		model->SetAnimation(it->second.get());
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

		CObjectEntry* oe = nullptr;
		if (auto [success, actorDef] = objectManager.FindActorDef(prop.m_ModelName.c_str()); success)
			oe = objectManager.FindObjectVariation(actorDef.GetBase(m_Base->m_QualityLevel), completeSelections);

		if (!oe)
		{
			LOGERROR("Failed to build prop model \"%s\" on actor \"%s\"", utf8_from_wstring(prop.m_ModelName), m_Base->GetIdentifier());
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
			std::unique_ptr<CModelAbstract> propmodel = oe->m_Model->Clone();
			if (propmodel->ToCModel())
				propmodel->ToCModel()->SetAnimation(oe->GetRandomAnimation("idle"));
			if (isAmmo)
				model->AddAmmoProp(proppoint, std::move(propmodel), oe);
			else
				model->AddProp(proppoint, std::move(propmodel), oe, prop.m_minHeight, prop.m_maxHeight, prop.m_selectable);
		}
		else
			LOGERROR("Failed to find matching prop point called \"%s\" in model \"%s\" for actor \"%s\"", ppn, m_ModelName.string8(), m_Base->GetIdentifier());
	}

	// Setup flags.
	if (m_Base->m_Properties.m_CastShadows)
	{
		model->SetFlags(model->GetFlags() | ModelFlag::CAST_SHADOWS);
	}

	if (m_Base->m_Properties.m_FloatOnWater)
	{
		model->SetFlags(model->GetFlags() | ModelFlag::FLOAT_ON_WATER);
	}

	return true;
}

CSkeletonAnim* CObjectEntry::GetRandomAnimation(const CStr& animationName, const CStr& ID) const
{
	std::vector<CSkeletonAnim*> anims = GetAnimations(animationName, ID);

	int totalFreq = 0;
	for (CSkeletonAnim* anim : anims)
		totalFreq += anim->m_Frequency;

	if (totalFreq == 0)
		return anims[rand(0, anims.size())];

	int r = rand(0, totalFreq);
	for (CSkeletonAnim* anim : anims)
	{
		r -= anim->m_Frequency;
		if (r < 0)
			return anim;
	}
	return NULL;
}

std::vector<CSkeletonAnim*> CObjectEntry::GetAnimations(const CStr& animationName, const CStr& ID) const
{
	std::vector<CSkeletonAnim*> anims;

	SkeletonAnimMap::const_iterator lower = m_Animations.lower_bound(animationName);
	SkeletonAnimMap::const_iterator upper = m_Animations.upper_bound(animationName);

	for (SkeletonAnimMap::const_iterator it = lower; it != upper; ++it)
	{
		if (ID.empty() || it->second->m_ID == ID)
			anims.push_back(it->second.get());
	}

	if (anims.empty())
	{
		lower = m_Animations.lower_bound("idle");
		upper = m_Animations.upper_bound("idle");
		for (SkeletonAnimMap::const_iterator it = lower; it != upper; ++it)
			anims.push_back(it->second.get());
	}

	ENSURE(!anims.empty());
	return anims;
}
