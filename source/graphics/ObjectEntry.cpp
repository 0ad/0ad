#include "precompiled.h"

#include "ObjectEntry.h"
#include "ObjectManager.h"
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

CObjectEntry::CObjectEntry(int type) : m_Model(0), m_Type(type)
{
	m_IdleAnim=0;
	m_WalkAnim=0;
	m_DeathAnim=0;
	m_CorpseAnim=0;
	m_MeleeAnim=0;
	m_RangedAnim=0;
	m_Properties.m_CastShadows=true;
	m_Properties.m_AutoFlatten=false;
}

CObjectEntry::~CObjectEntry()
{
	for (size_t i=0;i<m_Animations.size();i++) {
		delete m_Animations[i].m_AnimData;
	}

	delete m_Model;
}

bool CObjectEntry::BuildModel()
{
	// check we've enough data to consider building the object
	if (m_ModelName.Length()==0 || m_TextureName.Length()==0) {
		return false;
	}

	// get the root directory of this object
	CStr dirname=g_ObjMan.m_ObjectTypes[m_Type].m_Name;

	// remember the old model so we can replace any models using it later on
	CModelDefPtr oldmodeldef=m_Model ? m_Model->GetModelDef() : CModelDefPtr();

	const char* modelfilename = m_ModelName.c_str();

	// try and create a model
	CModelDefPtr modeldef (g_MeshManager.GetMesh(modelfilename));
	if (!modeldef)
	{
		LOG(ERROR, LOG_CATEGORY, "CObjectEntry::BuildModel(): Model %s failed to load", modelfilename);
		return false;
	}

	// delete old model, create new 
	delete m_Model;
	m_Model=new CModel;
	m_Model->SetTexture((const char*) m_TextureName);
    m_Model->SetMaterial(g_MaterialManager.LoadMaterial((const char *)m_Material));
	m_Model->InitModel(modeldef);

	// calculate initial object space bounds, based on vertex positions
	m_Model->CalcObjectBounds();

	// load animations
	for( uint t = 0; t < m_Animations.size(); t++ )
	{
		if( m_Animations[t].m_FileName.Length() > 0 )
		{
			const char* animfilename = m_Animations[t].m_FileName.c_str();
			m_Animations[t].m_AnimData = m_Model->BuildAnimation(animfilename,m_Animations[t].m_Speed);

			CStr AnimNameLC = m_Animations[t].m_AnimName.LowerCase();

			if( AnimNameLC == "idle" )
				m_IdleAnim = m_Animations[t].m_AnimData;
			else
			if( AnimNameLC == "walk" )
				m_WalkAnim = m_Animations[t].m_AnimData;
			else
			if( AnimNameLC == "attack" )
				m_MeleeAnim = m_Animations[t].m_AnimData;
			else
			if( AnimNameLC == "death" )
				m_DeathAnim = m_Animations[t].m_AnimData;
			else
			if( AnimNameLC == "decay" )
				m_CorpseAnim = m_Animations[t].m_AnimData;
			//else
			//	debug_out("Invalid animation name '%s'\n", (const char*)AnimNameLC);
		}
		else
		{
			// FIXME, RC - don't store invalid animations (possible?)
			m_Animations[t].m_AnimData=0;
		}
	}
	// start up idling
	m_Model->SetAnimation( m_IdleAnim );

	// build props - TODO, RC - need to fix up bounds here
	for (uint p=0;p<m_Props.size();p++) {
		const Prop& prop=m_Props[p];
		SPropPoint* proppoint=modeldef->FindPropPoint((const char*) prop.m_PropPointName);
		if (proppoint) {
			CObjectEntry* oe=g_ObjMan.FindObject(prop.m_ModelName);
			if (oe) {
				// try and build model if we haven't already got it
				if (!oe->m_Model) oe->BuildModel();
				if (oe->m_Model) {
					CModel* propmodel=oe->m_Model->Clone();
					m_Model->AddProp(proppoint,propmodel);
					if (oe->m_WalkAnim) propmodel->SetAnimation(oe->m_WalkAnim);
				} else {
					LOG(ERROR, LOG_CATEGORY, "Failed to build prop model \"%s\" on actor \"%s\"",(const char*) m_Name,(const char*) prop.m_ModelName);
				}
			}
		} else {
			LOG(ERROR, LOG_CATEGORY, "Failed to matching prop point called \"%s\" in model \"%s\"", (const char*)prop.m_PropPointName, modelfilename);
		}
	}

	// setup flags
	if (m_Properties.m_CastShadows) {
		m_Model->SetFlags(m_Model->GetFlags()|MODELFLAG_CASTSHADOWS);
	}

	// replace any units using old model to now use new model; also reprop models, if necessary
	// FIXME, RC - ugh, doesn't recurse correctly through props
	const std::vector<CUnit*>& units=g_UnitMan.GetUnits();
	for (uint i=0;i<units.size();++i) {
		CModel* unitmodel=units[i]->GetModel();
		if (unitmodel->GetModelDef()==oldmodeldef) {			
			unitmodel->InitModel(m_Model->GetModelDef());
			unitmodel->SetFlags(m_Model->GetFlags());

			const std::vector<CModel::Prop>& newprops=m_Model->GetProps();
			for (uint j=0;j<newprops.size();j++) {
				unitmodel->AddProp(newprops[j].m_Point,newprops[j].m_Model->Clone());
			}
		}

		std::vector<CModel::Prop>& mdlprops=unitmodel->GetProps();
		for (uint j=0;j<mdlprops.size();j++) {
			CModel::Prop& prop=mdlprops[j];
			if (prop.m_Model) {
				if (prop.m_Model->GetModelDef()==oldmodeldef) {
					delete prop.m_Model;
					prop.m_Model=m_Model->Clone();
				}
			}
		}
	}

	return true;
}

