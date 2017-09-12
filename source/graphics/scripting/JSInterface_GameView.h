/* Copyright (C) 2017 Wildfire Games.
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


#ifndef INCLUDED_JSINTERFACE_GAMEVIEW
#define INCLUDED_JSINTERFACE_GAMEVIEW

#include "scriptinterface/ScriptInterface.h"
#include "maths/FixedVector3D.h"
#include "simulation2/helpers/Position.h"
#include "simulation2/system/Entity.h"

#define DECLARE_BOOLEAN_SCRIPT_SETTING(NAME) \
	bool Get##NAME##Enabled(ScriptInterface::CxPrivate* pCxPrivate); \
	void Set##NAME##Enabled(ScriptInterface::CxPrivate* pCxPrivate, bool Enabled);

namespace JSI_GameView
{
	void RegisterScriptFunctions(const ScriptInterface& ScriptInterface);
	void RegisterScriptFunctions_Settings(const ScriptInterface& scriptInterface);

	DECLARE_BOOLEAN_SCRIPT_SETTING(Culling);
	DECLARE_BOOLEAN_SCRIPT_SETTING(LockCullCamera);
	DECLARE_BOOLEAN_SCRIPT_SETTING(ConstrainCamera);

	float CameraGetX(ScriptInterface::CxPrivate* pCxPrivate);
	float CameraGetZ(ScriptInterface::CxPrivate* pCxPrivate);
	void CameraMoveTo(ScriptInterface::CxPrivate* pCxPrivate, entity_pos_t x, entity_pos_t z);
	void SetCameraTarget(ScriptInterface::CxPrivate* pCxPrivate, float x, float y, float z);
	void SetCameraData(ScriptInterface::CxPrivate* pCxPrivate, entity_pos_t x, entity_pos_t y, entity_pos_t z, entity_pos_t rotx, entity_pos_t roty, entity_pos_t zoom);
	void CameraFollow(ScriptInterface::CxPrivate* pCxPrivate, entity_id_t entityid);
	void CameraFollowFPS(ScriptInterface::CxPrivate* pCxPrivate, entity_id_t entityid);
	entity_id_t GetFollowedEntity(ScriptInterface::CxPrivate* pCxPrivate);
	CFixedVector3D GetTerrainAtScreenPoint(ScriptInterface::CxPrivate* pCxPrivate, int x, int y);
}

#undef DECLARE_BOOLEAN_SCRIPT_SETTING

#endif
