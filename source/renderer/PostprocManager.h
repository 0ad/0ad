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

#ifndef INCLUDED_POSTPROCMANAGER
#define INCLUDED_POSTPROCMANAGER


#include "graphics/ShaderTechnique.h"

class CPostprocManager
{
private:
	
	// Two framebuffers, that we flip between at each shader pass.
	GLuint m_PingFbo, m_PongFbo;
	
	// Unique colour textures for the framebuffers.
	GLuint m_ColourTex1, m_ColourTex2;
	
	// The framebuffers share a depth/stencil texture.
	GLuint m_DepthTex;
	
	// A framebuffer and textures x2 for each blur level we render. 
	GLuint m_BloomFbo, m_BlurTex2a, m_BlurTex2b, m_BlurTex4a, m_BlurTex4b, m_BlurTex8a, m_BlurTex8b;
	
	// Indicates which of the ping-pong buffers is used for reading and which for drawing.
	bool m_WhichBuffer;
	
	// The name and shader technique we are using. "default" name means no technique is used
	// (i.e. while we do allocate the buffers, no effects are rendered).
	CStrW m_PostProcEffect;
	CShaderTechniquePtr m_PostProcTech;
	
	// The current screen dimensions in pixels.
	int m_Width, m_Height;
	
	// Is the postproc manager initialised? Buffers created? Default effect loaded?
	bool m_IsInitialised;
	
	// Creates blur textures at various scales, for bloom, DOF, etc.
	void ApplyBlur();
	
	// High quality GPU image scaling to half size. outTex must have exactly half the size
	// of inTex. inWidth and inHeight are the dimensions of inTex in texels.
	void ApplyBlurDownscale2x(GLuint inTex, GLuint outTex, int inWidth, int inHeight);
	
	// GPU-based Gaussian blur in two passes. inOutTex contains the input image and will be filled 
	// with the blurred image. tempTex must have the same size as inOutTex. 
	// inWidth and inHeight are the dimensions of the images in texels.
	void ApplyBlurGauss(GLuint inOutTex, GLuint tempTex, int inWidth, int inHeight);
	
	// Applies a pass of a given effect to the entire current framebuffer. The shader is 
	// provided with a number of general-purpose variables, including the rendered screen so far,
	// the depth buffer, a number of blur textures, the screen size, the zNear/zFar planes and
	// some other parameters used by the optional bloom/HDR pass.
	void ApplyEffect(CShaderTechniquePtr &shaderTech1, int pass);
	
public:
	CPostprocManager();
	~CPostprocManager();
	
	// Create all buffers/textures in GPU memory and set default effect.
	void Initialize();
	
	// Delete all allocated buffers/textures from GPU memory.
	void Cleanup();
	
	// Delete existing buffers/textures and create them again, using a new screen size if needed.
	// (the textures are also attached to the framebuffers)
	void RecreateBuffers();
	
	// Loads a new postproc effect. "default" effect does NOT load anything.
	void LoadEffect(CStrW &name);
	
	// Returns a list of xml files found in shaders/effects/postproc.
	std::vector<CStrW> GetPostEffects() const;
	
	// Returns the name of the current effect.
	inline const CStrW& GetPostEffect() const 
	{
		return m_PostProcEffect;
	}
	
	// Matching setter that calls LoadEffect.
	void SetPostEffect(CStrW name);

	// Clears the two colour buffers and depth buffer, and redirects all rendering
	// to our textures instead of directly to the system framebuffer. 
	void CaptureRenderOutput();
	
	// First renders blur textures, then calls ApplyEffect for each effect pass, 
	// ping-ponging the buffers at each step.
	void ApplyPostproc();
	
	// Blits the final postprocessed texture to the system framebuffer. The system framebuffer
	// is selected as the output buffer. Should be called before silhouette rendering.
	void ReleaseRenderOutput();
};


#endif //INCLUDED_POSTPROCMANAGER
