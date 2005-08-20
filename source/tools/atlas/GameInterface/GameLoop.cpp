#include "precompiled.h"

#include "GameLoop.h"

#include "MessagePasserImpl.h"
#include "Messages.h"
#include "handlers/MessageHandler.h"

#include "InputProcessor.h"

#include "lib/sdl.h"
#include "lib/ogl.h"
#include "lib/timer.h"
#include "ps/CLogger.h"

#ifdef NDEBUG
#pragma comment(lib, "AtlasUI")
#else
#pragma comment(lib, "AtlasUI_d")
#endif

using namespace AtlasMessage;

extern void Render_();

extern "C" { __declspec(dllimport) int __stdcall SwapBuffers(void*); }
	// HACK (and not exactly portable)
	//
	// (Er, actually that's what most of this file is. Oh well.)

// Loaded from DLL:
void (*Atlas_StartWindow)(wchar_t* type);
void (*Atlas_SetMessagePasser)(MessagePasser<mCommand>*, MessagePasser<mInput>*);

static MessagePasserImpl<mCommand> msgPasser_Command;
static MessagePasserImpl<mInput>   msgPasser_Input;

static InputProcessor g_Input;

static GameLoopState state;
GameLoopState* g_GameLoop = &state;


static void* LaunchWindow(void*)
{
	Atlas_StartWindow(L"ScenarioEditor");
	return NULL;
}


bool BeginAtlas(int argc, char* argv[], void* dll) 
{
	*(void**)&Atlas_StartWindow = dlsym(dll, "Atlas_StartWindow");
	*(void**)&Atlas_SetMessagePasser = dlsym(dll, "Atlas_SetMessagePasser");

	if (!Atlas_StartWindow || !Atlas_SetMessagePasser)
		return false;

	// Pass our message handler to Atlas
	Atlas_SetMessagePasser(&msgPasser_Command, &msgPasser_Input);

	// Create a new thread, and launch the Atlas window inside that thread
	pthread_t gameThread;
	pthread_create(&gameThread, NULL, LaunchWindow, NULL);

	state.argc = argc;
	state.argv = argv;
	state.running = true;
	state.rendering = false;
	state.currentDC = NULL;

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
			assert(length >= 0.0f);
			// TODO: filter out big jumps, e.g. when having done a lot of slow
			// processing in the last frame
			state.frameLength = length;
		}

		if (g_Input.ProcessInput(&state))
			recent_activity = true;

		//////////////////////////////////////////////////////////////////////////
		
		// if (!(in interactive-tool mode))
		{
			mCommand* msg;
			while ((msg = msgPasser_Command.Retrieve()) != NULL)
			{
				recent_activity = true;

				std::string name (msg->GetType());

				if (name == "CommandString")
				{
					// Allow some laziness: For commands that don't need any data other
					// than their name, we just use CommandString (and then need to
					// construct a reference to the appropriate handler for the
					// given string)
					name += "_";
					name += static_cast<mCommandString*>(msg)->name;
						// use 'static_cast' when casting messages, to make it clear
						// that it's slightly dangerous - we have to just assume that
						// GetType is correct, since we can't use proper RTTI
				}
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

				delete msg;
			}
		}

		// Exit, if desired
		if (! state.running)
			break;

		// Now do the same (roughly), for input events:
		{
			mInput* msg;
			while ((msg = msgPasser_Input.Retrieve()) != NULL)
			{
				recent_activity = true;

				std::string name (msg->GetType());
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

				delete msg;
			}
		}

		//////////////////////////////////////////////////////////////////////////

		if (state.rendering)
		{
			Render_();
			glFinish();
			SwapBuffers((void*)state.currentDC);
		}

		// Be nice to the processor if we're not doing anything useful, but
		// nice to the user if we are
		if (! recent_activity)
			SDL_Delay(100);
		else
			SDL_Delay(0);
		// Probable TODO: allow interruption of sleep by incoming messages
	}

	// TODO: delete all remaining messages, to avoid memory leak warnings

	pthread_join(gameThread, NULL);

	exit(0);
}
