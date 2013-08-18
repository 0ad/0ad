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
#include "ps/Overlay.h"
#include "renderer/VertexBufferManager.h"

class CSimulation2;

struct SWavesVertex {
	// vertex position
	CVector3D m_Position;
	u8 m_UV[2];
};
cassert(sizeof(SWavesVertex) == 16);



/**
 * Class WaterManager: Maintain water settings and textures.
 *
 * This could be extended to provide more advanced water rendering effects
 * (refractive/reflective water) in the future.
 */

class WaterManager
{
public:
	CTexturePtr m_WaterTexture[60];
	CTexturePtr m_NormalMap[60];
	CTexturePtr m_Foam;
	CTexturePtr m_Wave;
	float* m_WaveX;
	float* m_WaveZ;
	float* m_DistanceToShore;
	float* m_FoamFactor;
	
	size_t m_MapSize;
	ssize_t m_TexSize;

	GLuint m_depthTT;
	GLuint m_waveTT;

	// used to know what to update when updating parts of the terrain only.
	i32 m_updatei0;
	i32 m_updatej0;
	i32 m_updatei1;
	i32 m_updatej1;
	
	int m_WaterCurrentTex;
	CColor m_WaterColor;
	bool m_RenderWater;

	// Those variables register the current quality level. If there is a change, I have to recompile the shader.
	bool m_WaterNormal;
	bool m_WaterRealDepth;
	bool m_WaterFoam;
	bool m_WaterCoastalWaves;
	bool m_WaterRefraction;
	bool m_WaterReflection;
	bool m_WaterShadows;
	
	bool m_NeedsReloading;
	// requires also recreating the super fancy information.
	bool m_NeedInfoUpdate;

	bool m_WaterScroll;
	float m_WaterHeight;
	float m_WaterMaxAlpha;
	float m_WaterFullDepth;
	float m_WaterAlphaOffset;

	float m_SWaterSpeed;
	float m_TWaterSpeed;
	float m_SWaterTrans;
	float m_TWaterTrans;
	float m_SWaterScrollCounter;
	float m_TWaterScrollCounter;
	double m_WaterTexTimer;

	// Reflection and refraction textures for fancy water
	GLuint m_ReflectionTexture;
	GLuint m_RefractionTexture;
	size_t m_ReflectionTextureSize;
	size_t m_RefractionTextureSize;

	// Model-view-projection matrices for reflected & refracted cameras
	// (used to let the vertex shader do projective texturing)
	CMatrix3D m_ReflectionMatrix;
	CMatrix3D m_RefractionMatrix;

	// Shader parameters for fancy water
	CColor m_WaterTint;
	float m_RepeatPeriod;
	float m_Shininess;
	float m_SpecularStrength;
	float m_Waviness;
	float m_Murkiness;
	CColor m_ReflectionTint;
	float m_ReflectionTintStrength;
	
	// Waves
	// see the struct above.
	CVertexBuffer::VBChunk* m_VBWaves;
	CVertexBuffer::VBChunk* m_VBWavesIndices;

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
	 * CreateSuperfancyInfo: creates textures and wave vertices for superfancy water
	 */
	void CreateSuperfancyInfo(CSimulation2* simulation);

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
};


#endif // INCLUDED_WATERMANAGER
