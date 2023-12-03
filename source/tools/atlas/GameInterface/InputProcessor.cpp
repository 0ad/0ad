/* Copyright (C) 2010 Wildfire Games.
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

#include "InputProcessor.h"

#include "ps/Game.h"
#include "graphics/Camera.h"
#include "graphics/GameView.h"
#include "maths/Quaternion.h"

static float g_ViewZoomSmoothness = 0.02f;
static float g_ViewRotateScale = 0.02f;

static void Rotate(CCamera& camera, float speed)
{
	CVector3D upwards(0.0f, 1.0f, 0.0f);
	CVector3D origin = camera.m_Orientation.GetTranslation();
	CVector3D pivot = camera.GetFocus();
	CVector3D delta = origin - pivot;

	CQuaternion r;
	r.FromAxisAngle(upwards, speed);
	delta = r.Rotate(delta);
	camera.m_Orientation.Translate(-origin);
	camera.m_Orientation.Rotate(r);
	camera.m_Orientation.Translate(pivot + delta);
}

bool InputProcessor::ProcessInput(GameLoopState* state)
{
	if (! g_Game)
		return false;

	CCamera* camera = g_Game->GetView()->GetCamera();

	CVector3D leftwards = camera->m_Orientation.GetLeft();

	CVector3D inwards = camera->m_Orientation.GetIn();

	// Calculate a vector pointing forwards, parallel to the ground
	CVector3D forwards = inwards;
	forwards.Y = 0.0f;
	if (forwards.Length() < 0.001f) // be careful if the camera is looking straight down
		forwards = CVector3D(1.f, 0.f, 0.f);
	else
		forwards.Normalize();

	bool moved = false;

	GameLoopState::Input& input = state->input;

	if (state->input.scrollSpeed[0] != 0.0f)
	{
		camera->m_Orientation.Translate(forwards * (input.scrollSpeed[0] * state->realFrameLength));
		moved = true;
	}

	if (state->input.scrollSpeed[1] != 0.0f)
	{
		camera->m_Orientation.Translate(forwards * (-input.scrollSpeed[1] * state->realFrameLength));
		moved = true;
	}

	if (state->input.scrollSpeed[2] != 0.0f)
	{
		camera->m_Orientation.Translate(leftwards * (input.scrollSpeed[2] * state->realFrameLength));
		moved = true;
	}

	if (state->input.scrollSpeed[3] != 0.0f)
	{
		camera->m_Orientation.Translate(leftwards * (-input.scrollSpeed[3] * state->realFrameLength));
		moved = true;
	}

	if (state->input.scrollSpeed[4] != 0.0f)
	{
		Rotate(*camera, input.scrollSpeed[4] * state->realFrameLength * g_ViewRotateScale);
		moved = true;
	}

	if (state->input.scrollSpeed[5] != 0.0f)
	{
		Rotate(*camera, -input.scrollSpeed[5] * state->realFrameLength * g_ViewRotateScale);
		moved = true;
	}

	if (state->input.zoomDelta != 0.0f)
	{
		float zoom_proportion = powf(g_ViewZoomSmoothness, state->realFrameLength);
		camera->m_Orientation.Translate(inwards * (input.zoomDelta * (1.0f - zoom_proportion)));
		input.zoomDelta *= zoom_proportion;

		if (fabsf(input.zoomDelta) < 0.1f)
			input.zoomDelta = 0.0f;

		moved = true;
	}

	if (moved)
		camera->UpdateFrustum();

	return moved;
}
