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

#include "ps/Xeromyces.h"
#include "ps/XMLWriter.h"

#include "lib/res/vfs.h"

#define LOG_CATEGORY "graphics"


CObjectEntry::CObjectEntry(int type, CObjectBase* base)
: m_Model(0), m_Type(type), m_Base(base), m_Color(1.0f, 1.0f, 1.0f, 1.0f)
{
	m_IdleAnim=0;
	m_WalkAnim=0;
	m_DeathAnim=0;
	m_CorpseAnim=0;
	m_MeleeAnim=0;
	m_GatherAnim=0;
	m_RangedAnim=0;
	m_ProjectileModel=0;
	m_AmmunitionPoint=0;
	m_AmmunitionModel=0;
}

CObjectEntry::~CObjectEntry()
{
	for (size_t i=0;i<m_Animations.size();i++) {
		delete m_Animations[i].m_AnimData;
	}

	delete m_Model;
}

bool CObjectEntry::BuildRandomVariant(CObjectBase::variation_key& vars, CObjectBase::variation_key::iterator& vars_it)
{
	CStr chosenTexture;
	CStr chosenModel;
	CStr chosenColor;
	std::map<CStr, CObjectBase::Prop> chosenProps;
	std::map<CStr, CObjectBase::Anim> chosenAnims;

	// For each group in m_Base->m_Variants, take whichever variant is specified
	// by 'vars', and then store its data into the 'chosen' variables. If data
	// is specified more than once, the last value overrides all previous ones.

	for (std::vector<std::vector<CObjectBase::Variant> >::iterator grp = m_Base->m_Variants.begin();
		grp != m_Base->m_Variants.end();
		++grp)
	{
		if (vars_it == vars.end())
		{
			debug_warn("BuildRandomVariant is using too many vars");
			return false;
		}

		// Get the correct variant
		u8 var_id = *vars_it++;
		if (var_id < 0 || var_id >= grp->size())
		{
			LOG(ERROR, LOG_CATEGORY, "Internal error (BuildRandomVariant: %d not in 0..%d", var_id, grp->size()-1);
			continue;
		}
		CObjectBase::Variant& var ((*grp)[var_id]);

		// Apply its data:

		if (var.m_TextureFilename.Length())
			chosenTexture = var.m_TextureFilename;

		if (var.m_ModelFilename.Length())
			chosenModel = var.m_ModelFilename;

		if (var.m_Color.Length())
			chosenColor = var.m_Color;

		for (std::vector<CObjectBase::Prop>::iterator it = var.m_Props.begin(); it != var.m_Props.end(); ++it)
			chosenProps[it->m_PropPointName] = *it;

		for (std::vector<CObjectBase::Anim>::iterator it = var.m_Anims.begin(); it != var.m_Anims.end(); ++it)
			chosenAnims[it->m_AnimName] = *it;
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

	for (std::map<CStr, CObjectBase::Prop>::iterator it = chosenProps.begin(); it != chosenProps.end(); ++it)
		m_Props.push_back(it->second);

	for (std::map<CStr, CObjectBase::Anim>::iterator it = chosenAnims.begin(); it != chosenAnims.end(); ++it)
		m_Animations.push_back(it->second);



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

	// load animations
	for (size_t t = 0; t < m_Animations.size(); t++)
	{
		if (m_Animations[t].m_FileName.Length() > 0)
		{
			const char* animfilename = m_Animations[t].m_FileName;
			m_Animations[t].m_AnimData = m_Model->BuildAnimation(animfilename, m_Animations[t].m_Speed, m_Animations[t].m_ActionPos, m_Animations[t].m_ActionPos2);

			CStr AnimNameLC = m_Animations[t].m_AnimName.LowerCase();

			if (AnimNameLC == "idle")
				m_IdleAnim = m_Animations[t].m_AnimData;
			else
			if (AnimNameLC == "walk")
				m_WalkAnim = m_Animations[t].m_AnimData;
			else
			if (AnimNameLC == "attack")
				m_MeleeAnim = m_Animations[t].m_AnimData;
			else
			if (AnimNameLC == "chop")
				m_GatherAnim = m_Animations[t].m_AnimData;
			else
			if (AnimNameLC == "death")
				m_DeathAnim = m_Animations[t].m_AnimData;
			else
			if (AnimNameLC == "decay")
				m_CorpseAnim = m_Animations[t].m_AnimData;
			//else
			//	debug_out("Invalid animation name '%s'\n", (const char*)AnimNameLC);
		}
		else
		{
			// FIXME, RC - don't store invalid animations (possible?)
			m_Animations[t].m_AnimData = NULL;
		}
	}
	// start up idling
	if (! m_Model->SetAnimation(m_IdleAnim))
		LOG(ERROR, LOG_CATEGORY, "Failed to set idle animation in model \"%s\"", modelfilename);

	// build props - TODO, RC - need to fix up bounds here
	// TODO: Make sure random variations get handled correctly when a prop fails
	for (size_t p = 0; p < m_Props.size(); p++)
	{
		const CObjectBase::Prop& prop = m_Props[p];
	
		CObjectEntry* oe = g_ObjMan.FindObjectVariation(prop.m_ModelName, vars, vars_it);
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
				if (oe->m_IdleAnim)
					propmodel->SetAnimation(oe->m_IdleAnim);
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


CSkeletonAnim* CObjectEntry::GetNamedAnimation(CStr animationName)
{
	for (size_t t = 0; t < m_Animations.size(); t++)
		if (m_Animations[t].m_AnimName == animationName)
			return m_Animations[t].m_AnimData;

	return NULL;
}


CObjectBase::Prop* CObjectEntry::FindProp(const char* proppointname)
{
	for (size_t i = 0; i < m_Props.size(); i++)
		if (strcmp(proppointname, m_Props[i].m_PropPointName) == 0)
			return &m_Props[i];

	return NULL;
}
