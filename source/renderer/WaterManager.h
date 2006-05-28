/**
 * =========================================================================
 * File        : WaterManager.h
 * Project     : Pyrogenesis
 * Description : Water settings (speed, height) and texture management
 *
 * @author Nicolai Hähnle <nicolai@wildfiregames.com>
 * =========================================================================
 */

#ifndef WATERMANAGER_H
#define WATERMANAGER_H

#include "ps/Overlay.h"
#include "maths/Matrix3D.h"

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
	GLuint m_ReflectionTexture;
	GLuint m_RefractionTexture;
	uint m_ReflectionTextureSize;
	uint m_RefractionTextureSize;
	CMatrix3D m_ReflectionMatrix;	// model-view-projection matrix for reflected camera
	CMatrix3D m_RefractionMatrix;	// model-view-projection matrix for refraction camera

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
	uint cur_loading_water_tex;
	uint cur_loading_normal_map;
};


#endif // WATERMANAGER_H
