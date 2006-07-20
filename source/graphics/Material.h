#ifndef __H_MATERIAL_H__
#define __H_MATERIAL_H__

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
	SMaterialColor(const SMaterialColor &color)
	{
		r = color.r;
		g = color.g;
		b = color.b;
		a = color.a;
	}

	void operator =(const SMaterialColor color)
	{
		r = color.r;
		g = color.g;
		b = color.b;
		a = color.a;
	}
	bool operator ==(const SMaterialColor color);

	float Sum()
	{
		return (r + g + b + a);
	}
};

class CMaterial
{
public:
	CMaterial();
	CMaterial(const CMaterial &material);
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
	bool IsPlayer() { return (m_PlayerID != PLAYER_NONE); }
	// Get the player colour (in a non-zero amount of time, so don't call it
	// an unreasonable number of times. But it's fairly close to zero, so
	// don't worry too much about it.)
	SMaterialColor GetPlayerColor();

	void SetPlayerColor_PerPlayer() { m_PlayerID = PLAYER_COMINGSOON; }
	void SetPlayerColor_PerObject() { m_PlayerID = PLAYER_OTHER; }
	void SetPlayerColor(int id);
	void SetPlayerColor(CColor &colour);

	void SetTexture(const CStr& texture);
	void SetVertexProgram(const CStr& prog);
	void SetFragmentProgram(const CStr& prog);
	void SetDiffuse(const SMaterialColor &color);
	void SetAmbient(const SMaterialColor &color);
	void SetSpecular(const SMaterialColor &color);
	void SetEmissive(const SMaterialColor &color);
	void SetSpecularPower(float power);
	void SetUsesAlpha(bool flag);

	void operator =(const CMaterial &material);
	bool operator ==(const CMaterial &material);
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
	// If m_PlayerID >= 0, the colour is retrieved from g_Game whenever it's needed.
	//  (It's not cached, because the player might change colour.)
	// If m_PlayerID == PLAYER_OTHER, or if player-colouring has been globally
	//  disabled, m_TextureColor is used instead. This allows per-model colours to
	//  be specified, instead of only a single colour per player.
	// If m_PlayerID == PLAYER_NONE, there's no player colour at all.
	// If m_PlayerID == PLAYER_COMINGSOON, it's going to be linked to a player,
	//  but hasn't yet.
	enum { PLAYER_NONE = -1, PLAYER_OTHER = -2, PLAYER_COMINGSOON = -3 };
	int m_PlayerID;
	SMaterialColor m_TextureColor; // used as an alternative to the per-player colour
};

extern CMaterial NullMaterial;
extern CMaterial IdentityMaterial;

#endif
