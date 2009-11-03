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

#include "GameLoop.h"

#include "MessagePasserImpl.h"
#include "Messages.h"
#include "SharedMemory.h"
#include "Handlers/MessageHandler.h"
#include "ActorViewer.h"
#include "View.h"

#include "InputProcessor.h"

#include "lib/app_hooks.h"
#include "lib/external_libraries/sdl.h"
#include "lib/timer.h"
#include "ps/CLogger.h"
#include "ps/DllLoader.h"
#include "ps/Profile.h"

using namespace AtlasMessage;


namespace AtlasMessage
{
	extern void RegisterHandlers();
}

// Loaded from DLL:
void (*Atlas_StartWindow)(const wchar_t* type);
void (*Atlas_SetMessagePasser)(MessagePasser*);
void (*Atlas_GLSetCurrent)(void* cavas);
void (*Atlas_GLSwapBuffers)(void* canvas);
void (*Atlas_NotifyEndOfFrame)();
void (*Atlas_DisplayError)(const wchar_t* text, size_t flags);
void (*Atlas_ReportError)();
namespace AtlasMessage
{
	void* (*ShareableMallocFptr)(size_t);
	void (*ShareableFreeFptr)(void*);
}


static MessagePasserImpl msgPasser;
MessagePasser* AtlasMessage::g_MessagePasser = &msgPasser;


static InputProcessor g_Input;

static GameLoopState state;
GameLoopState* g_GameLoop = &state;


static void* LaunchWindow(void* data)
{
	const wchar_t* windowName = reinterpret_cast<const wchar_t*>(data);
	debug_SetThreadName("atlas_window");
	Atlas_StartWindow(windowName);
	return NULL;
}

// Work out which Atlas window to launch, given the command-line arguments
static const wchar_t* FindWindowName(const CmdLineArgs& args)
{
	if (args.Has("actorviewer"))
		return L"ActorViewer";
	else
		return L"ScenarioEditor";
}

static ErrorReaction AtlasDisplayError(const wchar_t* text, size_t flags)
{
	// TODO: after Atlas has been unloaded, don't do this
	Atlas_DisplayError(text, flags);

	return ER_CONTINUE;
}

bool BeginAtlas(const CmdLineArgs& args, const DllLoader& dll) 
{
	// Load required symbols from the DLL
	try
	{
		dll.LoadSymbol("Atlas_StartWindow", Atlas_StartWindow);
		dll.LoadSymbol("Atlas_SetMessagePasser", Atlas_SetMessagePasser);
		dll.LoadSymbol("Atlas_GLSetCurrent", Atlas_GLSetCurrent);
		dll.LoadSymbol("Atlas_GLSwapBuffers", Atlas_GLSwapBuffers);
		dll.LoadSymbol("Atlas_NotifyEndOfFrame", Atlas_NotifyEndOfFrame);
		dll.LoadSymbol("Atlas_DisplayError", Atlas_DisplayError);
		dll.LoadSymbol("Atlas_ReportError", Atlas_ReportError);
		dll.LoadSymbol("ShareableMalloc", ShareableMallocFptr);
		dll.LoadSymbol("ShareableFree", ShareableFreeFptr);
	}
	catch (PSERROR_DllLoader&)
	{
		debug_warn(L"Failed to initialise DLL");
		return false;
	}

	// Pass our message handler to Atlas
	Atlas_SetMessagePasser(&msgPasser);

	// Register all the handlers for message which might be passed back
	RegisterHandlers();

	// Create a new thread, and launch the Atlas window inside that thread
	const wchar_t* windowName = FindWindowName(args);
	pthread_t uiThread;
	pthread_create(&uiThread, NULL, LaunchWindow, reinterpret_cast<void*>(const_cast<wchar_t*>(windowName)));

	// Override ah_display_error to pass all errors to the Atlas UI
	AppHooks hooks = {0};
	hooks.display_error = AtlasDisplayError;
	app_hooks_update(&hooks);
	
	// Disable the game's cursor rendering
	extern CStrW g_CursorName;
	g_CursorName = L"";

	state.args = args;
	state.running = true;
	state.view = View::GetView_None();
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
			double time = timer_Time();
			static double last_time = time;
			float length = (float)(time-last_time);
			last_time = time;
			debug_assert(length >= 0.0f);
			// TODO: filter out big jumps, e.g. when having done a lot of slow
			// processing in the last frame
			state.frameLength = length;
		}

		// Process the input that was received in the past
		if (g_Input.ProcessInput(&state))
			recent_activity = true;

		//////////////////////////////////////////////////////////////////////////
		
		{
			IMessage* msg;
			while ((msg = msgPasser.Retrieve()) != NULL)
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
					LOG(CLogger::Error, L"atlas", L"Unrecognised message (%hs)", name.c_str());
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

//		vfs_reload_changed_files();

		state.view->Update(state.frameLength);

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
				if (!msgPasser.IsEmpty())
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

	// TODO: delete all remaining messages, to avoid memory leak warnings

	// Wait for the UI to exit
	pthread_join(uiThread, NULL);

	// Clean up
	View::DestroyViews();
	ScriptingHost::FinalShutdown();

	return true;
}
