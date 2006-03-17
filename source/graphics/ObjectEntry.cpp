#include "precompiled.h"

#include "ObjectEntry.h"
#include "ObjectManager.h"
#include "ObjectBase.h"
#include "Model.h"
#include "ModelDef.h"
#include "CLogger.h"
#include "MaterialManager.h"
#include "MeshManager.h"

#include "UnitManager.h"
#include "Unit.h"

#include "ps/XML/Xeromyces.h"
#include "ps/XML/XMLWriter.h"

#include "lib/res/file/vfs.h"

#include <sstream>

#define LOG_CATEGORY "graphics"

CObjectEntry::CObjectEntry(int type, CObjectBase* base)
: m_Base(base), m_Color(1.0f, 1.0f, 1.0f, 1.0f),
  m_ProjectileModel(NULL), m_AmmunitionModel(NULL), m_AmmunitionPoint(NULL), m_Model(NULL), m_Type(type)
{
}

template<typename T, typename S> static void delete_pair_2nd(std::pair<T,S> v) { delete v.second; }

CObjectEntry::~CObjectEntry()
{
	std::for_each(m_Animations.begin(), m_Animations.end(), delete_pair_2nd<CStr, CSkeletonAnim*>);

	delete m_Model;
}


bool CObjectEntry::BuildVariation(const std::vector<std::set<CStrW> >& selections)
{
	CStr chosenTexture;
	CStr chosenModel;
	CStr chosenColor;
	std::map<CStr, CObjectBase::Prop> chosenProps;
	std::multimap<CStr, CObjectBase::Anim> chosenAnims;

	for (std::vector<std::vector<CObjectBase::Variant> >::iterator grp = m_Base->m_Variants.begin();
		grp != m_Base->m_Variants.end();
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

			for (std::vector<std::set<CStrW> >::const_iterator selset = selections.begin(); selset < selections.end(); ++selset)
			{
				debug_assert(grp->size() < 256); // else they won't fit in 'choices'

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

		// Get the matched variant
		CObjectBase::Variant& var ((*grp)[match]);

		// Apply its data:

		if (var.m_TextureFilename.Length())
			chosenTexture = var.m_TextureFilename;

		if (var.m_ModelFilename.Length())
			chosenModel = var.m_ModelFilename;

		if (var.m_Color.Length())
			chosenColor = var.m_Color;

		for (std::vector<CObjectBase::Prop>::iterator it = var.m_Props.begin(); it != var.m_Props.end(); ++it)
			chosenProps[it->m_PropPointName] = *it;

		// If one variant defines one animation called e.g. "attack", and this
		// variant defines two different animations with the same name, the one
		// original should be erased, and replaced by the two new ones.
		//
		// So, erase all existing animations which are overridden by this variant:
		for (std::vector<CObjectBase::Anim>::iterator it = var.m_Anims.begin(); it != var.m_Anims.end(); ++it)
			chosenAnims.erase(chosenAnims.lower_bound(it->m_AnimName), chosenAnims.upper_bound(it->m_AnimName));
		// and then insert the new ones:
		for (std::vector<CObjectBase::Anim>::iterator it = var.m_Anims.begin(); it != var.m_Anims.end(); ++it)
			chosenAnims.insert(make_pair(it->m_AnimName, *it));
	}

	// Copy the chosen data onto this model:

	m_TextureName = chosenTexture;
	m_ModelName = chosenModel;

	if (chosenColor.Length())
	{
		std::stringstream str;
		str << chosenColor;
		int r, g, b;
		if (! (str >> r >> g >> b)) // Any trailing data is ignored
			LOG(ERROR, LOG_CATEGORY, "Invalid RGB colour '%s'", chosenColor.c_str());
		else
			m_Color = CColor(r/255.0f, g/255.0f, b/255.0f, 1.0f);
	}

	std::vector<CObjectBase::Prop> props;

	for (std::map<CStr, CObjectBase::Prop>::iterator it = chosenProps.begin(); it != chosenProps.end(); ++it)
		props.push_back(it->second);

	// Build the model:

	// get the root directory of this object
	CStr dirname = g_ObjMan.m_ObjectTypes[m_Type].m_Name;

	// remember the old model so we can replace any models using it later on
	CModelDefPtr oldmodeldef = m_Model ? m_Model->GetModelDef() : CModelDefPtr();

	const char* modelfilename = m_ModelName;

	// try and create a model
	CModelDefPtr modeldef (g_MeshManager.GetMesh(modelfilename));
	if (!modeldef)
	{
		LOG(ERROR, LOG_CATEGORY, "CObjectEntry::BuildModel(): Model %s failed to load", modelfilename);
		return false;
	}

	// delete old model, create new 
	delete m_Model;
	m_Model = new CModel;
	m_Model->SetTexture((const char*) m_TextureName);
	m_Model->SetMaterial(g_MaterialManager.LoadMaterial(m_Base->m_Material));
	m_Model->InitModel(modeldef);
	m_Model->SetPlayerColor(m_Color);

	// calculate initial object space bounds, based on vertex positions
	m_Model->CalcObjectBounds();

	// load the animations
	for (std::multimap<CStr, CObjectBase::Anim>::iterator it = chosenAnims.begin(); it != chosenAnims.end(); ++it)
	{
		CStr name = it->first.LowerCase();

		// TODO: Use consistent names everywhere, then remove this translation section.
		// (It's just mapping the names used in actors onto the names used by code.)
		if (name == "attack") name = "melee";
		else if (name == "chop") name = "gather";
		else if (name == "decay") name = "corpse";

		if (it->second.m_FileName.Length())
		{
			CSkeletonAnim* anim = m_Model->BuildAnimation(it->second.m_FileName, name, it->second.m_Speed, it->second.m_ActionPos, it->second.m_ActionPos2);
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
		m_Model->SetAnimation(anim);
	}
	else
	{
		// start up idling
		if (! m_Model->SetAnimation(GetRandomAnimation("idle")))
			LOG(ERROR, LOG_CATEGORY, "Failed to set idle animation in model \"%s\"", modelfilename);
	}

	// build props - TODO, RC - need to fix up bounds here
	// TODO: Make sure random variations get handled correctly when a prop fails
	for (size_t p = 0; p < props.size(); p++)
	{
		const CObjectBase::Prop& prop = props[p];
	
		CObjectEntry* oe = g_ObjMan.FindObjectVariation(prop.m_ModelName, selections);
		if (!oe)
		{
			LOG(ERROR, LOG_CATEGORY, "Failed to build prop model \"%s\" on actor \"%s\"", (const char*)prop.m_ModelName, (const char*)m_Base->m_ShortName);
			continue;
		}

		// Pluck out the special attachpoint 'projectile'
		if( prop.m_PropPointName == "projectile" )
		{
			m_ProjectileModel = oe->m_Model;
		}
		// Also the other special attachpoint 'loaded-<proppoint>'
		else if( ( prop.m_PropPointName.Length() > 7 ) && ( prop.m_PropPointName.Left( 7 ) == "loaded-" ) )
		{
			CStr ppn = prop.m_PropPointName.GetSubstring( 7, prop.m_PropPointName.Length() - 7 );
			m_AmmunitionModel = oe->m_Model;
			m_AmmunitionPoint = modeldef->FindPropPoint((const char*)ppn );
			if( !m_AmmunitionPoint )
				LOG(ERROR, LOG_CATEGORY, "Failed to find matching prop point called \"%s\" in model \"%s\" on actor \"%s\"", (const char*)ppn, modelfilename, (const char*)prop.m_ModelName);
		}
		else
		{
			SPropPoint* proppoint = modeldef->FindPropPoint((const char*) prop.m_PropPointName);
			if (proppoint)
			{
				CModel* propmodel = oe->m_Model->Clone();
				m_Model->AddProp(proppoint, propmodel);
				propmodel->SetAnimation(oe->GetRandomAnimation("idle"));
			}
			else
				LOG(ERROR, LOG_CATEGORY, "Failed to find matching prop point called \"%s\" in model \"%s\" on actor \"%s\"", (const char*)prop.m_PropPointName, modelfilename, (const char*)prop.m_ModelName);
		}
	}

	// setup flags
	if (m_Base->m_Properties.m_CastShadows)
	{
		m_Model->SetFlags(m_Model->GetFlags()|MODELFLAG_CASTSHADOWS);
	}

	// replace any units using old model to now use new model; also reprop models, if necessary
	// FIXME, RC - ugh, doesn't recurse correctly through props
/*	
	// (PT: Removed this, since I'm not entirely sure what it's useful for, and it
	//  gets a bit confusing with randomised actors)

	const std::vector<CUnit*>& units = g_UnitMan.GetUnits();
	for (size_t i = 0; i < units.size(); ++i)
	{
		CModel* unitmodel=units[i]->GetModel();
		if (unitmodel->GetModelDef() == oldmodeldef)
		{
			unitmodel->InitModel(m_Model->GetModelDef());
			unitmodel->SetFlags(m_Model->GetFlags());

			const std::vector<CModel::Prop>& newprops = m_Model->GetProps();
			for (size_t j = 0; j < newprops.size(); j++)
				unitmodel->AddProp(newprops[j].m_Point, newprops[j].m_Model->Clone());
		}

		std::vector<CModel::Prop>& mdlprops = unitmodel->GetProps();
		for (size_t j = 0; j < mdlprops.size(); j++)
		{
			CModel::Prop& prop = mdlprops[j];
			if (prop.m_Model)
			{
				if (prop.m_Model->GetModelDef() == oldmodeldef)
				{
					delete prop.m_Model;
					prop.m_Model = m_Model->Clone();
				}
			}
		}
	}
*/
	return true;
}

CSkeletonAnim* CObjectEntry::GetRandomAnimation(const CStr& animationName)
{
	SkeletonAnimMap::iterator lower = m_Animations.lower_bound(animationName);
	SkeletonAnimMap::iterator upper = m_Animations.upper_bound(animationName);
	size_t count = std::distance(lower, upper);
	if (count == 0)
	{
//		LOG(WARNING, LOG_CATEGORY, "Failed to find animation '%s' for actor '%s'", animationName.c_str(), m_ModelName.c_str());
		return NULL;
	}
	else
	{
		// TODO: Do we care about network synchronisation of random animations?
		int id = rand(0, (int)count);
		std::advance(lower, id);
		return lower->second;
	}
}
