/* Copyright (C) 2010 Wildfire Games.
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

#include "lib/ogl.h"
#include "ps/Game.h"
#include "ps/Overlay.h" // for CColor

CMaterial NullMaterial;

// Values as taken straight from the Blue Book (god bless the Blue Book)
static SMaterialColor IdentityDiffuse(0.8f, 0.8f, 0.8f, 1.0f);
static SMaterialColor IdentityAmbient(0.2f, 0.2f, 0.2f, 1.0f);
static SMaterialColor IdentitySpecular(0.0f, 0.0f, 0.0f, 1.0f);
static SMaterialColor IdentityEmissive(0.0f, 0.0f, 0.0f, 1.0f);

static SMaterialColor BrokenColor(0.3f, 0.3f, 0.3f, 1.0f);

CMaterial::CMaterial()
	: m_Diffuse(IdentityDiffuse),
	m_Ambient(IdentityAmbient),
	m_Specular(IdentitySpecular),
	m_Emissive(IdentityEmissive),
	m_SpecularPower(0.0f),
	m_Alpha(false),
	m_PlayerID(INVALID_PLAYER),
	m_TextureColor(BrokenColor),
	m_UsePlayerColor(false),
	m_UseTextureColor(false)
{
}

void CMaterial::Bind()
{
    glMaterialf(GL_FRONT, GL_SHININESS, m_SpecularPower);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, &m_Diffuse.r);
    glMaterialfv(GL_FRONT, GL_AMBIENT, &m_Ambient.r);
    glMaterialfv(GL_FRONT, GL_SPECULAR, &m_Specular.r);
    glMaterialfv(GL_FRONT, GL_EMISSION, &m_Emissive.r);

    ogl_WarnIfError();
}

void CMaterial::Unbind()
{
}

SMaterialColor CMaterial::GetDiffuse()
{
    return m_Diffuse;
}

SMaterialColor CMaterial::GetAmbient()
{
    return m_Ambient;
}

SMaterialColor CMaterial::GetSpecular()
{
    return m_Specular;
}

SMaterialColor CMaterial::GetEmissive()
{
    return m_Emissive;
}

SMaterialColor CMaterial::GetObjectColor()
{
	if (m_UseTextureColor)
		return m_TextureColor;

	debug_assert(m_UsePlayerColor);
		// this should never be called unless IsPlayer returned true

	return GetPlayerColor();
}

SMaterialColor CMaterial::GetPlayerColor()
{
	if (m_PlayerID == -1)
		return BrokenColor;

	CColor c(g_Game->GetPlayerColour(m_PlayerID));
	return SMaterialColor(c.r, c.g, c.b, c.a);
}

void CMaterial::SetPlayerID(player_id_t id)
{
	m_PlayerID = id;
}

void CMaterial::SetUsePlayerColor(bool use)
{
	m_UsePlayerColor = use;
}

void CMaterial::SetUseTextureColor(bool use)
{
	m_UseTextureColor = use;
}

void CMaterial::SetTextureColor(const CColor& colour)
{
	m_TextureColor = SMaterialColor(colour.r, colour.g, colour.b, colour.a);
}

void CMaterial::SetTexture(const CStr& texture)
{
    m_Texture = texture;
}

void CMaterial::SetVertexProgram(const CStr& prog)
{
    m_VertexProgram = prog;
}

void CMaterial::SetFragmentProgram(const CStr& prog)
{
    m_FragmentProgram = prog;
}

void CMaterial::SetDiffuse(const SMaterialColor& color)
{
	m_Diffuse = color;
}

void CMaterial::SetAmbient(const SMaterialColor& color)
{
	m_Ambient = color;
}

void CMaterial::SetSpecular(const SMaterialColor& color)
{
	m_Specular = color;
}

void CMaterial::SetEmissive(const SMaterialColor& color)
{
	m_Emissive = color;
}

void CMaterial::SetSpecularPower(float power)
{
    m_SpecularPower = power;
}

void CMaterial::SetUsesAlpha(bool flag)
{
    m_Alpha = flag;
}
