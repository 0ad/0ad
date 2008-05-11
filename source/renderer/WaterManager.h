/**
 * =========================================================================
 * File        : WaterManager.h
 * Project     : Pyrogenesis
 * Description : Water settings (speed, height) and texture management
 * =========================================================================
 */

#ifndef INCLUDED_WATERMANAGER
#define INCLUDED_WATERMANAGER

#include "ps/Overlay.h"
#include "maths/Matrix3D.h"
#include "lib/ogl.h"

/**
 * Class WaterManager: Maintain water settings and textures.
 *
 * This could be extended to provide more advanced water rendering effects
 * (refractive/reflective water) in the future.
 */
class WaterManager
{
public:
	Handle m_WaterTexture[60];
	Handle m_NormalMap[60];

	int m_WaterCurrentTex;
	CColor m_WaterColor;
	bool m_RenderWater;
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

private:
	/// State of progressive loading (in # of loaded textures)
	size_t cur_loading_water_tex;
	size_t cur_loading_normal_map;
};


#endif // INCLUDED_WATERMANAGER
