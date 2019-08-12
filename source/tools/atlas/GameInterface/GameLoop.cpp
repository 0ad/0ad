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

#include <thread>

#include "GameLoop.h"

#include "MessagePasserImpl.h"
#include "Messages.h"
#include "SharedMemory.h"
#include "Handlers/MessageHandler.h"
#include "ActorViewer.h"
#include "View.h"

#include "InputProcessor.h"

#include "graphics/TextureManager.h"
#include "gui/GUIManager.h"
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
void (*Atlas_NotifyEndOfFrame)();
void (*Atlas_DisplayError)(const wchar_t* text, size_t flags);
namespace AtlasMessage
{
	void* (*ShareableMallocFptr)(size_t);
	void (*ShareableFreeFptr)(void*);
}


MessagePasser* AtlasMessage::g_MessagePasser = NULL;


static InputProcessor g_Input;

static GameLoopState state;
GameLoopState* g_AtlasGameLoop = &state;


static ErrorReactionInternal AtlasDisplayError(const wchar_t* text, size_t flags)
{
	// TODO: after Atlas has been unloaded, don't do this
	Atlas_DisplayError(text, flags);

	return ERI_CONTINUE;
}

static void RendererIncrementalLoad()
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

static void RunEngine(const CmdLineArgs& args)
{
	debug_SetThreadName("engine_thread");

	// Set new main thread so that all the thread-safety checks pass
	ThreadUtil::SetMainThread();

	g_Profiler2.RegisterCurrentThread("atlasmain");

	MessagePasserImpl* msgPasser = (MessagePasserImpl*)AtlasMessage::g_MessagePasser;

	// Register all the handlers for message which might be passed back
	RegisterHandlers();

	// Override ah_display_error to pass all errors to the Atlas UI
	// TODO: this doesn't work well because it doesn't pause the game thread
	//  and the error box is ugly, so only use it if we fix those issues
	//  (use INIT_HAVE_DISPLAY_ERROR init flag to test this)
	AppHooks hooks = {0};
	hooks.display_error = AtlasDisplayError;
	app_hooks_update(&hooks);

	// Disable the game's cursor rendering
	extern CStrW g_CursorName;
	g_CursorName = L"";

	state.args = args;
	state.running = true;
	state.view = AtlasView::GetView_None();
	state.glCanvas = NULL;

	double last_activity = timer_Time();

	while (state.running)
	{
		bool recent_activity = false;

		//////////////////////////////////////////////////////////////////////////
		// (TODO: Work out why these things have to be in this order (to avoid
		// jumps when starting to move, etc))

		// Calculate frame length
		{
			const double time = timer_Time();
			static double last_time = time;
			const double realFrameLength = time-last_time;
			last_time = time;
			ENSURE(realFrameLength >= 0.0);
			// TODO: filter out big jumps, e.g. when having done a lot of slow
			// processing in the last frame
			state.realFrameLength = realFrameLength;
		}

		// Process the input that was received in the past
		if (g_Input.ProcessInput(&state))
			recent_activity = true;

		//////////////////////////////////////////////////////////////////////////

		{
			IMessage* msg;
			while ((msg = msgPasser->Retrieve()) != NULL)
			{
				recent_activity = true;

				std::string name (msg->GetName());

				msgHandlers::const_iterator it = GetMsgHandlers().find(name);
				if (it != GetMsgHandlers().end())
				{
					it->second(msg);
				}
				else
				{
					debug_warn(L"Unrecognised message");
					// CLogger might not be initialised, but this error will be sent
					// to the debug output window anyway so people can still see it
					LOGERROR("Unrecognised message (%s)", name.c_str());
				}

				if (msg->GetType() == IMessage::Query)
				{
					// For queries, we need to notify MessagePasserImpl::Query
					// that the query has now been processed.
					sem_post((sem_t*) static_cast<QueryMessage*>(msg)->m_Semaphore);
					// (msg may have been destructed at this point, so don't use it again)

					// It's quite possible that the querier is going to do a tiny
					// bit of processing on the query results and then issue another
					// query, and repeat lots of times in a loop. To avoid slowing
					// that down by rendering between every query, make this
					// thread yield now.
					SDL_Delay(0);
				}
				else
				{
					// For non-queries, we need to delete the object, since we
					// took ownership of it.
					AtlasMessage::ShareableDelete(msg);
				}
			}
		}

		// Exit, if desired
		if (! state.running)
			break;

		//////////////////////////////////////////////////////////////////////////

		// Do per-frame processing:

		ReloadChangedFiles();

		RendererIncrementalLoad();

		// Pump SDL events (e.g. hotkeys)
		SDL_Event_ ev;
		while (in_poll_priority_event(&ev))
			in_dispatch_event(&ev);

		if (g_GUI)
			g_GUI->TickObjects();

		state.view->Update(state.realFrameLength);

		state.view->Render();

		if (CProfileManager::IsInitialised())
			g_Profiler.Frame();


		double time = timer_Time();
		if (recent_activity)
			last_activity = time;

		// Be nice to the processor (by sleeping lots) if we're not doing anything
		// useful, and nice to the user (by just yielding to other threads) if we are
		bool yield = (time - last_activity > 0.5);

		// But make sure we aren't doing anything interesting right now, where
		// the user wants to see the screen updating even though they're not
		// interacting with it
		if (state.view->WantsHighFramerate())
			yield = false;

		if (yield) // if there was no recent activity...
		{
			double sleepUntil = time + 0.5; // only redraw at 2fps
			while (time < sleepUntil)
			{
				// To minimise latency when the user starts doing stuff, only
				// sleep for a short while, then check if anything's happened,
				// then go back to sleep
				// (TODO: This should probably be done with something like semaphores)
				Atlas_NotifyEndOfFrame(); // (TODO: rename to NotifyEndOfQuiteShortProcessingPeriodSoPleaseSendMeNewMessages or something)
				SDL_Delay(50);
				if (!msgPasser->IsEmpty())
					break;
				time = timer_Time();
			}
		}
		else
		{
			Atlas_NotifyEndOfFrame();
			SDL_Delay(0);
		}
	}
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
		dll.LoadSymbol("Atlas_NotifyEndOfFrame", Atlas_NotifyEndOfFrame);
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

	// Run the engine loop in a new thread
	std::thread engineThread = std::thread(RunEngine, std::ref(args));

	// Start Atlas UI on main thread
	// (required for wxOSX/Cocoa compatibility - see http://trac.wildfiregames.com/ticket/500)
	Atlas_StartWindow(L"ScenarioEditor");

	// Wait for the engine to exit
	engineThread.join();

	// TODO: delete all remaining messages, to avoid memory leak warnings

	// Restore main thread
	ThreadUtil::SetMainThread();

	// Clean up
	AtlasView::DestroyViews();
	AtlasMessage::g_MessagePasser = NULL;

	return true;
}
