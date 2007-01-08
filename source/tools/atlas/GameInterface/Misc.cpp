#include "precompiled.h"

// TODO: organise things better, rather than sticking them in Misc

#include "Messages.h"

#include "maths/Vector3D.h"
#include "ps/Game.h"
#include "graphics/GameView.h"
#include "graphics/Camera.h"

CVector3D AtlasMessage::Position::GetWorldSpace(bool floating) const
{
	switch (type)
	{
	case 0:
		return CVector3D(type0.x, type0.y, type0.z);
		break;

	case 1:
		return g_Game->GetView()->GetCamera()->GetWorldCoordinates(type1.x, type1.y, floating);
		break;

	case 2:
		debug_warn("Invalid Position acquisition (unchanged without previous)");
		return CVector3D(0.f, 0.f, 0.f);
		break;

	default:
		debug_warn("Invalid Position type");
		return CVector3D(0.f, 0.f, 0.f);
	}
}

CVector3D AtlasMessage::Position::GetWorldSpace(float h, bool floating) const
{
	switch (type)
	{
	case 1:
		return g_Game->GetView()->GetCamera()->GetWorldCoordinates(type1.x, type1.y, h);

	default:
		return GetWorldSpace(floating);
	}
}

CVector3D AtlasMessage::Position::GetWorldSpace(const CVector3D& prev, bool floating) const
{
	switch (type)
	{
	case 2:
		return prev;

	default:
		return GetWorldSpace(floating);
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
