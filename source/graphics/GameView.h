#ifndef _GameView_H
#define _GameView_H

class CGame;
class CGameAttributes;
class CWorld;
class CTerrain;
class CUnitManager;
class CModel;
class CCamera;

extern CCamera g_Camera;

class CGameView
{
	CGame *m_pGame;
	CWorld *m_pWorld;
	CCamera *m_pCamera;
	
	// RenderTerrain: iterate through all terrain patches and submit all patches
	// in viewing frustum to the renderer
	void RenderTerrain(CTerrain *pTerrain);
	
	// RenderModels: iterate through model list and submit all models in viewing
	// frustum to the Renderer
	void RenderModels(CUnitManager *pUnitMan);
	
	// SubmitModelRecursive: recurse down given model, submitting it and all its
	// descendents to the renderer
	void SubmitModelRecursive(CModel *pModel);
public:
	CGameView(CGame *pGame);
	
	void Initialize(CGameAttributes *pGameAttributes);
	
	/*
		Render the World
	*/
	void Render();
	
	// RenderNoCull: render absolutely everything to a blank frame to force
	// renderer to load required assets
	void RenderNoCull();
	
	inline CCamera *GetCamera()
	{	return m_pCamera; }
};

#endif
