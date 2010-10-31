/* Copyright (C) 2010 Wildfire Games.
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
#define MINIMAL_PCH 2
#include "lib/precompiled.h"

#include "lib/debug.h"
#include "lib/lib_errors.h"
#include "lib/secure_crt.h"
#include "lib/frequency_filter.h"
#include "lib/input.h"
#include "lib/ogl.h"
#include "lib/timer.h"
#include "lib/utf8.h"
#include "lib/external_libraries/sdl.h"
#include "lib/res/sound/snd_mgr.h"

#include "ps/ArchiveBuilder.h"
#include "ps/CConsole.h"
#include "ps/Filesystem.h"
#include "ps/Game.h"
#include "ps/Globals.h"
#include "ps/Hotkey.h"
#include "ps/Loader.h"
#include "ps/Profile.h"
#include "ps/Pyrogenesis.h"
#include "ps/Replay.h"
#include "ps/Util.h"
#include "ps/VideoMode.h"
#include "ps/GameSetup/GameSetup.h"
#include "ps/GameSetup/Atlas.h"
#include "ps/GameSetup/Config.h"
#include "ps/GameSetup/CmdLineArgs.h"
#include "ps/GameSetup/Paths.h"
#include "ps/XML/Xeromyces.h"
#include "network/NetClient.h"
#include "network/NetServer.h"
#include "network/NetSession.h"
#include "graphics/Camera.h"
#include "graphics/GameView.h"
#include "graphics/TextureManager.h"
#include "gui/GUIManager.h"
#include "renderer/Renderer.h"
#include "scripting/ScriptingHost.h"
#include "simulation2/Simulation2.h"

#define LOG_CATEGORY L"main"

extern bool g_GameRestarted;

void kill_mainloop();

// to avoid redundant and/or recursive resizing, we save the new
// size after VIDEORESIZE messages and only update the video mode
// once per frame.
// these values are the latest resize message, and reset to 0 once we've
// updated the video mode
static int g_ResizedW;
static int g_ResizedH;

// main app message handler
static InReaction MainInputHandler(const SDL_Event_* ev)
{
	switch(ev->ev.type)
	{
	case SDL_QUIT:
		kill_mainloop();
		break;

	case SDL_VIDEORESIZE:
		g_ResizedW = ev->ev.resize.w;
		g_ResizedH = ev->ev.resize.h;
		break;

	case SDL_HOTKEYDOWN:
		std::string hotkey = static_cast<const char*>(ev->ev.user.data1);
		if (hotkey == "exit")
		{
			kill_mainloop();
			return IN_HANDLED;
		}
		else if (hotkey == "screenshot")
		{
			WriteScreenshot(L".png");
			return IN_HANDLED;
		}
		else if (hotkey == "bigscreenshot")
		{
			WriteBigScreenshot(L".bmp", 10);
			return IN_HANDLED;
		}
		else if (hotkey == "togglefullscreen")
		{
			g_VideoMode.ToggleFullscreen();
			return IN_HANDLED;
		}
		break;
	}

	return IN_PASS;
}


// dispatch all pending events to the various receivers.
static void PumpEvents()
{
	PROFILE( "dispatch events" );
	in_dispatch_recorded_events();

	SDL_Event_ ev;
	while(SDL_PollEvent(&ev.ev))
		in_dispatch_event(&ev);
}


// return indication of whether archive is currently being built; this is
// used to prevent reloading during that time (see call site).
static bool ProgressiveBuildArchive()
{
ONCE(g_GUI->SendEventToAll("archivebuildercomplete"));
return false;
#if 0
	int ret = vfs_opt_auto_build("../logs/trace.txt", "mods/official/official%02d.zip", "mods/official/mini%02d.zip");
	if(ret == INFO::ALL_COMPLETE)
	{
		// nothing to do; will return false below
	}
	else if(ret < 0)
		DEBUG_DISPLAY_ERROR(L"Archive build failed");
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
#endif
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


static void RendererIncrementalLoad()
{
	const double maxTime = 0.1f;

	double startTime = timer_Time();
	bool more;
	do {
		more = g_Renderer.GetTextureManager().MakeProgress();
	}
	while (more && timer_Time() - startTime < maxTime);
}


static bool quit = false;	// break out of main loop

static void Frame()
{
	MICROLOG(L"Frame");
	ogl_WarnIfError();

	// get elapsed time
	const double time = timer_Time();
	g_frequencyFilter->Update(time);
	// .. old method - "exact" but contains jumps
#if 0
	static double last_time;
	const double time = timer_Time();
	const float TimeSinceLastFrame = (float)(time-last_time);
	last_time = time;
	ONCE(return);	// first call: set last_time and return

	// .. new method - filtered and more smooth, but errors may accumulate
#else
	const float TimeSinceLastFrame = 1.0 / g_frequencyFilter->SmoothedFrequency();
#endif
	debug_assert(TimeSinceLastFrame > 0.0f);

	// decide if update/render is necessary
	bool need_render = !g_app_minimized;
	bool need_update = true;

	// If we are not running a multiplayer game, disable updates when the game is
	// minimized or out of focus and relinquish the CPU a bit, in order to make 
	// debugging easier.
	if( !g_NetClient && !g_app_has_focus )
	{
		need_update = false;
		// don't use SDL_WaitEvent: don't want the main loop to freeze until app focus is restored
		SDL_Delay(10);
	}

	// TODO: throttling: limit update and render frequency to the minimum.
	// this is mostly relevant for "inactive" state, so that other windows
	// get enough CPU time, but it's always nice for power+thermal management.

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
		ReloadChangedFiles();
		PROFILE_END( "reload changed files");
	}

	PROFILE_START("progressive load");
	MICROLOG(L"progressive load");
	ProgressiveLoad();
	PROFILE_END("progressive load");

	PROFILE_START("renderer incremental load");
	RendererIncrementalLoad();
	PROFILE_END("renderer incremental load");

	PROFILE_START("input");
	MICROLOG(L"input");
	PumpEvents();
	PROFILE_END("input");

	// if the user quit by closing the window, the GL context will be broken and
	// may crash when we call Render() on some drivers, so leave this loop
	// before rendering
	if (quit)
		return;

	// respond to pumped resize events
	if (g_ResizedW || g_ResizedH)
	{
		g_VideoMode.ResizeWindow(g_ResizedW, g_ResizedH);
		g_ResizedW = g_ResizedH = 0;
	}

	PROFILE_START("network poll");
	if (g_NetClient)
		g_NetClient->Poll();
	PROFILE_END("network poll");

	ogl_WarnIfError();

	PROFILE_START("gui tick");
	MICROLOG(L"gui tick");
	g_GUI->TickObjects();
	PROFILE_END("gui tick");

	ogl_WarnIfError();

	PROFILE_START( "game logic" );
	if (g_Game && g_Game->IsGameStarted() && need_update)
	{
		PROFILE_START( "simulation update" );
		g_Game->Update(TimeSinceLastFrame);
		PROFILE_END( "simulation update" );

		PROFILE_START( "camera update" );
		g_Game->GetView()->Update(float(TimeSinceLastFrame));
		PROFILE_END( "camera update" );

		PROFILE_START( "sound update" );
		CCamera* camera = g_Game->GetView()->GetCamera();
		CMatrix3D& orientation = camera->m_Orientation;
		float* pos = &orientation._data[12];
		float* dir = &orientation._data[8];
		float* up  = &orientation._data[4];
		// HACK: otherwise sound effects are L/R flipped. No idea what else
		// is going wrong, because the listener and camera are supposed to
		// coincide in position and orientation.
		float down[3] = { -up[0], -up[1], -up[2] };
		if(snd_update(pos, dir, down) < 0)
			debug_printf(L"snd_update failed\n");
		PROFILE_END( "sound update" );
	}
	else
	{
		if(snd_update(0, 0, 0) < 0)
			debug_printf(L"snd_update (pos=0 version) failed\n");
	}
	PROFILE_END( "game logic" );

	// Immediately flush any messages produced by simulation code
	PROFILE_START("network flush");
	if (g_NetClient)
		g_NetClient->Flush();
	PROFILE_END("network flush");

	PROFILE_START( "update console" );
	g_Console->Update(TimeSinceLastFrame);
	PROFILE_END( "update console" );

	PROFILE_START("render");
	ogl_WarnIfError();
	if(need_render)
	{
		MICROLOG(L"render");
		Render();
		MICROLOG(L"finished render");
		PROFILE_START( "swap buffers" );
		SDL_GL_SwapBuffers();
		PROFILE_END( "swap buffers" );
	}
	ogl_WarnIfError();
	PROFILE_END("render");

	g_Profiler.Frame();

	g_GameRestarted = false;
}


static void MainControllerInit()
{
	// add additional input handlers only needed by this controller:

	// must be registered after gui_handler. Should mayhap even be last.
	in_add_handler(MainInputHandler);
}



static void MainControllerShutdown()
{
}


// stop the main loop and trigger orderly shutdown. called from several
// places: the event handler (SDL_QUIT and hotkey) and JS exitProgram.
void kill_mainloop()
{
	quit = true;
}


static bool restart_in_atlas = false;
// called by game code to indicate main() should restart in Atlas mode
// instead of terminating
void restart_mainloop_in_atlas()
{
	quit = true;
	restart_in_atlas = true;
}

// moved into a helper function to ensure args is destroyed before
// exit(), which may result in a memory leak.
static void RunGameOrAtlas(int argc, const char* argv[])
{
	CmdLineArgs args(argc, argv);

	// We need to initialise libxml2 in the main thread before
	// any thread uses it. So initialise it here before we
	// might run Atlas.
	CXeromyces::Startup();

	// run Atlas (if requested via args)
	bool ran_atlas = ATLAS_RunIfOnCmdLine(args, false);
	// Atlas handles the whole init/shutdown/etc sequence by itself;
	// when we get here, it has exited and we're done.
	if(ran_atlas)
		return;

	// run non-visual simulation replay if requested
	if (args.Has("replay"))
	{
		snd_disable(true);

		Paths paths(args);
		g_VFS = CreateVfs(20 * MiB);
		g_VFS->Mount(L"cache/", paths.Cache(), VFS_MOUNT_ARCHIVABLE);
		g_VFS->Mount(L"", paths.RData()/L"mods/public", VFS_MOUNT_MUST_EXIST);

		{
			CReplayPlayer replay;
			replay.Load(args.Get("replay"));
			replay.Replay();
		}

		g_VFS.reset();

		CXeromyces::Terminate();
		return;
	}

	// run in archive-building mode if requested
	if (args.Has("archivebuild"))
	{
		Paths paths(args);

		fs::wpath mod = wstring_from_utf8(args.Get("archivebuild"));
		fs::wpath zip;
		if (args.Has("archivebuild-output"))
			zip = wstring_from_utf8(args.Get("archivebuild-output"));
		else
			zip = mod.leaf()+L".zip";

		CArchiveBuilder builder(mod, paths.Cache());
		builder.Build(zip);

		CXeromyces::Terminate();
		return;
	}

	const double res = timer_Resolution();
	g_frequencyFilter = CreateFrequencyFilter(res, 30.0);

	// run the game
	Init(args, 0);
	InitGraphics(args, 0);
	MainControllerInit();
	while(!quit)
		Frame();
	Shutdown(0);
	ScriptingHost::FinalShutdown(); // this can't go in Shutdown() because that could be called multiple times per process, so stick it here instead
	MainControllerShutdown();

	if (restart_in_atlas)
	{
		ATLAS_RunIfOnCmdLine(args, true);
		return;
	}

	// Shut down libxml2 (done here to match the Startup call)
	CXeromyces::Terminate();
}

int main(int argc, char* argv[])
{
	EarlyInit();	// must come at beginning of main

	RunGameOrAtlas(argc, const_cast<const char**>(argv));

	return EXIT_SUCCESS;
}
