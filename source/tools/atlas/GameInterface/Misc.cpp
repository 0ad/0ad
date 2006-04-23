#include "precompiled.h"

// TODO: organise things better, rather than sticking them in Misc

#include "Messages.h"

#include "maths/Vector3D.h"
#include "ps/Game.h"
#include "graphics/GameView.h"

void AtlasMessage::Position::GetWorldSpace(CVector3D& vec) const
{
	switch (type)
	{
	case 0:
		vec.Set(type0.x, type0.y, type0.z);
		break;

	case 1:
		vec = g_Game->GetView()->GetCamera()->GetWorldCoordinates(type1.x, type1.y);
		break;

	case 2:
		debug_warn("Invalid Position acquisition (unchanged without previous)");
		vec.Set(0.f, 0.f, 0.f);
		break;

	default:
		debug_warn("Invalid Position type");
		vec.Set(0.f, 0.f, 0.f);
	}
}

void AtlasMessage::Position::GetWorldSpace(CVector3D& vec, float h) const
{
	switch (type)
	{
	case 1:
		vec = g_Game->GetView()->GetCamera()->GetWorldCoordinates(type1.x, type1.y, h);
		break;

	default:
		GetWorldSpace(vec);
	}
}

void AtlasMessage::Position::GetWorldSpace(CVector3D& vec, const CVector3D& prev) const
{
	switch (type)
	{
	case 2:
		vec = prev;
		break;

	default:
		GetWorldSpace(vec);
	}
}

void AtlasMessage::Position::GetScreenSpace(float& x, float& y) const
{
	switch (type)
	{
	case 0:
		g_Game->GetView()->GetCamera()->GetScreenCoordinates(CVector3D(type0.x, type0.y, type0.x), x, y);
		break;

	case 1:
		x = type1.x;
		y = type1.y;
		break;

	default:
		debug_warn("Invalid Position type");
		x = y = 0.f;
	}
}
