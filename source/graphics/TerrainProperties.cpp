/* Copyright (C) 2009 Wildfire Games.
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

#include <vector>
#include <string>

#include "TerrainProperties.h"
#include "TextureManager.h"
#include "ps/Overlay.h"

#include "ps/Parser.h"
#include "ps/XML/XeroXMB.h"
#include "ps/XML/Xeromyces.h"

#include "ps/CLogger.h"
#define LOG_CATEGORY L"graphics"


CTerrainProperties::CTerrainProperties(CTerrainPropertiesPtr parent):
	m_pParent(parent),
	m_BaseColor(0),
	m_HasBaseColor(false)
{
	if (m_pParent)
		m_Groups = m_pParent->m_Groups;	
}

CTerrainPropertiesPtr CTerrainProperties::FromXML(const CTerrainPropertiesPtr& parent, const VfsPath& pathname)
{
	CXeromyces XeroFile;
	if (XeroFile.Load(pathname) != PSRETURN_OK)
		return CTerrainPropertiesPtr();

	XMBElement root = XeroFile.GetRoot();
	CStr rootName = XeroFile.GetElementString(root.GetNodeName());

	// Check that we've got the right kind of xml document
	if (rootName != "Terrains")
	{
		LOG(CLogger::Error,
			LOG_CATEGORY,
			L"TextureManager: Loading %ls: Root node is not terrains (found \"%hs\")",
			pathname.string().c_str(),
			rootName.c_str());
		return CTerrainPropertiesPtr();
	}
	
	#define ELMT(x) int el_##x = XeroFile.GetElementID(#x)
	#define ATTR(x) int at_##x = XeroFile.GetAttributeID(#x)
	ELMT(terrain);
	#undef ELMT
	#undef ATTR
	
	// Ignore all non-terrain nodes, loading the first terrain node and
	// returning it.
	// Really, we only expect there to be one child and it to be of the right
	// type, though.
	XMBElementList children = root.GetChildNodes();
	for (int i=0; i<children.Count; ++i)
	{
		//debug_printf(L"Object %d\n", i);
		XMBElement child = children.Item(i);

		if (child.GetNodeName() == el_terrain)
		{
			CTerrainPropertiesPtr ret (new CTerrainProperties(parent));
			ret->LoadXml(child, &XeroFile, pathname);
			return ret;
		}
		else
		{
			LOG(CLogger::Warning, LOG_CATEGORY, 
				L"TerrainProperties: Loading %ls: Unexpected node %hs\n",
				pathname.string().c_str(),
				XeroFile.GetElementString(child.GetNodeName()).c_str());
			// Keep reading - typos shouldn't be showstoppers
		}
	}
	
	return CTerrainPropertiesPtr();
}

void CTerrainProperties::LoadXml(XMBElement node, CXeromyces *pFile, const VfsPath& pathname)
{
	#define ELMT(x) int elmt_##x = pFile->GetElementID(#x)
	#define ATTR(x) int attr_##x = pFile->GetAttributeID(#x)
	ELMT(doodad);
	ELMT(passable);
	ELMT(impassable);
	ELMT(event);
	// Terrain Attribs
	ATTR(mmap);
	ATTR(groups);
	ATTR(properties);
	// Doodad Attribs
	ATTR(name);
	ATTR(max);
	// Event attribs
	ATTR(on);
	#undef ELMT
	#undef ATTR

	// stomp on "unused" warnings
	UNUSED2(attr_name);
	UNUSED2(attr_on);
	UNUSED2(attr_max);
	UNUSED2(elmt_event);
	UNUSED2(elmt_passable);
	UNUSED2(elmt_doodad);

	XMBAttributeList attribs = node.GetAttributes();
	for (int i=0;i<attribs.Count;i++)
	{
		XMBAttribute attr = attribs.Item(i);

		if (attr.Name == attr_groups)
		{
			// Parse a comma-separated list of groups, add the new entry to
			// each of them
			CParser parser;
			CParserLine parserLine;
			parser.InputTaskType("GroupList", "<_$value_,>_$value_");
			
			if (!parserLine.ParseString(parser, CStr(attr.Value)))
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
			if (!col.ParseString(CStr(attr.Value), 255))
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
	
	XMBElementList children = node.GetChildNodes();
	for (int i=0; i<children.Count; ++i)
	{
		XMBElement child = children.Item(i);

		if (child.GetNodeName() == elmt_passable)
		{
			ReadPassability(true, child, pFile, pathname);
		}
		else if (child.GetNodeName() == elmt_impassable)
		{
			ReadPassability(false, child, pFile, pathname);
		}
		// TODO Parse information about doodads and events and store it
	}
}

void CTerrainProperties::ReadPassability(bool passable, XMBElement node, CXeromyces *pFile, const VfsPath& UNUSED(pathname))
{
	#define ATTR(x) int attr_##x = pFile->GetAttributeID(#x)		
	// Passable Attribs
	ATTR(type);
	ATTR(speed);
	ATTR(effect);
	ATTR(prints);
	#undef ATTR

	STerrainPassability pass(passable);
	// Set default speed
	pass.m_SpeedFactor = 100;

	bool hasType = false;
	XMBAttributeList attribs = node.GetAttributes();
	for (int i=0;i<attribs.Count;i++)
	{
		XMBAttribute attr = attribs.Item(i);
		
		if (attr.Name == attr_type)
		{
			// FIXME Should handle lists of types as well!
			pass.m_Type = attr.Value;
			hasType = true;
		}
		else if (attr.Name == attr_speed)
		{
			CStr val=attr.Value;
			CStr trimmedVal=val.Trim(PS_TRIM_BOTH);
			pass.m_SpeedFactor = trimmedVal.ToDouble();
			if (trimmedVal[trimmedVal.size()-1] == '%')
			{
				pass.m_SpeedFactor /= 100.0;
			}
			// FIXME speed=0 could/should be made to set the terrain impassable
		}
		else if (attr.Name == attr_effect)
		{
			// TODO Parse and store list of effects
		}
		else if (attr.Name == attr_prints)
		{
			// TODO Parse and store footprint effect
		}
	}
	
	if (!hasType)
	{
		m_DefaultPassability = pass;
	}
	else
	{	
		m_Passabilities.push_back(pass);
	}
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
		return 0xFFFFFFFF;
}

//const STerrainPassability &CTerrainProperties::GetPassability(HEntity entity)
//{
//	std::vector<STerrainPassability>::iterator it=m_Passabilities.begin();
//	for (;it != m_Passabilities.end();++it)
//	{
//		if (entity->m_classes.IsMember(it->m_Type))
//			return *it;
//	}
//	return m_DefaultPassability;
//}
