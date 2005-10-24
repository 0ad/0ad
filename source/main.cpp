/*

This module drives the game when running without Atlas (our integrated
map editor). It receives input and OS messages via SDL and feeds them
into the input dispatcher, where they are passed on to the game GUI and
simulation.
It also contains main(), which either runs the above controller or
that of Atlas depending on commandline parameters.

*/

#include "precompiled.h"

#ifdef SCED
# include "ui/StdAfx.h"
# undef ERROR
#endif // SCED

#include "lib/input.h"
#include "lib/sdl.h"
#include "lib/timer.h"
#include "lib/res/file/vfs.h"
#include "lib/res/sound/snd.h"

#include "ps/GameSetup/GameSetup.h"
#include "ps/GameSetup/Atlas.h"
#include "ps/GameSetup/Config.h"
#include "ps/Loader.h"
#include "gui/GUI.h"
#include "ps/CConsole.h"
#include "ps/Profile.h"
#include "ps/Util.h"
#include "ps/Game.h"
#include "ps/Hotkey.h"
#include "ps/Interact.h"
#include "ps/Network/SessionManager.h"
#include "simulation/Scheduler.h"
#include "sound/CMusicPlayer.h"

#define LOG_CATEGORY "main"

extern bool g_TerrainModified;

void kill_mainloop();


// main app message handler
static InReaction MainInputHandler(const SDL_Event* ev)
{
	switch(ev->type)
	{
	case SDL_QUIT:
		kill_mainloop();
		break;

	case SDL_HOTKEYDOWN:
		switch(ev->user.code)
		{
		case HOTKEY_EXIT:
			kill_mainloop();
			return IN_HANDLED;

		case HOTKEY_SCREENSHOT:
			WriteScreenshot("png");
			return IN_HANDLED;

		case HOTKEY_BIGSCREENSHOT:
			WriteBigScreenshot("bmp", 10);
			return IN_HANDLED;

		default:
			break;
		}
		break;
	}

	return IN_PASS;
}


// dispatch all pending events to the various receivers.
static void PumpEvents()
{
	in_dispatch_recorded_events();

	SDL_Event event;
	while(SDL_PollEvent(&event))
		in_dispatch_event(&event);
}



static int ProgressiveLoad()
{
	wchar_t description[100];
	int progress_percent;
	int ret = LDR_ProgressiveLoad(10e-3, description, ARRAY_SIZE(description), &progress_percent);
	switch(ret)
	{
		// no load active => no-op (skip code below)
	case 0:
		return 0;
		// current task didn't complete. we only care about this insofar as the
		// load process is therefore not yet finished.
	case ERR_TIMED_OUT:
		break;
		// just finished loading
	case LDR_ALL_FINISHED:
		g_Game->ReallyStartGame();
		wcscpy_s(description, ARRAY_SIZE(description), L"Game is starting..");
		// LDR_ProgressiveLoad returns L""; set to valid text to
		// avoid problems in converting to JSString
		break;
		// error!
	default:
		CHECK_ERR(ret);
		// can't do this above due to legit ERR_TIMED_OUT
		break;
	}

	GUI_DisplayLoadProgress(progress_percent, description);
	return 0;
}

CMusicPlayer music_player;

