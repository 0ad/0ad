#ifndef _EDITORDATA_H
#define _EDITORDATA_H

#include "MiniMap.h"
#include "InfoBox.h"
#include "PaintTextureTool.h"
#include "NaviCam.h"
#include "CStr.h"
#include "Terrain.h"
#include "Renderer.h"
#include "Camera.h"
#include "LightEnv.h"

class CEditorData
{
public:
	enum EditMode { SCENARIO_EDIT, UNIT_EDIT, BRUSH_EDIT, TEST_MODE };

public:
	CEditorData();

	void SetMode(EditMode mode);
	EditMode GetMode() const { return m_Mode; }

	// initialise editor at given width, height and bpp
	bool Init();
	void Terminate();

	void OnCameraChanged();
	void OnDraw();
	void OnScreenShot(const char* filename);

	CInfoBox& GetInfoBox() { return m_InfoBox; }

	// terrain plane
	CPlane m_TerrainPlane;

	// set the scenario name
	void SetScenarioName(const char* name) { m_ScenarioName=name; }
	// get the scenario name
	const char* GetScenarioName() const { return (const char*) m_ScenarioName; }

	bool LoadTerrain(const char* filename);

	// update time dependent data in the world to account for changes over 
	// the given time (in ms)
	void UpdateWorld(float time);

	void RenderNoCull();

private:
	bool InitScene();
	void LoadAlphaMaps();
	void InitResources();
	void InitCamera();
	void RenderTerrain();
	void RenderModels();
	void RenderWorld();
	void RenderObEdGrid();

	void InitSingletons();

	void StartTestMode();
	void StopTestMode();

	// current editing mode
	EditMode m_Mode;
	// camera to use in object viewing mode
	CCamera m_ObjectCamera;
	// transform of model in object viewing mode
	CMatrix3D m_ModelMatrix;
	// information panel
	CInfoBox m_InfoBox;
	// the (short) name of this scenario used in title bar and as default save name
	CStr m_ScenarioName;
};

extern CEditorData	g_EditorData;
extern CTerrain	g_Terrain;
extern CLightEnv g_LightEnv;

#endif
