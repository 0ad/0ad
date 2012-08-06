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

#include "MaterialManager.h"

#include "lib/ogl.h"
#include "maths/MathUtil.h"
#include "maths/Vector4D.h"
#include "ps/ConfigDB.h"
#include "ps/Filesystem.h"
#include "ps/PreprocessorWrapper.h"
#include "ps/XML/Xeromyces.h"
#include "renderer/Renderer.h"

#include <sstream>

CMaterialManager::CMaterialManager()
{
	qualityLevel = 5.0;
	CFG_GET_USER_VAL("materialmgr.quality", Float, qualityLevel);
	qualityLevel = clamp(qualityLevel, 0.0f, 10.0f);
}

CMaterial CMaterialManager::LoadMaterial(const VfsPath& pathname)
{
	if (pathname.empty())
		return CMaterial();

	std::map<VfsPath, CMaterial>::iterator iter = m_Materials.find(pathname);
	if (iter != m_Materials.end())
		return iter->second;

	CXeromyces xeroFile;
	if (xeroFile.Load(g_VFS, pathname) != PSRETURN_OK)
		return CMaterial();

	#define EL(x) int el_##x = xeroFile.GetElementID(#x)
	#define AT(x) int at_##x = xeroFile.GetAttributeID(#x)
	EL(alpha_blending);
	EL(alternative);
	EL(define);
	EL(shader);
	EL(uniform);
	EL(renderquery);
	EL(conditional_define);
	AT(effect);
	AT(if);
	AT(quality);
	AT(material);
	AT(name);
	AT(value);
	AT(type);
	AT(min);
	AT(max);
	AT(conf);
	#undef AT
	#undef EL

	CMaterial material;

	XMBElement root = xeroFile.GetRoot();
	
	CPreprocessorWrapper preprocessor;
	preprocessor.AddDefine("CFG_FORCE_ALPHATEST", g_Renderer.m_Options.m_ForceAlphaTest ? "1" : "0");
	
	CVector4D vec(qualityLevel,0,0,0);
	material.AddStaticUniform("qualityLevel", vec);

	XERO_ITER_EL(root, node)
	{
		int token = node.GetNodeName();
		XMBAttributeList attrs = node.GetAttributes();
		if (token == el_alternative)
		{
			CStr cond = attrs.GetNamedItem(at_if);
			if (!(!cond.empty() && preprocessor.TestConditional(cond)))
			{
				cond = attrs.GetNamedItem(at_quality);
				if (cond.empty())
					continue;
				else
				{	
					if (cond.ToFloat() <= qualityLevel)
						continue;
				}
			}
				
			material = LoadMaterial(VfsPath("art/materials") / attrs.GetNamedItem(at_material).FromUTF8());
			break;
		}
		else if (token == el_alpha_blending)
		{
			material.SetUsesAlphaBlending(true);
		}
		else if (token == el_shader)
		{
			material.SetShaderEffect(attrs.GetNamedItem(at_effect));
		}
		else if (token == el_define)
		{
			material.AddShaderDefine(attrs.GetNamedItem(at_name).c_str(), attrs.GetNamedItem(at_value).c_str());
		}
		else if (token == el_conditional_define)
		{
			std::vector<float> args;
			
			CStr type = attrs.GetNamedItem(at_type).c_str();
			int typeID = -1;
			
			if (type == CStr("draw_range"))
			{
				typeID = DCOND_DISTANCE;
				
				float valmin = -1.0f; 
				float valmax = -1.0f;
				
				CStr conf = attrs.GetNamedItem(at_conf);
				if (!conf.empty())
				{
					CFG_GET_USER_VAL("materialmgr." + conf + ".min", Float, valmin);
					CFG_GET_USER_VAL("materialmgr." + conf + ".max", Float, valmax);
				}
				else
				{
					CStr dmin = attrs.GetNamedItem(at_min);
					if (!dmin.empty())
						valmin = attrs.GetNamedItem(at_min).ToFloat();
					
					CStr dmax = attrs.GetNamedItem(at_max);
					if (!dmax.empty())
						valmax = attrs.GetNamedItem(at_max).ToFloat();
				}
				
				args.push_back(valmin);
				args.push_back(valmax);
				
				if (valmin >= 0.0f)
				{
					std::stringstream sstr;
					sstr << valmin;
					material.AddShaderDefine((conf + "_MIN").c_str(), sstr.str().c_str());
				}
				
				if (valmax >= 0.0f)
				{	
					std::stringstream sstr;
					sstr << valmax;
					material.AddShaderDefine((conf + "_MAX").c_str(), sstr.str().c_str());
				}
			}
			
			material.AddConditionalDefine(attrs.GetNamedItem(at_name).c_str(), 
						      attrs.GetNamedItem(at_value).c_str(), 
						      typeID, args);
		}		
		else if (token == el_uniform)
		{
			std::stringstream str(attrs.GetNamedItem(at_value));
			CVector4D vec;
			str >> vec.X >> vec.Y >> vec.Z >> vec.W;
			material.AddStaticUniform(attrs.GetNamedItem(at_name).c_str(), vec);
		}
		else if (token == el_renderquery)
		{
			material.AddRenderQuery(attrs.GetNamedItem(at_name).c_str());
		}
	}

	m_Materials[pathname] = material;
	return material;
}
