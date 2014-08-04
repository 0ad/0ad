/* Copyright (C) 2013 Wildfire Games.
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

#include "../GameLoop.h"
#include "../View.h"
#include "graphics/CinemaTrack.h"
#include "graphics/GameView.h"
#include "gui/GUIManager.h"
#include "gui/GUI.h"
#include "lib/external_libraries/libsdl.h"
#include "lib/sysdep/cpu.h"
#include "maths/MathUtil.h"
#include "ps/Game.h"
#include "ps/Util.h"
#include "ps/GameSetup/Config.h"
#include "ps/GameSetup/GameSetup.h"
#include "renderer/Renderer.h"

extern void (*Atlas_GLSwapBuffers)(void* context);

namespace AtlasMessage {

MESSAGEHANDLER(MessageTrace)
{
	((MessagePasserImpl*)g_MessagePasser)->SetTrace(msg->enable);
}

MESSAGEHANDLER(Screenshot)
{
	if (msg->big)
		WriteBigScreenshot(L".bmp", msg->tiles);
	else
		WriteScreenshot(L".png");
}

QUERYHANDLER(CinemaRecord)
{
	CCinemaManager* manager = g_Game->GetView()->GetCinema();
	manager->SetCurrentPath(*msg->path, false, false);

	const int w = msg->width, h = msg->height;

	{
		g_Renderer.Resize(w, h);
		SViewPort vp = { 0, 0, w, h };
		g_Game->GetView()->GetCamera()->SetViewPort(vp);
		g_Game->GetView()->SetCameraProjection();
	}

	unsigned char* img = new unsigned char [w*h*3];
	unsigned char* temp = new unsigned char[w*3];

	int num_frames = msg->framerate * msg->duration;

	AtlasView::GetView_Game()->SaveState(L"cinema_record");

	// Set it to update the simulation at normal speed
	AtlasView::GetView_Game()->SetSpeedMultiplier(1.f);

	for (int frame = 0; frame < num_frames; ++frame)
	{
		AtlasView::GetView_Game()->Update(1.f / msg->framerate);

		manager->MoveToPointAt((float)frame/msg->framerate);
		Render();
		Atlas_GLSwapBuffers((void*)g_AtlasGameLoop->glCanvas);

		glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, img);

		// Swap the rows around, else the image will be upside down
//* // TODO: BGR24 output doesn't need flipping, YUV420 and RGBA32 do
		for (int y = 0; y < h/2; ++y)
		{
			memcpy(temp, &img[y*w*3], w*3);
			memcpy(&img[y*w*3], &img[(h-1-y)*w*3], w*3);
			memcpy(&img[(h-1-y)*w*3], temp, w*3);
		}
//*/

		// Call the user-supplied function with this data, so they can
		// store it as a video
		sCinemaRecordCB cbdata = { img };
		msg->cb.Call(cbdata);
	}

	// Pause the game once we've finished
	AtlasView::GetView_Game()->SetSpeedMultiplier(0.f);

	AtlasView::GetView_Game()->RestoreState(L"cinema_record");
	// TODO: delete the saved state now that we don't need it any more

	delete[] img;
	delete[] temp;

	// Restore viewport
	{
		g_Renderer.Resize(g_xres, g_yres);
		SViewPort vp = { 0, 0, g_xres, g_yres };
		g_Game->GetView()->GetCamera()->SetViewPort(vp);
		g_Game->GetView()->SetCameraProjection();
	}

}

QUERYHANDLER(Ping)
{
	UNUSED2(msg);
}

MESSAGEHANDLER(SimStateSave)
{
	AtlasView::GetView_Game()->SaveState(*msg->label);
}

MESSAGEHANDLER(SimStateRestore)
{
	AtlasView::GetView_Game()->RestoreState(*msg->label);
}

QUERYHANDLER(SimStateDebugDump)
{
	msg->dump = AtlasView::GetView_Game()->DumpState(msg->binary);
}

MESSAGEHANDLER(SimPlay)
{
	AtlasView::GetView_Game()->SetSpeedMultiplier(msg->speed);
	AtlasView::GetView_Game()->SetTesting(msg->simTest);
}

MESSAGEHANDLER(JavaScript)
{
	g_GUI->GetActiveGUI()->GetScriptInterface()->LoadGlobalScript(L"Atlas", *msg->command);
}

MESSAGEHANDLER(GuiSwitchPage)
{
	g_GUI->SwitchPage(*msg->page, NULL, JS::UndefinedHandleValue);
}

MESSAGEHANDLER(GuiMouseButtonEvent)
{
	SDL_Event_ ev = { { 0 } };
	ev.ev.type = msg->pressed ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
	ev.ev.button.button = msg->button;
	ev.ev.button.state = msg->pressed ? SDL_PRESSED : SDL_RELEASED;
	float x, y;
	msg->pos->GetScreenSpace(x, y);
	ev.ev.button.x = (u16)clamp((int)x, 0, g_xres);
	ev.ev.button.y = (u16)clamp((int)y, 0, g_yres);
	in_dispatch_event(&ev);
}

MESSAGEHANDLER(GuiMouseMotionEvent)
{
	SDL_Event_ ev = { { 0 } };
	ev.ev.type = SDL_MOUSEMOTION;
	float x, y;
	msg->pos->GetScreenSpace(x, y);
	ev.ev.motion.x = (u16)clamp((int)x, 0, g_xres);
	ev.ev.motion.y = (u16)clamp((int)y, 0, g_yres);
	in_dispatch_event(&ev);
}

MESSAGEHANDLER(GuiKeyEvent)
{
	SDL_Event_ ev = { { 0 } };
	ev.ev.type = msg->pressed ? SDL_KEYDOWN : SDL_KEYUP;
#if SDL_VERSION_ATLEAST(2, 0, 0)
	ev.ev.key.keysym.sym = (SDL_Keycode)(int)msg->sdlkey;
#else
	ev.ev.key.keysym.sym = (SDLKey)(int)msg->sdlkey;
#endif
	ev.ev.key.keysym.unicode = msg->unichar;
	in_dispatch_event(&ev);
}

MESSAGEHANDLER(GuiCharEvent)
{
	// wxWidgets has special Char events but SDL doesn't, so convert it to
	// a keydown+keyup sequence. (We do the conversion here instead of on
	// the wx side to avoid nondeterministic behaviour caused by async messaging.)

	SDL_Event_ ev = { { 0 } };
	ev.ev.type = SDL_KEYDOWN;
#if SDL_VERSION_ATLEAST(2, 0, 0)
	ev.ev.key.keysym.sym = (SDL_Keycode)(int)msg->sdlkey;
#else
	ev.ev.key.keysym.sym = (SDLKey)(int)msg->sdlkey;
#endif
	ev.ev.key.keysym.unicode = msg->unichar;
	in_dispatch_event(&ev);

	ev.ev.type = SDL_KEYUP;
	in_dispatch_event(&ev);
}

}
