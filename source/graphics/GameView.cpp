#include "precompiled.h"

#include "Terrain.h"
#include "Renderer.h"
#include "GameView.h"
#include "Game.h"
#include "Camera.h"

extern CCamera g_Camera;

CGameView::CGameView(CGame *pGame):
	m_pGame(pGame),
	m_pWorld(pGame->GetWorld()),
	m_pCamera(&g_Camera)
{}

void CGameView::Initialize(CGameAttributes *pAttribs)
{}

void CGameView::Render()
{
	MICROLOG(L"render terrain");
	RenderTerrain(m_pWorld->GetTerrain());
	MICROLOG(L"render models");
	RenderModels(m_pWorld->GetUnitManager());
	MICROLOG(L"flush frame");
}

void CGameView::RenderTerrain(CTerrain *pTerrain)
{
	CFrustum frustum=m_pCamera->GetFrustum();
	u32 patchesPerSide=pTerrain->GetPatchesPerSide();
	for (uint j=0; j<patchesPerSide; j++) {
		for (uint i=0; i<patchesPerSide; i++) {
			CPatch* patch=pTerrain->GetPatch(i,j);
			if (frustum.IsBoxVisible (CVector3D(0,0,0),patch->GetBounds())) {
				g_Renderer.Submit(patch);
			}
		}
	}
}

void CGameView::RenderModels(CUnitManager *pUnitMan)
{
	CFrustum frustum=m_pCamera->GetFrustum();

	const std::vector<CUnit*>& units=pUnitMan->GetUnits();
	for (uint i=0;i<units.size();++i) {
		if (frustum.IsBoxVisible(CVector3D(0,0,0),units[i]->GetModel()->GetBounds())) {
			SubmitModelRecursive(units[i]->GetModel());
		}
	}
}

void CGameView::SubmitModelRecursive(CModel* model)
{
	g_Renderer.Submit(model);

	const std::vector<CModel::Prop>& props=model->GetProps();
	for (uint i=0;i<props.size();i++) {
		SubmitModelRecursive(props[i].m_Model);
	}
}

void CGameView::RenderNoCull()
{
	CUnitManager *pUnitMan=m_pWorld->GetUnitManager();
	CTerrain *pTerrain=m_pWorld->GetTerrain();

	uint i,j;
	const std::vector<CUnit*>& units=pUnitMan->GetUnits();
	for (i=0;i<units.size();++i) {
		SubmitModelRecursive(units[i]->GetModel());
	}

	u32 patchesPerSide=pTerrain->GetPatchesPerSide();
	for (j=0; j<patchesPerSide; j++) {
		for (i=0; i<patchesPerSide; i++) {
			CPatch* patch=pTerrain->GetPatch(i,j);
			g_Renderer.Submit(patch);
		}
	}
}
