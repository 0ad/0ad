/* Copyright (C) 2014 Wildfire Games.
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
#include "lib/status.h"
#include "lib/secure_crt.h"
#include "lib/frequency_filter.h"
#include "lib/input.h"
#include "lib/ogl.h"
#include "lib/timer.h"
#include "lib/external_libraries/libsdl.h"

#include "ps/ArchiveBuilder.h"
#include "ps/CConsole.h"
#include "ps/CLogger.h"
#include "ps/Filesystem.h"
#include "ps/Game.h"
#include "ps/Globals.h"
#include "ps/Hotkey.h"
#include "ps/Loader.h"
#include "ps/Profile.h"
#include "ps/Profiler2.h"
#include "ps/Pyrogenesis.h"
#include "ps/Replay.h"
#include "ps/TouchInput.h"
#include "ps/UserReport.h"
#include "ps/Util.h"
#include "ps/VideoMode.h"
#include "ps/World.h"
#include "ps/GameSetup/GameSetup.h"
#include "ps/GameSetup/Atlas.h"
#include "ps/GameSetup/Config.h"
#include "ps/GameSetup/CmdLineArgs.h"
#include "ps/GameSetup/Paths.h"
#include "ps/XML/Xeromyces.h"
#include "network/NetClient.h"
#include "network/NetServer.h"
#include "network/NetSession.h"
#include "lobby/IXmppClient.h"
#include "graphics/Camera.h"
#include "graphics/GameView.h"
#include "graphics/TextureManager.h"
#include "gui/GUIManager.h"
#include "renderer/Renderer.h"
#include "simulation2/Simulation2.h"

#if OS_UNIX
#include <unistd.h> // geteuid
#endif // OS_UNIX

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
#if SDL_VERSION_ATLEAST(2, 0, 0)
	case SDL_WINDOWEVENT:
		switch(ev->ev.window.event)
		{
		case SDL_WINDOWEVENT_ENTER:
			RenderCursor(true);
			break;
		case SDL_WINDOWEVENT_LEAVE:
			RenderCursor(false);
			break;
		case SDL_WINDOWEVENT_RESIZED:
			g_ResizedW = ev->ev.window.data1;
			g_ResizedH = ev->ev.window.data2;
			break;
		}
		break;
#else
	case SDL_ACTIVEEVENT:
		if (ev->ev.active.state & SDL_APPMOUSEFOCUS)
		{
			// Tell renderer not to render cursor if mouse focus is lost
			//	this restores system cursor, until/if focus is regained
			if (!ev->ev.active.gain)
				RenderCursor(false);
			else
				RenderCursor(true);
		}
		break;

	case SDL_VIDEORESIZE:
		g_ResizedW = ev->ev.resize.w;
		g_ResizedH = ev->ev.resize.h;
		break;
#endif

	case SDL_QUIT:
		kill_mainloop();
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
#if !OS_MACOSX
			// TODO: Fix fullscreen toggling on OS X, see http://trac.wildfiregames.com/ticket/741
			g_VideoMode.ToggleFullscreen();
#else
			LOGWARNING(L"Toggling fullscreen and resizing are disabled on OS X due to a known bug. Please use the config file to change display settings.");
#endif
			return IN_HANDLED;
		}
		else if (hotkey == "profile2.enable")
		{
			g_Profiler2.EnableGPU();
			g_Profiler2.EnableHTTP();
			return IN_HANDLED;
		}
		break;
	}

	return IN_PASS;
}


// dispatch all pending events to the various receivers.
static void PumpEvents()
{
	JSContext* cx = g_GUI->GetScriptInterface()->GetContext();
	JSAutoRequest rq(cx);
	
	PROFILE3("dispatch events");

	SDL_Event_ ev;
	while (in_poll_event(&ev))
	{
		PROFILE2("event");
		if (g_GUI)
		{
			JS::RootedValue tmpVal(cx);
			ScriptInterface::ToJSVal(cx, &tmpVal, ev);
			std::string data = g_GUI->GetScriptInterface()->StringifyJSON(tmpVal);
			PROFILE2_ATTR("%s", data.c_str());
		}
		in_dispatch_event(&ev);
	}

	g_TouchInput.Frame();
}


static int ProgressiveLoad()
{
	PROFILE3("progressive load");

	wchar_t description[100];
	int progress_percent;
	try
	{
		Status ret = LDR_ProgressiveLoad(10e-3, description, ARRAY_SIZE(description), &progress_percent);
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
			WARN_RETURN_STATUS_IF_ERR(ret);
			// can't do this above due to legit ERR::TIMED_OUT
			break;
		}
	}
	catch (PSERROR_Game_World_MapLoadFailed& e)
	{
		// Map loading failed

		// Call script function to do the actual work
		//	(delete game data, switch GUI page, show error, etc.)
		CancelLoad(CStr(e.what()).FromUTF8());
	}

	GUI_DisplayLoadProgress(progress_percent, description);
	return 0;
}


static void RendererIncrementalLoad()
{
	PROFILE3("renderer incremental load");

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
	g_Profiler2.RecordFrameStart();
	PROFILE2("frame");
	g_Profiler2.IncrementFrameNumber();
	PROFILE2_ATTR("%d", g_Profiler2.GetFrameNumber());

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
	const float realTimeSinceLastFrame = 1.0 / g_frequencyFilter->SmoothedFrequency();
#endif
	ENSURE(realTimeSinceLastFrame > 0.0f);

	// decide if update/render is necessary
	bool need_render = !g_app_minimized;
	bool need_update = true;

	// If we are not running a multiplayer game, disable updates when the game is
	// minimized or out of focus and relinquish the CPU a bit, in order to make 
	// debugging easier.
	if(g_PauseOnFocusLoss && !g_NetClient && !g_app_has_focus)
	{
		PROFILE3("non-focus delay");
		need_update = false;
		// don't use SDL_WaitEvent: don't want the main loop to freeze until app focus is restored
		SDL_Delay(10);
	}

	// TODO: throttling: limit update and render frequency to the minimum.
	// this is mostly relevant for "inactive" state, so that other windows
	// get enough CPU time, but it's always nice for power+thermal management.


	// this scans for changed files/directories and reloads them, thus
	// allowing hotloading (changes are immediately assimilated in-game).
	ReloadChangedFiles();

	ProgressiveLoad();

	RendererIncrementalLoad();

	PumpEvents();

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

	if (g_NetClient)
		g_NetClient->Poll();

	ogl_WarnIfError();

	g_GUI->TickObjects();

	ogl_WarnIfError();

	if (g_Game && g_Game->IsGameStarted() && need_update)
	{
		g_Game->Update(realTimeSinceLastFrame);

		g_Game->GetView()->Update(float(realTimeSinceLastFrame));
	}

	// Immediately flush any messages produced by simulation code
	if (g_NetClient)
		g_NetClient->Flush();

	// Keep us connected to any XMPP servers
	if (g_XmppClient)
		g_XmppClient->recv();

	g_UserReporter.Update();

	g_Console->Update(realTimeSinceLastFrame);

	ogl_WarnIfError();
	if(need_render)
	{
		Render();

		PROFILE3("swap buffers");
#if SDL_VERSION_ATLEAST(2, 0, 0)
		SDL_GL_SwapWindow(g_VideoMode.GetWindow());
#else
		SDL_GL_SwapBuffers();
#endif
	}
	ogl_WarnIfError();

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
		// TODO: Support mods
		Paths paths(args);
		g_VFS = CreateVfs(20 * MiB);
		g_VFS->Mount(L"cache/", paths.Cache(), VFS_MOUNT_ARCHIVABLE);
		g_VFS->Mount(L"", paths.RData()/"mods"/"public", VFS_MOUNT_MUST_EXIST);

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

		OsPath mod(args.Get("archivebuild"));
		OsPath zip;
		if (args.Has("archivebuild-output"))
			zip = args.Get("archivebuild-output");
		else
			zip = mod.Filename().ChangeExtension(L".zip");

		CArchiveBuilder builder(mod, paths.Cache());

		// Add mods provided on the command line
		// NOTE: We do not handle mods in the user mod path here
		std::vector<CStr> mods = args.GetMultiple("mod");
		for (size_t i = 0; i < mods.size(); ++i)
			builder.AddBaseMod(paths.RData()/"mods"/mods[i]);

		builder.Build(zip, args.Has("archivebuild-compress"));

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
	MainControllerShutdown();

	if (restart_in_atlas)
	{
		ATLAS_RunIfOnCmdLine(args, true);
		return;
	}

	// Shut down libxml2 (done here to match the Startup call)
	CXeromyces::Terminate();
}

#if OS_ANDROID
// In Android we compile the engine as a shared library, not an executable,
// so rename main() to a different symbol that the wrapper library can load
#undef main
#define main pyrogenesis_main
extern "C" __attribute__((visibility ("default"))) int main(int argc, char* argv[]);
#endif

extern "C" int main(int argc, char* argv[])
{
#if OS_UNIX
	// Don't allow people to run the game with root permissions,
	//	because bad things can happen, check before we do anything
	if (geteuid() == 0)
	{
		std::cerr << "********************************************************\n"
				  << "WARNING: Attempted to run the game with root permission!\n"
				  << "This is not allowed because it can alter home directory \n"
				  << "permissions and opens your system to vulnerabilities.   \n"
				  << "(You received this message because you were either      \n"
				  <<"  logged in as root or used e.g. the 'sudo' command.) \n"
				  << "********************************************************\n\n";
		return EXIT_FAILURE;
	}
#endif // OS_UNIX

	EarlyInit();	// must come at beginning of main

	RunGameOrAtlas(argc, const_cast<const char**>(argv));

	// Shut down profiler initialised by EarlyInit
	g_Profiler2.Shutdown();

	return EXIT_SUCCESS;
}
