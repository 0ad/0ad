/* Copyright (C) 2013 Wildfire Games.
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
#include <map>

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
#include "simulation2/components/ICmpPlayer.h"
#include "simulation2/components/ICmpPlayerManager.h"
#include "simulation2/components/ICmpSelectable.h"
#include "simulation2/components/ICmpTemplateManager.h"
#include "simulation2/components/ICmpVisual.h"
#include "simulation2/helpers/Selection.h"
#include "ps/XML/XMLWriter.h"

namespace AtlasMessage {

namespace
{
	bool SortObjectsList(const sObjectsListItem& a, const sObjectsListItem& b)
	{
		return wcscmp(a.name.c_str(), b.name.c_str()) < 0;
	}
}

QUERYHANDLER(GetObjectsList)
{
	std::vector<sObjectsListItem> objects;

	CmpPtr<ICmpTemplateManager> cmpTemplateManager(*g_Game->GetSimulation2(), SYSTEM_ENTITY);
	if (cmpTemplateManager)
	{
		std::vector<std::string> names = cmpTemplateManager->FindAllPlaceableTemplates(true);

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


static std::vector<entity_id_t> g_Selection;
typedef std::map<player_id_t, CColor> PlayerColorMap;

// Helper function to find color of player owning the given entity,
//	returns white if entity has no owner. Uses caching to avoid
//	expensive script calls.
static CColor GetOwnerPlayerColor(PlayerColorMap& colourMap, entity_id_t id)
{
	// Default color - white
	CColor color(1.0f, 1.0f, 1.0f, 1.0f);

	CSimulation2& sim = *g_Game->GetSimulation2();
	CmpPtr<ICmpOwnership> cmpOwnership(sim, id);
	if (cmpOwnership)
	{
		player_id_t owner = cmpOwnership->GetOwner();
		if (colourMap.find(owner) != colourMap.end())
			return colourMap[owner];
		else
		{
			CmpPtr<ICmpPlayerManager> cmpPlayerManager(sim, SYSTEM_ENTITY);
			entity_id_t playerEnt = cmpPlayerManager->GetPlayerByID(owner);
			CmpPtr<ICmpPlayer> cmpPlayer(sim, playerEnt);
			if (cmpPlayer)
				color = colourMap[owner] = cmpPlayer->GetColour();
		}
	}
	return color;
}

MESSAGEHANDLER(SetSelectionPreview)
{
	CSimulation2& sim = *g_Game->GetSimulation2();

	// Cache player colours for performance
	PlayerColorMap playerColors;

	// Clear old selection rings
	for (size_t i = 0; i < g_Selection.size(); ++i)
	{
		// We can't set only alpha here, because that won't trigger desaturation
		//	so we set the complete color (not too evil since it's cached)
		CmpPtr<ICmpSelectable> cmpSelectable(sim, g_Selection[i]);
		if (cmpSelectable)
		{
			CColor color = GetOwnerPlayerColor(playerColors, g_Selection[i]);
			color.a = 0.0f;
			cmpSelectable->SetSelectionHighlight(color, false);
		}
	}

	g_Selection = *msg->ids;

	// Set new selection rings
	for (size_t i = 0; i < g_Selection.size(); ++i)
	{
		CmpPtr<ICmpSelectable> cmpSelectable(sim, g_Selection[i]);
		if (cmpSelectable)
			cmpSelectable->SetSelectionHighlight(GetOwnerPlayerColor(playerColors, g_Selection[i]), true);
	}
}

QUERYHANDLER(GetObjectSettings)
{
	AtlasView* view = AtlasView::GetView(msg->view);
	CSimulation2* simulation = view->GetSimulation2();

	sObjectSettings settings;
	settings.player = 0;

	CmpPtr<ICmpOwnership> cmpOwnership(*simulation, view->GetEntityId(msg->id));
	if (cmpOwnership)
	{
		int32_t player = cmpOwnership->GetOwner();
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

QUERYHANDLER(GetObjectMapSettings)
{
	std::vector<entity_id_t> ids = *msg->ids;
	
	CmpPtr<ICmpTemplateManager> cmpTemplateManager(*g_Game->GetSimulation2(), SYSTEM_ENTITY);
	ENSURE(cmpTemplateManager);
	
	XML_Start();
	{
		XML_Element("Entities");
		{
			for (size_t i = 0; i < ids.size(); i++)
			{
				entity_id_t id = (entity_id_t)ids[i];
				XML_Element("Entity");
				{
					//Template name
					XML_Setting("Template", cmpTemplateManager->GetCurrentTemplateName(id));
					
					//Player
					CmpPtr<ICmpOwnership> cmpOwnership(*g_Game->GetSimulation2(), id);
					if (cmpOwnership)
						XML_Setting("Player", (int)cmpOwnership->GetOwner());
					
					//Adding position to make some relative position later
					CmpPtr<ICmpPosition> cmpPosition(*g_Game->GetSimulation2(), id);
					if (cmpPosition)
					{
						CFixedVector3D pos = cmpPosition->GetPosition();
						CFixedVector3D rot = cmpPosition->GetRotation();
						{
							XML_Element("Position");
							XML_Attribute("x", pos.X);
							XML_Attribute("z", pos.Z);
							// TODO: height offset etc
						}
						{
							XML_Element("Orientation");
							XML_Attribute("y", rot.Y);
														// TODO: X, Z maybe
						}
					}
				}
			}
		}
	}
	
	const CStr& data = XML_GetOutput();
	msg->xmldata = std::wstring(data.begin(), data.end());
}


BEGIN_COMMAND(SetObjectSettings)
{
	player_id_t m_PlayerOld, m_PlayerNew;
	std::set<CStr> m_SelectionsOld, m_SelectionsNew;

	void Do()
	{
		sObjectSettings settings = msg->settings;

		AtlasView* view = AtlasView::GetView(msg->view);
		CSimulation2* simulation = view->GetSimulation2();

		CmpPtr<ICmpOwnership> cmpOwnership(*simulation, view->GetEntityId(msg->id));
		m_PlayerOld = 0;
		if (cmpOwnership)
		{
			int32_t player = cmpOwnership->GetOwner();
			if (player != -1)
				m_PlayerOld = player;
		}

		// TODO: selections
//		m_SelectionsOld = unit->GetActorSelections();

		m_PlayerNew = (player_id_t)settings.player;

		std::vector<std::wstring> selections = *settings.selections;
		for (std::vector<std::wstring>::iterator it = selections.begin(); it != selections.end(); ++it)
		{
			m_SelectionsNew.insert(CStrW(*it).ToUTF8());
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
		AtlasView* view = AtlasView::GetView(msg->view);
		CSimulation2* simulation = view->GetSimulation2();

		CmpPtr<ICmpOwnership> cmpOwnership(*simulation, view->GetEntityId(msg->id));
		if (cmpOwnership)
			cmpOwnership->SetOwner(player);

		// TODO: selections
//		unit->SetActorSelections(selections);
	}
};
END_COMMAND(SetObjectSettings);

//////////////////////////////////////////////////////////////////////////

static CStrW g_PreviewUnitName;
static entity_id_t g_PreviewEntityID = INVALID_ENTITY;
static std::vector<entity_id_t> g_PreviewEntitiesID;

static CVector3D GetUnitPos(const Position& pos, bool floating)
{
	static CVector3D vec;
	vec = pos.GetWorldSpace(vec, floating); // if msg->pos is 'Unchanged', use the previous pos

	// Clamp the position to the edges of the world:

	// Use 'clamp' with a value slightly less than the width, so that converting
	// to integer (rounding towards zero) will put it on the tile inside the edge
	// instead of just outside
	float mapWidth = (g_Game->GetWorld()->GetTerrain()->GetVerticesPerSide()-1)*TERRAIN_TILE_SIZE;
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

QUERYHANDLER(GetCurrentSelection)
{
	msg->ids = g_Selection;
}

MESSAGEHANDLER(ObjectPreviewToEntity)
{
	UNUSED2(msg);

	if (g_PreviewEntitiesID.size() == 0)
		return;

	CmpPtr<ICmpTemplateManager> cmpTemplateManager(*g_Game->GetSimulation2(), SYSTEM_ENTITY);
	ENSURE(cmpTemplateManager);

	PlayerColorMap playerColor;

	//I need to re create the objects finally delete preview objects
	std::vector<entity_id_t>::const_iterator i;
	for (i = g_PreviewEntitiesID.begin(); i != g_PreviewEntitiesID.end(); i++)
	{
		//Get template
		entity_id_t ent = *i;
		std::string templateName = cmpTemplateManager->GetCurrentTemplateName(ent);
		std::wstring wTemplateName(templateName.begin() + 8, templateName.end());
		//Create new entity
		entity_id_t new_ent = g_Game->GetSimulation2()->AddEntity(wTemplateName);
		if (new_ent == INVALID_ENTITY)
			continue;

		//get position, get rotation
		CmpPtr<ICmpPosition> cmpPositionNew(*g_Game->GetSimulation2(), new_ent);
		CmpPtr<ICmpPosition> cmpPositionOld(*g_Game->GetSimulation2(), ent);

		if (cmpPositionNew && cmpPositionOld)
		{
			CVector3D pos = cmpPositionOld->GetPosition();
			cmpPositionNew->JumpTo(entity_pos_t::FromFloat(pos.X), entity_pos_t::FromFloat(pos.Z));

			//now rotate
			CFixedVector3D rotation = cmpPositionOld->GetRotation();
			cmpPositionNew->SetYRotation(rotation.Y);
		}

		//get owner
		CmpPtr<ICmpOwnership> cmpOwnershipNew(*g_Game->GetSimulation2(), new_ent);
		CmpPtr<ICmpOwnership> cmpOwnershipOld(*g_Game->GetSimulation2(), ent);
		if (cmpOwnershipNew && cmpOwnershipOld)
			cmpOwnershipNew->SetOwner(cmpOwnershipOld->GetOwner());

		//getVisual
		CmpPtr<ICmpVisual> cmpVisualNew(*g_Game->GetSimulation2(), new_ent);
		CmpPtr<ICmpVisual> cmpVisualOld(*g_Game->GetSimulation2(), ent);
		if (cmpVisualNew && cmpVisualOld)
			cmpVisualNew->SetActorSeed(cmpVisualOld->GetActorSeed());

		//Update g_selectedObject and higligth
		g_Selection.push_back(new_ent);
		CmpPtr<ICmpSelectable> cmpSelectable(*g_Game->GetSimulation2(), new_ent);
		if (cmpSelectable)
			cmpSelectable->SetSelectionHighlight(GetOwnerPlayerColor(playerColor, new_ent), true);

		g_Game->GetSimulation2()->DestroyEntity(ent);
	}
	g_PreviewEntitiesID.clear();

}

MESSAGEHANDLER(MoveObjectPreview)
{
	if (g_PreviewEntitiesID.size()==0)
		return;

	//TODO:Change pivot 
	entity_id_t referenceEntity = *g_PreviewEntitiesID.begin();

	// All selected objects move relative to a pivot object,
	//	so get its position and whether it's floating
	CFixedVector3D referencePos;
	
	CmpPtr<ICmpPosition> cmpPosition(*g_Game->GetSimulation2(), referenceEntity);
	if (cmpPosition && cmpPosition->IsInWorld())
		referencePos = cmpPosition->GetPosition();
		

	// Calculate directional vector of movement for pivot object,
	//	we apply the same movement to all objects
	CVector3D targetPos = GetUnitPos(msg->pos, true);
	CFixedVector3D fTargetPos(entity_pos_t::FromFloat(targetPos.X), entity_pos_t::FromFloat(targetPos.Y), entity_pos_t::FromFloat(targetPos.Z));
	CFixedVector3D dir = fTargetPos - referencePos;

	for (size_t i = 0; i < g_PreviewEntitiesID.size(); ++i)
	{
		entity_id_t id = (entity_id_t)g_PreviewEntitiesID[i];
		CFixedVector3D posFinal;
		CmpPtr<ICmpPosition> cmpPosition(*g_Game->GetSimulation2(), id);
		if (cmpPosition && cmpPosition->IsInWorld())
		{
			// Calculate this object's position
			CFixedVector3D posFixed = cmpPosition->GetPosition();
			posFinal = posFixed + dir;
		}
		cmpPosition->JumpTo(posFinal.X, posFinal.Z);
	}
}

MESSAGEHANDLER(ObjectPreview)
{
	// If the selection has changed...
	if (*msg->id != g_PreviewUnitName || (!msg->cleanObjectPreviews))
	{
		// Delete old entity
		if (g_PreviewEntityID != INVALID_ENTITY && msg->cleanObjectPreviews)
		{
			//Time to delete all preview objects
			if (g_PreviewEntitiesID.size() > 0)
			{
				std::vector<entity_id_t>::const_iterator i;
				for (i = g_PreviewEntitiesID.begin(); i != g_PreviewEntitiesID.end(); i++)
				{
					g_Game->GetSimulation2()->DestroyEntity(*i);
				}
				g_PreviewEntitiesID.clear();
			}
		}

		// Create the new entity
		if ((*msg->id).empty())
			g_PreviewEntityID = INVALID_ENTITY;
		else
		{
			g_PreviewEntityID = g_Game->GetSimulation2()->AddLocalEntity(L"preview|" + *msg->id);
			g_PreviewEntitiesID.push_back(g_PreviewEntityID);
		}
		

		g_PreviewUnitName = *msg->id;
	}

	if (g_PreviewEntityID != INVALID_ENTITY)
	{
		// Update the unit's position and orientation:

		CmpPtr<ICmpPosition> cmpPosition(*g_Game->GetSimulation2(), g_PreviewEntityID);
		if (cmpPosition)
		{
			CVector3D pos = GetUnitPos(msg->pos, cmpPosition->IsFloating());
			cmpPosition->JumpTo(entity_pos_t::FromFloat(pos.X), entity_pos_t::FromFloat(pos.Z));

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

			cmpPosition->SetYRotation(entity_angle_t::FromFloat(angle));
		}

		// TODO: handle random variations somehow

		CmpPtr<ICmpVisual> cmpVisual(*g_Game->GetSimulation2(), g_PreviewEntityID);
		if (cmpVisual)
			cmpVisual->SetActorSeed(msg->actorseed);

		CmpPtr<ICmpOwnership> cmpOwnership(*g_Game->GetSimulation2(), g_PreviewEntityID);
		if (cmpOwnership)
			cmpOwnership->SetOwner((player_id_t)msg->settings->player);
	}
}

BEGIN_COMMAND(CreateObject)
{
	CVector3D m_Pos;
	float m_Angle;
	player_id_t m_Player;
	entity_id_t m_EntityID;
	u32 m_ActorSeed;

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

		m_Player = (player_id_t)msg->settings->player;
		m_ActorSeed = msg->actorseed;
		// TODO: variation/selection strings

		Redo();
	}

	void Redo()
	{
		m_EntityID = g_Game->GetSimulation2()->AddEntity(*msg->id);
		if (m_EntityID == INVALID_ENTITY)
			return;

		CmpPtr<ICmpPosition> cmpPosition(*g_Game->GetSimulation2(), m_EntityID);
		if (cmpPosition)
		{
			cmpPosition->JumpTo(entity_pos_t::FromFloat(m_Pos.X), entity_pos_t::FromFloat(m_Pos.Z));
			cmpPosition->SetYRotation(entity_angle_t::FromFloat(m_Angle));
		}

		CmpPtr<ICmpOwnership> cmpOwnership(*g_Game->GetSimulation2(), m_EntityID);
		if (cmpOwnership)
			cmpOwnership->SetOwner(m_Player);

		CmpPtr<ICmpVisual> cmpVisual(*g_Game->GetSimulation2(), m_EntityID);
		if (cmpVisual)
		{
			cmpVisual->SetActorSeed(m_ActorSeed);
			// TODO: variation/selection strings
		}
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
	
	// Normally this function would be called with a player ID to check LOS,
	//	but in Atlas the entire map is revealed, so just pass INVALID_PLAYER
	std::vector<entity_id_t> ents = EntitySelection::PickEntitiesAtPoint(*g_Game->GetSimulation2(), *g_Game->GetView()->GetCamera(), x, y, INVALID_PLAYER, msg->selectActors);

	// Multiple entities may have been picked, but they are sorted by distance,
	//	so only take the first one
	if (!ents.empty())
	{
		msg->id = ents[0];

		// Calculate offset of object from original mouse click position
		//	so it gets moved by that offset
		CmpPtr<ICmpPosition> cmpPosition(*g_Game->GetSimulation2(), (entity_id_t)ents[0]);
		if (!cmpPosition || !cmpPosition->IsInWorld())
		{
			// error
			msg->offsetx = msg->offsety = 0;
		}
		else
		{
			CFixedVector3D fixed = cmpPosition->GetPosition();
			CVector3D centre = CVector3D(fixed.X.ToFloat(), fixed.Y.ToFloat(), fixed.Z.ToFloat());

			float cx, cy;
			g_Game->GetView()->GetCamera()->GetScreenCoordinates(centre, cx, cy);

			msg->offsetx = (int)(cx - x);
			msg->offsety = (int)(cy - y);
		}
	}
	else
	{
		// No entity picked
		msg->id = INVALID_ENTITY;
	}
}


QUERYHANDLER(PickObjectsInRect)
{
	float x0, y0, x1, y1;
	msg->start->GetScreenSpace(x0, y0);
	msg->end->GetScreenSpace(x1, y1);

	// Since owner selections are meaningless in Atlas, use INVALID_PLAYER
	msg->ids = EntitySelection::PickEntitiesInRect(*g_Game->GetSimulation2(), *g_Game->GetView()->GetCamera(), x0, y0, x1, y1, INVALID_PLAYER, msg->selectActors);
}


QUERYHANDLER(PickSimilarObjects)
{
	CmpPtr<ICmpTemplateManager> cmpTemplateManager(*g_Game->GetSimulation2(), SYSTEM_ENTITY);
	ENSURE(cmpTemplateManager);

	entity_id_t ent = msg->id;
	std::string templateName = cmpTemplateManager->GetCurrentTemplateName(ent);

	// If unit has ownership, only pick units from the same player
	player_id_t owner = INVALID_PLAYER;
	CmpPtr<ICmpOwnership> cmpOwnership(*g_Game->GetSimulation2(), ent);
	if (cmpOwnership)
		owner = cmpOwnership->GetOwner();

	msg->ids = EntitySelection::PickSimilarEntities(*g_Game->GetSimulation2(), *g_Game->GetView()->GetCamera(), templateName, owner, false, true, true, false);
}


BEGIN_COMMAND(MoveObjects)
{
	// Mapping from object to position
	typedef std::map<entity_id_t, CVector3D> ObjectPositionMap;
	ObjectPositionMap m_PosOld, m_PosNew;

	void Do()
	{
		std::vector<entity_id_t> ids = *msg->ids;

		// All selected objects move relative to a pivot object,
		//	so get its position and whether it's floating
		CVector3D pivotPos(0, 0, 0);
		bool pivotFloating = false;

		CmpPtr<ICmpPosition> cmpPosition(*g_Game->GetSimulation2(), (entity_id_t)msg->pivot);
		if (cmpPosition && cmpPosition->IsInWorld())
		{
			pivotFloating = cmpPosition->IsFloating();
			CFixedVector3D pivotFixed = cmpPosition->GetPosition();
			pivotPos = CVector3D(pivotFixed.X.ToFloat(), pivotFixed.Y.ToFloat(), pivotFixed.Z.ToFloat());
		}

		// Calculate directional vector of movement for pivot object,
		//	we apply the same movement to all objects
		CVector3D targetPos = GetUnitPos(msg->pos, pivotFloating);
		CVector3D dir = targetPos - pivotPos;

		for (size_t i = 0; i < ids.size(); ++i)
		{
			entity_id_t id = (entity_id_t)ids[i];
			CmpPtr<ICmpPosition> cmpPosition(*g_Game->GetSimulation2(), id);
			if (!cmpPosition || !cmpPosition->IsInWorld())
			{
				// error
				m_PosOld[id] = m_PosNew[id] = CVector3D(0, 0, 0);
			}
			else
			{
				// Calculate this object's position
				CFixedVector3D posFixed = cmpPosition->GetPosition();
				CVector3D pos = CVector3D(posFixed.X.ToFloat(), posFixed.Y.ToFloat(), posFixed.Z.ToFloat());
				m_PosNew[id] = pos + dir;
				m_PosOld[id] = pos;			
			}
		}

		SetPos(m_PosNew);
	}

	void SetPos(ObjectPositionMap& map)
	{
		ObjectPositionMap::iterator it;
		for (it = map.begin(); it != map.end(); ++it)
		{
			CmpPtr<ICmpPosition> cmpPosition(*g_Game->GetSimulation2(), (entity_id_t)it->first);
			if (!cmpPosition)
				return;

			// Set 2D position, ignoring height
			CVector3D pos = it->second;
			cmpPosition->JumpTo(entity_pos_t::FromFloat(pos.X), entity_pos_t::FromFloat(pos.Z));
		}
	}

	void Redo()
	{
		SetPos(m_PosNew);
	}

	void Undo()
	{
		SetPos(m_PosOld);
	}

	void MergeIntoPrevious(cMoveObjects* prev)
	{
		// TODO: do something valid if prev selection != this selection
		ENSURE(*(prev->msg->ids) == *(msg->ids));
		prev->m_PosNew = m_PosNew;
	}
};
END_COMMAND(MoveObjects)


BEGIN_COMMAND(RotateObject)
{
	float m_AngleOld, m_AngleNew;

	void Do()
	{
		CmpPtr<ICmpPosition> cmpPosition(*g_Game->GetSimulation2(), (entity_id_t)msg->id);
		if (!cmpPosition)
			return;

		m_AngleOld = cmpPosition->GetRotation().Y.ToFloat();
		if (msg->usetarget)
		{
			CMatrix3D transform = cmpPosition->GetInterpolatedTransform(0.f);
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
		CmpPtr<ICmpPosition> cmpPosition(*g_Game->GetSimulation2(), (entity_id_t)msg->id);
		if (!cmpPosition)
			return;

		cmpPosition->SetYRotation(entity_angle_t::FromFloat(angle));
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
		ENSURE(prev->msg->id == msg->id);
		prev->m_AngleNew = m_AngleNew;
	}
};
END_COMMAND(RotateObject)


BEGIN_COMMAND(DeleteObjects)
{
	// Saved copy of the important aspects of a unit, to allow undo
	struct OldObject
	{
		entity_id_t entityID;
		CStr templateName;
		int32_t owner;
		CFixedVector3D pos;
		CFixedVector3D rot;
	};

	std::vector<OldObject> oldObjects;

	cDeleteObjects()
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
		ENSURE(cmpTemplateManager);

		std::vector<entity_id_t> ids = *msg->ids;
		for (size_t i = 0; i < ids.size(); ++i)
		{
			OldObject obj;

			obj.entityID = (entity_id_t)ids[i];
			obj.templateName = cmpTemplateManager->GetCurrentTemplateName(obj.entityID);

			CmpPtr<ICmpOwnership> cmpOwnership(sim, obj.entityID);
			if (cmpOwnership)
				obj.owner = cmpOwnership->GetOwner();

			CmpPtr<ICmpPosition> cmpPosition(sim, obj.entityID);
			if (cmpPosition)
			{
				obj.pos = cmpPosition->GetPosition();
				obj.rot = cmpPosition->GetRotation();
			}

			oldObjects.push_back(obj);
			g_Game->GetSimulation2()->DestroyEntity(obj.entityID);
		}
	}

	void Undo()
	{
		CSimulation2& sim = *g_Game->GetSimulation2();

		for (size_t i = 0; i < oldObjects.size(); ++i)
		{
			entity_id_t ent = sim.AddEntity(oldObjects[i].templateName.FromUTF8(), oldObjects[i].entityID);
			if (ent == INVALID_ENTITY)
			{
				LOGERROR(L"Failed to load entity template '%hs'", oldObjects[i].templateName.c_str());
			}
			else
			{
				CmpPtr<ICmpPosition> cmpPosition(sim, oldObjects[i].entityID);
				if (cmpPosition)
				{
					cmpPosition->JumpTo(oldObjects[i].pos.X, oldObjects[i].pos.Z);
					cmpPosition->SetXZRotation(oldObjects[i].rot.X, oldObjects[i].rot.Z);
					cmpPosition->SetYRotation(oldObjects[i].rot.Y);
				}

				CmpPtr<ICmpOwnership> cmpOwnership(sim, oldObjects[i].entityID);
				if (cmpOwnership)
					cmpOwnership->SetOwner(oldObjects[i].owner);
			}
		}

		oldObjects.clear();
	}
};
END_COMMAND(DeleteObjects)

QUERYHANDLER(GetPlayerObjects)
{
	std::vector<entity_id_t> ids;
	player_id_t playerID = msg->player;

	const CSimulation2::InterfaceListUnordered& cmps = g_Game->GetSimulation2()->GetEntitiesWithInterfaceUnordered(IID_Ownership);
	for (CSimulation2::InterfaceListUnordered::const_iterator eit = cmps.begin(); eit != cmps.end(); ++eit)
	{
		if (static_cast<ICmpOwnership*>(eit->second)->GetOwner() == playerID)
		{
			ids.push_back(eit->first);
		}
	}

	msg->ids = ids;
}

MESSAGEHANDLER(SetBandbox)
{
	AtlasView::GetView_Game()->SetBandbox(msg->show, (float)msg->sx0, (float)msg->sy0, (float)msg->sx1, (float)msg->sy1);
}

}
