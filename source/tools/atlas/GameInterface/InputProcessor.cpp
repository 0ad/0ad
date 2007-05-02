#include "precompiled.h"

#include "InputProcessor.h"

#include "ps/Game.h"
#include "graphics/Camera.h"
#include "graphics/GameView.h"

static float g_ViewZoomSmoothness = 0.02f; // TODO: configurable, like GameView

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
		camera->m_Orientation.Translate(forwards * (input.scrollSpeed[0] * state->frameLength));
		moved = true;
	}

	if (state->input.scrollSpeed[1] != 0.0f)
	{
		camera->m_Orientation.Translate(forwards * (-input.scrollSpeed[1] * state->frameLength));
		moved = true;
	}

	if (state->input.scrollSpeed[2] != 0.0f)
	{
		camera->m_Orientation.Translate(leftwards * (input.scrollSpeed[2] * state->frameLength));
		moved = true;
	}

	if (state->input.scrollSpeed[3] != 0.0f)
	{
		camera->m_Orientation.Translate(leftwards * (-input.scrollSpeed[3] * state->frameLength));
		moved = true;
	}

	if (state->input.zoomDelta != 0.0f)
	{
		float zoom_proportion = powf(g_ViewZoomSmoothness, state->frameLength);
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
