#include "precompiled.h"

#include <float.h>

#include "MessageHandler.h"
#include "../CommandProc.h"

#include "simulation/BaseEntityCollection.h"
#include "simulation/EntityManager.h"
#include "graphics/Unit.h"
#include "graphics/UnitManager.h"
#include "graphics/Model.h"
#include "graphics/ObjectManager.h"
#include "maths/Matrix3D.h"
#include "maths/MathUtil.h"
#include "ps/CLogger.h"
#include "ps/Game.h"
#include "lib/ogl.h"

#define LOG_CATEGORY "editor"

namespace AtlasMessage {

static bool SortObjectsList(const sObjectsListItem& a, const sObjectsListItem& b)
{
	return a.name < b.name;
}

QUERYHANDLER(GetObjectsList)
{
	{
		std::vector<CStrW> names;
		g_EntityTemplateCollection.getBaseEntityNames(names);
		for (std::vector<CStrW>::iterator it = names.begin(); it != names.end(); ++it)
		{
			//CBaseEntity* baseent = g_EntityTemplateCollection.getTemplate(*it);
			sObjectsListItem e;
			e.id = L"(e) " + *it;
			e.name = *it; //baseent->m_Tag
			e.type = 0;
			msg->objects.push_back(e);
		}
	}

	{
		std::vector<CStr> names;
		//g_ObjMan.GetPropObjectNames(names);
		g_ObjMan.GetAllObjectNames(names);
		for (std::vector<CStr>::iterator it = names.begin(); it != names.end(); ++it)
		{
			sObjectsListItem e;
			e.id = L"(n) " + CStrW(*it);
			e.name = CStrW(*it).AfterFirst(/*L"props/"*/ L"actors/");
			e.type = 1;
			msg->objects.push_back(e);
		}
	}
	std::sort(msg->objects.begin(), msg->objects.end(), SortObjectsList);
}


static std::vector<ObjectID> g_Selection;
void AtlasRenderSelection()
{
	glDisable(GL_DEPTH_TEST);
	for (size_t i = 0; i < g_Selection.size(); ++i)
	{
		CUnit* unit = g_UnitMan.FindByID(g_Selection[i]);
		if (unit)
		{
			if (unit->GetEntity())
			{
				unit->GetEntity()->renderSelectionOutline();
			}
			else
			{
				const CBound& bound = unit->GetModel()->GetBounds();
				// Expand bounds by 10% around the centre
				CVector3D centre;
				bound.GetCentre(centre);
				CVector3D a = (bound[0] - centre) * 1.1f + centre;
				CVector3D b = (bound[1] - centre) * 1.1f + centre;
				float h = g_Game->GetWorld()->GetTerrain()->getExactGroundLevel(centre.X, centre.Z);
				glColor3f(0.8f, 0.8f, 0.8f);
				glBegin(GL_LINE_LOOP);
					glVertex3f(a.X, h, a.Z);
					glVertex3f(a.X, h, b.Z);
					glVertex3f(b.X, h, b.Z);
					glVertex3f(b.X, h, a.Z);
				glEnd();
			}
		}
	}
	glEnable(GL_DEPTH_TEST);
}

MESSAGEHANDLER(SetSelectionPreview)
{
	g_Selection = msg->ids;
}

//////////////////////////////////////////////////////////////////////////


static CUnit* g_PreviewUnit = NULL;
static CStrW g_PreviewUnitID;

// Returns roughly the largest number smaller than f (i.e. closer to zero)
static float flt_minus_epsilon(float f)
{
	return f - (FLT_EPSILON * f);
}

static CVector3D GetUnitPos(const Position& pos)
{
	static CVector3D vec;
	pos.GetWorldSpace(vec, vec); // if msg->pos is 'Unchanged', use the previous pos

	float xOnMap = clamp(vec.X, 0.f, flt_minus_epsilon((g_Game->GetWorld()->GetTerrain()->GetVerticesPerSide()-1)*CELL_SIZE));
	float zOnMap = clamp(vec.Z, 0.f, flt_minus_epsilon((g_Game->GetWorld()->GetTerrain()->GetVerticesPerSide()-1)*CELL_SIZE));
	if (xOnMap != vec.X || zOnMap != vec.Z)
	{
		vec.X = xOnMap;
		vec.Z = zOnMap;
		vec.Y = g_Game->GetWorld()->GetTerrain()->getExactGroundLevel(xOnMap, zOnMap);
	}

	return vec;
}

static bool ParseObjectName(const CStrW& obj, bool& isEntity, CStrW& name)
{
	if (obj.substr(0, 4) == L"(e) ")
	{
		isEntity = true;
		name = obj.substr(4);
		return true;
	}
	else if (obj.substr(0, 4) == L"(n) ")
	{
		isEntity = false;
		name = obj.substr(4);
		return true;
	}
	else
	{
		return false;
	}
}

MESSAGEHANDLER(ObjectPreview)
{
	if (msg->id != g_PreviewUnitID)
	{
		// Delete old unit
		if (g_PreviewUnit)
		{
			g_UnitMan.RemoveUnit(g_PreviewUnit);
			delete g_PreviewUnit;
			g_PreviewUnit = NULL;
		}

		bool isEntity;
		CStrW name;
		if (ParseObjectName(msg->id, isEntity, name))
		{
			std::set<CStrW> selections; // TODO: get selections from user

			// Create new unit
			if (isEntity)
			{
				CBaseEntity* base = g_EntityTemplateCollection.getTemplate(name);
				if (base) // (ignore errors)
				{
					g_PreviewUnit = g_UnitMan.CreateUnit(base->m_actorName, NULL, selections);
					g_PreviewUnit->SetPlayerID(msg->player);
					// TODO: variations
				}
			}
			else
			{
				g_PreviewUnit = g_UnitMan.CreateUnit(CStr(name), NULL, selections);
			}
		}

		g_PreviewUnitID = msg->id;
	}

	if (g_PreviewUnit)
	{
		CVector3D pos = GetUnitPos(msg->pos);

		float s, c;

		if (msg->usetarget)
		{
			CVector3D target;
			msg->target.GetWorldSpace(target, pos.Y);
			CVector2D dir(target.X-pos.X, target.Z-pos.Z);
			dir = dir.normalize();
			s = dir.x;
			c = dir.y;
		}
		else
		{
			s = sin(msg->angle);
			c = cos(msg->angle);
		}

		CMatrix3D m;
		m._11 = -c;     m._12 = 0.0f;   m._13 = -s;     m._14 = pos.X;
		m._21 = 0.0f;   m._22 = 1.0f;   m._23 = 0.0f;   m._24 = pos.Y;
		m._31 = s;      m._32 = 0.0f;   m._33 = -c;     m._34 = pos.Z;
		m._41 = 0.0f;   m._42 = 0.0f;   m._43 = 0.0f;   m._44 = 1.0f;
		g_PreviewUnit->GetModel()->SetTransform(m);
	}
}

BEGIN_COMMAND(CreateObject)

