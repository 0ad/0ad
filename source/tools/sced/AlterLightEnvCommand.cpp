#include "precompiled.h"

#include "AlterLightEnvCommand.h"
#include "UnitManager.h"
#include "ObjectManager.h"
#include "Model.h"
#include "Unit.h"
#include "Terrain.h"

extern CTerrain g_Terrain;
extern CLightEnv g_LightEnv;


CAlterLightEnvCommand::CAlterLightEnvCommand(const CLightEnv& env) : m_LightEnv(env)
{
}


CAlterLightEnvCommand::~CAlterLightEnvCommand()
{
}

void CAlterLightEnvCommand::Execute()
{
	// save current lighting environment
	m_SavedLightEnv=g_LightEnv;
	// apply this commands lighting environment onto the global lighting environment
	ApplyData(m_LightEnv);
}


void CAlterLightEnvCommand::ApplyData(const CLightEnv& env)
{
	// copy given lighting environment to global environment
	g_LightEnv=env;

	// dirty the vertices on all patches
	u32 patchesPerSide=g_Terrain.GetPatchesPerSide();
	u32 i,j;
	for (j=0;j<patchesPerSide;j++) {
		for (i=0;i<patchesPerSide;i++) {
			CPatch* patch=g_Terrain.GetPatch(i,j);
			patch->SetDirty(RENDERDATA_UPDATE_VERTICES);
		}
	}

	// dirty all models
	const std::vector<CUnit*>& units=g_UnitMan.GetUnits();
	for (i=0;i<units.size();++i) {
		CUnit* unit=units[i];
		unit->GetModel()->SetDirty(RENDERDATA_UPDATE_VERTICES);
	}

	CObjectEntry* selobject=g_ObjMan.GetSelectedObject();
	if (selobject && selobject->m_Model) {
		selobject->m_Model->SetDirty(RENDERDATA_UPDATE_VERTICES);
	}
}


void CAlterLightEnvCommand::Undo()
{
	ApplyData(m_SavedLightEnv);
}

void CAlterLightEnvCommand::Redo()
{
	ApplyData(m_LightEnv);
}
