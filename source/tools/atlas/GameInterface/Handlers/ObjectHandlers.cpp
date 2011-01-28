/* Copyright (C) 2010 Wildfire Games.
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

#include <cfloat>

#include "MessageHandler.h"
#include "../CommandProc.h"
#include "../SimState.h"
#include "../View.h"

#include "graphics/GameView.h"
#include "graphics/Model.h"
#include "graphics/ObjectBase.h"
#include "graphics/ObjectEntry.h"
#include "graphics/ObjectManager.h"
#include "graphics/Terrain.h"
#include "graphics/Unit.h"
#include "graphics/UnitManager.h"
#include "lib/ogl.h"
#include "maths/MathUtil.h"
#include "maths/Matrix3D.h"
#include "ps/CLogger.h"
#include "ps/Game.h"
#include "ps/World.h"
#include "renderer/Renderer.h"
#include "renderer/WaterManager.h"
#include "simulation2/Simulation2.h"
#include "simulation2/components/ICmpOwnership.h"
#include "simulation2/components/ICmpPosition.h"
#include "simulation2/components/ICmpSelectable.h"
#include "simulation2/components/ICmpTemplateManager.h"


namespace AtlasMessage {

namespace
{
	bool SortObjectsList(const sObjectsListItem& a, const sObjectsListItem& b)
	{
		return wcscmp(a.name.c_str(), b.name.c_str()) < 0;
	}

	bool IsFloating(const CUnit* unit)
	{
		if (! unit)
			return false;

		CmpPtr<ICmpPosition> cmpPosition(*g_Game->GetSimulation2(), unit->GetID());
		if (cmpPosition.null())
			return false;
		return cmpPosition->IsFloating();
	}

	CUnitManager& GetUnitManager()
	{
		return g_Game->GetWorld()->GetUnitManager();
	}
}

QUERYHANDLER(GetObjectsList)
{
	std::vector<sObjectsListItem> objects;

	CmpPtr<ICmpTemplateManager> cmp(*g_Game->GetSimulation2(), SYSTEM_ENTITY);
	if (!cmp.null())
	{
		std::vector<std::string> names = cmp->FindAllTemplates(true);

		for (std::vector<std::string>::iterator it = names.begin(); it != names.end(); ++it)
		{
			std::wstring name(it->begin(), it->end());

			sObjectsListItem e;
			e.id = name;
			if (name.substr(0, 6) == L"actor|")
			{
				e.name = name.substr(6);
				e.type = 1;
			}
			else
			{
				e.name = name;
				e.type = 0;
			}
			objects.push_back(e);
		}
	}

	std::sort(objects.begin(), objects.end(), SortObjectsList);
	msg->objects = objects;
}


static std::vector<ObjectID> g_Selection;

MESSAGEHANDLER(SetSelectionPreview)
{
	for (size_t i = 0; i < g_Selection.size(); ++i)
	{
		CmpPtr<ICmpSelectable> cmpSelectable(*g_Game->GetSimulation2(), g_Selection[i]);
		if (!cmpSelectable.null())
			cmpSelectable->SetSelectionHighlight(CColor(1, 1, 1, 0));
	}

	g_Selection = *msg->ids;

	for (size_t i = 0; i < g_Selection.size(); ++i)
	{
		CmpPtr<ICmpSelectable> cmpSelectable(*g_Game->GetSimulation2(), g_Selection[i]);
		if (!cmpSelectable.null())
			cmpSelectable->SetSelectionHighlight(CColor(1, 1, 1, 1));
	}
}

QUERYHANDLER(GetObjectSettings)
{
	View* view = View::GetView(msg->view);
	CSimulation2* simulation = view->GetSimulation2();

	sObjectSettings settings;
	settings.player = 0;

	CmpPtr<ICmpOwnership> cmpOwner (*simulation, view->GetEntityId(msg->id));
	if (!cmpOwner.null())
	{
		int32_t player = cmpOwner->GetOwner();
		if (player != -1)
			settings.player = player;
	}

	// TODO: selections

/*
	// Get the unit's possible variants and selected variants
	std::vector<std::vector<CStr> > groups = unit->GetObject().m_Base->GetVariantGroups();
	const std::set<CStr>& selections = unit->GetActorSelections();

	// Iterate over variant groups
	std::vector<std::vector<std::wstring> > variantgroups;
	std::set<std::wstring> selections_set;
	variantgroups.reserve(groups.size());
	for (size_t i = 0; i < groups.size(); ++i)
	{
		// Copy variants into output structure

		std::vector<std::wstring> group;
		group.reserve(groups[i].size());
		int choice = -1;

		for (size_t j = 0; j < groups[i].size(); ++j)
		{
			group.push_back(CStrW(groups[i][j]));

			// Find the first string in 'selections' that matches one of this
			// group's variants
			if (choice == -1)
				if (selections.find(groups[i][j]) != selections.end())
					choice = (int)j;
		}

		// Assuming one of the variants was selected (which it really ought
		// to be), remember that one's name
		if (choice != -1)
			selections_set.insert(CStrW(groups[i][choice]));

		variantgroups.push_back(group);
	}

	settings.variantgroups = variantgroups;
	settings.selections = std::vector<std::wstring> (selections_set.begin(), selections_set.end()); // convert set->vector
*/

	msg->settings = settings;
}

