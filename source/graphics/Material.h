#ifndef __H_MATERIAL_H__
#define __H_MATERIAL_H__

#include "CStr.h"

struct SMaterialColor
{
public:
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

	union
	{
		struct
		{
			float r;
			float g;
			float b;
			float a;
		};
		float data[4];
	};

    void operator =(const SMaterialColor color)
    {
        r = color.r;
        g = color.g;
        b = color.b;
        a = color.a;
    }
    bool operator ==(const SMaterialColor color)
    {
        return (
            r == color.r &&
            g == color.g &&
            b == color.b &&
            a == color.a
        );
    }

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
	SMaterialColor GetDiffuse();
	SMaterialColor GetAmbient();
	SMaterialColor GetSpecular();
	SMaterialColor GetEmissive();
	float GetSpecularPower() { return m_SpecularPower; }
	bool UsesAlpha() { return m_Alpha; }

    void SetTexture(const CStr &texture);
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

	// Alpha required flag
	bool m_Alpha;
};

extern CMaterial NullMaterial;
extern CMaterial IdentityMaterial;

#endif
