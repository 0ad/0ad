/*

This module drives the game when running without Atlas (our integrated
map editor). It receives input and OS messages via SDL and feeds them
into the input dispatcher, where they are passed on to the game GUI and
simulation.
It also contains main(), which either runs the above controller or
that of Atlas depending on commandline parameters.

*/

// not for any PCH effort, but instead for the (common) definitions
// included there.
#include "lib/precompiled.h"

#include "lib/input.h"
#include "lib/sdl.h"
#include "lib/timer.h"
#include "lib/res/file/vfs.h"
#include "lib/res/sound/snd_mgr.h"
#include "lib/res/file/vfs_optimizer.h"

#include "ps/GameSetup/GameSetup.h"
#include "ps/GameSetup/Atlas.h"
#include "ps/GameSetup/Config.h"
#include "ps/GameSetup/CmdLineArgs.h"
#include "ps/Loader.h"
#include "ps/CConsole.h"
#include "ps/Profile.h"
#include "ps/Util.h"
#include "ps/Game.h"
#include "ps/Hotkey.h"
#include "ps/Globals.h"
#include "ps/Interact.h"
#include "network/SessionManager.h"
#include "simulation/TriggerManager.h"
#include "graphics/GameView.h"
#include "simulation/Scheduler.h"
#include "sound/CMusicPlayer.h"
#include "gui/GUI.h"

#if OS_WIN
# include "lib/sysdep/win/win.h"
#endif

#define LOG_CATEGORY "main"

extern bool g_TerrainModified;
extern bool g_GameRestarted;

void kill_mainloop();


