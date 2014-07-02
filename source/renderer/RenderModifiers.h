/* Copyright (C) 2012 Wildfire Games.
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

/*
 * RenderModifiers can affect the fragment stage behaviour of some
 * ModelRenderers. This file defines some common RenderModifiers in
 * addition to the base class.
 * 
 * TODO: See comment in CRendererInternals::Models - we no longer use multiple
 * subclasses of RenderModifier, so most of the stuff here is unnecessary
 * abstraction which should probably be cleaned up.
 */

#ifndef INCLUDED_RENDERMODIFIERS
#define INCLUDED_RENDERMODIFIERS

#include "ModelRenderer.h"
#include "graphics/ShaderProgram.h"
#include "graphics/ShaderTechnique.h"
#include "graphics/Texture.h"

class CLightEnv;
class CMatrix3D;
class CModel;
class ShadowMap;

/**
 * Class RenderModifier: Some ModelRenderer implementations provide vertex
 * management behaviour but allow fragment stages to be modified by a plugged in
 * RenderModifier.
 *
 * You should use RenderModifierPtr when referencing RenderModifiers.
 */
class RenderModifier
{
public:
	RenderModifier() { }
	virtual ~RenderModifier() { }

	/**
	 * BeginPass: Setup OpenGL for the given rendering pass.
	 *
	 * Must be implemented by derived classes.
	 *
	 * @param pass The current pass number (pass == 0 is the first pass)
	 *
	 * @return The streamflags that indicate which vertex components
	 * are required by the fragment stages (see STREAM_XYZ constants).
	 */
	virtual void BeginPass(const CShaderProgramPtr& shader) = 0;

	/**
	 * PrepareModel: Called before rendering the given model.
	 *
	 * Default behaviour does nothing.
	 *
	 * @param pass The current pass number (pass == 0 is the first pass)
	 * @param model The model that is about to be rendered.
	 */
	virtual void PrepareModel(const CShaderProgramPtr& shader, CModel* model) = 0;
};


/**
 * Class LitRenderModifier: Abstract base class for RenderModifiers that apply
 * a shadow map.
 * LitRenderModifiers expect the diffuse brightness in the primary color (instead of ambient + diffuse).
 */
class LitRenderModifier : public RenderModifier
{
public:
	LitRenderModifier();
	~LitRenderModifier();

	/**
	 * SetShadowMap: Set the shadow map that will be used for rendering.
	 * Must be called by the user of the RenderModifier.
	 *
	 * The shadow map must be non-null and use depth texturing, or subsequent rendering
	 * using this RenderModifier will fail.
	 *
	 * @param shadow the shadow map
	 */
	void SetShadowMap(const ShadowMap* shadow);

	/**
	 * SetLightEnv: Set the light environment that will be used for rendering.
	 * Must be called by the user of the RenderModifier.
	 *
	 * @param lightenv the light environment (must be non-null)
	 */
	void SetLightEnv(const CLightEnv* lightenv);

	const ShadowMap* GetShadowMap() const { return m_Shadow; }
	const CLightEnv* GetLightEnv() const { return m_LightEnv; }

private:
	const ShadowMap* m_Shadow;
	const CLightEnv* m_LightEnv;
};

/**
 * A RenderModifier that sets uniforms and textures appropriately for rendering models.
 */
class ShaderRenderModifier : public LitRenderModifier
{
public:
	ShaderRenderModifier();

	// Implementation
	void BeginPass(const CShaderProgramPtr& shader);
	void PrepareModel(const CShaderProgramPtr& shader, CModel* model);

private:
	CShaderProgram::Binding m_BindingInstancingTransform;
	CShaderProgram::Binding m_BindingShadingColor;
	CShaderProgram::Binding m_BindingPlayerColor;
};

#endif // INCLUDED_RENDERMODIFIERS
