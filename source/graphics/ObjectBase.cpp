#include "precompiled.h"

#include "ObjectBase.h"

#include "ObjectManager.h"
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
	m_Variants.clear();

	CStr filePath ("art/actors/");
	filePath += filename;

	CXeromyces XeroFile;
	if (XeroFile.Load(filePath) != PSRETURN_OK)
		return false;

	m_Name = filename;

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
				m_ShortName = element_value;

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
		m_ShortName = CStr(filename).AfterLast("/").BeforeLast(".xml");

		// Define all the elements used in the XML file
		#define EL(x) int el_##x = XeroFile.getElementID(#x)
		#define AT(x) int at_##x = XeroFile.getAttributeID(#x)
		EL(castshadow);
		EL(material);
		EL(group);
		EL(variant);
		EL(animations);
		EL(animation);
		EL(props);
		EL(prop);
		EL(mesh);
		EL(texture);
		EL(colour);
		AT(file);
		AT(name);
		AT(speed);
		AT(event);
		AT(load);
		AT(attachpoint);
		AT(actor);
		AT(frequency);
		#undef AT
		#undef EL

		// Set up the vector<vector<T>> m_Variants to contain the right number
		// of elements, to avoid wasteful copying/reallocation later.
		{
			// Count the variants in each group
			std::vector<int> variantSizes;
			XERO_ITER_EL(root, child)
			{
				if (child.getNodeName() == el_group)
				{
					variantSizes.push_back(0);
					XERO_ITER_EL(child, variant)
						++variantSizes.back();
				}
			}

			m_Variants.resize(variantSizes.size());
			// Set each vector to match the number of variants
			for (size_t i = 0; i < variantSizes.size(); ++i)
				m_Variants[i].resize(variantSizes[i]);
		}


		// (This XML-reading code is rather worryingly verbose...)

		std::vector<std::vector<Variant> >::iterator currentGroup = m_Variants.begin();

		XERO_ITER_EL(root, child)
		{
			int child_name = child.getNodeName();

			if (child_name == el_group)
			{
				std::vector<Variant>::iterator currentVariant = currentGroup->begin();
				XERO_ITER_EL(child, variant)
				{
					XERO_ITER_ATTR(variant, attr)
					{
						if (attr.Name == at_name)
							currentVariant->m_VariantName = attr.Value;

						else if (attr.Name == at_frequency)
							currentVariant->m_Frequency = CStr(attr.Value).ToInt();
					}


					XERO_ITER_EL(variant, option)
					{
						int option_name = option.getNodeName();

						if (option_name == el_mesh)
							currentVariant->m_ModelFilename = "art/meshes/" + CStr(option.getText());

						else if (option_name == el_texture)
							currentVariant->m_TextureFilename = "art/textures/skins/" + CStr(option.getText());

						else if (option_name == el_colour)
							currentVariant->m_Color = option.getText();

						else if (option_name == el_animations)
						{
							XERO_ITER_EL(option, anim_element)
							{
								Anim anim;

								XERO_ITER_ATTR(anim_element, ae)
								{
									if (ae.Name == at_name)
										anim.m_AnimName = ae.Value;
									else if (ae.Name == at_file)
										anim.m_FileName = "art/animation/" + CStr(ae.Value);
									else if (ae.Name == at_speed)
									{
										anim.m_Speed = CStr(ae.Value).ToInt() / 100.f;
										if (anim.m_Speed <= 0.0) anim.m_Speed = 1.0f;
									}
									else if (ae.Name == at_event)
									{
										anim.m_ActionPos = CStr(ae.Value).ToDouble();
										if (anim.m_ActionPos < 0.0) anim.m_ActionPos = 0.0;
										else if (anim.m_ActionPos > 100.0) anim.m_ActionPos = 1.0;
										else if (anim.m_ActionPos > 1.0) anim.m_ActionPos /= 100.0;
									}
									else if (ae.Name == at_load)
									{
										anim.m_ActionPos2 = CStr(ae.Value).ToDouble();
										if (anim.m_ActionPos2 < 0.0) anim.m_ActionPos2 = 0.0;
										else if (anim.m_ActionPos2 > 100.0) anim.m_ActionPos2 = 1.0;
										else if (anim.m_ActionPos2 > 1.0) anim.m_ActionPos2 /= 100.0;
									}
									else
										; // unrecognised element
								}
								currentVariant->m_Anims.push_back(anim);
							}

						}
						else if (option_name == el_props)
						{
							XERO_ITER_EL(option, prop_element)
							{
								Prop prop;

								XERO_ITER_ATTR(prop_element, pe)
								{
									if (pe.Name == at_attachpoint)
										prop.m_PropPointName = pe.Value;
									else if (pe.Name == at_actor)
										prop.m_ModelName = pe.Value;
									else
										; // unrecognised element
								}
								currentVariant->m_Props.push_back(prop);
							}
						}
						else
							; // unrecognised element
					}

					++currentVariant;
				}

				if (currentGroup->size() == 0)
				{
					LOG(ERROR, LOG_CATEGORY, "Actor group has zero variants ('%s')", filename);
				}

				++currentGroup;
			}
			else if (child_name == el_material)
			{
				m_Material = "art/materials/" + CStr(child.getText());
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
		// Ignore groups with nothing inside. (A warning will have been
		// emitted by the loading code.)
		if (grp->size() == 0)
			continue;

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

		assert(randNum < 0);
			// This should always happen; otherwise it
			// wouldn't have chosen any of the variants.
	}

	assert(choices.size() == m_Variants.size());


	// Also, make choices for all props:

	// Work out which props have been chosen
	std::map<CStr, CStr> chosenProps;
	CObjectBase::variation_key::const_iterator choice_it = choices.begin();
	for (std::vector<std::vector<CObjectBase::Variant> >::iterator grp = m_Variants.begin();
		grp != m_Variants.end();
		++grp)
	{
		CObjectBase::Variant& var (grp->at(*(choice_it++)));
		for (std::vector<CObjectBase::Prop>::iterator it = var.m_Props.begin(); it != var.m_Props.end(); ++it)
		{
			chosenProps[it->m_PropPointName] = it->m_ModelName;
		}
	}

	// Load each prop, and call CalculateVariation on them:
	for (std::map<CStr, CStr>::iterator it = chosenProps.begin(); it != chosenProps.end(); ++it)
	{
		CObjectBase* prop = g_ObjMan.FindObjectBase(it->second);
		if (prop)
		{
			variation_key propChoices;
			prop->CalculateVariation(strings, propChoices);
			choices.insert(choices.end(), propChoices.begin(), propChoices.end());
		}
	}

	// (TODO: This seems rather fragile, e.g. if props fail to load)
}
