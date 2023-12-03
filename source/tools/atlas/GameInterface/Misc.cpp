/* Copyright (C) 2009 Wildfire Games.
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
		debug_warn(L"Invalid Position acquisition (unchanged without previous)");
		return CVector3D(0.f, 0.f, 0.f);
		break;

	default:
		debug_warn(L"Invalid Position type");
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
		debug_warn(L"Invalid Position type");
		x = y = 0.f;
	}
}
