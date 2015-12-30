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

#include "MessageHandler.h"
#include "../GameLoop.h"
#include "../View.h"

#include "maths/MathUtil.h"
#include "maths/Vector3D.h"
#include "maths/Quaternion.h"
#include "ps/Game.h"
#include "renderer/Renderer.h"
#include "graphics/GameView.h"
#include "graphics/CinemaManager.h"

#include "ps/World.h"
#include "graphics/Terrain.h"

#include <assert.h>

namespace AtlasMessage {

MESSAGEHANDLER(CameraReset)
{
	if (!g_Game || g_Game->GetView()->GetCinema()->IsPlaying())
		return;

	CVector3D focus = g_Game->GetView()->GetCamera()->GetFocus();

	CVector3D target;
	if (!g_Game->GetWorld()->GetTerrain()->IsOnMap(focus.X, focus.Z))
	{
		target = CVector3D(
			g_Game->GetWorld()->GetTerrain()->GetMaxX()/2.f,
			focus.Y,
			g_Game->GetWorld()->GetTerrain()->GetMaxZ()/2.f);
	}
	else
	{
		target = focus;
	}

	g_Game->GetView()->ResetCameraTarget(target);

	UNUSED2(msg);
}

MESSAGEHANDLER(ScrollConstant)
{
	if (!g_Game || g_Game->GetView()->GetCinema()->IsPlaying())
		return;

	if (msg->dir < 0 || msg->dir > 5)
	{
		debug_warn(L"ScrollConstant: invalid direction");
	}
	else
	{
		g_AtlasGameLoop->input.scrollSpeed[msg->dir] = msg->speed;
	}
}

// TODO: change all these g_Game->...GetCamera() bits to use the current AtlasView's
// camera instead.

MESSAGEHANDLER(Scroll)
{
	if (!g_Game || g_Game->GetView()->GetCinema()->IsPlaying()) // TODO: do this better (probably a separate AtlasView class for cinematics)
		return;

	static CVector3D targetPos;
	static float targetDistance = 0.f;

	CMatrix3D& camera = g_Game->GetView()->GetCamera()->m_Orientation;

	static CVector3D lastCameraPos = camera.GetTranslation();

	// Ensure roughly correct motion when dragging is combined with other
	// movements.
	if (lastCameraPos != camera.GetTranslation())
		targetPos += camera.GetTranslation() - lastCameraPos;

	// General operation:
	//
	// When selecting a target point to drag, remember targetPos (a world-space
	// point on the terrain, underneath the mouse) and targetDistance (from the
	// camera to the target point).
	//
	// When dragging to a different position, the target point should remain
	// under the moved mouse; so calculate the ray through the camera and mouse,
	// multiply by targetDistance and add to targetPos, resulting in the required
	// camera position.

	if (msg->type == eScrollType::FROM)
	{
		targetPos = msg->pos->GetWorldSpace();
		targetDistance = (targetPos - camera.GetTranslation()).Length();
	}
	else if (msg->type == eScrollType::TO)
	{
		CVector3D origin, dir;
		float x, y;
		msg->pos->GetScreenSpace(x, y);
		g_Game->GetView()->GetCamera()->BuildCameraRay((int)x, (int)y, origin, dir);
		dir *= targetDistance;
		camera.Translate(targetPos - dir - origin);
		g_Game->GetView()->GetCamera()->UpdateFrustum();
	}
	else
	{
		debug_warn(L"Scroll: Invalid type");
	}
	lastCameraPos = camera.GetTranslation();
}

MESSAGEHANDLER(SmoothZoom)
{
	if (!g_Game || g_Game->GetView()->GetCinema()->IsPlaying())
		return;

	g_AtlasGameLoop->input.zoomDelta += msg->amount;
}

MESSAGEHANDLER(RotateAround)
{
	if (!g_Game || g_Game->GetView()->GetCinema()->IsPlaying())
		return;

	static CVector3D focusPos;
	static float lastX = 0.f, lastY = 0.f;

	CMatrix3D& camera = g_Game->GetView()->GetCamera()->m_Orientation;

	if (msg->type == eRotateAroundType::FROM)
	{
		msg->pos->GetScreenSpace(lastX, lastY); // get mouse position
		focusPos = msg->pos->GetWorldSpace(); // get point on terrain under mouse
	}
	else if (msg->type == eRotateAroundType::TO)
	{
		float x, y;
		msg->pos->GetScreenSpace(x, y); // get mouse position

		// Rotate around X and Y axes by amounts depending on the mouse delta
		float rotX = 6.f * (y-lastY) / g_Renderer.GetHeight();
		float rotY = 6.f * (x-lastX) / g_Renderer.GetWidth();

		CQuaternion q0, q1;
		q0.FromAxisAngle(camera.GetLeft(), -rotX);
		q1.FromAxisAngle(CVector3D(0.f, 1.f, 0.f), rotY);
		CQuaternion q = q0*q1;

		CVector3D origin = camera.GetTranslation();
		CVector3D offset = q.Rotate(origin - focusPos);

		q *= camera.GetRotation();
		q.Normalize(); // to avoid things blowing up when turning upside-down, for some reason I don't understand
		q.ToMatrix(camera);

		// Make sure up is still pointing up, regardless of any rounding errors.
		// (Maybe this distorts the camera in other ways, but at least the errors
		// are far less noticeable to me.)
		camera._21 = 0.f; // (_21 = Y component returned by GetLeft())

		camera.Translate(focusPos + offset);
		g_Game->GetView()->GetCamera()->UpdateFrustum();

		lastX = x;
		lastY = y;
	}
	else
	{
		debug_warn(L"RotateAround: Invalid type");
	}
}

MESSAGEHANDLER(LookAt)
{
	// TODO: different camera depending on msg->view
	CCamera& camera = AtlasView::GetView_Actor()->GetCamera();

	CVector3D tgt = msg->target->GetWorldSpace();
	CVector3D eye = msg->pos->GetWorldSpace();
 	tgt.Y = -tgt.Y; // ??? why is this needed?
 	eye.Y = -eye.Y; // ???

	// Based on http://www.opengl.org/documentation/specs/man_pages/hardcopy/GL/html/glu/lookat.html
	CVector3D f = tgt - eye;
	f.Normalize();
	CVector3D s = f.Cross(CVector3D(0, 1, 0));
	CVector3D u = s.Cross(f);
	s.Normalize(); // (not in that man page, but necessary for correctness, and done by Mesa)
	u.Normalize();
	CMatrix3D M (
		s[0], s[1], s[2], 0,
		u[0], u[1], u[2], 0,
		-f[0], -f[1], -f[2], 0,
		0, 0, 0, 1
	);

	camera.m_Orientation = M.GetTranspose();
	camera.m_Orientation.Translate(-eye);

	camera.UpdateFrustum();
}

QUERYHANDLER(GetView)
{
	if (!g_Game)
		return;

	CVector3D focus = g_Game->GetView()->GetCamera()->GetFocus();
	sCameraInfo info;

	info.pX = focus.X;
	info.pY = focus.Y;
	info.pZ = focus.Z;

	CQuaternion quatRot = g_Game->GetView()->GetCamera()->m_Orientation.GetRotation();
	quatRot.Normalize();
	CVector3D rotation = quatRot.ToEulerAngles();

	info.rX = RADTODEG(rotation.X); 
	info.rY = RADTODEG(rotation.Y);
	info.rZ = RADTODEG(rotation.Z);

	msg->info = info;
}

MESSAGEHANDLER(SetView)
{
	if (!g_Game || g_Game->GetView()->GetCinema()->IsPlaying())
		return;

	CGameView* view = g_Game->GetView();
	view->ResetCameraTarget(view->GetCamera()->GetFocus());

	sCameraInfo cam = msg->info;

	view->ResetCameraTarget(CVector3D(cam.pX, cam.pY, cam.pZ));

	// TODO: Rotation
}

}
