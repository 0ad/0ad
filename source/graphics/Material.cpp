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

CMaterial::CMaterial()
	: m_Diffuse(IdentityDiffuse),
    m_Ambient(IdentityAmbient),
    m_Specular(IdentitySpecular),
    m_Emissive(IdentityEmissive),
	m_SpecularPower(0.0f),
	m_Alpha(false)
{
    ComputeHash();
}

CMaterial::CMaterial(const CMaterial &material)
{
    (*this) = material;
}

CMaterial::~CMaterial()
{
}

void CMaterial::operator =(const CMaterial &material)
{
	m_Diffuse = material.m_Diffuse;
    m_Ambient = material.m_Ambient;
    m_Specular = material.m_Specular;
    m_Emissive = material.m_Emissive;

    m_SpecularPower = material.m_SpecularPower;
    m_Alpha = material.m_Alpha;
    ComputeHash();
}

bool CMaterial::operator ==(const CMaterial &material)
{
    return(
        m_Texture == m_Texture &&
        m_Diffuse == material.m_Diffuse &&
        m_Ambient == material.m_Ambient &&
        m_Specular == material.m_Specular &&
        m_Emissive == material.m_Emissive &&
        m_SpecularPower == material.m_SpecularPower &&
        m_Alpha == material.m_Alpha
    );
}

void CMaterial::Bind()
{
    glMaterialfv(GL_FRONT, GL_DIFFUSE, &m_Diffuse.data[0]);
    glMaterialfv(GL_FRONT, GL_AMBIENT, &m_Ambient.data[0]);
    glMaterialfv(GL_FRONT, GL_SPECULAR, &m_Specular.data[0]);
    glMaterialfv(GL_FRONT, GL_EMISSION, &m_Emissive.data[0]);
    glMaterialf(GL_FRONT, GL_SHININESS, m_SpecularPower);

    oglCheck();
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

void CMaterial::SetTexture(const CStr &texture)
{
    m_Texture = texture;
    ComputeHash();
}

void CMaterial::SetDiffuse(SMaterialColor &color)
{
	m_Diffuse = color;
    ComputeHash();
}

void CMaterial::SetAmbient(SMaterialColor &color)
{
	m_Ambient = color;
    ComputeHash();
}

void CMaterial::SetSpecular(SMaterialColor &color)
{
	m_Specular = color;
    ComputeHash();
}

void CMaterial::SetEmissive(SMaterialColor &color)
{
	m_Emissive = color;
    ComputeHash();
}

void CMaterial::SetSpecularPower(float power)
{
    m_SpecularPower = power;
    ComputeHash();
}

void CMaterial::SetUsesAlpha(bool flag)
{
    m_Alpha = flag;
    ComputeHash();
}

void CMaterial::ComputeHash()
{
    m_Hash = 
        m_Diffuse.Sum() +
        m_Ambient.Sum() +
        m_Specular.Sum() +
        m_Emissive.Sum() +
        m_SpecularPower +
        (float)m_Texture.GetHashCode();
}
