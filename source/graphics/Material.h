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
};

class CMaterial
{
public:
	CMaterial();
	CMaterial(const CMaterial &material);
	virtual ~CMaterial();

	bool Apply();

	CStr GetTexture() { return m_Texture; }
	SMaterialColor GetDiffuse();
	SMaterialColor GetAmbient();
	SMaterialColor GetSpecular();
	SMaterialColor GetEmissive();
	float GetSpecularPower() { return m_SpecularPower; }
	GLenum GetSourceBlend() { return m_SourceBlend; }
	GLenum GetDestBlend() { return m_DestBlend; }
	GLenum GetAlphaFunc() { return m_AlphaFunc; }
	float GetAlphaClamp() { return m_AlphaClamp; }
	bool UsesAlpha() { return m_Alpha; }

    void SetTexture(CStr &texture) { m_Texture = texture; }
	void SetDiffuse(SMaterialColor &color);
	void SetAmbient(SMaterialColor &color);
	void SetSpecular(SMaterialColor &color);
	void SetEmissive(SMaterialColor &color);
    void SetSpecularPower(float power) { m_SpecularPower = power; }
    void SetSourceBlend(GLenum func) { m_SourceBlend = func; }
    void SetDestBlend(GLenum func) { m_DestBlend = func; }
    void SetAlphaFunc(GLenum func) { m_AlphaFunc = func; }
    void SetAlphaClamp(float clamp) { m_AlphaClamp = clamp; }
    void SetUsesAlpha(bool flag) { m_Alpha = flag; }

	void operator =(CMaterial &material);
protected:
	// Various reflective color properties
	SMaterialColor *m_Diffuse;
	SMaterialColor *m_Ambient;
	SMaterialColor *m_Specular;
	SMaterialColor *m_Emissive;
	float m_SpecularPower;

	// Path to the materials texture
	CStr m_Texture;
	
	// OpenGL blend and alpha func states for the materials render pass
	GLenum m_SourceBlend;
	GLenum m_DestBlend;
	GLenum m_AlphaFunc;
	float m_AlphaClamp;

	// Alpha required flag
	bool m_Alpha;
};

extern CMaterial NullMaterial;
extern CMaterial IdentityMaterial;

#endif