#include "precompiled.h"

#include "GameLoop.h"

#include "MessagePasserImpl.h"
#include "Messages.h"
#include "handlers/MessageHandler.h"

#include "lib/sdl.h"
#include "lib/ogl.h"
#include "ps/CLogger.h"
//#include "gui/GUI.h"
//#include "renderer/Renderer.h"
//#include "ps/Game.h"
//#include "ps/Loader.h"

#ifdef NDEBUG
#pragma comment(lib, "AtlasUI")
#else
#pragma comment(lib, "AtlasUI_d")
#endif

using namespace AtlasMessage;

extern __declspec(dllimport) void Atlas_StartWindow(wchar_t* type);
extern __declspec(dllimport) void Atlas_SetMessagePasser(MessagePasser*);

static MessagePasserImpl msgPasser;

static void* LaunchWindow(void*)
{
	Atlas_StartWindow(L"ScenarioEditor");
	return NULL;
}

extern void Render_();

//extern int g_xres, g_yres;

extern "C" { __declspec(dllimport) int __stdcall SwapBuffers(void*); }
	// HACK (and not exactly portable)
	//
	// (Er, actually that's what most of this file is. Oh well.)

static GameLoopState state;
GameLoopState* g_GameLoop = &state;


void BeginAtlas(int argc, char** argv) 
{
	// Pass our message handler to Atlas
	Atlas_SetMessagePasser(&msgPasser);

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
		IMessage* msg;
		while (msg = msgPasser.Retrieve())
		{
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
			handlers::const_iterator it = GetHandlers().find(name);
			if (it != GetHandlers().end())
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

		if (! state.running)
			break;

		if (state.rendering)
		{
			Render_();
			SwapBuffers(state.currentDC);
		}

		SDL_Delay(50);
	}

	pthread_join(gameThread, NULL);
}
