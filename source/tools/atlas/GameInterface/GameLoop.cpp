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

#include "GameLoop.h"

#include "MessagePasserImpl.h"
#include "Messages.h"
#include "SharedMemory.h"
#include "Handlers/MessageHandler.h"
#include "ActorViewer.h"
#include "View.h"

#include "InputProcessor.h"

#include "graphics/TextureManager.h"
#include "lib/app_hooks.h"
#include "lib/external_libraries/libsdl.h"
#include "lib/timer.h"
#include "ps/CLogger.h"
#include "ps/DllLoader.h"
#include "ps/Filesystem.h"
#include "ps/Profile.h"
#include "ps/ThreadUtil.h"
#include "ps/GameSetup/Paths.h"
#include "renderer/Renderer.h"

using namespace AtlasMessage;

#include <thread>


namespace AtlasMessage
{
	extern void RegisterHandlers();
}

// Loaded from DLL:
void (*Atlas_StartWindow)(const wchar_t* type);
void (*Atlas_SetDataDirectory)(const wchar_t* path);
void (*Atlas_SetConfigDirectory)(const wchar_t* path);
void (*Atlas_SetMessagePasser)(MessagePasser*);
void (*Atlas_GLSetCurrent)(void* cavas);
void (*Atlas_GLSwapBuffers)(void* canvas);
void (*Atlas_DisplayError)(const wchar_t* text, size_t flags);
namespace AtlasMessage
{
	void* (*ShareableMallocFptr)(size_t);
	void (*ShareableFreeFptr)(void*);
}


MessagePasser* AtlasMessage::g_MessagePasser = NULL;


static GameLoopState state;
GameLoopState* g_AtlasGameLoop = &state;

void RendererIncrementalLoad()
{
	// TODO: shouldn't duplicate this code from main.cpp

	if (!CRenderer::IsInitialised())
		return;

	const double maxTime = 0.1f;

	double startTime = timer_Time();
	bool more;
	do {
		more = g_Renderer.GetTextureManager().MakeProgress();
	}
	while (more && timer_Time() - startTime < maxTime);
}

bool BeginAtlas(const CmdLineArgs& args, const DllLoader& dll)
{
	// Load required symbols from the DLL
	try
	{
		dll.LoadSymbol("Atlas_StartWindow", Atlas_StartWindow);
		dll.LoadSymbol("Atlas_SetMessagePasser", Atlas_SetMessagePasser);
		dll.LoadSymbol("Atlas_SetDataDirectory", Atlas_SetDataDirectory);
		dll.LoadSymbol("Atlas_SetConfigDirectory", Atlas_SetConfigDirectory);
		dll.LoadSymbol("Atlas_GLSetCurrent", Atlas_GLSetCurrent);
		dll.LoadSymbol("Atlas_GLSwapBuffers", Atlas_GLSwapBuffers);
		dll.LoadSymbol("Atlas_DisplayError", Atlas_DisplayError);
		dll.LoadSymbol("ShareableMalloc", ShareableMallocFptr);
		dll.LoadSymbol("ShareableFree", ShareableFreeFptr);
	}
	catch (PSERROR_DllLoader&)
	{
		debug_warn(L"Failed to initialise DLL");
		return false;
	}

	// Construct a message passer for communicating with Atlas
	// (here so that its scope lasts beyond the game thread)
	MessagePasserImpl msgPasser;
	AtlasMessage::g_MessagePasser = &msgPasser;

	// Pass our message handler to Atlas
	Atlas_SetMessagePasser(&msgPasser);

	// Tell Atlas the location of the data directory
	const Paths paths(args);
	Atlas_SetDataDirectory(paths.RData().string().c_str());

	// Tell Atlas the location of the user config directory
	Atlas_SetConfigDirectory(paths.Config().string().c_str());

	RegisterHandlers();

	// Disable the game's cursor rendering
	extern CStrW g_CursorName;
	g_CursorName = L"";

	state.args = args;
	state.running = true;
	state.view = AtlasView::GetView_None();
	state.glCanvas = NULL;

	// Start Atlas UI on main thread
	// (required for wxOSX/Cocoa compatibility - see http://trac.wildfiregames.com/ticket/500)
	Atlas_StartWindow(L"ScenarioEditor");

	// TODO: delete all remaining messages, to avoid memory leak warnings

	// Restore main thread
	Threading::SetMainThread();

	// Clean up
	AtlasView::DestroyViews();
	AtlasMessage::g_MessagePasser = NULL;

	return true;
}