BEGIN_COMMAND(SetObjectSettings)
{
	player_id_t m_PlayerOld, m_PlayerNew;
	std::set<CStr> m_SelectionsOld, m_SelectionsNew;

	void Do()
	{
		sObjectSettings settings = msg->settings;

		View* view = View::GetView(msg->view);
		CSimulation2* simulation = view->GetSimulation2();

		CmpPtr<ICmpOwnership> cmpOwner (*simulation, view->GetEntityId(msg->id));
		m_PlayerOld = 0;
		if (!cmpOwner.null())
		{
			int32_t player = cmpOwner->GetOwner();
			if (player != -1)
				m_PlayerOld = player;
		}

		// TODO: selections
//		m_SelectionsOld = unit->GetActorSelections();

		m_PlayerNew = (player_id_t)settings.player;

		std::vector<std::wstring> selections = *settings.selections;
		for (std::vector<std::wstring>::iterator it = selections.begin(); it != selections.end(); ++it)
		{
			m_SelectionsNew.insert(CStr(*it));
		}

		Redo();
	}

	void Redo()
	{
		Set(m_PlayerNew, m_SelectionsNew);
	}

	void Undo()
	{
		Set(m_PlayerOld, m_SelectionsOld);
	}

private:
	void Set(player_id_t player, const std::set<CStr>& UNUSED(selections))
	{
		View* view = View::GetView(msg->view);
		CSimulation2* simulation = view->GetSimulation2();

		CmpPtr<ICmpOwnership> cmpOwner (*simulation, view->GetEntityId(msg->id));
		if (!cmpOwner.null())
			cmpOwner->SetOwner(player);

		// TODO: selections
//		unit->SetActorSelections(selections);
	}
};
END_COMMAND(SetObjectSettings);

//////////////////////////////////////////////////////////////////////////

static CStrW g_PreviewUnitName;
static entity_id_t g_PreviewEntityID = INVALID_ENTITY;

static CVector3D GetUnitPos(const Position& pos, bool floating)
{
	static CVector3D vec;
	vec = pos.GetWorldSpace(vec, floating); // if msg->pos is 'Unchanged', use the previous pos

	// Clamp the position to the edges of the world:

	// Use 'clamp' with a value slightly less than the width, so that converting
	// to integer (rounding towards zero) will put it on the tile inside the edge
	// instead of just outside
	float mapWidth = (g_Game->GetWorld()->GetTerrain()->GetVerticesPerSide()-1)*CELL_SIZE;
	float delta = 1e-6f; // fraction of map width - must be > FLT_EPSILON

	float xOnMap = clamp(vec.X, 0.f, mapWidth * (1.f - delta));
	float zOnMap = clamp(vec.Z, 0.f, mapWidth * (1.f - delta));

	// Don't waste time with GetExactGroundLevel unless we've changed
	if (xOnMap != vec.X || zOnMap != vec.Z)
	{
		vec.X = xOnMap;
		vec.Z = zOnMap;
		vec.Y = g_Game->GetWorld()->GetTerrain()->GetExactGroundLevel(xOnMap, zOnMap);
	}

	return vec;
}

MESSAGEHANDLER(ObjectPreview)
{
	// If the selection has changed...
	if (*msg->id != g_PreviewUnitName)
	{
		// Delete old entity
		if (g_PreviewEntityID != INVALID_ENTITY)
			g_Game->GetSimulation2()->DestroyEntity(g_PreviewEntityID);

		// Create the new entity
		if ((*msg->id).empty())
			g_PreviewEntityID = INVALID_ENTITY;
		else
			g_PreviewEntityID = g_Game->GetSimulation2()->AddLocalEntity(L"preview|" + *msg->id);

		g_PreviewUnitName = *msg->id;
	}

	if (g_PreviewEntityID != INVALID_ENTITY)
	{
		// Update the unit's position and orientation:

		CmpPtr<ICmpPosition> cmpPos (*g_Game->GetSimulation2(), g_PreviewEntityID);
		if (!cmpPos.null())
		{
			CVector3D pos = GetUnitPos(msg->pos, cmpPos->IsFloating());
			cmpPos->JumpTo(entity_pos_t::FromFloat(pos.X), entity_pos_t::FromFloat(pos.Z));

			float angle;
			if (msg->usetarget)
			{
				// Aim from pos towards msg->target
				CVector3D target = msg->target->GetWorldSpace(pos.Y);
				angle = atan2(target.X-pos.X, target.Z-pos.Z);
			}
			else
			{
				angle = msg->angle;
			}

			cmpPos->SetYRotation(entity_angle_t::FromFloat(angle));
		}

		// TODO: handle random variations somehow

		CmpPtr<ICmpOwnership> cmpOwner (*g_Game->GetSimulation2(), g_PreviewEntityID);
		if (!cmpOwner.null())
			cmpOwner->SetOwner((player_id_t)msg->settings->player);
	}
}