// main app message handler
static InReaction MainInputHandler(const SDL_Event_* ev)
{
	switch(ev->ev.type)
	{
	case SDL_QUIT:
		kill_mainloop();
		break;

	case SDL_HOTKEYDOWN:
		switch(ev->ev.user.code)
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

	SDL_Event_ ev;
	while(SDL_PollEvent(&ev.ev))
		in_dispatch_event(&ev);
}


// return indication of whether archive is currently being built; this is
// used to prevent reloading during that time (see call site).
static bool ProgressiveBuildArchive()
{
	int ret = vfs_opt_auto_build("../logs/trace.txt", "mods/official/official%02d.zip", "mods/official/mini%02d.zip");
	if(ret == INFO::ALL_COMPLETE)
	{
		// nothing to do; will return false below
	}
	else if(ret < 0)
		DISPLAY_ERROR(L"Archive build failed");
	else if(ret == INFO::OK)
		g_GUI.SendEventToAll("archivebuildercomplete");
	// in progress
	else
	{
		int percent = (int)ret;
		g_ScriptingHost.SetGlobal("g_ArchiveBuilderProgress", INT_TO_JSVAL(percent));
		g_GUI.SendEventToAll("archivebuilderprogress");
		return true;
	}

	return false;
}


static int ProgressiveLoad()
{
	wchar_t description[100];
	int progress_percent;
	LibError ret = LDR_ProgressiveLoad(10e-3, description, ARRAY_SIZE(description), &progress_percent);
	switch(ret)
	{
		// no load active => no-op (skip code below)
	case INFO::OK:
		return 0;
		// current task didn't complete. we only care about this insofar as the
		// load process is therefore not yet finished.
	case ERR::TIMED_OUT:
		break;
		// just finished loading
	case INFO::ALL_COMPLETE:
		g_Game->ReallyStartGame();
		wcscpy_s(description, ARRAY_SIZE(description), L"Game is starting..");
		// LDR_ProgressiveLoad returns L""; set to valid text to
		// avoid problems in converting to JSString
		break;
		// error!
	default:
		CHECK_ERR(ret);
		// can't do this above due to legit ERR::TIMED_OUT
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

	// get elapsed time
	calc_fps();
	// .. old method - "exact" but contains jumps
#if 0
	static double last_time;
	const double time = get_time();
	const float TimeSinceLastFrame = (float)(time-last_time);
	last_time = time;
	ONCE(return);	// first call: set last_time and return

	// .. new method - filtered and more smooth, but errors may accumulate
#else
	const float TimeSinceLastFrame = spf;
#endif
	debug_assert(TimeSinceLastFrame >= 0.0f);

	// decide if update/render is necessary
	bool need_render, need_update;
	if( g_app_minimized )
	{
		// TODO: eventually update ought to be re-enabled so the server host
		// can Alt+Tab out without the match hanging. however, game updates
		// are currently really slow and disabling them makes debugging nicer.
		need_update = false;
		need_render = false;

		// inactive; relinquish CPU for a little while
		// don't use SDL_WaitEvent: don't want the main loop to freeze until app focus is restored
		SDL_Delay(10);
	}
	else if( !g_app_has_focus )
	{
		need_update = false;	// see above
		need_render = true;

		SDL_Delay(5);	// see above
	}
	else	// active
	{
		need_update = true;
		need_render = true;
	}
	// TODO: throttling: limit update and render frequency to the minimum.
	// this is mostly relevant for "inactive" state, so that other windows
	// get enough CPU time, but it's always nice for power+thermal management.


	PROFILE_START( "update music" );
	music_player.update();
	PROFILE_END( "update music" );

	bool is_building_archive;	// must come before PROFILE_START's {
	PROFILE_START("build archive");
	MICROLOG(L"build archive");
	is_building_archive = ProgressiveBuildArchive();
	PROFILE_END( "build archive");

	// this scans for changed files/directories and reloads them, thus
	// allowing hotloading (changes are immediately assimilated in-game).
	// must not be done during archive building because it changes the
	// archive file each iteration, but keeps it locked; reloading
	// would trigger a warning because the file can't be opened.
	if(!is_building_archive)
	{
		PROFILE_START("reload changed files");
		MICROLOG(L"reload changed files");
		vfs_reload_changed_files(); 
		PROFILE_END( "reload changed files");
	}

	PROFILE_START("progressive load");
	MICROLOG(L"progressive load");
	ProgressiveLoad();
	PROFILE_END( "progressive load");

	PROFILE_START("input");
	MICROLOG(L"input");
	PumpEvents();
	g_SessionManager.Poll();
	PROFILE_END("input");

	oglCheck();

	PROFILE_START("gui tick");
	MICROLOG(L"gui tick");
#ifndef NO_GUI
	g_GUI.TickObjects();
#endif
	PROFILE_END("gui tick");

	oglCheck();

	PROFILE_START( "game logic" );
	if (g_Game && g_Game->IsGameStarted() && need_update)
	{
		PROFILE_START( "simulation update" );
		g_Game->Update(TimeSinceLastFrame);
		PROFILE_END( "simulation update" );

		if (!g_FixedFrameTiming)
		{
			PROFILE( "camera update" );
			g_Game->GetView()->Update(float(TimeSinceLastFrame));
		}
		
		PROFILE_START("trigger update");
		g_TriggerManager.Update( (float)TimeSinceLastFrame );
		PROFILE_END("trigger udpate");

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

	PROFILE_START("render");
	oglCheck();
	if(need_render)
	{
		MICROLOG(L"render");
		Render();
		MICROLOG(L"finished render");
		PROFILE_START( "swap buffers" );
		SDL_GL_SwapBuffers();
		PROFILE_END( "swap buffers" );
	}
	oglCheck();
	PROFILE_END("render");

	g_Profiler.Frame();

	if(g_FixedFrameTiming && frameCount==100)
		kill_mainloop();

	g_TerrainModified = false;
	g_GameRestarted = false;
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


#include "lib/res/file/trace.h"

int main(int argc, char* argv[])
{
	// If you ever want to catch a particular allocation:
	//_CrtSetBreakAlloc(321);

	// see discussion at declaration of win_pre_main_init.
#if OS_WIN
	win_pre_main_init();
#endif

	{ // scope for args

		CmdLineArgs args(argc, argv);

		bool ran_atlas = ATLAS_RunIfOnCmdLine(args);

		// Atlas handles the whole init/shutdown/etc sequence by itself,
		// so we skip all this and just exit if Atlas was run
		if (! ran_atlas)
		{
			Init(args, 0);
			MainControllerInit();

			while(!quit)
				Frame();

			Shutdown(0);
			MainControllerShutdown();
		}
	}

	debug_printf("Shutdown complete, calling exit() now\n");
	
	exit(0);
}