	CVector3D m_Pos;
	float m_Angle;
	int m_ID;
	int m_Player;

	void Do()
	{
		m_Pos = GetUnitPos(d->pos);
		m_Player = d->player;

		if (d->usetarget)
		{
			CVector3D target;
			d->target.GetWorldSpace(target, m_Pos.Y);
			CVector2D dir(target.X-m_Pos.X, target.Z-m_Pos.Z);
			m_Angle = atan2(dir.x, dir.y);
		}
		else
		{
			m_Angle = d->angle;
		}

		m_ID = g_UnitMan.GetNewID();

		Redo();
	}

	void Redo()
	{
		bool isEntity;
		CStrW name;
		if (ParseObjectName(d->id, isEntity, name))
		{
			std::set<CStrW> selections;

			if (isEntity)
			{
				CBaseEntity* base = g_EntityTemplateCollection.getTemplate(name);
				if (! base)
					LOG(ERROR, LOG_CATEGORY, "Failed to load entity template '%ls'", name.c_str());
				else
				{
					HEntity ent = g_EntityManager.create(base, m_Pos, m_Angle, selections);

					if (! ent)
						LOG(ERROR, LOG_CATEGORY, "Failed to create entity of type '%ls'", name.c_str());
					else
					{
						ent->SetPlayer(g_Game->GetPlayer(m_Player));

						ent->m_actor->SetID(m_ID);
					}
				}
			}
			else
			{
				CUnit* unit = g_UnitMan.CreateUnit(CStr(name), NULL, selections);
				if (! unit)
					LOG(ERROR, LOG_CATEGORY, "Failed to load nonentity actor '%ls'", name.c_str());
				else
				{
					unit->SetID(m_ID);

					float s = sin(m_Angle);
					float c = cos(m_Angle);

					CMatrix3D m;
					m._11 = -c;     m._12 = 0.0f;   m._13 = -s;     m._14 = m_Pos.X;
					m._21 = 0.0f;   m._22 = 1.0f;   m._23 = 0.0f;   m._24 = m_Pos.Y;
					m._31 = s;      m._32 = 0.0f;   m._33 = -c;     m._34 = m_Pos.Z;
					m._41 = 0.0f;   m._42 = 0.0f;   m._43 = 0.0f;   m._44 = 1.0f;
					unit->GetModel()->SetTransform(m);

				}
			}
		}
	}

