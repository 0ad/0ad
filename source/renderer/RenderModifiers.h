/**
 * =========================================================================
 * File        : RenderModifiers.h
 * Project     : Pyrogenesis
 * Description : RenderModifiers can affect the fragment stage behaviour
 *             : of some ModelRenderers. This file defines some common
 *             : RenderModifiers in addition to the base class.
 *
 * @author Nicolai Hähnle <nicolai@wildfiregames.com>
 * =========================================================================
 */

#ifndef RENDERMODIFIERS_H
#define RENDERMODIFIERS_H

#include "ModelRenderer.h"


class CModel;
class CTexture;


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
	virtual u32 BeginPass(uint pass) = 0;
	
	/**
	 * EndPass: Cleanup OpenGL state after the given pass. This function
	 * may indicate that additional passes are needed.
	 *
	 * Must be implemented by derived classes.
	 *
	 * @param pass The current pass number (pass == 0 is the first pass)
	 *
	 * @return true if rendering is complete, false if an additional pass
	 * is needed. If false is returned, BeginPass is then called with an
	 * increased pass number.
	 */
	virtual bool EndPass(uint pass) = 0;
	
	/**
	 * PrepareTexture: Called before rendering models that use the given
	 * texture.
	 *
	 * Must be implemented by derived classes.
	 *
	 * @param pass The current pass number (pass == 0 is the first pass)
	 * @param texture The texture used by subsequent models
	 */
	virtual void PrepareTexture(uint pass, CTexture* texture) = 0;
	
	/**
	 * PrepareModel: Called before rendering the given model.
	 * 
	 * Default behaviour does nothing.
	 *
	 * @param pass The current pass number (pass == 0 is the first pass)
	 * @param model The model that is about to be rendered.
	 */
	virtual void PrepareModel(uint pass, CModel* model);
};


/**
 * Class RenderModifierRenderer: Interface to a model renderer that can render
 * its models via a RenderModifier that sets up fragment stages.
 */
class RenderModifierRenderer : public ModelRenderer
{
public:
	RenderModifierRenderer() { }
	virtual ~RenderModifierRenderer();
	
};


/**
 * Class PlainRenderModifier: RenderModifier that simply uses opaque textures
 * modulated by primary color. It is used for normal, no-frills models.
 */
class PlainRenderModifier : public RenderModifier
{
public:
	PlainRenderModifier();
	~PlainRenderModifier();

	// Implementation
	u32 BeginPass(uint pass);
	bool EndPass(uint pass);
	void PrepareTexture(uint pass, CTexture* texture);
	void PrepareModel(uint pass, CModel* model);
};

/**
 * Class WireframeRenderModifier: RenderModifier that renders wireframe models.
 */
class WireframeRenderModifier : public RenderModifier
{
public:
	WireframeRenderModifier();
	~WireframeRenderModifier();

	// Implementation
	u32 BeginPass(uint pass);
	bool EndPass(uint pass);
	void PrepareTexture(uint pass, CTexture* texture);
	void PrepareModel(uint pass, CModel* model);
};


/**
 * Class SolidColorRenderModifier: Render all models using the same
 * solid color without lighting.
 *
 * You have to specify the color using a glColor*() calls before rendering.
 */
class SolidColorRenderModifier : public RenderModifier
{
public:
	SolidColorRenderModifier();
	~SolidColorRenderModifier();

	// Implementation
	u32 BeginPass(uint pass);
	bool EndPass(uint pass);
	void PrepareTexture(uint pass, CTexture* texture);
	void PrepareModel(uint pass, CModel* model);
};

#endif // RENDERMODIFIERS_H
