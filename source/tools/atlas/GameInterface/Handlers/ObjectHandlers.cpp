#include "precompiled.h"

#include "MessageHandler.h"
#include "../CommandProc.h"

#include "simulation/BaseEntityCollection.h"
#include "simulation/EntityManager.h"
#include "graphics/Unit.h"
#include "graphics/UnitManager.h"
#include "graphics/Model.h"
#include "maths/Matrix3D.h"
#include "ps/CLogger.h"
#include "ps/Game.h"
#include "lib/ogl.h"

#define LOG_CATEGORY "editor"

namespace AtlasMessage {

QUERYHANDLER(GetEntitiesList)
{
	std::vector<CStrW> names;
	g_EntityTemplateCollection.getBaseEntityNames(names);
	for (std::vector<CStrW>::iterator it = names.begin(); it != names.end(); ++it)
	{
		//CBaseEntity* baseent = g_EntityTemplateCollection.getTemplate(*it);
		sEntitiesListItem e;
		e.id = *it;
		e.name = *it; //baseent->m_Tag
		msg->entities.push_back(e);
	}
}


static std::vector<ObjectID> g_Selection;
void AtlasRenderSelection()
{
	glDisable(GL_DEPTH_TEST);
	for (size_t i = 0; i < g_Selection.size(); ++i)
	{
		if (g_Selection[i])
		{
			CUnit* unit = static_cast<CUnit*>(g_Selection[i]);
			if (unit->GetEntity())
				unit->GetEntity()->renderSelectionOutline();
			else if (unit->GetModel())
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

static CVector3D GetUnitPos(const Position& pos)
{
	static CVector3D vec;
	pos.GetWorldSpace(vec, vec); // if msg->pos is 'Unchanged', use the previous pos
	return vec;
}

MESSAGEHANDLER(EntityPreview)
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

		if (msg->id.length())
		{
			// Create new unit
			CBaseEntity* base = g_EntityTemplateCollection.getTemplate(msg->id);
			if (base) // (ignore errors)
			{
				g_PreviewUnit = g_UnitMan.CreateUnit(base->m_actorName, 0);
				// TODO: set player (for colour)
				// TODO: variations
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

BEGIN_COMMAND(CreateEntity)

	HEntity m_Entity;
	CVector3D m_Pos;
	float m_Angle;

	void Do()
	{
		m_Pos = GetUnitPos(d->pos);

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

		Redo();
	}

	void Redo()
	{
		CBaseEntity* base = g_EntityTemplateCollection.getTemplate(d->id);
		if (! base)
			LOG(ERROR, LOG_CATEGORY, "Failed to load entity template '%ls'", d->id.c_str());
		else
		{
			HEntity ent = g_EntityManager.create(base, m_Pos, m_Angle);

			if (! ent)
				LOG(ERROR, LOG_CATEGORY, "Failed to create entity of type '%ls'", d->id.c_str());
			else
			{
				// TODO: player ID
				ent->SetPlayer(g_Game->GetLocalPlayer());

				m_Entity = ent;
			}
		}
	}

	void Undo()
	{
		m_Entity->kill();
		m_Entity = HEntity();
	}

END_COMMAND(CreateEntity)


QUERYHANDLER(SelectObject)
{
	float x, y;
	msg->pos.GetScreenSpace(x, y);
	
	CVector3D rayorigin, raydir;
	g_Game->GetView()->GetCamera()->BuildCameraRay(x, y, rayorigin, raydir);

	CUnit* target = g_UnitMan.PickUnit(rayorigin, raydir);

	msg->id = static_cast<void*>(target);
}


BEGIN_COMMAND(MoveObject)

	CVector3D m_PosOld, m_PosNew;

	void Do()
	{
		if (! d->id)
			return;

		CUnit* unit = static_cast<CUnit*>(d->id);

		if (unit->GetEntity())
		{
			m_PosOld = unit->GetEntity()->m_position;
		}
		else if (unit->GetModel())
		{
			CMatrix3D m = unit->GetModel()->GetTransform();
			m_PosOld = m.GetTranslation();
		}

		m_PosNew = GetUnitPos(d->pos);

		SetPos(m_PosNew);
	}

	void SetPos(CVector3D& pos)
	{
		if (! d->id)
			return;

		CUnit* unit = static_cast<CUnit*>(d->id);

		if (unit->GetEntity())
		{
			unit->GetEntity()->m_position = pos;
		}
		else if (unit->GetModel())
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
		if (! d->id)
			return;

		CUnit* unit = static_cast<CUnit*>(d->id);

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
		else if (unit->GetModel())
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
		if (! d->id)
			return;

		CUnit* unit = static_cast<CUnit*>(d->id);

		if (unit->GetEntity())
		{
			unit->GetEntity()->m_orientation = angle;
		}
		else if (unit->GetModel())
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

	bool m_ObjectAlive;

	void Construct()
	{
		m_ObjectAlive = true;
	}

	void Destruct()
	{
		if (! m_ObjectAlive)
		{
			if (! d->id)
				return;

			CUnit* unit = static_cast<CUnit*>(d->id);

			if (unit->GetEntity())
				unit->GetEntity()->kill();
			else
			{
				g_UnitMan.RemoveUnit(unit);
				delete unit;
			}
		}
	}

	void Do()
	{
		Redo();
	}

	void Redo()
	{
		if (! d->id)
			return;

		CUnit* unit = static_cast<CUnit*>(d->id);

		if (unit->GetEntity())
			// HACK: I don't know the proper way of undoably deleting entities...
			unit->GetEntity()->m_destroyed = true;

		g_UnitMan.RemoveUnit(unit);

		m_ObjectAlive = false;
	}

	void Undo()
	{
		if (! d->id)
			return;

		CUnit* unit = static_cast<CUnit*>(d->id);

		if (unit->GetEntity())
			unit->GetEntity()->m_destroyed = false;

		g_UnitMan.AddUnit(unit);

		m_ObjectAlive = true;
	}

END_COMMAND(DeleteObject)


}
