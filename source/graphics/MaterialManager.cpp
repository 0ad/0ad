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
#include "lib/ogl.h"
#include "ps/Filesystem.h"
#include "ps/XML/Xeromyces.h"
#include "MaterialManager.h"

static bool ParseUsage(CStr temp)
{
    temp = temp.LowerCase().Trim(PS_TRIM_BOTH);
    if(temp == "blend" ||
        temp == "true" ||
        temp == "yes" ||
        temp.ToInt() > 0)
        return true;

    return false;
}

CMaterialManager::CMaterialManager()
{
}

CMaterialManager::~CMaterialManager()
{
	std::map<VfsPath, CMaterial*>::iterator iter;
	for(iter = m_Materials.begin(); iter != m_Materials.end(); iter++)
		delete (*iter).second;

	m_Materials.clear();
}

CMaterial& CMaterialManager::LoadMaterial(const VfsPath& pathname)
{
	if(pathname.empty())
		return NullMaterial;

	std::map<VfsPath, CMaterial*>::iterator iter = m_Materials.find(pathname);
	if(iter != m_Materials.end())
	{
		if((*iter).second)
			return *(*iter).second;
	}

	CXeromyces xeroFile;
	if(xeroFile.Load(g_VFS, pathname) != PSRETURN_OK)
		return NullMaterial;

	#define EL(x) int el_##x = xeroFile.GetElementID(#x)
	#define AT(x) int at_##x = xeroFile.GetAttributeID(#x)
	EL(texture);
	EL(alpha);
	AT(usage);
	#undef AT
	#undef EL

	CMaterial *material = NULL;
	try
	{
		XMBElement root = xeroFile.GetRoot();
		XMBElementList childNodes = root.GetChildNodes();
		material = new CMaterial();

		for(int i = 0; i < childNodes.Count; i++)
		{
			XMBElement node = childNodes.Item(i);
			int token = node.GetNodeName();
			XMBAttributeList attrs = node.GetAttributes();
			CStr temp;
			if(token == el_texture)
			{
				CStr value(node.GetText());
				material->SetTexture(value);
			}
			else if(token == el_alpha)
			{
				temp = CStr(attrs.GetNamedItem(at_usage));

				// Determine whether the alpha is used for basic transparency or player color
				if (temp == "playercolor")
					material->SetUsePlayerColor(true);
				else if (temp == "objectcolor")
					material->SetUseTextureColor(true);
				else
					material->SetUsesAlpha(ParseUsage(temp));
			}
		}

		m_Materials[pathname] = material;
	}
	catch(...)
	{
		SAFE_DELETE(material);
		throw;
	}

	return *material;
}
