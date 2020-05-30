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

#ifndef INCLUDED_JSINTERFACE_GAMEVIEW
#define INCLUDED_JSINTERFACE_GAMEVIEW

#include "maths/FixedVector3D.h"
#include "scriptinterface/ScriptInterface.h"
#include "simulation2/helpers/Position.h"
#include "simulation2/system/Entity.h"

#define DECLARE_BOOLEAN_SCRIPT_SETTING(NAME) \
	bool Get##NAME##Enabled(ScriptInterface::RealmPrivate* pRealmPrivate); \
	void Set##NAME##Enabled(ScriptInterface::RealmPrivate* pRealmPrivate, bool Enabled);

namespace JSI_GameView
{
	void RegisterScriptFunctions(const ScriptInterface& ScriptInterface);
	void RegisterScriptFunctions_Settings(const ScriptInterface& scriptInterface);

	DECLARE_BOOLEAN_SCRIPT_SETTING(Culling);
	DECLARE_BOOLEAN_SCRIPT_SETTING(LockCullCamera);
	DECLARE_BOOLEAN_SCRIPT_SETTING(ConstrainCamera);

	JS::Value GetCameraPivot(ScriptInterface::RealmPrivate* pRealmPrivate);
	void CameraMoveTo(ScriptInterface::RealmPrivate* pRealmPrivate, entity_pos_t x, entity_pos_t z);
	void SetCameraTarget(ScriptInterface::RealmPrivate* pRealmPrivate, float x, float y, float z);
	void SetCameraData(ScriptInterface::RealmPrivate* pRealmPrivate, entity_pos_t x, entity_pos_t y, entity_pos_t z, entity_pos_t rotx, entity_pos_t roty, entity_pos_t zoom);
	void CameraFollow(ScriptInterface::RealmPrivate* pRealmPrivate, entity_id_t entityid);
	void CameraFollowFPS(ScriptInterface::RealmPrivate* pRealmPrivate, entity_id_t entityid);
	entity_id_t GetFollowedEntity(ScriptInterface::RealmPrivate* pRealmPrivate);
	CFixedVector3D GetTerrainAtScreenPoint(ScriptInterface::RealmPrivate* pRealmPrivate, int x, int y);
}

#undef DECLARE_BOOLEAN_SCRIPT_SETTING

#endif // INCLUDED_JSINTERFACE_GAMEVIEW
