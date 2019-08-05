/* Copyright (C) 2019 Wildfire Games.
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

#include "precompiled.h"

#include "JSInterface_GameView.h"

#include "graphics/Camera.h"
#include "graphics/GameView.h"
#include "graphics/Terrain.h"
#include "ps/Game.h"
#include "ps/World.h"
#include "ps/CLogger.h"
#include "ps/Profile.h"
#include "scriptinterface/ScriptInterface.h"

#define IMPLEMENT_BOOLEAN_SCRIPT_SETTING(NAME) \
bool JSI_GameView::Get##NAME##Enabled(ScriptInterface::CxPrivate* UNUSED(pCxPrivate)) \
{ \
	if (!g_Game || !g_Game->GetView()) \
	{ \
		LOGERROR("Trying to get a setting from GameView when it's not initialized!"); \
		return false; \
	} \
	return g_Game->GetView()->Get##NAME##Enabled(); \
} \
\
void JSI_GameView::Set##NAME##Enabled(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), bool Enabled) \
{ \
	if (!g_Game || !g_Game->GetView()) \
	{ \
		LOGERROR("Trying to set a setting of GameView when it's not initialized!"); \
		return; \
	} \
	g_Game->GetView()->Set##NAME##Enabled(Enabled); \
}

IMPLEMENT_BOOLEAN_SCRIPT_SETTING(Culling);
IMPLEMENT_BOOLEAN_SCRIPT_SETTING(LockCullCamera);
IMPLEMENT_BOOLEAN_SCRIPT_SETTING(ConstrainCamera);

#undef IMPLEMENT_BOOLEAN_SCRIPT_SETTING


#define REGISTER_BOOLEAN_SCRIPT_SETTING(NAME) \
scriptInterface.RegisterFunction<bool, &JSI_GameView::Get##NAME##Enabled>("GameView_Get" #NAME "Enabled"); \
scriptInterface.RegisterFunction<void, bool, &JSI_GameView::Set##NAME##Enabled>("GameView_Set" #NAME "Enabled");

void JSI_GameView::RegisterScriptFunctions_Settings(const ScriptInterface& scriptInterface)
{
	REGISTER_BOOLEAN_SCRIPT_SETTING(Culling);
	REGISTER_BOOLEAN_SCRIPT_SETTING(LockCullCamera);
	REGISTER_BOOLEAN_SCRIPT_SETTING(ConstrainCamera);
}

#undef REGISTER_BOOLEAN_SCRIPT_SETTING

/**
 * Get the current X coordinate of the camera.
 */
float JSI_GameView::CameraGetX(ScriptInterface::CxPrivate* UNUSED(pCxPrivate))
{
	if (!g_Game || !g_Game->GetView())
		return -1;

	return g_Game->GetView()->GetCameraPivot().X;
}

/**
 * Get the current Z coordinate of the camera.
 */
float JSI_GameView::CameraGetZ(ScriptInterface::CxPrivate* UNUSED(pCxPrivate))
{
	if (!g_Game || !g_Game->GetView())
		return -1;

	return g_Game->GetView()->GetCameraPivot().Z;
}

/**
 * Move camera to a 2D location.
 */
void JSI_GameView::CameraMoveTo(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), entity_pos_t x, entity_pos_t z)
{
	if (!g_Game || !g_Game->GetWorld() || !g_Game->GetView() || !g_Game->GetWorld()->GetTerrain())
		return;

	CTerrain* terrain = g_Game->GetWorld()->GetTerrain();

	CVector3D target;
	target.X = x.ToFloat();
	target.Z = z.ToFloat();
	target.Y = terrain->GetExactGroundLevel(target.X, target.Z);

	g_Game->GetView()->MoveCameraTarget(target);
}

/**
 * Set the camera to look at the given location.
 */
void JSI_GameView::SetCameraTarget(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), float x, float y, float z)
{
	g_Game->GetView()->ResetCameraTarget(CVector3D(x, y, z));
}

/**
 * Set the data (position, orientation and zoom) of the camera.
 */
void JSI_GameView::SetCameraData(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), entity_pos_t x, entity_pos_t y, entity_pos_t z, entity_pos_t rotx, entity_pos_t roty, entity_pos_t zoom)
{
	if (!g_Game || !g_Game->GetWorld() || !g_Game->GetView() || !g_Game->GetWorld()->GetTerrain())
		return;

	CVector3D Pos = CVector3D(x.ToFloat(), y.ToFloat(), z.ToFloat());
	float RotX = rotx.ToFloat();
	float RotY = roty.ToFloat();
	float Zoom = zoom.ToFloat();

	g_Game->GetView()->SetCamera(Pos, RotX, RotY, Zoom);
}

/**
 * Start / stop camera following mode.
 * @param entityid unit id to follow. If zero, stop following mode
 */
void JSI_GameView::CameraFollow(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), entity_id_t entityid)
{
	if (!g_Game || !g_Game->GetView())
		return;

	g_Game->GetView()->CameraFollow(entityid, false);
}

/**
 * Start / stop first-person camera following mode.
 * @param entityid unit id to follow. If zero, stop following mode.
 */
void JSI_GameView::CameraFollowFPS(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), entity_id_t entityid)
{
	if (!g_Game || !g_Game->GetView())
		return;

	g_Game->GetView()->CameraFollow(entityid, true);
}

entity_id_t JSI_GameView::GetFollowedEntity(ScriptInterface::CxPrivate* UNUSED(pCxPrivate))
{
	if (!g_Game || !g_Game->GetView())
		return INVALID_ENTITY;

	return g_Game->GetView()->GetFollowedEntity();
}

CFixedVector3D JSI_GameView::GetTerrainAtScreenPoint(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), int x, int y)
{
	CVector3D pos = g_Game->GetView()->GetCamera()->GetWorldCoordinates(x, y, true);
	return CFixedVector3D(fixed::FromFloat(pos.X), fixed::FromFloat(pos.Y), fixed::FromFloat(pos.Z));
}

void JSI_GameView::RegisterScriptFunctions(const ScriptInterface& scriptInterface)
{
	RegisterScriptFunctions_Settings(scriptInterface);

	scriptInterface.RegisterFunction<float, &CameraGetX>("CameraGetX");
	scriptInterface.RegisterFunction<float, &CameraGetZ>("CameraGetZ");
	scriptInterface.RegisterFunction<void, entity_pos_t, entity_pos_t, &CameraMoveTo>("CameraMoveTo");
	scriptInterface.RegisterFunction<void, float, float, float, &SetCameraTarget>("SetCameraTarget");
	scriptInterface.RegisterFunction<void, entity_pos_t, entity_pos_t, entity_pos_t, entity_pos_t, entity_pos_t, entity_pos_t, &SetCameraData>("SetCameraData");
	scriptInterface.RegisterFunction<void, entity_id_t, &CameraFollow>("CameraFollow");
	scriptInterface.RegisterFunction<void, entity_id_t, &CameraFollowFPS>("CameraFollowFPS");
	scriptInterface.RegisterFunction<entity_id_t, &GetFollowedEntity>("GetFollowedEntity");
	scriptInterface.RegisterFunction<CFixedVector3D, int, int, &GetTerrainAtScreenPoint>("GetTerrainAtScreenPoint");
}
