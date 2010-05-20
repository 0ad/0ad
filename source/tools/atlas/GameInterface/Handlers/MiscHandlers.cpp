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
#include "../MessagePasserImpl.h"

#include "ps/GameSetup/Config.h"
#include "ps/Util.h"

#include "ps/Game.h"
#include "graphics/GameView.h"
#include "graphics/CinemaTrack.h"
#include "lib/sysdep/cpu.h"
#include "renderer/Renderer.h"
#include "ps/GameSetup/GameSetup.h"
#include "../GameLoop.h"
#include "../View.h"

extern void (*Atlas_GLSwapBuffers)(void* context);

namespace AtlasMessage {

MESSAGEHANDLER(MessageTrace)
{
	((MessagePasserImpl*)g_MessagePasser)->SetTrace(msg->enable);
}

MESSAGEHANDLER(Screenshot)
{
	// TODO: allow non-big screenshots too
	WriteBigScreenshot(L".bmp", msg->tiles);
}

QUERYHANDLER(CinemaRecord)
{
	CCinemaManager* manager = g_Game->GetView()->GetCinema();
	manager->SetCurrentPath(*msg->path, false, false);

	const int w = msg->width, h = msg->height;

	{
		g_Renderer.Resize(w, h);
		SViewPort vp = { 0, 0, w, h };
		g_Game->GetView()->GetCamera()->SetViewPort(&vp);
		g_Game->GetView()->GetCamera()->SetProjection(CGameView::defaultNear, CGameView::defaultFar, CGameView::defaultFOV);
	}

	unsigned char* img = new unsigned char [w*h*3];
	unsigned char* temp = new unsigned char[w*3];

	int num_frames = msg->framerate * msg->duration;

	View::GetView_Game()->SaveState(L"cinema_record");

	// Set it to update the simulation at normal speed
	View::GetView_Game()->SetSpeedMultiplier(1.f);

	for (int frame = 0; frame < num_frames; ++frame)
	{
		View::GetView_Game()->Update(1.f / msg->framerate);

		manager->MoveToPointAt((float)frame/msg->framerate);
		Render();
		Atlas_GLSwapBuffers((void*)g_GameLoop->glCanvas);

		glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, img);

		// Swap the rows around, else the image will be upside down
//* // TODO: BGR24 output doesn't need flipping, YUV420 and RGBA32 do
		for (int y = 0; y < h/2; ++y)
		{
			cpu_memcpy(temp, &img[y*w*3], w*3);
			cpu_memcpy(&img[y*w*3], &img[(h-1-y)*w*3], w*3);
			cpu_memcpy(&img[(h-1-y)*w*3], temp, w*3);
		}
//*/

		// Call the user-supplied function with this data, so they can
		// store it as a video
		sCinemaRecordCB cbdata = { img };
		msg->cb.Call(cbdata);
	}

	// Pause the game once we've finished
	View::GetView_Game()->SetSpeedMultiplier(0.f);

	View::GetView_Game()->RestoreState(L"cinema_record");
	// TODO: delete the saved state now that we don't need it any more

	delete[] img;
	delete[] temp;

	// Restore viewport
	{
		g_Renderer.Resize(g_xres, g_yres);
		SViewPort vp = { 0, 0, g_xres, g_yres };
		g_Game->GetView()->GetCamera()->SetViewPort(&vp);
		g_Game->GetView()->GetCamera()->SetProjection(CGameView::defaultNear, CGameView::defaultFar, CGameView::defaultFOV);
	}

}

QUERYHANDLER(Ping)
{
	UNUSED2(msg);
}

MESSAGEHANDLER(SimStateSave)
{
	View::GetView_Game()->SaveState(*msg->label);
}

MESSAGEHANDLER(SimStateRestore)
{
	View::GetView_Game()->RestoreState(*msg->label);
}

QUERYHANDLER(SimStateDebugDump)
{
	msg->dump = View::GetView_Game()->DumpState(msg->binary);
}

MESSAGEHANDLER(SimPlay)
{
	View::GetView_Game()->SetSpeedMultiplier(msg->speed);
}

MESSAGEHANDLER(JavaScript)
{
	g_ScriptingHost.ExecuteScript(*msg->command, L"Atlas");
}


}
