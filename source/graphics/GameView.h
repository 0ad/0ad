#ifndef _GameView_H
#define _GameView_H

#include "Camera.h"
#include "Vector3D.h"

#include "scripting/ScriptableObject.h"

// note: cannot forward declare "struct SDL_Event" - that triggers an
// internal error in VC7.1 at msc1.cpp(2701).
// we include the full header instead. *sigh*
#include "sdl.h"

class CGame;
class CGameAttributes;
class CWorld;
class CTerrain;
class CUnitManager;
class CProjectileManager;
class CModel;

class CGameView: public CJSObject<CGameView>
{
	CGame *m_pGame;
	CWorld *m_pWorld;
	CCamera m_Camera;
	
	////////////////////////////////////////
	// Settings
	float m_ViewScrollSpeed;
	float m_ViewRotateSensitivity;
	float m_ViewRotateSensitivityKeyboard;
	float m_ViewRotateAboutTargetSensitivity;
	float m_ViewRotateAboutTargetSensitivityKeyboard;
	float m_ViewDragSensitivity;
	float m_ViewZoomSensitivityWheel;
	float m_ViewZoomSensitivity;
	float m_ViewZoomSmoothness; // 0.0 = instantaneous zooming, 1.0 = so slow it never moves
	float m_ViewSnapSmoothness; // Just the same.

	////////////////////////////////////////
	// Camera Controls State
	CVector3D m_CameraDelta;
	CVector3D m_CameraPivot;
	//float m_CameraZoom;
	std::vector<CVector3D> m_CameraTargets;

	// RenderTerrain: iterate through all terrain patches and submit all patches
	// in viewing frustum to the renderer
	void RenderTerrain(CTerrain *pTerrain);
	
	// RenderModels: iterate through model list and submit all models in viewing
	// frustum to the Renderer
	void RenderModels(CUnitManager *pUnitMan, CProjectileManager *pProjectileManager);
	
	// SubmitModelRecursive: recurse down given model, submitting it and all its
	// descendents to the renderer
	void SubmitModelRecursive(CModel *pModel);
	
	// InitResources(): Load all graphics resources (textures, actor objects and
	// alpha maps) required by the game
	void InitResources();

	// UnloadResources(): Unload all graphics resources loaded by InitResources
	void UnloadResources();
	
	// JS Interface
	bool JSI_StartCustomSelection(JSContext *cx, uintN argc, jsval *argv);
	bool JSI_EndCustomSelection(JSContext *cx, uintN argc, jsval *argv);

	static void ScriptingInit();

public:
	CGameView(CGame *pGame);
	~CGameView();
	
	void RegisterInit(CGameAttributes *pAttribs);
	int Initialize(CGameAttributes *pGameAttributes);

	// Update: Update all the view information (i.e. rotate camera, scroll, 
	// whatever). This will *not* change any World information - only the
	// *presentation*
	void Update(float DeltaTime);
	
	// Render: Render the World
	void Render();
	
	// RenderNoCull: render absolutely everything to a blank frame to force
	// renderer to load required assets
	void RenderNoCull();
	
	// Camera Control Functions (used by input handler)
	void ResetCamera();
	void ResetCameraOrientation();
	void RotateAboutTarget();
	
	void PushCameraTarget( const CVector3D& target );
	void SetCameraTarget( const CVector3D& target );
	void PopCameraTarget();
	
	inline CCamera *GetCamera()
	{	return &m_Camera; }
};

extern int game_view_handler(const SDL_Event* ev);

#endif
