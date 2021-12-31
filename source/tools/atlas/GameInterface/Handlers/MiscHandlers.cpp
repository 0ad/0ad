/* Copyright (C) 2021 Wildfire Games.
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
#include "graphics/GameView.h"
#include "gui/GUIManager.h"
#include "gui/CGUI.h"
#include "lib/external_libraries/libsdl.h"
#include "maths/MathUtil.h"
#include "ps/Game.h"
#include "ps/Util.h"
#include "ps/GameSetup/Config.h"
#include "ps/GameSetup/GameSetup.h"
#include "scriptinterface/ScriptInterface.h"
#include "simulation2/Simulation2.h"
#include "simulation2/components/ICmpSoundManager.h"

extern void (*Atlas_GLSwapBuffers)(void* context);

namespace AtlasMessage
{

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

QUERYHANDLER(Ping)
{
	UNUSED2(msg);
}

MESSAGEHANDLER(SimStopMusic)
{
	UNUSED2(msg);

	CmpPtr<ICmpSoundManager> cmpSoundManager(*g_Game->GetSimulation2(), SYSTEM_ENTITY);
	if (cmpSoundManager)
		cmpSoundManager->StopMusic();
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
	ev.ev.button.clicks = msg->clicks;
	float x, y;
	msg->pos->GetScreenSpace(x, y);
	ev.ev.button.x = static_cast<u16>(Clamp<int>(x, 0, g_xres));
	ev.ev.button.y = static_cast<u16>(Clamp<int>(y, 0, g_yres));
	in_dispatch_event(&ev);
}

MESSAGEHANDLER(GuiMouseMotionEvent)
{
	SDL_Event_ ev = { { 0 } };
	ev.ev.type = SDL_MOUSEMOTION;
	float x, y;
	msg->pos->GetScreenSpace(x, y);
	ev.ev.motion.x = static_cast<u16>(Clamp<int>(x, 0, g_xres));
	ev.ev.motion.y = static_cast<u16>(Clamp<int>(y, 0, g_yres));
	in_dispatch_event(&ev);
}

MESSAGEHANDLER(GuiKeyEvent)
{
	SDL_Event_ ev = { { 0 } };
	ev.ev.type = msg->pressed ? SDL_KEYDOWN : SDL_KEYUP;
	ev.ev.key.keysym.sym = (SDL_Keycode)(int)msg->sdlkey;
	ev.ev.key.keysym.scancode = SDL_GetScancodeFromKey((SDL_Keycode)(int)msg->sdlkey);
	in_dispatch_event(&ev);
}

MESSAGEHANDLER(GuiCharEvent)
{
	// Simulate special 'text input' events in the SDL
	// This isn't quite compatible with WXWidget's handling,
	// so to avoid trouble we only send 'letter-like' ASCII input.
	SDL_Event_ ev = { { 0 } };
	ev.ev.type = SDL_TEXTEDITING;
	ev.ev.text.type = SDL_TEXTEDITING;
	ev.ev.text.text[0] = (char)msg->sdlkey;
	ev.ev.text.text[1] = (char)0;
	in_dispatch_event(&ev);

	ev.ev.type = SDL_TEXTINPUT;
	ev.ev.text.type = SDL_TEXTINPUT;
	ev.ev.text.text[0] = (char)msg->sdlkey;
	ev.ev.text.text[1] = (char)0;
	in_dispatch_event(&ev);
}

} // namespace AtlasMessage
