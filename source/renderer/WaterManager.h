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
 * Water settings (speed, height) and texture management
 */

#ifndef INCLUDED_WATERMANAGER
#define INCLUDED_WATERMANAGER

#include "graphics/Texture.h"
#include "lib/ogl.h"
#include "maths/Matrix3D.h"
#include "maths/Vector2D.h"
#include "ps/Overlay.h"
#include "renderer/VertexBufferManager.h"

class CSimulation2;
class CFrustum;

struct CoastalPoint;
struct WaveObject;

/**
 * Class WaterManager: Maintain rendering-related water settings and textures
 * Anything that affects gameplay should go in CcmpWaterManager.cpp and passed to this (possibly as copy).
 */

class WaterManager
{
public:
	CTexturePtr m_WaterTexture[60];
	CTexturePtr m_NormalMap[60];

	float* m_WindStrength;	// How strong the waves are at point X. % of waviness.
	float* m_DistanceHeightmap; // How far from the shore a point is. Manhattan
	CVector3D* m_BlurredNormalMap;	// Cache a slightly blurred map of the normals of the terrain.

	std::vector< std::deque<CoastalPoint> > CoastalPointsChains;
	
	// Waves vertex buffers
	std::vector< WaveObject* > m_ShoreWaves;	// TODO: once we get C++11, remove pointer
	// Waves indices buffer. Only one since All Wave Objects have the same.
	CVertexBuffer::VBChunk* m_ShoreWaves_VBIndices;
	
	size_t m_MapSize;
	ssize_t m_TexSize;

	CTexturePtr m_WaveTex;
	CTexturePtr m_FoamTex;

	GLuint m_depthTT;
	GLuint m_FancyTextureNormal;
	GLuint m_FancyTextureOther;
	GLuint m_FancyTextureDepth;
	GLuint m_ReflFboDepthTexture;
	GLuint m_RefrFboDepthTexture;

	// used to know what to update when updating parts of the terrain only.
	u32 m_updatei0;
	u32 m_updatej0;
	u32 m_updatei1;
	u32 m_updatej1;
	
	int m_WaterCurrentTex;
	bool m_RenderWater;

	// Force the use of the fixed function for rendering.
	bool m_WaterUgly;
	// Those variables register the current quality level. If there is a change, I have to recompile the shader.
	// Use real depth or use the fake precomputed one.
	bool m_WaterRealDepth;
	// Use fancy shore effects and show trails behind ships
	bool m_WaterFancyEffects;
	// Use refractions instead of simply making the water more or less transparent.
	bool m_WaterRefraction;
	// Use complete reflections instead of showing merely the sky.
	bool m_WaterReflection;
	// Show shadows on the water.
	bool m_WaterShadows;
	
	bool m_NeedsReloading;
	// requires also recreating the super fancy information.
	bool m_NeedInfoUpdate;

	float m_WaterHeight;

	double m_WaterTexTimer;
	float m_RepeatPeriod;

	// Reflection and refraction textures for fancy water
	GLuint m_ReflectionTexture;
	GLuint m_RefractionTexture;
	size_t m_ReflectionTextureSize;
	size_t m_RefractionTextureSize;

	// framebuffer objects
	GLuint m_RefractionFbo;
	GLuint m_ReflectionFbo;
	GLuint m_FancyEffectsFBO;

	// Model-view-projection matrices for reflected & refracted cameras
	// (used to let the vertex shader do projective texturing)
	CMatrix3D m_ReflectionMatrix;
	CMatrix3D m_RefractionMatrix;

	// Water parameters
	std::wstring m_WaterType;		// Which texture to use.
	CColor m_WaterColor;	// Color of the water without refractions. This is what you're seeing when the water's deep or murkiness high.
	CColor m_WaterTint;		// Tint of refraction in the water.
	float m_Waviness;		// How big the waves are.
	float m_Murkiness;		// How murky the water is.
	float m_WindAngle;	// In which direction the water waves go.
	
public:
	WaterManager();
	~WaterManager();

	/**
	 * LoadWaterTextures: Load water textures from within the
	 * progressive load framework.
	 *
	 * @return 0 if loading has completed, a value from 1 to 100 (in percent of completion)
	 * if more textures need to be loaded and a negative error value on failure.
	 */
	int LoadWaterTextures();
	
	/**
	 * UnloadWaterTextures: Free any loaded water textures and reset the internal state
	 * so that another call to LoadWaterTextures will begin progressive loading.
	 */
	void UnloadWaterTextures();

	/**
	 * RecomputeWindStrength: calculates the intensity of waves
	 */
	void RecomputeWindStrength();

	/**
	 * RecomputeDistanceHeightmap: recalculates (or calculates) the distance heightmap.
	 */
	void RecomputeDistanceHeightmap();

	/**
	 * RecomputeBlurredNormalMap: calculates the blurred normal map of the terrain. Slow.
	 */
	void RecomputeBlurredNormalMap();
	
	/**
	 * CreateWaveMeshes: Creates the waves objects (and meshes).
	 */
	void CreateWaveMeshes();

	/**
	 * Updates the map size. Will trigger a complete recalculation of fancy water information the next turn.
	 */
	void SetMapSize(size_t size);

	/**
	 * Updates the settings to the one from the renderer, and sets m_NeedsReloading.
	 */
	void UpdateQuality();
	
	/**
	 * Returns true if fancy water shaders will be used (i.e. the hardware is capable
	 * and it hasn't been configured off)
	 */
	bool WillRenderFancyWater();
	
	void RenderWaves(const CFrustum& frustrum);
};


#endif // INCLUDED_WATERMANAGER
