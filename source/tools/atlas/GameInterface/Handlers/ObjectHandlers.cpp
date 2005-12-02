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

	void Construct()
	{
	}
	void Destruct()
	{
	}

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


}