static void Frame()
{
	MICROLOG(L"Frame");
	oglCheck();

	PROFILE_START( "update music" );
	music_player.update();
	PROFILE_END( "update music" );

	calc_fps();
// old method - "exact" but contains jumps
#if 0
	static double last_time;
	const double time = get_time();
	const float TimeSinceLastFrame = (float)(time-last_time);
	last_time = time;
	ONCE(return);	// first call: set last_time and return
			
// new method - filtered and more smooth, but errors may accumulate
#else
	const float TimeSinceLastFrame = spf;
#endif
	debug_assert(TimeSinceLastFrame >= 0.0f);

	PROFILE_START( "reload changed files" );
	MICROLOG(L"reload files");
	vfs_reload_changed_files(); 
	PROFILE_END( "reload changed files" );

	PROFILE_START( "progressive load" );
	ProgressiveLoad();
	PROFILE_END( "progressive load" );

	PROFILE_START( "input" );
	MICROLOG(L"input");
	PumpEvents();
	g_SessionManager.Poll();
	PROFILE_END( "input" );

	oglCheck();

	PROFILE_START( "gui tick" );
#ifndef NO_GUI
	g_GUI.TickObjects();
#endif
	PROFILE_END( "gui tick" );

	oglCheck();

	PROFILE_START( "game logic" );
	if (g_Game && g_Game->IsGameStarted())
	{
		PROFILE_START( "simulation update" );
		g_Game->Update(TimeSinceLastFrame);
		PROFILE_END( "simulation update" );

		if (!g_FixedFrameTiming)
		{
			PROFILE( "camera update" );
			g_Game->GetView()->Update(float(TimeSinceLastFrame));
		}

		PROFILE_START( "selection and interaction ui" );
		// TODO Where does GameView end and other things begin?
		g_Mouseover.update( TimeSinceLastFrame );
		g_Selection.update();
		g_BuildingPlacer.update( TimeSinceLastFrame );
		PROFILE_END( "selection and interaction ui" );

		PROFILE_START( "sound update" );
		CCamera* camera = g_Game->GetView()->GetCamera();
		CMatrix3D& orientation = camera->m_Orientation;

		float* pos = &orientation._data[12];
		float* dir = &orientation._data[8];
		float* up  = &orientation._data[4];
		if(snd_update(pos, dir, up) < 0)
			debug_printf("snd_update failed\n");
		PROFILE_END( "sound update" );
	}
	else
	{
		// CSimulation would do this with the proper turn length if we were in
		// a game. This is basically just to keep script timers running.
		uint ms_elapsed = (uint)(TimeSinceLastFrame*1000);
		g_Scheduler.update(ms_elapsed);
		if(snd_update(0, 0, 0) < 0)
			debug_printf("snd_update (pos=0 version) failed\n");
	}

	PROFILE_END( "game logic" );

	PROFILE_START( "update console" );
	g_Console->Update(TimeSinceLastFrame);
	PROFILE_END( "update console" );

	PROFILE_START( "render" );
	oglCheck();

	if(g_active)
	{
		MICROLOG(L"render");
		Render();
		MICROLOG(L"finished render");
		PROFILE_START( "swap buffers" );
		SDL_GL_SwapBuffers();
		PROFILE_END( "swap buffers" );
	}
	// inactive; relinquish CPU for a little while
	// don't use SDL_WaitEvent: don't want the main loop to freeze until app focus is restored
	else
		SDL_Delay(10);

	oglCheck();
	PROFILE_END( "render" );

	g_Profiler.Frame();

	if(g_FixedFrameTiming && frameCount==100)
		kill_mainloop();

	// clear terrain modified flag
	g_TerrainModified = false;
}


static void MainControllerInit()
{
	// add additional input handlers only needed by this controller:

	// gui_handler needs to be registered after (i.e. called before!) the
	// hotkey handler so that input boxes can be typed in without
	// setting off hotkeys.
#ifndef NO_GUI
	in_add_handler(gui_handler);
#endif

	// must be registered after gui_handler. Should mayhap even be last.
	in_add_handler(MainInputHandler);
}



static void MainControllerShutdown()
{
	music_player.release();
}


static bool quit = false;	// break out of main loop

// stop the main loop and trigger orderly shutdown. called from several
// places: the event handler (SDL_QUIT and hotkey) and JS exitProgram.
void kill_mainloop()
{
	quit = true;
}


#ifndef SCED

int main(int argc, char* argv[])
{
	debug_printf("MAIN &argc=%p &argv=%p\n", &argc, &argv);

	ATLAS_RunIfOnCmdLine(argc, argv);

	Init(argc, argv, 0);
	MainControllerInit();

	while(!quit)
		Frame();

	Shutdown();
	MainControllerShutdown();

	debug_printf("Shutdown complete, calling exit() now\n");
	
	exit(0);
}

#else // SCED:

void ScEd_Init()
{
	g_Quickstart = true;
	Init(0, NULL, INIT_HAVE_VMODE|INIT_NO_GUI);
}

void ScEd_Shutdown()
{
	Shutdown();
}

#endif // SCED
