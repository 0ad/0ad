/* Copyright (C) 2019 Wildfire Games.
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

#include "graphics/GameView.h"
#include "graphics/ObjectManager.h"
#include "lib/external_libraries/libsdl.h"
#include "lib/ogl.h"
#include "maths/MathUtil.h"
#include "ps/CConsole.h"
#include "ps/Game.h"
#include "ps/VideoMode.h"
#include "ps/GameSetup/Config.h"
#include "ps/GameSetup/GameSetup.h"
#include "renderer/Renderer.h"

namespace AtlasMessage {

// see comment in GameLoop.cpp about ah_display_error before using INIT_HAVE_DISPLAY_ERROR
const int g_InitFlags = INIT_HAVE_VMODE|INIT_NO_GUI;

MESSAGEHANDLER(Init)
{
	UNUSED2(msg);

	g_Quickstart = true;

	// Mount mods if there are any specified as command line parameters
	if (!Init(g_AtlasGameLoop->args, g_InitFlags | INIT_MODS|INIT_MODS_PUBLIC))
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

MESSAGEHANDLER(InitSDL)
{
	UNUSED2(msg);

	// When using GLX (Linux), SDL has to load the GL library to find
	// glXGetProcAddressARB before it can load any extensions.
	// When running in Atlas, we skip the SDL video initialisation code
	// which loads the library, and so SDL_GL_GetProcAddress fails (in
	// ogl.cpp importExtensionFunctions).
	// (TODO: I think this is meant to be context-independent, i.e. it
	// doesn't matter that we're getting extensions from SDL-initialised
	// GL stuff instead of from the wxWidgets-initialised GL stuff, but that
	// should be checked.)
	// So, make sure it's loaded:
	SDL_InitSubSystem(SDL_INIT_VIDEO);
	SDL_GL_LoadLibrary(NULL); // NULL = use default
	// (it shouldn't hurt if this is called multiple times, I think)
}

MESSAGEHANDLER(InitGraphics)
{
	UNUSED2(msg);

	ogl_Init();

	InitGraphics(g_AtlasGameLoop->args, g_InitFlags, {});

#if OS_WIN
	// HACK (to stop things looking very ugly when scrolling) - should
	// use proper config system.
	if(ogl_HaveExtension("WGL_EXT_swap_control"))
		pwglSwapIntervalEXT(1);
#endif
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

//////////////////////////////////////////////////////////////////////////

MESSAGEHANDLER(RenderStyle)
{
	g_Renderer.SetTerrainRenderMode(msg->wireframe ? EDGED_FACES : SOLID);
	g_Renderer.SetWaterRenderMode(msg->wireframe ? EDGED_FACES : SOLID);
	g_Renderer.SetModelRenderMode(msg->wireframe ? EDGED_FACES : SOLID);
}

}
