#include "precompiled.h"

#include "ogl.h"
#include "lib/res/graphics/ogl_shader.h"
#include "ps/CLogger.h"
#include "renderer/Renderer.h"
#include "renderer/RenderPathVertexShader.h"

#define LOG_CATEGORY "graphics"


RenderPathVertexShader::RenderPathVertexShader()
{
	m_ModelLight = 0;
	m_ModelLight_Ambient = -1;
	m_ModelLight_SunDir = -1;
	m_ModelLight_SunColor = -1;

	m_InstancingLight = 0;
	m_InstancingLight_Ambient = -1;
	m_InstancingLight_SunDir = -1;
	m_InstancingLight_SunColor = -1;
	m_InstancingLight_Instancing1 = -1;
	m_InstancingLight_Instancing2 = -1;
	m_InstancingLight_Instancing3 = -1;

	m_Instancing = 0;
	m_Instancing_Instancing1 = -1;
	m_Instancing_Instancing2 = -1;
	m_Instancing_Instancing3 = -1;
}

RenderPathVertexShader::~RenderPathVertexShader()
{
	if (m_ModelLight)
		ogl_program_free(m_ModelLight);
	if (m_InstancingLight)
		ogl_program_free(m_InstancingLight);
	if (m_Instancing)
		ogl_program_free(m_Instancing);
}

// Initialize this render path.
// Use delayed initialization so that we can fallback to a different render path
// if anything went wrong and use the destructor to clean things up.
bool RenderPathVertexShader::Init()
{
	if (!g_Renderer.m_Caps.m_VertexShader)
		return false;

	m_ModelLight = ogl_program_load("shaders/model_light.xml");
	if (m_ModelLight < 0)
	{
		LOG(WARNING, LOG_CATEGORY, "Failed to load shaders/model_light.xml: %i\n", (int)m_ModelLight);
		return false;
	}

	m_InstancingLight = ogl_program_load("shaders/instancing_light.xml");
	if (m_InstancingLight < 0)
	{
		LOG(WARNING, LOG_CATEGORY, "Failed to load shaders/instancing_light.xml: %i\n", (int)m_InstancingLight);
		return false;
	}

	m_Instancing = ogl_program_load("shaders/instancing.xml");
	if (m_Instancing < 0)
	{
		LOG(WARNING, LOG_CATEGORY, "Failed to load shaders/instancing.xml: %i\n", (int)m_Instancing);
		return false;
	}

	return true;
}


// This is quite the hack, but due to shader reloads,
// the uniform locations might have changed under us.
void RenderPathVertexShader::BeginFrame()
{
	m_ModelLight_Ambient = ogl_program_get_uniform_location(m_ModelLight, "ambient");
	m_ModelLight_SunDir = ogl_program_get_uniform_location(m_ModelLight, "sunDir");
	m_ModelLight_SunColor = ogl_program_get_uniform_location(m_ModelLight, "sunColor");

	m_InstancingLight_Ambient = ogl_program_get_uniform_location(
			m_InstancingLight, "ambient");
	m_InstancingLight_SunDir = ogl_program_get_uniform_location(
			m_InstancingLight, "sunDir");
	m_InstancingLight_SunColor = ogl_program_get_uniform_location(
			m_InstancingLight, "sunColor");
	m_InstancingLight_Instancing1 = ogl_program_get_attrib_location(
			m_InstancingLight, "Instancing1");
	m_InstancingLight_Instancing2 = ogl_program_get_attrib_location(
			m_InstancingLight, "Instancing2");
	m_InstancingLight_Instancing3 = ogl_program_get_attrib_location(
			m_InstancingLight, "Instancing3");

	m_Instancing_Instancing1 = ogl_program_get_attrib_location(
			m_Instancing, "Instancing1");
	m_Instancing_Instancing2 = ogl_program_get_attrib_location(
			m_Instancing, "Instancing2");
	m_Instancing_Instancing3 = ogl_program_get_attrib_location(
			m_Instancing, "Instancing3");
}
