/* Copyright (C) 2012 Wildfire Games.
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
#include "TerrainProperties.h"

#include <vector>
#include <string>

#include "ps/Filesystem.h"
#include "ps/CLogger.h"

#include "ps/Parser.h"
#include "ps/XML/XeroXMB.h"
#include "ps/XML/Xeromyces.h"

#include "TerrainTextureManager.h"
#include "ps/Overlay.h"

#include "maths/MathUtil.h"

CTerrainProperties::CTerrainProperties(CTerrainPropertiesPtr parent):
	m_pParent(parent),
	m_BaseColor(0),
	m_HasBaseColor(false),
	m_TextureAngle((float)M_PI / 4.f),
	m_TextureSize(32.f),
	m_MovementClass("default")
{
	if (m_pParent)
		m_Groups = m_pParent->m_Groups;	
}

CTerrainPropertiesPtr CTerrainProperties::FromXML(const CTerrainPropertiesPtr& parent, const VfsPath& pathname)
{
	CXeromyces XeroFile;
	if (XeroFile.Load(g_VFS, pathname) != PSRETURN_OK)
		return CTerrainPropertiesPtr();

	XMBElement root = XeroFile.GetRoot();
	CStr rootName = XeroFile.GetElementString(root.GetNodeName());

	// Check that we've got the right kind of xml document
	if (rootName != "Terrains")
	{
		LOGERROR(
			L"TerrainProperties: Loading %ls: Root node is not terrains (found \"%hs\")",
			pathname.string().c_str(),
			rootName.c_str());
		return CTerrainPropertiesPtr();
	}
	
	#define ELMT(x) int el_##x = XeroFile.GetElementID(#x)
	ELMT(terrain);
	#undef ELMT
	
	// Ignore all non-terrain nodes, loading the first terrain node and
	// returning it.
	// Really, we only expect there to be one child and it to be of the right
	// type, though.
	XERO_ITER_EL(root, child)
	{
		if (child.GetNodeName() == el_terrain)
		{
			CTerrainPropertiesPtr ret (new CTerrainProperties(parent));
			ret->LoadXml(child, &XeroFile, pathname);
			return ret;
		}
		else
		{
			LOGWARNING(
				L"TerrainProperties: Loading %ls: Unexpected node %hs\n",
				pathname.string().c_str(),
				XeroFile.GetElementString(child.GetNodeName()).c_str());
			// Keep reading - typos shouldn't be showstoppers
		}
	}
	
	return CTerrainPropertiesPtr();
}

void CTerrainProperties::LoadXml(XMBElement node, CXeromyces *pFile, const VfsPath& UNUSED(pathname))
{
	#define ELMT(x) int elmt_##x = pFile->GetElementID(#x)
	#define ATTR(x) int attr_##x = pFile->GetAttributeID(#x)
	// Terrain Attribs
	ATTR(mmap);
	ATTR(groups);
	ATTR(movementclass);
	ATTR(angle);
	ATTR(size);
	#undef ELMT
	#undef ATTR

	XERO_ITER_ATTR(node, attr)
	{
		if (attr.Name == attr_groups)
		{
			// Parse a comma-separated list of groups, add the new entry to
			// each of them
			CParser parser;
			CParserLine parserLine;
			parser.InputTaskType("GroupList", "<_$value_,>_$value_");
			
			if (!parserLine.ParseString(parser, attr.Value))
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
			if (!col.ParseString(attr.Value, 255))
				continue;
			
			// m_BaseColor is BGRA
			u8 *baseColor = (u8*)&m_BaseColor;
			baseColor[0] = (u8)(col.b*255);
			baseColor[1] = (u8)(col.g*255);
			baseColor[2] = (u8)(col.r*255);
			baseColor[3] = (u8)(col.a*255);
			m_HasBaseColor = true;
		}
		else if (attr.Name == attr_angle)
		{
			m_TextureAngle = DEGTORAD(attr.Value.ToFloat());
		}
		else if (attr.Name == attr_size)
		{
			m_TextureSize = attr.Value.ToFloat();
		}
		else if (attr.Name == attr_movementclass)
		{
			m_MovementClass = attr.Value;
		}
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