	void Undo()
	{
		CUnit* unit = g_UnitMan.FindByID(m_ID);
		if (unit)
		{
			if (unit->GetEntity())
				unit->GetEntity()->kill();
			else
			{
				g_UnitMan.RemoveUnit(unit);
				delete unit;
			}
		}
	}

END_COMMAND(CreateObject)


QUERYHANDLER(SelectObject)
{
	float x, y;
	msg->pos.GetScreenSpace(x, y);
	
	CVector3D rayorigin, raydir;
	g_Game->GetView()->GetCamera()->BuildCameraRay((int)x, (int)y, rayorigin, raydir);

	CUnit* target = g_UnitMan.PickUnit(rayorigin, raydir);

	if (target)
		msg->id = target->GetID();
	else
		msg->id = -1;

	if (target)
	{
		CVector3D centre = target->GetModel()->GetTransform().GetTranslation();
		centre.Y = g_Game->GetWorld()->GetTerrain()->getExactGroundLevel(centre.X, centre.Z);
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

	CVector3D m_PosOld, m_PosNew;

	void Do()
	{
		CUnit* unit = g_UnitMan.FindByID(d->id);
		if (! unit) return;

		if (unit->GetEntity())
		{
			m_PosOld = unit->GetEntity()->m_position;
		}
		else
		{
			CMatrix3D m = unit->GetModel()->GetTransform();
			m_PosOld = m.GetTranslation();
		}

		m_PosNew = GetUnitPos(d->pos);

		SetPos(m_PosNew);
	}

	void SetPos(CVector3D& pos)
	{
		CUnit* unit = g_UnitMan.FindByID(d->id);
		if (! unit) return;

		if (unit->GetEntity())
		{
			unit->GetEntity()->m_position = pos;
		}
		else
		{
			CMatrix3D m = unit->GetModel()->GetTransform();
			m.Translate(pos - m.GetTranslation());
			unit->GetModel()->SetTransform(m);
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

	void MergeWithSelf(cMoveObject* prev)
	{
		// TODO: merge correctly when prev unit != this unit
		prev->m_PosNew = m_PosNew;
	}

END_COMMAND(MoveObject)


BEGIN_COMMAND(RotateObject)

	float m_AngleOld, m_AngleNew;
	CMatrix3D m_TransformOld, m_TransformNew;

	void Do()
	{
		CUnit* unit = g_UnitMan.FindByID(d->id);
		if (! unit) return;

		if (unit->GetEntity())
		{
			m_AngleOld = unit->GetEntity()->m_orientation;
			if (d->usetarget)
			{
				CVector3D& pos = unit->GetEntity()->m_position;
				CVector3D target;
				d->target.GetWorldSpace(target, pos.Y);
				CVector2D dir(target.X-pos.X, target.Z-pos.Z);
				m_AngleNew = atan2(dir.x, dir.y);
			}
			else
			{
				m_AngleNew = d->angle;
			}
		}
		else
		{
			m_TransformOld = unit->GetModel()->GetTransform();

			CVector3D pos = unit->GetModel()->GetTransform().GetTranslation();

			float s, c;
			if (d->usetarget)
			{
				CVector3D target;
				d->target.GetWorldSpace(target, pos.Y);
				CVector2D dir(target.X-pos.X, target.Z-pos.Z);
				dir = dir.normalize();
				s = dir.x;
				c = dir.y;
			}
			else
			{
				s = sinf(d->angle);
				c = cosf(d->angle);
			}
			CMatrix3D& m = m_TransformNew;
			m._11 = -c;     m._12 = 0.0f;   m._13 = -s;     m._14 = pos.X;
			m._21 = 0.0f;   m._22 = 1.0f;   m._23 = 0.0f;   m._24 = pos.Y;
			m._31 = s;      m._32 = 0.0f;   m._33 = -c;     m._34 = pos.Z;
			m._41 = 0.0f;   m._42 = 0.0f;   m._43 = 0.0f;   m._44 = 1.0f;
		}

		SetAngle(m_AngleNew, m_TransformNew);
	}

	void SetAngle(float angle, CMatrix3D& transform)
	{
		CUnit* unit = g_UnitMan.FindByID(d->id);
		if (! unit) return;

		if (unit->GetEntity())
		{
			unit->GetEntity()->m_orientation = angle;
		}
		else
		{
			unit->GetModel()->SetTransform(transform);
		}
	}

	void Redo()
	{
		SetAngle(m_AngleNew, m_TransformNew);
	}

	void Undo()
	{
		SetAngle(m_AngleOld, m_TransformOld);
	}

	void MergeWithSelf(cRotateObject* prev)
	{
		// TODO: merge correctly when prev unit != this unit
		prev->m_AngleNew = m_AngleNew;
		prev->m_TransformNew = m_TransformNew;
	}

END_COMMAND(RotateObject)


BEGIN_COMMAND(DeleteObject)

	CUnit* m_UnitInLimbo;

	void Construct()
	{
		m_UnitInLimbo = NULL;
	}

	void Destruct()
	{
		if (m_UnitInLimbo)
		{
			if (m_UnitInLimbo->GetEntity())
				m_UnitInLimbo->GetEntity()->kill();
			else
				delete m_UnitInLimbo;
		}
	}

	void Do()
	{
		Redo();
	}

	void Redo()
	{
		CUnit* unit = g_UnitMan.FindByID(d->id);
		if (! unit) return;

		if (unit->GetEntity())
		{
			// HACK: I don't know the proper way of undoably deleting entities...
			unit->GetEntity()->m_destroyed = true;
		}

		g_UnitMan.RemoveUnit(unit);
		m_UnitInLimbo = unit;
	}

	void Undo()
	{
		if (m_UnitInLimbo->GetEntity())
			m_UnitInLimbo->GetEntity()->m_destroyed = false;

		g_UnitMan.AddUnit(m_UnitInLimbo);
		m_UnitInLimbo = NULL;
	}

END_COMMAND(DeleteObject)


}