CSkeletonAnim* CObjectEntry::GetNamedAnimation( CStr animationName )
{
	for( uint t = 0; t < m_Animations.size(); t++ )
	{
		if( m_Animations[t].m_AnimName == animationName )
			return( m_Animations[t].m_AnimData );
	}
	return( NULL );
}

bool CObjectEntry::Load(const char* filename)
{

	CXeromyces XeroFile;
	if (XeroFile.Load(filename) != PSRETURN_OK)
		return false;

	// Define all the elements and attributes used in the XML file
	#define EL(x) int el_##x = XeroFile.getElementID(#x)
	#define AT(x) int at_##x = XeroFile.getAttributeID(#x)
	EL(name);
	EL(modelname);
	EL(texturename);
    EL(material);
	EL(animations);
	EL(props);
	EL(properties);
	AT(attachpoint);
	AT(model);
	AT(name);
	AT(file);
	AT(speed);
	AT(autoflatten);
	AT(castshadows);
	#undef AT
	#undef EL

	XMBElement root = XeroFile.getRoot();

	XMBElementList children = root.getChildNodes();

	for (int i = 0; i < children.Count; ++i) {

		XMBElement child = children.item(i);

		int element_name = child.getNodeName();
		CStr element_value (child.getText());

		if (element_name == el_name)
			m_Name=element_value;

		else if (element_name == el_modelname)
			m_ModelName=element_value;

		else if (element_name == el_texturename)
			m_TextureName=element_value;

        else if(element_name == el_material)
            m_Material = element_value;

		else if (element_name == el_properties) {

			XMBAttributeList attributes=child.getAttributes();
			for (int j = 0; j < attributes.Count; ++j) {
				XMBAttribute attrib=attributes.item(j);

				int attrib_name = attrib.Name;

				if (attrib_name == at_autoflatten) {
					CStr str (attrib.Value);
					m_Properties.m_AutoFlatten=str.ToInt() ? true : false;

				} else if (attrib_name == at_castshadows) {
					CStr str = (attrib.Value);
					m_Properties.m_CastShadows=str.ToInt() ? true : false;
				} 
			}
		}

		else if (element_name == el_animations)
		{
			XMBElementList animations=child.getChildNodes();

			for (int j = 0; j < animations.Count; ++j) {
				XMBElement anim_element = animations.item(j);
				XMBAttributeList attributes=anim_element.getAttributes();
				if (attributes.Count) {
					Anim anim;

					anim.m_AnimName = attributes.getNamedItem(at_name);
					anim.m_FileName = attributes.getNamedItem(at_file);
					CStr speedstr = attributes.getNamedItem(at_speed);

					anim.m_Speed=float(speedstr.ToInt())/100.0f;
					if (anim.m_Speed<=0.0) anim.m_Speed=1.0f;

					m_Animations.push_back(anim);
				}
			}
		}
		else if (element_name == el_props)
		{
			XMBElementList props=child.getChildNodes();

			for (int j = 0; j < props.Count; ++j) {
				XMBElement prop_element = props.item(j);
				XMBAttributeList attributes=prop_element.getAttributes();
				if (attributes.Count) {
					Prop prop;

					prop.m_PropPointName = attributes.getNamedItem(at_attachpoint);
					prop.m_ModelName = attributes.getNamedItem(at_model);

					m_Props.push_back(prop);
				}
			}
		}
	}

	return true;

}

bool CObjectEntry::Save(const char* filename)
{
	Handle h = vfs_open(filename, FILE_WRITE|FILE_NO_AIO);
	if (h <= 0)
	{
		debug_warn("actor open failed");
		return false;
	}

	XML_Start("iso-8859-1");
	XML_Doctype("Object", "/art/actors/object.dtd");

	XML_Comment("File automatically generated by ScEd");

	{
		XML_Element("Object");

		XML_Setting("Name", m_Name);
		XML_Setting("ModelName", m_ModelName);
		XML_Setting("TextureName", m_TextureName);

		if(m_Material.Trim(PS_TRIM_BOTH).Length())
			XML_Setting("Material", m_Material);

		{
			XML_Element("Properties");
			XML_Attribute("autoflatten", m_Properties.m_AutoFlatten ? 1 : 0);
			XML_Attribute("castshadows", m_Properties.m_CastShadows ? 1 : 0);
		}

		if (m_Animations.size()>0)
		{
			XML_Element("Animations");

			for (uint i=0;i<m_Animations.size();i++)
			{
				XML_Element("Animation");
				XML_Attribute("name", m_Animations[i].m_AnimName);
				XML_Attribute("file", m_Animations[i].m_FileName);
				XML_Attribute("speed", int(m_Animations[i].m_Speed*100));
			}
		}

		if (m_Props.size()>0)
		{
			XML_Element("Props");

			for (uint i=0;i<m_Props.size();i++)
			{
				XML_Element("Prop");
				XML_Attribute("attachpoint", m_Props[i].m_PropPointName);
				XML_Attribute("model", m_Props[i].m_ModelName);
			}
		}
	}

	if (! XML_StoreVFS(h))
	{
		// Error occurred while saving data
		debug_warn("actor save failed");
		vfs_close(h);
		return false;
	}

	vfs_close(h);
	return true;
}


CObjectEntry::Prop* CObjectEntry::FindProp(const char* proppointname)
{
	for (size_t i=0;i<m_Props.size();i++) {
		if (strcmp(proppointname,m_Props[i].m_PropPointName)==0) return &m_Props[i];
	}

	return 0;
}
