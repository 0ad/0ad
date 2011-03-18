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

#ifndef INCLUDED_MATERIAL
#define INCLUDED_MATERIAL

#include "ps/CStr.h"
#include "simulation2/helpers/Player.h"

// FIXME: This material system is almost entirely unused and probably broken

struct CColor;

struct SMaterialColor
{
public:
	float r;
	float g;
	float b;
	float a;

	SMaterialColor() : r(0.0f), g(0.0f), b(0.0f), a(1.0f) {}
	SMaterialColor(float _r, float _g, float _b, float _a)
	{
		r = _r;
		g = _g;
		b = _b;
		a = _a;
	}
};

class CMaterial
{
public:
	CMaterial();

	void Bind();
	void Unbind();

	const CStr& GetTexture() { return m_Texture; }
	const CStr& GetVertexProgram() { return m_VertexProgram; }
	const CStr& GetFragmentProgram() { return m_FragmentProgram; }
	SMaterialColor GetDiffuse();
	SMaterialColor GetAmbient();
	SMaterialColor GetSpecular();
	SMaterialColor GetEmissive();
	float GetSpecularPower() { return m_SpecularPower; }

	bool UsesAlpha() { return m_Alpha; }

	// Determines whether or not the model goes into the PlayerRenderer
	bool IsPlayer() { return (m_UseTextureColor || m_UsePlayerColor); }

	// Get the player colour or texture colour to be applied to this object
	SMaterialColor GetObjectColor();
	// Get the player colour
	SMaterialColor GetPlayerColor();

	void SetPlayerID(player_id_t id);
	void SetTextureColor(const CColor &colour);

	void SetUsePlayerColor(bool use);
	void SetUseTextureColor(bool use);

	void SetTexture(const CStr& texture);
	void SetVertexProgram(const CStr& prog);
	void SetFragmentProgram(const CStr& prog);
	void SetDiffuse(const SMaterialColor& color);
	void SetAmbient(const SMaterialColor& color);
	void SetSpecular(const SMaterialColor& color);
	void SetEmissive(const SMaterialColor& color);
	void SetSpecularPower(float power);
	void SetUsesAlpha(bool flag);

private:
	// Various reflective color properties
	SMaterialColor m_Diffuse;
	SMaterialColor m_Ambient;
	SMaterialColor m_Specular;
	SMaterialColor m_Emissive;
	float m_SpecularPower;

	// Path to the materials texture
	CStr m_Texture;

	// Paths to vertex/fragment programs
	CStr m_VertexProgram;
	CStr m_FragmentProgram;

	// Alpha required flag
	bool m_Alpha;

	player_id_t m_PlayerID;
	SMaterialColor m_TextureColor; // used as an alternative to the per-player colour

	bool m_UsePlayerColor;
	bool m_UseTextureColor;
};

extern CMaterial NullMaterial;

#endif
