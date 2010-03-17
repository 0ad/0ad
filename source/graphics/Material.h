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

#ifndef INCLUDED_MATERIAL
#define INCLUDED_MATERIAL

#include "ps/CStr.h"

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
	SMaterialColor(const SMaterialColor& color)
	{
		r = color.r;
		g = color.g;
		b = color.b;
		a = color.a;
	}

	void operator=(const SMaterialColor& color)
	{
		r = color.r;
		g = color.g;
		b = color.b;
		a = color.a;
	}
	bool operator==(const SMaterialColor& color);

	float Sum()
	{
		return (r + g + b + a);
	}
};

class CMaterial
{
public:
	CMaterial();
	CMaterial(const CMaterial& material);
	virtual ~CMaterial();

	void Bind();
	void Unbind();
	float GetHash() { return m_Hash; }

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
	bool IsPlayer() { return (m_PlayerID != PLAYER_ID_NONE); }
	// Get the player colour (in a non-zero amount of time, so don't call it
	// an unreasonable number of times. But it's fairly close to zero, so
	// don't worry too much about it.)
	SMaterialColor GetPlayerColor();

	void SetPlayerColor_PerPlayer() { m_PlayerID = PLAYER_ID_COMING_SOON; }
	void SetPlayerColor_PerObject() { m_PlayerID = PLAYER_ID_OTHER; }
	void SetPlayerColor(size_t id);
	void SetPlayerColor(const CColor &colour);

	void SetTexture(const CStr& texture);
	void SetVertexProgram(const CStr& prog);
	void SetFragmentProgram(const CStr& prog);
	void SetDiffuse(const SMaterialColor& color);
	void SetAmbient(const SMaterialColor& color);
	void SetSpecular(const SMaterialColor& color);
	void SetEmissive(const SMaterialColor& color);
	void SetSpecularPower(float power);
	void SetUsesAlpha(bool flag);

	void operator=(const CMaterial& material);
	bool operator==(const CMaterial& material);
protected:
	void ComputeHash();

	float m_Hash;

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

	// Player-colour settings.
	// If m_PlayerID <= PLAYER_ID_LAST_VALID, the colour is retrieved from
	//  g_Game whenever it's needed.
	//  (It's not cached, because the player might change colour.)
	// If m_PlayerID == PLAYER_ID_OTHER, or if player-colouring has been globally
	//  disabled, m_TextureColor is used instead. This allows per-model colours to
	//  be specified, instead of only a single colour per player.
	// If m_PlayerID == PLAYER_ID_NONE, there's no player colour at all.
	// If m_PlayerID == PLAYER_ID_COMING_SOON, it's going to be linked to a player,
	//  but hasn't yet.
	static const size_t PLAYER_ID_NONE = SIZE_MAX-1;
	static const size_t PLAYER_ID_OTHER = SIZE_MAX-2;
	static const size_t PLAYER_ID_COMING_SOON = SIZE_MAX-3;
	static const size_t PLAYER_ID_LAST_VALID = SIZE_MAX-4;
	size_t m_PlayerID;
	SMaterialColor m_TextureColor; // used as an alternative to the per-player colour
};

extern CMaterial NullMaterial;
extern CMaterial IdentityMaterial;

#endif
