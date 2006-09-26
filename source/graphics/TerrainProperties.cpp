#include "precompiled.h"

#include <string>

#include "TerrainProperties.h"
#include "TextureManager.h"
#include "ps/Overlay.h"

#include "ps/Parser.h"
#include "ps/XML/XeroXMB.h"
#include "ps/XML/Xeromyces.h"

#include "ps/CLogger.h"
#define LOG_CATEGORY "graphics"

CTerrainProperties::CTerrainProperties(CTerrainPropertiesPtr parent):
	m_pParent(parent),
	m_BaseColor(0),
	m_HasBaseColor(false)
{
	if (m_pParent)
		m_Groups = m_pParent->m_Groups;	
}

CTerrainPropertiesPtr CTerrainProperties::FromXML(CTerrainPropertiesPtr parent, const char* path)
{
	CXeromyces XeroFile;
	if (XeroFile.Load(path) != PSRETURN_OK)
		return CTerrainPropertiesPtr();

	XMBElement root = XeroFile.getRoot();
	CStr rootName = XeroFile.getElementString(root.getNodeName());

	// Check that we've got the right kind of xml document
	if (rootName != "Terrains")
	{
		LOG(ERROR,
			LOG_CATEGORY,
			"TextureManager: Loading %s: Root node is not terrains (found \"%s\")",
			path,
			rootName.c_str());
		return CTerrainPropertiesPtr();
	}
	
	#define ELMT(x) int el_##x = XeroFile.getElementID(#x)
	#define ATTR(x) int at_##x = XeroFile.getAttributeID(#x)
	ELMT(terrain);
	#undef ELMT
	#undef ATTR
	
	// Ignore all non-terrain nodes, loading the first terrain node and
	// returning it.
	// Really, we only expect there to be one child and it to be of the right
	// type, though.
	XMBElementList children = root.getChildNodes();
	for (int i=0; i<children.Count; ++i)
	{
		//debug_printf("Object %d\n", i);
		XMBElement child = children.item(i);

		if (child.getNodeName() == el_terrain)
		{
			CTerrainPropertiesPtr ret (new CTerrainProperties(parent));
			ret->LoadXML(child, &XeroFile);
			return ret;
		}
		else
		{
			LOG(WARNING, LOG_CATEGORY, 
				"TerrainProperties: Loading %s: Unexpected node %s\n",
				path,
				XeroFile.getElementString(child.getNodeName()).c_str());
			// Keep reading - typos shouldn't be showstoppers
		}
	}
	
	return CTerrainPropertiesPtr();
}

void CTerrainProperties::LoadXML(XMBElement node, CXeromyces *pFile)
{
	#define ELMT(x) int elmt_##x = pFile->getElementID(#x)
	#define ATTR(x) int attr_##x = pFile->getAttributeID(#x)
	ELMT(doodad);
	ELMT(passable);
	ELMT(event);
	// Terrain Attribs
	ATTR(mmap);
	ATTR(groups);
	ATTR(properties);
	// Passable Attribs
	ATTR(type);
	ATTR(speed);
	ATTR(effect);
	// Doodad Attribs
	ATTR(name);
	ATTR(max);
	// Event attribs
	ATTR(on);
	#undef ELMT
	#undef ATTR

	// stomp on "unused" warnings
	UNUSED2(attr_effect);
	UNUSED2(attr_name);
	UNUSED2(attr_type);
	UNUSED2(attr_on);
	UNUSED2(attr_speed);
	UNUSED2(attr_max);
	UNUSED2(elmt_event);
	UNUSED2(elmt_passable);
	UNUSED2(elmt_doodad);

	XMBAttributeList attribs = node.getAttributes();
	for (int i=0;i<attribs.Count;i++)
	{
		XMBAttribute attr = attribs.item(i);

		if (attr.Name == attr_groups)
		{
			// Parse a comma-separated list of groups, add the new entry to
			// each of them
			CParser parser;
			CParserLine parserLine;
			parser.InputTaskType("GroupList", "<_$value_,>_$value_");
			
			if (!parserLine.ParseString(parser, (CStr)attr.Value))
				continue;
			m_Groups.clear();
			for (size_t i=0;i<parserLine.GetArgCount();i++)
			{
				std::string value;
				if (!parserLine.GetArgString(i, value))
					continue;
				CTerrainGroup *pType = g_TexMan.FindGroup(value);
				m_Groups.push_back(pType);
			}
		}
		else if (attr.Name == attr_mmap)
		{
			CColor col;
			if (!col.ParseString((CStr)attr.Value, 255))
				continue;
			
			// m_BaseColor is BGRA
			u8 *baseColor = (u8*)&m_BaseColor;
			baseColor[0] = (u8)(col.b*255);
			baseColor[1] = (u8)(col.g*255);
			baseColor[2] = (u8)(col.r*255);
			baseColor[3] = (u8)(col.a*255);
			m_HasBaseColor = true;
		}
		else if (attr.Name == attr_properties)
		{
			// TODO Parse a list of properties and store them somewhere
		}
	}
	
	// TODO Parse information in child nodes (doodads, passable, events) and
	// store them somewhere
}

bool CTerrainProperties::HasBaseColor()
{
	return m_HasBaseColor || (m_pParent && m_pParent->HasBaseColor());
}

u32 CTerrainProperties::GetBaseColor()
{
	if (m_HasBaseColor || !m_pParent)
		return m_BaseColor;
	else if (m_pParent)
		return m_pParent->GetBaseColor();
	else
		// White, full opacity.. but this value shouldn't ever be used
		return 0xffffffff;
}
