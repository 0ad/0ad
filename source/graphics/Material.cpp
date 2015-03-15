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

#include "Material.h"

static CColor BrokenColor(0.3f, 0.3f, 0.3f, 1.0f);

CMaterial::CMaterial() :
	m_AlphaBlending(false)
{
}

void CMaterial::SetShaderEffect(const CStr& effect)
{
	m_ShaderEffect = CStrIntern(effect);
}

void CMaterial::AddShaderDefine(CStrIntern key, CStrIntern value)
{
	m_ShaderDefines.Add(key, value);
	m_CombinedShaderDefines.clear();
}

void CMaterial::AddConditionalDefine(const char* defname, const char* defvalue, int type, std::vector<float> &args)
{
	m_ConditionalDefines.Add(defname, defvalue, type, args);
	m_CombinedShaderDefines.clear();
}

void CMaterial::AddStaticUniform(const char* key, const CVector4D& value)
{
	m_StaticUniforms.Add(key, value);
}

void CMaterial::AddSampler(const TextureSampler& texture)
{
	m_Samplers.push_back(texture);
	if (texture.Name == str_baseTex)
		m_DiffuseTexture = texture.Sampler;
}

void CMaterial::AddRenderQuery(const char* key)
{
	m_RenderQueries.Add(key);
}

void CMaterial::AddRequiredSampler(const CStr& samplerName)
{
	CStrIntern string(samplerName);
	m_RequiredSamplers.push_back(string);
}


// Set up m_CombinedShaderDefines so that index i contains m_ShaderDefines, plus
// the extra defines from m_ConditionalDefines[j] for all j where bit j is set in i.
// This lets GetShaderDefines() cheaply return the defines for any combination of conditions.
//
// (This might scale badly if we had a large number of conditional defines per material,
// but currently we don't expect to have many.)
void CMaterial::RecomputeCombinedShaderDefines()
{
	m_CombinedShaderDefines.clear();
	int size = m_ConditionalDefines.GetSize();

	// Loop over all 2^n combinations of flags
	for (int i = 0; i < (1 << size); i++)
	{
		CShaderDefines defs = m_ShaderDefines;
		for (int j = 0; j < size; j++)
		{
			if (i & (1 << j))
			{
				const CShaderConditionalDefines::CondDefine& def = m_ConditionalDefines.GetItem(j);
				defs.Add(def.m_DefName, def.m_DefValue);
			}
		}
		m_CombinedShaderDefines.push_back(defs);
	}
}