BEGIN_COMMAND(CreateObject)
{
	CVector3D m_Pos;
	float m_Angle;
	size_t m_ID; // old simulation system
	player_id_t m_Player;
	entity_id_t m_EntityID; // new simulation system

	void Do()
	{
		// Calculate the position/orientation to create this unit with
		
		m_Pos = GetUnitPos(msg->pos, true); // don't really care about floating

		if (msg->usetarget)
		{
			// Aim from m_Pos towards msg->target
			CVector3D target = msg->target->GetWorldSpace(m_Pos.Y);
			m_Angle = atan2(target.X-m_Pos.X, target.Z-m_Pos.Z);
		}
		else
		{
			m_Angle = msg->angle;
		}

		// TODO: variations too
		m_Player = (player_id_t)msg->settings->player;

		Redo();
	}

	void Redo()
	{
		m_EntityID = g_Game->GetSimulation2()->AddEntity(*msg->id);
		if (m_EntityID == INVALID_ENTITY)
			return;

		CmpPtr<ICmpPosition> cmpPos (*g_Game->GetSimulation2(), m_EntityID);
		if (!cmpPos.null())
		{
			cmpPos->JumpTo(entity_pos_t::FromFloat(m_Pos.X), entity_pos_t::FromFloat(m_Pos.Z));
			cmpPos->SetYRotation(entity_angle_t::FromFloat(m_Angle));
		}

		CmpPtr<ICmpOwnership> cmpOwner (*g_Game->GetSimulation2(), m_EntityID);
		if (!cmpOwner.null())
			cmpOwner->SetOwner(m_Player);

		// TODO: handle random variations somehow
	}

	void Undo()
	{
		if (m_EntityID != INVALID_ENTITY)
		{
			g_Game->GetSimulation2()->DestroyEntity(m_EntityID);
			m_EntityID = INVALID_ENTITY;
		}
	}
};
END_COMMAND(CreateObject)


QUERYHANDLER(PickObject)
{
	float x, y;
	msg->pos->GetScreenSpace(x, y);
	
	CVector3D rayorigin, raydir;
	g_Game->GetView()->GetCamera()->BuildCameraRay((int)x, (int)y, rayorigin, raydir);

	CUnit* target = GetUnitManager().PickUnit(rayorigin, raydir);

	if (target)
		msg->id = target->GetID();
	else
		msg->id = INVALID_ENTITY;

	if (target)
	{
		// Get screen coordinates of the point on the ground underneath the
		// object's model-centre, so that callers know the offset to use when
		// working out the screen coordinates to move the object to.
		
		CVector3D centre = target->GetModel().GetTransform().GetTranslation();

		centre.Y = g_Game->GetWorld()->GetTerrain()->GetExactGroundLevel(centre.X, centre.Z);
		if (IsFloating(target))
			centre.Y = std::max(centre.Y, g_Renderer.GetWaterManager()->m_WaterHeight);

		float cx, cy;
		g_Game->GetView()->GetCamera()->GetScreenCoordinates(centre, cx, cy);

		msg->offsetx = (int)(cx - x);
		msg->offsety = (int)(cy - y);
	}
	else
	{
		msg->offsetx = msg->offsety = 0;
	}
}


