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




struct Variant
{
	CStr m_VariantName;
	int m_Frequency;
	CStr m_ModelFilename;
	CStr m_TextureFilename;

	std::vector<CObjectEntry::Anim> m_Anims;
	std::vector<CObjectEntry::Prop> m_Props;
};



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
	for (uint t = 0; t < m_Animations.size(); t++)
	{
		if (m_Animations[t].m_FileName.Length() > 0)
		{
			const char* animfilename = m_Animations[t].m_FileName.c_str();
			m_Animations[t].m_AnimData = m_Model->BuildAnimation(animfilename,m_Animations[t].m_Speed);

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
			m_Animations[t].m_AnimData=0;
		}
	}
	// start up idling
	if (! m_Model->SetAnimation(m_IdleAnim))
		LOG(ERROR, LOG_CATEGORY, "Failed to set idle animation in model \"%s\"", modelfilename);

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

	XMBElement root = XeroFile.getRoot();

	if (root.getNodeName() == XeroFile.getElementID("object"))
	{
		//// Old-format actor file ////

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

		XERO_ITER_EL(root, child)
		{
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

			else if (element_name == el_properties)
			{

				XERO_ITER_ATTR(child, attrib)
				{
					int attrib_name = attrib.Name;

					if (attrib_name == at_autoflatten)
					{
						CStr str (attrib.Value);
						m_Properties.m_AutoFlatten=str.ToInt() ? true : false;
					}
					else if (attrib_name == at_castshadows)
					{
						CStr str = (attrib.Value);
						m_Properties.m_CastShadows=str.ToInt() ? true : false;
					} 
				}
			}

			else if (element_name == el_animations)
			{
				XERO_ITER_EL(child, anim_element)
				{
					XMBAttributeList attributes = anim_element.getAttributes();

					if (attributes.Count)
					{
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
				XERO_ITER_EL(child, prop_element)
				{

					XMBAttributeList attributes = prop_element.getAttributes();
					if (attributes.Count)
					{
						Prop prop;

						prop.m_PropPointName = attributes.getNamedItem(at_attachpoint);
						prop.m_ModelName = attributes.getNamedItem(at_model);

						m_Props.push_back(prop);
					}
				}
			}
		}

	}
	else if (root.getNodeName() == XeroFile.getElementID("actor"))
	{
		//// New-format actor file ////

		// Use the filename for the model's name
		m_Name = CStr(filename).AfterLast("/").BeforeLast(".xml");

		// Define all the elements used in the XML file
		#define EL(x) int el_##x = XeroFile.getElementID(#x)
		EL(castshadow);
		EL(material);
		EL(group);
		EL(variant);
		EL(animations);
		EL(animation);
		EL(file);
		EL(name);
		EL(speed);
		EL(props);
		EL(prop);
		EL(attachpoint);
		EL(model);
		EL(frequency);
		EL(mesh);
		EL(texture);
		#undef EL

		std::vector< std::vector<Variant> > actorVariants;

		XERO_ITER_EL(root, child)
		{
			int element_name = child.getNodeName();
			CStr element_value (child.getText());

			if (element_name == el_group)
			{
				actorVariants.resize(actorVariants.size()+1);

				XERO_ITER_EL(child, variant)
				{
					actorVariants.back().resize(actorVariants.back().size()+1);

					XERO_ITER_EL(variant, option)
					{
						int option_name = option.getNodeName();

						if (option_name == el_name)
							actorVariants.back().back().m_VariantName = option.getText();

						else if (option_name == el_frequency)
							actorVariants.back().back().m_Frequency = CStr(option.getText()).ToInt();

						else if (option_name == el_mesh)
							actorVariants.back().back().m_ModelFilename = option.getText();

						else if (option_name == el_texture)
							actorVariants.back().back().m_TextureFilename = option.getText();

						else if (option_name == el_animations)
						{
							XERO_ITER_EL(option, anim_element)
							{
								Anim anim;

								XERO_ITER_EL(anim_element, ae)
								{
									int ae_name = ae.getNodeName();
									if (ae_name == el_name)
										anim.m_AnimName = ae.getText();
									else if (ae_name == el_file)
										anim.m_FileName = ae.getText();
									else if (ae_name == el_speed)
									{
										anim.m_Speed = CStr(ae.getText()).ToInt() / 100.f;
										if (anim.m_Speed <= 0.0) anim.m_Speed = 1.0f;
									}
									else
										; // unrecognised element
								}
								actorVariants.back().back().m_Anims.push_back(anim);
							}

						}
						else if (option_name == el_props)
						{
							XERO_ITER_EL(option, prop_element)
							{
								Prop prop;

								XERO_ITER_EL(prop_element, pe)
								{
									int pe_name = pe.getNodeName();
									if (pe_name == el_attachpoint)
										prop.m_PropPointName = pe.getText();
									else if (pe_name == el_model)
										prop.m_ModelName = pe.getText();
									else
										; // unrecognised element
								}
								actorVariants.back().back().m_Props.push_back(prop);
							}
						}
						else
							; // unrecognised element
					}
				}
			}
			else if (element_name == el_material)
			{
				m_Material = child.getText();
			}
			else
				; // unrecognised element
		}

		// Fill in this actor with the appropriate variant choices:

		CStr chosenTexture;
		CStr chosenModel;
		std::map<CStr, Prop> chosenProps;
		std::map<CStr, Anim> chosenAnims;

		for (std::vector<std::vector<Variant> >::iterator grp = actorVariants.begin();
			grp != actorVariants.end();
			++grp)
		{
			// TODO: choose correctly
			Variant& var ((*grp)[0]);

			if (var.m_TextureFilename.Length())
				chosenTexture = var.m_TextureFilename;

			if (var.m_ModelFilename.Length())
				chosenModel = var.m_ModelFilename;

			for (std::vector<Prop>::iterator it = var.m_Props.begin(); it != var.m_Props.end(); ++it)
				chosenProps[it->m_PropPointName] = *it;

			for (std::vector<Anim>::iterator it = var.m_Anims.begin(); it != var.m_Anims.end(); ++it)
				chosenAnims[it->m_AnimName] = *it;
		}

		m_TextureName = chosenTexture;
		m_ModelName = chosenModel;

		for (std::map<CStr, Prop>::iterator it = chosenProps.begin(); it != chosenProps.end(); ++it)
			m_Props.push_back(it->second);

		for (std::map<CStr, Anim>::iterator it = chosenAnims.begin(); it != chosenAnims.end(); ++it)
			m_Animations.push_back(it->second);
	}
	else
	{
		LOG(ERROR, LOG_CATEGORY, "Invalid actor format (unrecognised root element '%s')", XeroFile.getElementString(root.getNodeName()));
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
