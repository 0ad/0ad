#include "precompiled.h"

#include "lib/ogl.h"
#include "Material.h"
#include "ps/Player.h"
#include "ps/Game.h"
#include "ps/Overlay.h" // for CColor

CMaterial NullMaterial;
CMaterial IdentityMaterial;

// Values as taken straight from the Blue Book (god bless the Blue Book)
static SMaterialColor IdentityDiffuse(0.8f, 0.8f, 0.8f, 1.0f);
static SMaterialColor IdentityAmbient(0.2f, 0.2f, 0.2f, 1.0f);
static SMaterialColor IdentitySpecular(0.0f, 0.0f, 0.0f, 1.0f);
static SMaterialColor IdentityEmissive(0.0f, 0.0f, 0.0f, 1.0f);

static SMaterialColor BrokenColor(0.3f, 0.3f, 0.3f, 1.0f);

bool SMaterialColor::operator==(const SMaterialColor& color)
{
	return (
		r == color.r &&
		g == color.g &&
		b == color.b &&
		a == color.a
		);
}

CMaterial::CMaterial()
	: m_Diffuse(IdentityDiffuse),
	m_Ambient(IdentityAmbient),
	m_Specular(IdentitySpecular),
	m_Emissive(IdentityEmissive),
	m_SpecularPower(0.0f),
	m_Alpha(false),
	m_PlayerID(PLAYER_ID_NONE),
	m_TextureColor(BrokenColor)
{
	ComputeHash();
}

CMaterial::CMaterial(const CMaterial& material)
{
	(*this) = material;
}

CMaterial::~CMaterial()
{
}

void CMaterial::operator=(const CMaterial& material)
{
	m_Diffuse = material.m_Diffuse;
	m_Ambient = material.m_Ambient;
	m_Specular = material.m_Specular;
	m_Emissive = material.m_Emissive;

	m_SpecularPower = material.m_SpecularPower;
	m_Alpha = material.m_Alpha;
	m_PlayerID = material.m_PlayerID;
	m_TextureColor = material.m_TextureColor;
	ComputeHash();
}

bool CMaterial::operator==(const CMaterial& material)
{
	return(
		m_Texture == m_Texture &&
		m_Diffuse == material.m_Diffuse &&
		m_Ambient == material.m_Ambient &&
		m_Specular == material.m_Specular &&
		m_Emissive == material.m_Emissive &&
		m_SpecularPower == material.m_SpecularPower &&
		m_Alpha == material.m_Alpha &&
		m_PlayerID == material.m_PlayerID &&
		m_TextureColor == material.m_TextureColor
	);
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

SMaterialColor CMaterial::GetPlayerColor()
{
	debug_assert(m_PlayerID != PLAYER_ID_NONE);
		// because this should never be called unless IsPlayer returned true

	if (m_PlayerID == PLAYER_ID_OTHER /* TODO: or if player-colour is globally disabled */ )
		return m_TextureColor;

	if (m_PlayerID <= PLAYER_ID_LAST_VALID)
	{
		CPlayer* player = g_Game->GetPlayer(m_PlayerID);
		if (player)
		{
			const SPlayerColour& c (player->GetColour());
			return SMaterialColor(c.r, c.g, c.b, c.a);
		}
	}

	// Oops, something failed.
	return BrokenColor;
}

void CMaterial::SetPlayerColor(size_t id)
{
	if (m_PlayerID == PLAYER_ID_COMING_SOON || m_PlayerID <= PLAYER_ID_LAST_VALID)
		m_PlayerID = id;
}

void CMaterial::SetPlayerColor(CColor& colour)
{
	m_TextureColor = SMaterialColor(colour.r, colour.g, colour.b, colour.a);
}


void CMaterial::SetTexture(const CStr& texture)
{
    m_Texture = texture;
    ComputeHash();
}

void CMaterial::SetVertexProgram(const CStr& prog)
{
    m_VertexProgram = prog;
    ComputeHash();
}

void CMaterial::SetFragmentProgram(const CStr& prog)
{
    m_FragmentProgram = prog;
    ComputeHash();
}

void CMaterial::SetDiffuse(const SMaterialColor& color)
{
	m_Diffuse = color;
    ComputeHash();
}

void CMaterial::SetAmbient(const SMaterialColor& color)
{
	m_Ambient = color;
    ComputeHash();
}

void CMaterial::SetSpecular(const SMaterialColor& color)
{
	m_Specular = color;
    ComputeHash();
}

void CMaterial::SetEmissive(const SMaterialColor& color)
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
        (float)m_Texture.GetHashCode() +
        (float)m_VertexProgram.GetHashCode() +
        (float)m_FragmentProgram.GetHashCode();
}
