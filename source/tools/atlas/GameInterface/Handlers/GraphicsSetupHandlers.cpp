/* Copyright (C) 2022 Wildfire Games.
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
#include "../CommandProc.h"
#include "../ActorViewer.h"
#include "../View.h"
#include "../InputProcessor.h"

#include "graphics/GameView.h"
#include "graphics/ObjectManager.h"
#include "gui/GUIManager.h"
#include "lib/external_libraries/libsdl.h"
#include "lib/timer.h"
#include "maths/MathUtil.h"
#include "ps/CConsole.h"
#include "ps/CLogger.h"
#include "ps/Filesystem.h"
#include "ps/Profile.h"
#include "ps/Profiler2.h"
#include "ps/Game.h"
#include "ps/VideoMode.h"
#include "ps/GameSetup/Config.h"
#include "ps/GameSetup/GameSetup.h"
#include "renderer/Renderer.h"
#include "renderer/SceneRenderer.h"

#if OS_WIN
// We don't include wutil header directly to prevent including Windows headers.
extern void wutil_SetAppWindow(void* hwnd);
#endif

namespace AtlasMessage
{

InputProcessor g_Input;

// This keeps track of the last in-game user input.
// It is used to throttle FPS to save CPU & GPU.
double last_user_activity;

// see comment in GameLoop.cpp about ah_display_error before using INIT_HAVE_DISPLAY_ERROR
const int g_InitFlags = INIT_HAVE_VMODE | INIT_NO_GUI;

MESSAGEHANDLER(Init)
{
	UNUSED2(msg);

	g_Quickstart = true;

	// Mount mods if there are any specified as command line parameters
	if (!Init(g_AtlasGameLoop->args, g_InitFlags | INIT_MODS| INIT_MODS_PUBLIC))
	{
		// There are no mods specified on the command line,
		// but there are in the config file, so mount those.
		Shutdown(SHUTDOWN_FROM_CONFIG);
		ENSURE(Init(g_AtlasGameLoop->args, g_InitFlags));
	}

	// Initialise some graphics state for Atlas.
	// (This must be done after Init loads the config DB,
	// but before the UI constructs its GL canvases.)
	g_VideoMode.InitNonSDL();
}

MESSAGEHANDLER(InitAppWindow)
{
#if OS_WIN
	wutil_SetAppWindow(msg->handle);
#else
	UNUSED2(msg);
#endif
}

MESSAGEHANDLER(InitSDL)
{
	UNUSED2(msg);

	// When using GLX (Linux), SDL has to load the GL library to find
	// glXGetProcAddressARB before it can load any extensions.
	// When running in Atlas, we skip the SDL video initialisation code
	// which loads the library, and so SDL_GL_GetProcAddress fails (in
	// ogl.cpp importExtensionFunctions).
	// So, make sure it's loaded:
	SDL_InitSubSystem(SDL_INIT_VIDEO);
	// wxWidgets doesn't use a proper approach to dynamically load functions and
	// doesn't provide GetProcAddr-like function. Technically we need to call
	// SDL_GL_LoadLibrary inside GL device creation, but that might lead to a
	// crash on Windows because of a wrong order of initialization between SDL
	// and wxWidgets context management. So leave the call as is while it works.
	// Refs:
	//   http://trac.wxwidgets.org/ticket/9213
	//   http://trac.wxwidgets.org/ticket/9215
	if (SDL_GL_LoadLibrary(nullptr) && g_Logger)
		LOGERROR("SDL failed to load GL library: '%s'", SDL_GetError());
}

MESSAGEHANDLER(InitGraphics)
{
	UNUSED2(msg);

	g_VideoMode.CreateBackendDevice(false);

	InitGraphics(g_AtlasGameLoop->args, g_InitFlags, {});
}


MESSAGEHANDLER(Shutdown)
{
	UNUSED2(msg);

	// Empty the CommandProc, to get rid of its references to entities before
	// we kill the EntityManager
	GetCommandProc().Destroy();

	AtlasView::DestroyViews();
	g_AtlasGameLoop->view = AtlasView::GetView_None();

	int flags = 0;
	Shutdown(flags);
}


QUERYHANDLER(Exit)
{
	UNUSED2(msg);
	g_AtlasGameLoop->running = false;
}


MESSAGEHANDLER(RenderEnable)
{
	g_AtlasGameLoop->view->SetEnabled(false);
	g_AtlasGameLoop->view = AtlasView::GetView(msg->view);
	g_AtlasGameLoop->view->SetEnabled(true);
}

MESSAGEHANDLER(SetViewParamB)
{
	AtlasView* view = AtlasView::GetView(msg->view);
	view->SetParam(*msg->name, msg->value);
}

MESSAGEHANDLER(SetViewParamI)
{
	AtlasView* view = AtlasView::GetView(msg->view);
	view->SetParam(*msg->name, msg->value);
}

MESSAGEHANDLER(SetViewParamC)
{
	AtlasView* view = AtlasView::GetView(msg->view);
	view->SetParam(*msg->name, msg->value);
}

MESSAGEHANDLER(SetViewParamS)
{
	AtlasView* view = AtlasView::GetView(msg->view);
	view->SetParam(*msg->name, *msg->value);
}

MESSAGEHANDLER(SetActorViewer)
{
	if (msg->flushcache)
	{
		// TODO EXTREME DANGER: this'll break horribly if any units remain
		// in existence and use their actors after we've deleted all the actors.
		// (The actor viewer currently only has one unit at a time, so it's
		// alright.)
		// Should replace this with proper actor hot-loading system, or something.

		AtlasView::GetView_Actor()->GetActorViewer().SetActor(L"", "", -1);
		AtlasView::GetView_Actor()->GetActorViewer().UnloadObjects();
//		vfs_reload_changed_files();
	}
	AtlasView::GetView_Actor()->SetSpeedMultiplier(msg->speed);
	AtlasView::GetView_Actor()->GetActorViewer().SetActor(*msg->id, *msg->animation, msg->playerID);
}

//////////////////////////////////////////////////////////////////////////

MESSAGEHANDLER(SetCanvas)
{
	// Need to set the canvas size before possibly doing any rendering,
	// else we'll get GL errors when trying to render to 0x0
	CVideoMode::UpdateRenderer(msg->width, msg->height);

	g_AtlasGameLoop->glCanvas = msg->canvas;
	Atlas_GLSetCurrent(const_cast<void*>(g_AtlasGameLoop->glCanvas));
}


MESSAGEHANDLER(ResizeScreen)
{
	CVideoMode::UpdateRenderer(msg->width, msg->height);

#if OS_MACOSX
	// OS X seems to require this to update the GL canvas
	Atlas_GLSetCurrent(const_cast<void*>(g_AtlasGameLoop->glCanvas));
#endif
}

QUERYHANDLER(RenderLoop)
{
	{
		const double time = timer_Time();
		static double last_time = time;
		const double realFrameLength = time-last_time;
		last_time = time;
		ENSURE(realFrameLength >= 0.0);
		// TODO: filter out big jumps, e.g. when having done a lot of slow
		// processing in the last frame
		g_AtlasGameLoop->realFrameLength = realFrameLength;
	}

	if (g_Input.ProcessInput(g_AtlasGameLoop))
		last_user_activity = timer_Time();

	msg->timeSinceActivity = timer_Time() - last_user_activity;

	ReloadChangedFiles();

	RendererIncrementalLoad();

	// Pump SDL events (e.g. hotkeys)
	SDL_Event_ ev;
	while (in_poll_priority_event(&ev))
		in_dispatch_event(&ev);

	if (g_GUI)
		g_GUI->TickObjects();

	g_AtlasGameLoop->view->Update(g_AtlasGameLoop->realFrameLength);

	g_AtlasGameLoop->view->Render();

	if (CProfileManager::IsInitialised())
		g_Profiler.Frame();

	msg->wantHighFPS = g_AtlasGameLoop->view->WantsHighFramerate();
}

//////////////////////////////////////////////////////////////////////////

MESSAGEHANDLER(RenderStyle)
{
	g_Renderer.GetSceneRenderer().SetTerrainRenderMode(msg->wireframe ? EDGED_FACES : SOLID);
	g_Renderer.GetSceneRenderer().SetWaterRenderMode(msg->wireframe ? EDGED_FACES : SOLID);
	g_Renderer.GetSceneRenderer().SetModelRenderMode(msg->wireframe ? EDGED_FACES : SOLID);
	g_Renderer.GetSceneRenderer().SetOverlayRenderMode(msg->wireframe ? EDGED_FACES : SOLID);
}

} // namespace AtlasMessage
