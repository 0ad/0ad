#include "ogl.h"
#include "res/res.h"
#include "Material.h"

#define SAFE_DELETE(x) \
	if((x)) { delete (x); (x) = NULL; }

CMaterial NullMaterial;
CMaterial IdentityMaterial;

// Values as taken straight from the Blue Book (god bless the Blue Book)
static SMaterialColor IdentityDiffuse(0.8f, 0.8f, 0.8f, 1.0f);
static SMaterialColor IdentityAmbient(0.2f, 0.2f, 0.2f, 1.0f);
static SMaterialColor IdentitySpecular(0.0f, 0.0f, 0.0f, 1.0f);
static SMaterialColor IdentityEmissive(0.0f, 0.0f, 0.0f, 1.0f);

static SMaterialColor *CopyColor(SMaterialColor *color)
{
	if(!color)
		return NULL;

	if(color == &IdentityDiffuse
		|| color == &IdentityAmbient
		|| color == &IdentitySpecular
		|| color == &IdentityEmissive)
		return NULL;

	SMaterialColor *ret = new SMaterialColor();
	memcpy(ret, color, sizeof(SMaterialColor));
	return ret;
}

CMaterial::CMaterial()
	: m_Diffuse(NULL),
	m_Ambient(NULL),
	m_Specular(NULL),
	m_Emissive(NULL),
	m_SpecularPower(0.0f),
	m_SourceBlend(GL_NONE),
	m_DestBlend(GL_NONE),
	m_AlphaFunc(GL_NONE),
	m_AlphaClamp(1.0f),
	m_Alpha(false)
{
}

CMaterial::CMaterial(const CMaterial &material)
{
    (*this) = const_cast<CMaterial &>(material);
}

CMaterial::~CMaterial()
{
	SAFE_DELETE(m_Diffuse);
	SAFE_DELETE(m_Ambient);
	SAFE_DELETE(m_Specular);
	SAFE_DELETE(m_Emissive);
}

void CMaterial::operator =(CMaterial &material)
{
	m_Diffuse = CopyColor(material.m_Diffuse);
    m_Ambient = CopyColor(material.m_Ambient);
    m_Specular = CopyColor(material.m_Specular);
    m_Emissive = CopyColor(material.m_Emissive);

    m_SpecularPower = material.m_SpecularPower;
    m_SourceBlend = material.m_SourceBlend;
    m_DestBlend = material.m_DestBlend;
    m_AlphaFunc = material.m_AlphaFunc;
    m_AlphaClamp = material.m_AlphaClamp;
    m_Alpha = material.m_Alpha;
}

SMaterialColor CMaterial::GetDiffuse()
{
	if(!m_Diffuse)
		return IdentityDiffuse;

	return *m_Diffuse;
}

SMaterialColor CMaterial::GetAmbient()
{
	if(!m_Ambient)
		return IdentityAmbient;

	return *m_Ambient;
}

SMaterialColor CMaterial::GetSpecular()
{
	if(!m_Specular)
		return IdentitySpecular;

	return *m_Specular;
}

SMaterialColor CMaterial::GetEmissive()
{
	if(!m_Emissive)
		return IdentityEmissive;

	return *m_Emissive;
}

#define SETMC(var) \
	if((var)) (*var) = color; \
	else (var) = CopyColor(&color) ;

void CMaterial::SetDiffuse(SMaterialColor &color)
{
	SETMC(m_Diffuse);
}

void CMaterial::SetAmbient(SMaterialColor &color)
{
	SETMC(m_Ambient);
}

void CMaterial::SetSpecular(SMaterialColor &color)
{
	SETMC(m_Specular);
}

void CMaterial::SetEmissive(SMaterialColor &color)
{
	SETMC(m_Emissive);
}

#undef SETMC