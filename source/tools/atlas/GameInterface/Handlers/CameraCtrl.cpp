#include "precompiled.h"

#include "MessageHandler.h"
#include "../GameLoop.h"

#include "maths/Vector3D.h"
#include "maths/Quaternion.h"
#include "ps/Game.h"
#include "renderer/Renderer.h"

#include <assert.h>

namespace AtlasMessage {


MESSAGEHANDLER(ScrollConstant)
{
	if (msg->dir < 0 || msg->dir > 3)
	{
		debug_warn("ScrollConstant: invalid direction");
	}
	else
	{
		g_GameLoop->input.scrollSpeed[msg->dir] = msg->speed;
	}
}

MESSAGEHANDLER(Scroll)
{
	static CVector3D targetPos;
	static float targetDistance = 0.f;

	CMatrix3D& camera = g_Game->GetView()->GetCamera()->m_Orientation;

	static CVector3D lastCameraPos = camera.GetTranslation();

	// Ensure roughly correct motion when dragging is combined with other
	// movements
	if (lastCameraPos != camera.GetTranslation())
		targetPos += camera.GetTranslation() - lastCameraPos;

	if (msg->type == mScroll::FROM)
	{
		msg->pos.GetWorldSpace(targetPos);
		targetDistance = (targetPos - camera.GetTranslation()).GetLength();
	}
	else if (msg->type == mScroll::TO)
	{
		CVector3D origin, dir;
		float x, y;
		msg->pos.GetScreenSpace(x, y);
		g_Game->GetView()->GetCamera()->BuildCameraRay(x, y, origin, dir);
		dir *= targetDistance;
		camera.Translate(targetPos - dir - origin);
	}
	else
	{
		debug_warn("Scroll: Invalid type");
	}
	lastCameraPos = camera.GetTranslation();
}

MESSAGEHANDLER(SmoothZoom)
{
	g_GameLoop->input.zoomDelta += msg->amount;
}

MESSAGEHANDLER(RotateAround)
{
	static CVector3D focusPos;
	static float lastX = 0.f, lastY = 0.f;

	CMatrix3D& camera = g_Game->GetView()->GetCamera()->m_Orientation;

	if (msg->type == mRotateAround::FROM)
	{
		msg->pos.GetScreenSpace(lastX, lastY);
		msg->pos.GetWorldSpace(focusPos);
	}
	else if (msg->type == mRotateAround::TO)
	{
		float x, y;
		msg->pos.GetScreenSpace(x, y);

		float rotX = 6.f * (y-lastY) / g_Renderer.GetHeight();
		float rotY = 6.f * (x-lastX) / g_Renderer.GetWidth();

		// TODO: Stop the camera tilting

		CQuaternion q0, q1;
		q0.FromAxisAngle(camera.GetLeft(), -rotX);
		q1.FromAxisAngle(CVector3D(0.f, 1.f, 0.f), rotY);
		CQuaternion q = q0*q1;

		CVector3D origin = camera.GetTranslation();
		CVector3D offset = q.Rotate(origin - focusPos);

		q *= camera.GetRotation();
		q.Normalize(); // to avoid things blowing up for some reason when turning upside-down
		q.ToMatrix(camera);
		camera.Translate(focusPos + offset);

		lastX = x;
		lastY = y;
	}
	else
	{
		debug_warn("RotateAround: Invalid type");
	}
}


}
