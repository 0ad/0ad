#include "precompiled.h"

#include "ObjectBase.h"

#include "Xeromyces.h"
#include "CLogger.h"

#define LOG_CATEGORY "graphics"

CObjectBase::CObjectBase()
{
	m_Properties.m_CastShadows = true;
	m_Properties.m_AutoFlatten = false;
}

bool CObjectBase::Load(const char* filename)
{
//static int x=0; debug_out("load %s %d\n", filename, ++x);

	m_Variants.clear();

	CXeromyces XeroFile;
	if (XeroFile.Load(filename) != PSRETURN_OK)
		return false;

	m_FileName = filename;

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

		m_Variants.resize(1);
		m_Variants.back().resize(1);
		m_Variants.back().back().m_VariantName = "Base";

		XERO_ITER_EL(root, child)
		{
			int element_name = child.getNodeName();
			CStr element_value (child.getText());

			if (element_name == el_name)
				m_Name = element_value;

			else if (element_name == el_modelname)
				m_Variants.back().back().m_ModelFilename = element_value;

			else if (element_name == el_texturename)
				m_Variants.back().back().m_TextureFilename = element_value;

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
						m_Properties.m_AutoFlatten = str.ToInt() ? true : false;
					}
					else if (attrib_name == at_castshadows)
					{
						CStr str = (attrib.Value);
						m_Properties.m_CastShadows = str.ToInt() ? true : false;
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

						m_Variants.back().back().m_Anims.push_back(anim);
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

						m_Variants.back().back().m_Props.push_back(prop);
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

		// (This code is rather worryingly verbose...)

		XERO_ITER_EL(root, child)
		{
			int element_name = child.getNodeName();
			CStr element_value (child.getText());

			if (element_name == el_group)
			{
				m_Variants.resize(m_Variants.size()+1);

				XERO_ITER_EL(child, variant)
				{
					m_Variants.back().resize(m_Variants.back().size()+1);

					XERO_ITER_EL(variant, option)
					{
						int option_name = option.getNodeName();

						if (option_name == el_name)
							m_Variants.back().back().m_VariantName = option.getText();

						else if (option_name == el_frequency)
							m_Variants.back().back().m_Frequency = CStr(option.getText()).ToInt();

						else if (option_name == el_mesh)
							m_Variants.back().back().m_ModelFilename = option.getText();

						else if (option_name == el_texture)
							m_Variants.back().back().m_TextureFilename = option.getText();

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
								m_Variants.back().back().m_Anims.push_back(anim);
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
								m_Variants.back().back().m_Props.push_back(prop);
							}
						}
						else
							; // unrecognised element
					}
				}

				if (m_Variants.back().size() == 0)
				{
					LOG(ERROR, LOG_CATEGORY, "Actor group has zero variants ('%s')", filename);
					m_Variants.pop_back();
				}
			}
			else if (element_name == el_material)
			{
				m_Material = child.getText();
			}
			else
				; // unrecognised element

			// TODO: castshadow, etc
		}

	}
	else
	{
		LOG(ERROR, LOG_CATEGORY, "Invalid actor format (unrecognised root element '%s')", XeroFile.getElementString(root.getNodeName()));
		return false;
	}

	return true;
}

/* // TODO: Allow saving? (Maybe not necessary)

bool CObjectBase::Save(const char* filename)
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
*/

void CObjectBase::CalculateVariation(std::set<CStr>& strings, variation_key& choices)
{
	// Calculate a complete list of choices, one per group. In each group,
	// if one of the variants has a name matching a string in 'strings', use
	// that one. If more than one matches, choose randomly from those matching
	// ones. If none match, choose randomly from all variants.
	//
	// When choosing randomly, make use of each variant's frequency. If all
	// variants have frequency 0, treat them as if they were 1.

	choices.clear();

	for (std::vector<std::vector<CObjectBase::Variant> >::iterator grp = m_Variants.begin();
		grp != m_Variants.end();
		++grp)
	{
		assert(grp->size() > 0);

		// If there's only a single variant, choose that one
		if (grp->size() == 1)
		{
			choices.push_back(0);
			continue;
		}
			
		// Determine the variants that match the provided strings:

		std::vector<u8> matches;
		typedef std::vector<u8>::const_iterator Iter;

		assert(grp->size() < 256); // else they won't fit in the vector

		for (uint i = 0; i < grp->size(); ++i)
			if (strings.count((*grp)[i].m_VariantName))
				matches.push_back(i);

		// If there's only one match, choose that one
		if (matches.size() == 1)
		{
			choices.push_back(matches[0]);
			continue;
		}
		
		// Otherwise, choose randomly from the others.

		// If none matched the specified strings, choose from all the variants
		if (matches.size() == 0)
			for (uint i = 0; i < grp->size(); ++i)
				matches.push_back(i);

		// Sum the frequencies:
		int totalFreq = 0;
		for (Iter it = matches.begin(); it != matches.end(); ++it)
			totalFreq += (*grp)[*it].m_Frequency;

		// Someone might be silly and set all variants to have freq==0, in
		// which case we just pretend they're all 1
		bool allZero = false;
		if (totalFreq == 0)
		{
			totalFreq = (int)matches.size();
			allZero = true;
		}

		// Choose a random number in the interval [0..totalFreq).
		// (It shouldn't be necessary to use a network-synchronised RNG,
		// since actors are meant to have purely visual manifestations.)
		int randNum = (int)( ((float)rand() / RAND_MAX) * totalFreq );

		assert(randNum < totalFreq);

		// and use that to choose one of the variants
		for (Iter it = matches.begin(); it != matches.end(); ++it)
		{
			randNum -= (allZero ? 1 : (*grp)[*it].m_Frequency);
			if (randNum < 0)
			{
				choices.push_back(*it);
				break;
			}
		}

		assert(randNum < 0); // which should always happen; otherwise it
		                     // wouldn't have chosen any of the variants.
	}

	assert(choices.size() == m_Variants.size());
}


// TODO: Remove this, once all the actors are renamed properly
bool CObjectBase::LoadName(const CStr& filename, CStr& out)
{
	CXeromyces XeroFile;
	if (XeroFile.Load(filename) != PSRETURN_OK)
		return false;

	XMBElement root = XeroFile.getRoot();

	if (root.getNodeName() == XeroFile.getElementID("object"))
	{
		//// Old-format actor file ////

		#define EL(x) int el_##x = XeroFile.getElementID(#x)
		EL(name);
		#undef EL

		XERO_ITER_EL(root, child)
		{
			if (child.getNodeName() == el_name)
			{
				out = child.getText();
				return true;
			}
		}
		LOG(ERROR, LOG_CATEGORY, "Invalid actor format (couldn't find 'name')");
		return false;
	}
	else if (root.getNodeName() == XeroFile.getElementID("actor"))
	{
		//// New-format actor file ////

		// Use the filename for the model's name
		out = CStr(filename).AfterLast("/").BeforeLast(".xml");
		return true;
	}
	else
	{
		LOG(ERROR, LOG_CATEGORY, "Invalid actor format (unrecognised root element '%s')", XeroFile.getElementString(root.getNodeName()));
		return false;
	}
}
