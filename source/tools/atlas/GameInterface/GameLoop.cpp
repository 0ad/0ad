#include "precompiled.h"

#include "GameLoop.h"

#include "MessagePasserImpl.h"
#include "Messages.h"
#include "SharedMemory.h"
#include "Brushes.h"
#include "Handlers/MessageHandler.h"

#include "InputProcessor.h"

#include "lib/sdl.h"
#include "lib/ogl.h"
#include "lib/timer.h"
#include "lib/res/file/vfs.h"
#include "ps/CLogger.h"
#include "ps/GameSetup/GameSetup.h"
#include "ps/Game.h"
#include "ps/World.h"
#include "simulation/Simulation.h"
#include "simulation/EntityManager.h"

using namespace AtlasMessage;


namespace AtlasMessage
{
	extern void AtlasRenderSelection();
	extern void RegisterHandlers();
}

void AtlasRender()
{
	Render();
	AtlasRenderSelection();
}


// Loaded from DLL:
void (*Atlas_StartWindow)(wchar_t* type);
void (*Atlas_SetMessagePasser)(MessagePasser*);
void (*Atlas_GLSetCurrent)(void* context);
void (*Atlas_GLSwapBuffers)(void* context);
void (*Atlas_NotifyEndOfFrame)();
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


static void* LaunchWindow(void*)
{
	debug_set_thread_name("atlas_window");
	Atlas_StartWindow(L"ScenarioEditor");
	return NULL;
}


bool BeginAtlas(int argc, char* argv[], void* dll) 
{
	// Load required symbols from the DLL
#define GET(x) *(void**)&x = dlsym(dll, #x); debug_assert(x); if (! x) return false;
	GET(Atlas_StartWindow);
	GET(Atlas_SetMessagePasser);
	GET(Atlas_GLSetCurrent);
	GET(Atlas_GLSwapBuffers);
	GET(Atlas_NotifyEndOfFrame);
#undef GET
#define GET(x) *(void**)&x##Fptr = dlsym(dll, #x); debug_assert(x##Fptr); if (! x##Fptr) return false;
	GET(ShareableMalloc);
	GET(ShareableFree);
#undef GET

	// Pass our message handler to Atlas
	Atlas_SetMessagePasser(&msgPasser);

	// Register all the handlers for message which might be passed back
	RegisterHandlers();

	// Create a new thread, and launch the Atlas window inside that thread
	pthread_t gameThread;
	pthread_create(&gameThread, NULL, LaunchWindow, NULL);

	state.argc = argc;
	state.argv = argv;
	state.running = true;
	state.rendering = false;
	state.worldloaded = false;
	state.glContext = NULL;

	double last_activity = get_time();

	while (state.running)
	{
		bool recent_activity = false;

		//////////////////////////////////////////////////////////////////////////
		// (TODO: Work out why these things have to be in this order (to avoid
		// jumps when starting to move, etc)

		// Calculate frame length
		{
			const double time = get_time();
			static double last_time = time;
			const float length = (float)(time-last_time);
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
					debug_warn("Unrecognised message");
					// TODO: CLogger might not be initialised
					LOG(ERROR, "atlas", "Unrecognised message (%s)", name.c_str());
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

		vfs_reload_changed_files();
		
		if (state.worldloaded)
		{
			g_EntityManager.updateAll(0);
			g_Game->GetSimulation()->Update(0.0);
		}

		if (state.rendering)
		{
			AtlasRender();
			Atlas_GLSwapBuffers((void*)state.glContext);
		}

		double time = get_time();
		if (recent_activity)
			last_activity = time;

		// Be nice to the processor (by sleeping lots) if we're not doing anything
		// useful, and nice to the user (by just yielding to other threads) if we are
		
		if (time - last_activity > 0.5) // if there was no recent activity...
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
				time = get_time();
			}
		}
		else
		{
			Atlas_NotifyEndOfFrame();
			SDL_Delay(0);
		}
	}

	// TODO: delete all remaining messages, to avoid memory leak warnings

	pthread_join(gameThread, NULL);

	exit(0);
}
