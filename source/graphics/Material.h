#ifndef __H_MATERIAL_H__
#define __H_MATERIAL_H__

#include "CStr.h"

struct SMaterialColor
{
public:
    float r;
    float g;
    float b;
    float a;

    SMaterialColor() { r = 0.0f; g = 0.0f; b = 0.0f; a = 1.0f; }
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

	CStr GetTexture() { return m_Texture; }
    CStr GetVertexProgram() { return m_VertexProgram; }
    CStr GetFragmentProgram() { return m_FragmentProgram; }
	SMaterialColor GetDiffuse();
	SMaterialColor GetAmbient();
	SMaterialColor GetSpecular();
	SMaterialColor GetEmissive();
	float GetSpecularPower() { return m_SpecularPower; }
	bool UsesAlpha() { return m_Alpha; }

    void SetTexture(const CStr &texture);
    void SetVertexProgram(const CStr &prog);
    void SetFragmentProgram(const CStr &prog);
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
};

extern CMaterial NullMaterial;
extern CMaterial IdentityMaterial;

#endif