BEGIN_COMMAND(MoveObject)
{
	CVector3D m_PosOld, m_PosNew;

	void Do()
	{
		CmpPtr<ICmpPosition> cmpPos(*g_Game->GetSimulation2(), (entity_id_t)msg->id);
		if (cmpPos.null())
		{
			// error
			m_PosOld = m_PosNew = CVector3D(0, 0, 0);
		}
		else
		{
			m_PosNew = GetUnitPos(msg->pos, cmpPos->IsFloating());

			CFixedVector3D pos = cmpPos->GetPosition();
			m_PosOld = CVector3D(pos.X.ToFloat(), pos.Y.ToFloat(), pos.Z.ToFloat());
		}

		SetPos(m_PosNew);
	}

	void SetPos(CVector3D& pos)
	{
		CmpPtr<ICmpPosition> cmpPos(*g_Game->GetSimulation2(), (entity_id_t)msg->id);
		if (cmpPos.null())
			return;

		cmpPos->JumpTo(entity_pos_t::FromFloat(pos.X), entity_pos_t::FromFloat(pos.Z));
	}

	void Redo()
	{
		SetPos(m_PosNew);
	}

	void Undo()
	{
		SetPos(m_PosOld);
	}

	void MergeIntoPrevious(cMoveObject* prev)
	{
		// TODO: do something valid if prev unit != this unit
		debug_assert(prev->msg->id == msg->id);
		prev->m_PosNew = m_PosNew;
	}
};
END_COMMAND(MoveObject)


BEGIN_COMMAND(RotateObject)
{
	float m_AngleOld, m_AngleNew;

	void Do()
	{
		CmpPtr<ICmpPosition> cmpPos(*g_Game->GetSimulation2(), (entity_id_t)msg->id);
		if (cmpPos.null())
			return;

		m_AngleOld = cmpPos->GetRotation().Y.ToFloat();
		if (msg->usetarget)
		{
			CMatrix3D transform = cmpPos->GetInterpolatedTransform(0.f, false);
			CVector3D pos = transform.GetTranslation();
			CVector3D target = msg->target->GetWorldSpace(pos.Y);
			m_AngleNew = atan2(target.X-pos.X, target.Z-pos.Z);
		}
		else
		{
			m_AngleNew = msg->angle;
		}

		SetAngle(m_AngleNew);
	}

	void SetAngle(float angle)
	{
		CmpPtr<ICmpPosition> cmpPos(*g_Game->GetSimulation2(), (entity_id_t)msg->id);
		if (cmpPos.null())
			return;

		cmpPos->SetYRotation(entity_angle_t::FromFloat(angle));
	}

	void Redo()
	{
		SetAngle(m_AngleNew);
	}

	void Undo()
	{
		SetAngle(m_AngleOld);
	}

	void MergeIntoPrevious(cRotateObject* prev)
	{
		// TODO: do something valid if prev unit != this unit
		debug_assert(prev->msg->id == msg->id);
		prev->m_AngleNew = m_AngleNew;
	}
};
END_COMMAND(RotateObject)


BEGIN_COMMAND(DeleteObject)
{
	// Saved copy of the important aspects of a unit, to allow undo
	entity_id_t m_EntityID;
	std::string m_TemplateName;
	int32_t m_Owner;
	CFixedVector3D m_Pos;
	CFixedVector3D m_Rot;
	// TODO: random selections

	cDeleteObject()
	: m_EntityID(INVALID_ENTITY), m_Owner(-1)
	{
	}

	void Do()
	{
		Redo();
	}

	void Redo()
	{
		CSimulation2& sim = *g_Game->GetSimulation2();
		CmpPtr<ICmpTemplateManager> cmpTemplateManager(sim, SYSTEM_ENTITY);
		debug_assert(!cmpTemplateManager.null());

		m_EntityID = (entity_id_t)msg->id;
		m_TemplateName = cmpTemplateManager->GetCurrentTemplateName(m_EntityID);

		CmpPtr<ICmpOwnership> cmpOwner(sim, m_EntityID);
		if (!cmpOwner.null())
			m_Owner = cmpOwner->GetOwner();

		CmpPtr<ICmpPosition> cmpPosition(sim, m_EntityID);
		if (!cmpPosition.null())
		{
			m_Pos = cmpPosition->GetPosition();
			m_Rot = cmpPosition->GetRotation();
		}

		g_Game->GetSimulation2()->DestroyEntity(m_EntityID);
	}

	void Undo()
	{
		CSimulation2& sim = *g_Game->GetSimulation2();
		entity_id_t ent = sim.AddEntity(CStrW(m_TemplateName), m_EntityID);
		if (ent == INVALID_ENTITY)
			LOGERROR(L"Failed to load entity template '%hs'", m_TemplateName.c_str());
		else
		{
			CmpPtr<ICmpPosition> cmpPosition(sim, m_EntityID);
			if (!cmpPosition.null())
			{
				cmpPosition->JumpTo(m_Pos.X, m_Pos.Z);
				cmpPosition->SetXZRotation(m_Rot.X, m_Rot.Z);
				cmpPosition->SetYRotation(m_Rot.Y);
			}

			CmpPtr<ICmpOwnership> cmpOwner(sim, m_EntityID);
			if (!cmpOwner.null())
				cmpOwner->SetOwner(m_Owner);
		}
	}
};
END_COMMAND(DeleteObject)


}
