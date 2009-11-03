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
#include "ps/XML/Xeromyces.h"
#include "MaterialManager.h"

static float ClampFloat(float value, float min, float max)
{
    if(value < min)
        return min;
    else if(value > max)
        return max;

    return value;
}

static SMaterialColor ParseColor(const CStr& colorStr_)
{
    SMaterialColor color;
    
    CStr colorStr(colorStr_.Trim(PS_TRIM_BOTH));
    CStr tmp;
    int idx = 0;
    long pos = colorStr.Find(' ');
    while(colorStr.length())
    {
        if(pos == -1)
            pos = (long)colorStr.length();

        tmp = colorStr.substr(0, pos);
        colorStr = colorStr.substr(pos);
        colorStr = colorStr.Trim(PS_TRIM_LEFT);
        pos = colorStr.Find(' ');

        float value = 0.0f;
        if(tmp.Find('.') != -1)
            value = tmp.ToFloat();
        else
        {
            int intValue = tmp.ToInt();
            if(intValue > 0)
            {
                if(intValue > 255)
                    intValue = 255;

                value = ((float)intValue) / 255.0f;
            }
        }

        value = ClampFloat(value, 0.0f, 1.0f);
        switch(idx)
        {
        case 0:
            color.r = value;
            break;

        case 1:
            color.g = value;
            break;

        case 2:
            color.b = value;
            break;

        case 3:
            color.a = value;
            break;

        default:
            break;
        }

        if(idx >= 3)
            break;

        idx++;
    }

    return color;
}

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

#if 0 // unused
static GLenum ParseAlphaFunc(CStr temp)
{
    temp = temp.LowerCase().Trim(PS_TRIM_BOTH);
    if(temp.empty())
        return GL_NONE;

    if(temp == CStr("never"))
        return GL_NEVER;
    else if(temp == CStr("less"))
        return GL_LESS;
    else if(temp == CStr("lequal"))
        return GL_LEQUAL;
    else if(temp == CStr("greater"))
        return GL_GREATER;
    else if(temp == CStr("gequal"))
        return GL_GEQUAL;
    else if(temp == CStr("equal"))
        return GL_EQUAL;
    else if(temp == CStr("notequal"))
        return GL_NOTEQUAL;
    else if(temp == CStr("always"))
        return GL_ALWAYS;
    else
        return GL_NONE;
}

static GLenum ParseBlendFunc(CStr temp)
{
    temp = temp.LowerCase().Trim(PS_TRIM_BOTH);
    if(temp.empty())
        return GL_NONE;

    if(temp == CStr("zero"))
        return GL_ZERO;
    else if(temp == CStr("one"))
        return GL_ONE;
    else if(temp == CStr("sourcecolor"))
        return GL_SRC_COLOR;
    else if(temp == CStr("oneminussourcecolor"))
        return GL_ONE_MINUS_SRC_COLOR;
    else if(temp == CStr("destcolor"))
        return GL_DST_COLOR;
    else if(temp == CStr("oneminusdestcolor"))
        return GL_ONE_MINUS_DST_COLOR;
    else if(temp == CStr("sourcealpha"))
        return GL_SRC_ALPHA;
    else if(temp == CStr("oneminussourcealpha"))
        return GL_ONE_MINUS_SRC_ALPHA;
    else if(temp == CStr("destalpha"))
        return GL_DST_ALPHA;
    else if(temp == CStr("oneminusdestalpha"))
        return GL_ONE_MINUS_DST_ALPHA;
    else if(temp == CStr("sourcealphasaturate"))
        return GL_SRC_ALPHA_SATURATE;

    return GL_NONE;
}
#endif

CMaterialManager::CMaterialManager()
{
}

CMaterialManager::~CMaterialManager()
{
    std::map<VfsPath, CMaterial*>::iterator iter;
    for(iter = m_Materials.begin(); iter != m_Materials.end(); iter++)
    {
        if((*iter).second)
            delete (*iter).second;
    }

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
	if(xeroFile.Load(pathname) != PSRETURN_OK)
		return NullMaterial;

	#define EL(x) int el_##x = xeroFile.GetElementID(#x)
	#define AT(x) int at_##x = xeroFile.GetAttributeID(#x)
	EL(texture);
	EL(vertexprogram);
	EL(fragmentprogram);

	EL(colors);
	AT(diffuse);
	AT(ambient);
	AT(specular);
	//AT(emissive);
	AT(specularpower);

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
			else if(token == el_vertexprogram)
			{
				CStr value(node.GetText());
				material->SetVertexProgram(value);
			}
			else if(token == el_fragmentprogram)
			{
				CStr value(node.GetText());
				material->SetFragmentProgram(value);
			}
			else if(token == el_colors)
			{
				temp = CStr(attrs.GetNamedItem(at_diffuse));
				if(! temp.empty())
					material->SetDiffuse(ParseColor(temp));

				temp = CStr(attrs.GetNamedItem(at_ambient));
				if(! temp.empty())
					material->SetAmbient(ParseColor(temp));

				temp = CStr(attrs.GetNamedItem(at_specular));
				if(! temp.empty())
					material->SetSpecular(ParseColor(temp));

				temp = CStr(attrs.GetNamedItem(at_specularpower));
				if(! temp.empty())
					material->SetSpecularPower(ClampFloat(temp.ToFloat(), 0.0f, 1.0f));
			}
			else if(token == el_alpha)
			{
				temp = CStr(attrs.GetNamedItem(at_usage));

				// Determine whether the alpha is used for basic transparency or player color
				if (temp == "playercolor")
					material->SetPlayerColor_PerPlayer();
				else if (temp == "objectcolor")
					material->SetPlayerColor_PerObject();
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
