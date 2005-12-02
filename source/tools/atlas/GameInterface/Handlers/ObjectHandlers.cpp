#include "precompiled.h"

#include "MessageHandler.h"

#include "simulation/BaseEntityCollection.h"
#include "graphics/Unit.h"
#include "graphics/UnitManager.h"
#include "graphics/Model.h"
#include "maths/Matrix3D.h"

namespace AtlasMessage {

QUERYHANDLER(GetEntitiesList)
{
	std::vector<CStrW> names;
	g_EntityTemplateCollection.getBaseEntityNames(names);
	for (std::vector<CStrW>::iterator it = names.begin(); it != names.end(); ++it)
	{
		sEntitiesListItem e;
		e.name = *it;
		msg->entities.push_back(e);
	}
}


static CUnit* g_PreviewUnit = NULL;
static CStrW g_PreviewUnitName;

MESSAGEHANDLER(EntityPreview)
{
	if (msg->name != g_PreviewUnitName)
	{
		// Delete old unit
		if (g_PreviewUnit)
		{
			g_UnitMan.RemoveUnit(g_PreviewUnit);
			delete g_PreviewUnit;
			g_PreviewUnit = NULL;
		}

		if (msg->name.length())
		{
			// Create new unit
			CBaseEntity* base = g_EntityTemplateCollection.getTemplate(msg->name);
			if (base)
			{
				g_PreviewUnit = g_UnitMan.CreateUnit(base->m_actorName, 0);
				// TODO: set player (for colour)
				// TODO: variations
			}

		}
		g_PreviewUnitName = msg->name;
	}

	// Position/orient unit
	if (g_PreviewUnit)
	{
		static CVector3D pos;
		msg->pos.GetWorldSpace(pos, pos); // if msg->pos is 'Unchanged', use the previous pos

		float s, c;
/*
		if (msg->usetarget)
		{
			// TODO
			s=1; c=0;
		}
		else
*/
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


}
