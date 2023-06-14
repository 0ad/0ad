/* Copyright (C) 2023 Wildfire Games.
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

#include "ps/GameSetup/GameSetup.h"

#include "graphics/GameView.h"
#include "graphics/MapReader.h"
#include "graphics/TerrainTextureManager.h"
#include "gui/CGUI.h"
#include "gui/GUIManager.h"
#include "gui/Scripting/JSInterface_GUIManager.h"
#include "i18n/L10n.h"
#include "lib/app_hooks.h"
#include "lib/config2.h"
#include "lib/external_libraries/libsdl.h"
#include "lib/file/common/file_stats.h"
#include "lib/input.h"
#include "lib/timer.h"
#include "lobby/IXmppClient.h"
#include "network/NetServer.h"
#include "network/NetClient.h"
#include "network/NetMessage.h"
#include "network/NetMessages.h"
#include "network/scripting/JSInterface_Network.h"
#include "ps/CConsole.h"
#include "ps/CLogger.h"
#include "ps/ConfigDB.h"
#include "ps/Filesystem.h"
#include "ps/Game.h"
#include "ps/GameSetup/Atlas.h"
#include "ps/GameSetup/Paths.h"
#include "ps/GameSetup/Config.h"
#include "ps/GameSetup/CmdLineArgs.h"
#include "ps/GameSetup/HWDetect.h"
#include "ps/Globals.h"
#include "ps/GUID.h"
#include "ps/Hotkey.h"
#include "ps/Joystick.h"
#include "ps/Loader.h"
#include "ps/Mod.h"
#include "ps/ModIo.h"
#include "ps/Profile.h"
#include "ps/ProfileViewer.h"
#include "ps/Profiler2.h"
#include "ps/Pyrogenesis.h"	// psSetLogDir
#include "ps/scripting/JSInterface_Console.h"
#include "ps/scripting/JSInterface_Game.h"
#include "ps/scripting/JSInterface_Main.h"
#include "ps/scripting/JSInterface_VFS.h"
#include "ps/TouchInput.h"
#include "ps/UserReport.h"
#include "ps/Util.h"
#include "ps/VideoMode.h"
#include "ps/VisualReplay.h"
#include "ps/World.h"
#include "renderer/Renderer.h"
#include "renderer/SceneRenderer.h"
#include "renderer/VertexBufferManager.h"
#include "scriptinterface/FunctionWrapper.h"
#include "scriptinterface/JSON.h"
#include "scriptinterface/ScriptInterface.h"
#include "scriptinterface/ScriptStats.h"
#include "scriptinterface/ScriptContext.h"
#include "scriptinterface/ScriptConversions.h"
#include "simulation2/Simulation2.h"
#include "simulation2/scripting/JSInterface_Simulation.h"
#include "soundmanager/scripting/JSInterface_Sound.h"
#include "soundmanager/ISoundManager.h"
#include "tools/atlas/GameInterface/GameLoop.h"

#if !(OS_WIN || OS_MACOSX || OS_ANDROID) // assume all other platforms use X11 for wxWidgets
#define MUST_INIT_X11 1
#include <X11/Xlib.h>
#else
#define MUST_INIT_X11 0
#endif

extern void RestartEngine();

#include <fstream>
#include <iostream>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/split.hpp>

ERROR_GROUP(System);
ERROR_TYPE(System, SDLInitFailed);
ERROR_TYPE(System, VmodeFailed);
ERROR_TYPE(System, RequiredExtensionsMissing);

thread_local std::shared_ptr<ScriptContext> g_ScriptContext;

bool g_InDevelopmentCopy;
bool g_CheckedIfInDevelopmentCopy = false;

ErrorReactionInternal psDisplayError(const wchar_t* UNUSED(text), size_t UNUSED(flags))
{
	// If we're fullscreen, then sometimes (at least on some particular drivers on Linux)
	// displaying the error dialog hangs the desktop since the dialog box is behind the
	// fullscreen window. So we just force the game to windowed mode before displaying the dialog.
	// (But only if we're in the main thread, and not if we're being reentrant.)
	if (Threading::IsMainThread())
	{
		static bool reentering = false;
		if (!reentering)
		{
			reentering = true;
			g_VideoMode.SetFullscreen(false);
			reentering = false;
		}
	}

	// We don't actually implement the error display here, so return appropriately
	return ERI_NOT_IMPLEMENTED;
}

void MountMods(const Paths& paths, const std::vector<CStr>& mods)
{
	OsPath modPath = paths.RData()/"mods";
	OsPath modUserPath = paths.UserData()/"mods";

	size_t userFlags = VFS_MOUNT_WATCH|VFS_MOUNT_ARCHIVABLE;
	size_t baseFlags = userFlags|VFS_MOUNT_MUST_EXIST;
	size_t priority = 0;
	for (size_t i = 0; i < mods.size(); ++i)
	{
		priority = i + 1; // Mods are higher priority than regular mountings, which default to priority 0

		OsPath modName(mods[i]);
		// Only mount mods from the user path if they don't exist in the 'rdata' path.
		if (DirectoryExists(modPath / modName / ""))
			g_VFS->Mount(L"", modPath / modName / "", baseFlags, priority);
		else
			g_VFS->Mount(L"", modUserPath / modName / "", userFlags, priority);
	}

	// Mount the user mod last. In dev copy, mount it with a low priority. Otherwise, make it writable.
	g_VFS->Mount(L"", modUserPath / "user" / "", userFlags, InDevelopmentCopy() ? 0 : priority + 1);
}

static void InitVfs(const CmdLineArgs& args, int flags)
{
	TIMER(L"InitVfs");

	const bool setup_error = (flags & INIT_HAVE_DISPLAY_ERROR) == 0;

	const Paths paths(args);

	OsPath logs(paths.Logs());
	CreateDirectories(logs, 0700);

	psSetLogDir(logs);
	// desired location for crashlog is now known. update AppHooks ASAP
	// (particularly before the following error-prone operations):
	AppHooks hooks = {0};
	hooks.bundle_logs = psBundleLogs;
	hooks.get_log_dir = psLogDir;
	if (setup_error)
		hooks.display_error = psDisplayError;
	app_hooks_update(&hooks);

	g_VFS = CreateVfs();

	const OsPath readonlyConfig = paths.RData()/"config"/"";

	// Mount these dirs with highest priority so that mods can't overwrite them.
	g_VFS->Mount(L"cache/", paths.Cache(), VFS_MOUNT_ARCHIVABLE, VFS_MAX_PRIORITY);	// (adding XMBs to archive speeds up subsequent reads)
	if (readonlyConfig != paths.Config())
		g_VFS->Mount(L"config/", readonlyConfig, 0, VFS_MAX_PRIORITY-1);
	g_VFS->Mount(L"config/", paths.Config(), 0, VFS_MAX_PRIORITY);
	g_VFS->Mount(L"screenshots/", paths.UserData()/"screenshots"/"", 0, VFS_MAX_PRIORITY);
	g_VFS->Mount(L"saves/", paths.UserData()/"saves"/"", VFS_MOUNT_WATCH, VFS_MAX_PRIORITY);

	// Engine localization files (regular priority, these can be overwritten).
	g_VFS->Mount(L"l10n/", paths.RData()/"l10n"/"");

	// Mods will be mounted later.

	// note: don't bother with g_VFS->TextRepresentation - directories
	// haven't yet been populated and are empty.
}


static void InitPs(bool setup_gui, const CStrW& gui_page, ScriptInterface* srcScriptInterface, JS::HandleValue initData)
{
	{
		// console
		TIMER(L"ps_console");

		g_Console->Init();
	}

	// hotkeys
	{
		TIMER(L"ps_lang_hotkeys");
		LoadHotkeys(g_ConfigDB);
	}

	if (!setup_gui)
	{
		// We do actually need *some* kind of GUI loaded, so use the
		// (currently empty) Atlas one
		g_GUI->SwitchPage(L"page_atlas.xml", srcScriptInterface, initData);
		return;
	}

	// GUI uses VFS, so this must come after VFS init.
	g_GUI->SwitchPage(gui_page, srcScriptInterface, initData);
}

void InitInput()
{
	g_Joystick.Initialise();

	// register input handlers
	// This stack is constructed so the first added, will be the last
	//  one called. This is important, because each of the handlers
	//  has the potential to block events to go further down
	//  in the chain. I.e. the last one in the list added, is the
	//  only handler that can block all messages before they are
	//  processed.
	in_add_handler(game_view_handler);

	in_add_handler(CProfileViewer::InputThunk);

	in_add_handler(HotkeyInputActualHandler);

	// gui_handler needs to be registered after (i.e. called before!) the
	// hotkey handler so that input boxes can be typed in without
	// setting off hotkeys.
	in_add_handler(gui_handler);
	// Likewise for the console.
	in_add_handler(conInputHandler);

	in_add_handler(touch_input_handler);

	// Should be called after scancode map update (i.e. after the global input, but before UI).
	// This never blocks the event, but it does some processing necessary for hotkeys,
	// which are triggered later down the input chain.
	// (by calling this before the UI, we can use 'EventWouldTriggerHotkey' in the UI).
	in_add_handler(HotkeyInputPrepHandler);

	// These two must be called first (i.e. pushed last)
	// GlobalsInputHandler deals with some important global state,
	// such as which scancodes are being pressed, mouse buttons pressed, etc.
	// while HotkeyStateChange updates the map of active hotkeys.
	in_add_handler(GlobalsInputHandler);
	in_add_handler(HotkeyStateChange);
}


static void ShutdownPs()
{
	SAFE_DELETE(g_GUI);

	UnloadHotkeys();
}

static void InitSDL()
{
#if OS_LINUX
	// In fullscreen mode when SDL is compiled with DGA support, the mouse
	// sensitivity often appears to be unusably wrong (typically too low).
	// (This seems to be reported almost exclusively on Ubuntu, but can be
	// reproduced on Gentoo after explicitly enabling DGA.)
	// Disabling the DGA mouse appears to fix that problem, and doesn't
	// have any obvious negative effects.
	setenv("SDL_VIDEO_X11_DGAMOUSE", "0", 0);
#endif

	if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_NOPARACHUTE) < 0)
	{
		LOGERROR("SDL library initialization failed: %s", SDL_GetError());
		throw PSERROR_System_SDLInitFailed();
	}
	atexit(SDL_Quit);

	// Text input is active by default, disable it until it is actually needed.
	SDL_StopTextInput();

#if SDL_VERSION_ATLEAST(2, 0, 9)
	// SDL2 >= 2.0.9 defaults to 32 pixels (to support touch screens) but that can break our double-clicking.
	SDL_SetHint(SDL_HINT_MOUSE_DOUBLE_CLICK_RADIUS, "1");
#endif

#if SDL_VERSION_ATLEAST(2, 0, 14) && OS_WIN
	// SDL2 >= 2.0.14 Before SDL 2.0.14, this defaulted to true. In 2.0.14 they switched to false
	// breaking the behavior on Windows.
	// https://github.com/libsdl-org/SDL/commit/1947ca7028ab165cc3e6cbdb0b4b7c4db68d1710
	// https://github.com/libsdl-org/SDL/issues/5033
	SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "1");
#endif

#if OS_MACOSX
	// Some Mac mice only have one button, so they can't right-click
	// but SDL2 can emulate that with Ctrl+Click
	bool macMouse = false;
	CFG_GET_VAL("macmouse", macMouse);
	SDL_SetHint(SDL_HINT_MAC_CTRL_CLICK_EMULATE_RIGHT_CLICK, macMouse ? "1" : "0");
#endif
}

static void ShutdownSDL()
{
	SDL_Quit();
}


void EndGame()
{
	SAFE_DELETE(g_NetClient);
	SAFE_DELETE(g_NetServer);
	SAFE_DELETE(g_Game);

	if (CRenderer::IsInitialised())
	{
		ISoundManager::CloseGame();
		g_Renderer.GetSceneRenderer().ResetState();
	}
}

void Shutdown(int flags)
{
	const bool hasRenderer = CRenderer::IsInitialised();

	if ((flags & SHUTDOWN_FROM_CONFIG))
		goto from_config;

	EndGame();

	SAFE_DELETE(g_XmppClient);

	SAFE_DELETE(g_ModIo);

	ShutdownPs();

	if (hasRenderer)
	{
		TIMER_BEGIN(L"shutdown Renderer");
		g_Renderer.~CRenderer();
		g_VBMan.Shutdown();
		TIMER_END(L"shutdown Renderer");
	}

	g_RenderingOptions.ClearHooks();

	g_Profiler2.ShutdownGPU();

	if (hasRenderer)
		g_VideoMode.Shutdown();

	TIMER_BEGIN(L"shutdown SDL");
	ShutdownSDL();
	TIMER_END(L"shutdown SDL");

	TIMER_BEGIN(L"shutdown UserReporter");
	g_UserReporter.Deinitialize();
	TIMER_END(L"shutdown UserReporter");

	// Cleanup curl now that g_ModIo and g_UserReporter have been shutdown.
	curl_global_cleanup();

	delete &g_L10n;

from_config:
	TIMER_BEGIN(L"shutdown ConfigDB");
	CConfigDB::Shutdown();
	TIMER_END(L"shutdown ConfigDB");

	SAFE_DELETE(g_Console);

	// This is needed to ensure that no callbacks from the JSAPI try to use
	// the profiler when it's already destructed
	g_ScriptContext.reset();

	// resource
	// first shut down all resource owners, and then the handle manager.
	TIMER_BEGIN(L"resource modules");

		ISoundManager::SetEnabled(false);

		g_VFS.reset();

		file_stats_dump();

	TIMER_END(L"resource modules");

	TIMER_BEGIN(L"shutdown misc");
		timer_DisplayClientTotals();

		CNetHost::Deinitialize();

		// should be last, since the above use them
		SAFE_DELETE(g_Logger);
		delete &g_Profiler;
		delete &g_ProfileViewer;

		SAFE_DELETE(g_ScriptStatsTable);
	TIMER_END(L"shutdown misc");
}

#if OS_UNIX
static void FixLocales()
{
#if OS_MACOSX || OS_BSD
	// OS X requires a UTF-8 locale in LC_CTYPE so that *wprintf can handle
	// wide characters. Peculiarly the string "UTF-8" seems to be acceptable
	// despite not being a real locale, and it's conveniently language-agnostic,
	// so use that.
	setlocale(LC_CTYPE, "UTF-8");
#endif


	// On misconfigured systems with incorrect locale settings, we'll die
	// with a C++ exception when some code (e.g. Boost) tries to use locales.
	// To avoid death, we'll detect the problem here and warn the user and
	// reset to the default C locale.


	// For informing the user of the problem, use the list of env vars that
	// glibc setlocale looks at. (LC_ALL is checked first, and LANG last.)
	const char* const LocaleEnvVars[] = {
		"LC_ALL",
		"LC_COLLATE",
		"LC_CTYPE",
		"LC_MONETARY",
		"LC_NUMERIC",
		"LC_TIME",
		"LC_MESSAGES",
		"LANG"
	};

	try
	{
		// this constructor is similar to setlocale(LC_ALL, ""),
		// but instead of returning NULL, it throws runtime_error
		// when the first locale env variable found contains an invalid value
		std::locale("");
	}
	catch (std::runtime_error&)
	{
		LOGWARNING("Invalid locale settings");

		for (size_t i = 0; i < ARRAY_SIZE(LocaleEnvVars); i++)
		{
			if (char* envval = getenv(LocaleEnvVars[i]))
				LOGWARNING("  %s=\"%s\"", LocaleEnvVars[i], envval);
			else
				LOGWARNING("  %s=\"(unset)\"", LocaleEnvVars[i]);
		}

		// We should set LC_ALL since it overrides LANG
		if (setenv("LC_ALL", std::locale::classic().name().c_str(), 1))
			debug_warn(L"Invalid locale settings, and unable to set LC_ALL env variable.");
		else
			LOGWARNING("Setting LC_ALL env variable to: %s", getenv("LC_ALL"));
	}
}
#else
static void FixLocales()
{
	// Do nothing on Windows
}
#endif

void EarlyInit()
{
	// If you ever want to catch a particular allocation:
	//_CrtSetBreakAlloc(232647);

	Threading::SetMainThread();

	debug_SetThreadName("main");
	// add all debug_printf "tags" that we are interested in:
	debug_filter_add("TIMER");
	debug_filter_add("FILES");

	timer_Init();

	// initialise profiler early so it can profile startup,
	// but only after LatchStartTime
	g_Profiler2.Initialise();

	FixLocales();

	// Because we do GL calls from a secondary thread, Xlib needs to
	// be told to support multiple threads safely.
	// This is needed for Atlas, but we have to call it before any other
	// Xlib functions (e.g. the ones used when drawing the main menu
	// before launching Atlas)
#if MUST_INIT_X11
	int status = XInitThreads();
	if (status == 0)
		debug_printf("Error enabling thread-safety via XInitThreads\n");
#endif

	// Initialise the low-quality rand function
	srand(time(NULL));	// NOTE: this rand should *not* be used for simulation!
}

bool Autostart(const CmdLineArgs& args);

/**
 * Returns true if the user has intended to start a visual replay from command line.
 */
bool AutostartVisualReplay(const std::string& replayFile);

bool Init(const CmdLineArgs& args, int flags)
{
	// Do this as soon as possible, because it chdirs
	// and will mess up the error reporting if anything
	// crashes before the working directory is set.
	InitVfs(args, flags);

	// This must come after VFS init, which sets the current directory
	// (required for finding our output log files).
	g_Logger = new CLogger;

	new CProfileViewer;
	new CProfileManager;	// before any script code

	g_ScriptStatsTable = new CScriptStatsTable;
	g_ProfileViewer.AddRootTable(g_ScriptStatsTable);

	// Set up the console early, so that debugging
	// messages can be logged to it. (The console's size
	// and fonts are set later in InitPs())
	g_Console = new CConsole();

	// g_ConfigDB, command line args, globals
	CONFIG_Init(args);

	// Using a global object for the context is a workaround until Simulation and AI use
	// their own threads and also their own contexts.
	const int contextSize = 384 * 1024 * 1024;
	const int heapGrowthBytesGCTrigger = 20 * 1024 * 1024;
	g_ScriptContext = ScriptContext::CreateContext(contextSize, heapGrowthBytesGCTrigger);

	// On the first Init (INIT_MODS), check for command-line arguments
	// or use the default mods from the config and enable those.
	// On later engine restarts (e.g. the mod selector), we will skip this path,
	// to avoid overwriting the newly selected mods.
	if (flags & INIT_MODS)
	{
		ScriptInterface modInterface("Engine", "Mod", g_ScriptContext);
		g_Mods.UpdateAvailableMods(modInterface);
		std::vector<CStr> mods;
		if (args.Has("mod"))
			mods = args.GetMultiple("mod");
		else
		{
			CStr modsStr;
			CFG_GET_VAL("mod.enabledmods", modsStr);
			boost::split(mods, modsStr, boost::algorithm::is_space(), boost::token_compress_on);
		}

		if (!g_Mods.EnableMods(mods, flags & INIT_MODS_PUBLIC))
		{
			// In non-visual mode, fail entirely.
			if (args.Has("autostart-nonvisual"))
			{
				LOGERROR("Trying to start with incompatible mods: %s.", boost::algorithm::join(g_Mods.GetIncompatibleMods(), ", "));
				return false;
			}
		}
	}
	// If there are incompatible mods, switch to the mod selector so players can resolve the problem.
	if (g_Mods.GetIncompatibleMods().empty())
		MountMods(Paths(args), g_Mods.GetEnabledMods());
	else
		MountMods(Paths(args), { "mod" });

	// Special command-line mode to dump the entity schemas instead of running the game.
	// (This must be done after loading VFS etc, but should be done before wasting time
	// on anything else.)
	if (args.Has("dumpSchema"))
	{
		CSimulation2 sim(NULL, g_ScriptContext, NULL);
		sim.LoadDefaultScripts();
		std::ofstream f("entity.rng", std::ios_base::out | std::ios_base::trunc);
		f << sim.GenerateSchema();
		debug_printf("Generated entity.rng\n");
		return false;
	}

	CNetHost::Initialize();

#if CONFIG2_AUDIO
	if (!args.Has("autostart-nonvisual") && !g_DisableAudio)
		ISoundManager::CreateSoundManager();
#endif

	new L10n;

	// Optionally start profiler HTTP output automatically
	// (By default it's only enabled by a hotkey, for security/performance)
	bool profilerHTTPEnable = false;
	CFG_GET_VAL("profiler2.autoenable", profilerHTTPEnable);
	if (profilerHTTPEnable)
		g_Profiler2.EnableHTTP();

	// Initialise everything except Win32 sockets (because our networking
	// system already inits those)
	curl_global_init(CURL_GLOBAL_ALL & ~CURL_GLOBAL_WIN32);

	if (!g_Quickstart)
		g_UserReporter.Initialize(); // after config

	PROFILE2_EVENT("Init finished");
	return true;
}

void InitGraphics(const CmdLineArgs& args, int flags, const std::vector<CStr>& installedMods)
{
	const bool setup_vmode = (flags & INIT_HAVE_VMODE) == 0;

	if(setup_vmode)
	{
		InitSDL();

		if (!g_VideoMode.InitSDL())
			throw PSERROR_System_VmodeFailed(); // abort startup
	}

	RunHardwareDetection();

	// Optionally start profiler GPU timings automatically
	// (By default it's only enabled by a hotkey, for performance/compatibility)
	bool profilerGPUEnable = false;
	CFG_GET_VAL("profiler2.autoenable", profilerGPUEnable);
	if (profilerGPUEnable)
		g_Profiler2.EnableGPU();

	if(!g_Quickstart)
	{
		WriteSystemInfo();
		// note: no longer vfs_display here. it's dog-slow due to unbuffered
		// file output and very rarely needed.
	}

	if(g_DisableAudio)
		ISoundManager::SetEnabled(false);

	g_GUI = new CGUIManager();

	CStr8 renderPath = "default";
	CFG_GET_VAL("renderpath", renderPath);
	if (RenderPathEnum::FromString(renderPath) == FIXED)
	{
		// It doesn't make sense to continue working here, because we're not
		// able to display anything.
		DEBUG_DISPLAY_FATAL_ERROR(
			L"Your graphics card doesn't appear to be fully compatible with OpenGL shaders."
			L" The game does not support pre-shader graphics cards."
			L" You are advised to try installing newer drivers and/or upgrade your graphics card."
			L" For more information, please see http://www.wildfiregames.com/forum/index.php?showtopic=16734"
		);
	}

	g_RenderingOptions.ReadConfigAndSetupHooks();

	// create renderer
	new CRenderer(g_VideoMode.GetBackendDevice());

	InitInput();

	// TODO: Is this the best place for this?
	if (VfsDirectoryExists(L"maps/"))
		CXeromyces::AddValidator(g_VFS, "map", "maps/scenario.rng");

	try
	{
		if (!AutostartVisualReplay(args.Get("replay-visual")) && !Autostart(args))
		{
			const bool setup_gui = ((flags & INIT_NO_GUI) == 0);
			// We only want to display the splash screen at startup
			std::shared_ptr<ScriptInterface> scriptInterface = g_GUI->GetScriptInterface();
			ScriptRequest rq(scriptInterface);
			JS::RootedValue data(rq.cx);
			if (g_GUI)
			{
				Script::CreateObject(rq, &data, "isStartup", true);
				if (!installedMods.empty())
					Script::SetProperty(rq, data, "installedMods", installedMods);
			}
			InitPs(setup_gui, installedMods.empty() ? L"page_pregame.xml" : L"page_modmod.xml", g_GUI->GetScriptInterface().get(), data);
		}
	}
	catch (PSERROR_Game_World_MapLoadFailed& e)
	{
		// Map Loading failed

		// Start the engine so we have a GUI
		InitPs(true, L"page_pregame.xml", NULL, JS::UndefinedHandleValue);

		// Call script function to do the actual work
		//	(delete game data, switch GUI page, show error, etc.)
		CancelLoad(CStr(e.what()).FromUTF8());
	}
}

bool InitNonVisual(const CmdLineArgs& args)
{
	return Autostart(args);
}

/**
 * Temporarily loads a scenario map and retrieves the "ScriptSettings" JSON
 * data from it.
 * The scenario map format is used for scenario and skirmish map types (random
 * games do not use a "map" (format) but a small JavaScript program which
 * creates a map on the fly). It contains a section to initialize the game
 * setup screen.
 * @param mapPath Absolute path (from VFS root) to the map file to peek in.
 * @return ScriptSettings in JSON format extracted from the map.
 */
CStr8 LoadSettingsOfScenarioMap(const VfsPath &mapPath)
{
	CXeromyces mapFile;
	const char *pathToSettings[] =
	{
		"Scenario", "ScriptSettings", ""	// Path to JSON data in map
	};

	Status loadResult = mapFile.Load(g_VFS, mapPath);

	if (INFO::OK != loadResult)
	{
		LOGERROR("LoadSettingsOfScenarioMap: Unable to load map file '%s'", mapPath.string8());
		throw PSERROR_Game_World_MapLoadFailed("Unable to load map file, check the path for typos.");
	}
	XMBElement mapElement = mapFile.GetRoot();

	// Select the ScriptSettings node in the map file...
	for (int i = 0; pathToSettings[i][0]; ++i)
	{
		int childId = mapFile.GetElementID(pathToSettings[i]);

		XMBElementList nodes = mapElement.GetChildNodes();
		auto it = std::find_if(nodes.begin(), nodes.end(), [&childId](const XMBElement& child) {
			return child.GetNodeName() == childId;
		});

		if (it != nodes.end())
			mapElement = *it;
	}
	// ... they contain a JSON document to initialize the game setup
	// screen
	return mapElement.GetText();
}

// TODO: this essentially duplicates the CGUI logic to load directory or scripts.
// NB: this won't make sure to not double-load scripts, unlike the GUI.
void AutostartLoadScript(const ScriptInterface& scriptInterface, const VfsPath& path)
{
	if (path.IsDirectory())
	{
		VfsPaths pathnames;
		vfs::GetPathnames(g_VFS, path, L"*.js", pathnames);
		for (const VfsPath& file : pathnames)
			scriptInterface.LoadGlobalScriptFile(file);
	}
	else
		scriptInterface.LoadGlobalScriptFile(path);
}

// TODO: this essentially duplicates the CGUI function
CParamNode GetTemplate(const std::string& templateName)
{
	// This is very cheap to create so let's just do it every time.
	CTemplateLoader templateLoader;

	const CParamNode& templateRoot = templateLoader.GetTemplateFileData(templateName).GetOnlyChild();
	if (!templateRoot.IsOk())
		LOGERROR("Invalid template found for '%s'", templateName.c_str());

	return templateRoot;
}

/*
 * Command line options for autostart
 * (keep synchronized with binaries/system/readme.txt):
 *
 * -autostart="TYPEDIR/MAPNAME"    enables autostart and sets MAPNAME;
 *                                 TYPEDIR is skirmishes, scenarios, or random
 * -autostart-biome=BIOME          sets BIOME for a random map
 * -autostart-seed=SEED            sets randomization seed value (default 0, use -1 for random)
 * -autostart-ai=PLAYER:AI         sets the AI for PLAYER (e.g. 2:petra)
 * -autostart-aidiff=PLAYER:DIFF   sets the DIFFiculty of PLAYER's AI
 *                                 (default 3, 0: sandbox, 5: very hard)
 * -autostart-aiseed=AISEED        sets the seed used for the AI random
 *                                 generator (default 0, use -1 for random)
 * -autostart-player=NUMBER        sets the playerID in non-networked games (default 1, use -1 for observer)
 * -autostart-civ=PLAYER:CIV       sets PLAYER's civilisation to CIV (skirmish and random maps only).
 *                                 Use random for a random civ.
 * -autostart-team=PLAYER:TEAM     sets the team for PLAYER (e.g. 2:2).
 * -autostart-ceasefire=NUM        sets a ceasefire duration NUM
 *                                 (default 0 minutes)
 * -autostart-nonvisual            disable any graphics and sounds
 * -autostart-victory=SCRIPTNAME   sets the victory conditions with SCRIPTNAME
 *                                 located in simulation/data/settings/victory_conditions/
 *                                 (default conquest). When the first given SCRIPTNAME is
 *                                 "endless", no victory conditions will apply.
 * -autostart-wonderduration=NUM   sets the victory duration NUM for wonder victory condition
 *                                 (default 10 minutes)
 * -autostart-relicduration=NUM    sets the victory duration NUM for relic victory condition
 *                                 (default 10 minutes)
 * -autostart-reliccount=NUM       sets the number of relics for relic victory condition
 *                                 (default 2 relics)
 * -autostart-disable-replay       disable saving of replays
 *
 * Multiplayer:
 * -autostart-playername=NAME      sets local player NAME (default 'anonymous')
 * -autostart-host                 sets multiplayer host mode
 * -autostart-host-players=NUMBER  sets NUMBER of human players for multiplayer
 *                                 game (default 2)
 * -autostart-client=IP            sets multiplayer client to join host at
 *                                 given IP address
 * Random maps only:
 * -autostart-size=TILES           sets random map size in TILES (default 192)
 * -autostart-players=NUMBER       sets NUMBER of players on random map
 *                                 (default 2)
 *
 * Examples:
 * 1) "Bob" will host a 2 player game on the Arcadia map:
 * -autostart="scenarios/arcadia" -autostart-host -autostart-host-players=2 -autostart-playername="Bob"
 *  "Alice" joins the match as player 2:
 * -autostart-client=127.0.0.1 -autostart-playername="Alice"
 * The players use the developer overlay to control players.
 *
 * 2) Load Alpine Lakes random map with random seed, 2 players (Athens and Britons), and player 2 is PetraBot:
 * -autostart="random/alpine_lakes" -autostart-seed=-1 -autostart-players=2 -autostart-civ=1:athen -autostart-civ=2:brit -autostart-ai=2:petra
 *
 * 3) Observe the PetraBot on a triggerscript map:
 * -autostart="random/jebel_barkal" -autostart-seed=-1 -autostart-players=2 -autostart-civ=1:athen -autostart-civ=2:brit -autostart-ai=1:petra -autostart-ai=2:petra -autostart-player=-1
 */
bool Autostart(const CmdLineArgs& args)
{
	if (!args.Has("autostart-client") && !args.Has("autostart"))
		return false;

	// Get optional playername.
	CStrW userName = L"anonymous";
	if (args.Has("autostart-playername"))
		userName = args.Get("autostart-playername").FromUTF8();

	// Create some scriptinterface to store the js values for the settings.
	ScriptInterface scriptInterface("Engine", "Game Setup", g_ScriptContext);

	ScriptRequest rq(scriptInterface);

	// We use the javascript gameSettings to handle options, but that requires running JS.
	// Since we don't want to use the full Gui manager, we load an entrypoint script
	// that can run the priviledged "LoadScript" function, and then call the appropriate function.
	ScriptFunction::Register<&AutostartLoadScript>(rq, "LoadScript");
	// Load the entire folder to allow mods to extend the entrypoint without copying the whole file.
	AutostartLoadScript(scriptInterface, VfsPath(L"autostart/"));

	// Provide some required functions to the script.
	if (args.Has("autostart-nonvisual"))
		ScriptFunction::Register<&GetTemplate>(rq, "GetTemplate");
	else
	{
		JSI_GUIManager::RegisterScriptFunctions(rq);
		// TODO: this loads pregame, which is hardcoded to exist by various code paths. That ought be changed.
		InitPs(false, L"page_pregame.xml", g_GUI->GetScriptInterface().get(), JS::UndefinedHandleValue);
	}

	JSI_Game::RegisterScriptFunctions(rq);
	JSI_Main::RegisterScriptFunctions(rq);
	JSI_Simulation::RegisterScriptFunctions(rq);
	JSI_VFS::RegisterScriptFunctions_ReadWriteAnywhere(rq);
	JSI_Network::RegisterScriptFunctions(rq);

	JS::RootedValue sessionInitData(rq.cx);

	if (args.Has("autostart-client"))
	{
		CStr ip = args.Get("autostart-client");
		if (ip.empty())
			ip = "127.0.0.1";

		Script::CreateObject(
			rq,
			&sessionInitData,
			"playerName", userName,
			"ip", ip,
			"port", PS_DEFAULT_PORT,
			"storeReplay", !args.Has("autostart-disable-replay"));

		JS::RootedValue global(rq.cx, rq.globalValue());
		if (!ScriptFunction::CallVoid(rq, global, "autostartClient", sessionInitData, true))
			return false;

		bool shouldQuit = false;
		while (!shouldQuit)
		{
			g_NetClient->Poll();
			ScriptFunction::Call(rq, global, "onTick", shouldQuit);
			std::this_thread::sleep_for(std::chrono::microseconds(200));
		}

		if (args.Has("autostart-nonvisual"))
		{
			LDR_NonprogressiveLoad();
			g_Game->ReallyStartGame();
		}
		return true;
	}

	CStr autoStartName = args.Get("autostart");

	if (autoStartName.empty())
		return false;

	JS::RootedValue attrs(rq.cx);
	JS::RootedValue settings(rq.cx);
	JS::RootedValue playerData(rq.cx);

	Script::CreateObject(rq, &attrs);
	Script::CreateObject(rq, &settings);
	Script::CreateArray(rq, &playerData);

	// The directory in front of the actual map name indicates which type
	// of map is being loaded. Drawback of this approach is the association
	// of map types and folders is hard-coded, but benefits are:
	// - No need to pass the map type via command line separately
	// - Prevents mixing up of scenarios and skirmish maps to some degree
	Path mapPath = Path(autoStartName);
	std::wstring mapDirectory = mapPath.Parent().Filename().string();
	std::string mapType;

	if (mapDirectory == L"random")
	{
		// Get optional map size argument (default 192)
		uint mapSize = 192;
		if (args.Has("autostart-size"))
		{
			CStr size = args.Get("autostart-size");
			mapSize = size.ToUInt();
		}

		Script::SetProperty(rq, settings, "Size", mapSize);		// Random map size (in patches)

		if (args.Has("autostart-biome"))
		{
			CStr biome = args.Get("autostart-biome");
			Script::SetProperty(rq, settings, "Biome", biome);
		}

		// Get optional number of players (default 2)
		size_t numPlayers = 2;
		if (args.Has("autostart-players"))
		{
			CStr num = args.Get("autostart-players");
			numPlayers = num.ToUInt();
		}

		// Set up player data
		for (size_t i = 0; i < numPlayers; ++i)
		{
			JS::RootedValue player(rq.cx);

			// We could load player_defaults.json here, but that would complicate the logic
			// even more and autostart is only intended for developers anyway
			Script::CreateObject(rq, &player, "Civ", "athen");

			Script::SetPropertyInt(rq, playerData, i, player);
		}
		mapType = "random";
	}
	else if (mapDirectory == L"scenarios")
		mapType = "scenario";
	else if (mapDirectory == L"skirmishes")
		mapType = "skirmish";
	else
	{
		LOGERROR("Autostart: Unrecognized map type '%s'", utf8_from_wstring(mapDirectory));
		throw PSERROR_Game_World_MapLoadFailed("Unrecognized map type.\nConsult readme.txt for the currently supported types.");
	}

	Script::SetProperty(rq, attrs, "mapType", mapType);
	Script::SetProperty(rq, attrs, "map", "maps/" + autoStartName);
	Script::SetProperty(rq, settings, "mapType", mapType);
	Script::SetProperty(rq, settings, "CheatsEnabled", true);

	// The seed is used for both random map generation and simulation
	u32 seed = 0;
	if (args.Has("autostart-seed"))
	{
		CStr seedArg = args.Get("autostart-seed");
		if (seedArg == "-1")
			seed = rand();
		else
			seed = seedArg.ToULong();
	}
	Script::SetProperty(rq, settings, "Seed", seed);

	// Set seed for AIs
	u32 aiseed = 0;
	if (args.Has("autostart-aiseed"))
	{
		CStr seedArg = args.Get("autostart-aiseed");
		if (seedArg == "-1")
			aiseed = rand();
		else
			aiseed = seedArg.ToULong();
	}
	Script::SetProperty(rq, settings, "AISeed", aiseed);

	// Set player data for AIs
	//		attrs.settings = { PlayerData: [ { AI: ... }, ... ] }
	//		            or = { PlayerData: [ null, { AI: ... }, ... ] } when gaia set
	int offset = 1;
	JS::RootedValue player(rq.cx);
	if (Script::GetPropertyInt(rq, playerData, 0, &player) && player.isNull())
		offset = 0;

	// Set teams
	if (args.Has("autostart-team"))
	{
		std::vector<CStr> civArgs = args.GetMultiple("autostart-team");
		for (size_t i = 0; i < civArgs.size(); ++i)
		{
			int playerID = civArgs[i].BeforeFirst(":").ToInt();

			// Instead of overwriting existing player data, modify the array
			JS::RootedValue currentPlayer(rq.cx);
			if (!Script::GetPropertyInt(rq, playerData, playerID-offset, &currentPlayer) || currentPlayer.isUndefined())
				Script::CreateObject(rq, &currentPlayer);

			int teamID = civArgs[i].AfterFirst(":").ToInt() - 1;
			Script::SetProperty(rq, currentPlayer, "Team", teamID);
			Script::SetPropertyInt(rq, playerData, playerID-offset, currentPlayer);
		}
	}

	int ceasefire = 0;
	if (args.Has("autostart-ceasefire"))
		ceasefire = args.Get("autostart-ceasefire").ToInt();
	Script::SetProperty(rq, settings, "Ceasefire", ceasefire);

	if (args.Has("autostart-ai"))
	{
		std::vector<CStr> aiArgs = args.GetMultiple("autostart-ai");
		for (size_t i = 0; i < aiArgs.size(); ++i)
		{
			int playerID = aiArgs[i].BeforeFirst(":").ToInt();

			// Instead of overwriting existing player data, modify the array
			JS::RootedValue currentPlayer(rq.cx);
			if (!Script::GetPropertyInt(rq, playerData, playerID-offset, &currentPlayer) || currentPlayer.isUndefined())
				Script::CreateObject(rq, &currentPlayer);

			Script::SetProperty(rq, currentPlayer, "AI", aiArgs[i].AfterFirst(":"));
			Script::SetProperty(rq, currentPlayer, "AIDiff", 3);
			Script::SetProperty(rq, currentPlayer, "AIBehavior", "balanced");
			Script::SetPropertyInt(rq, playerData, playerID-offset, currentPlayer);
		}
	}
	// Set AI difficulty
	if (args.Has("autostart-aidiff"))
	{
		std::vector<CStr> civArgs = args.GetMultiple("autostart-aidiff");
		for (size_t i = 0; i < civArgs.size(); ++i)
		{
			int playerID = civArgs[i].BeforeFirst(":").ToInt();

			// Instead of overwriting existing player data, modify the array
			JS::RootedValue currentPlayer(rq.cx);
			if (!Script::GetPropertyInt(rq, playerData, playerID-offset, &currentPlayer) || currentPlayer.isUndefined())
				Script::CreateObject(rq, &currentPlayer);

			Script::SetProperty(rq, currentPlayer, "AIDiff", civArgs[i].AfterFirst(":").ToInt());
			Script::SetPropertyInt(rq, playerData, playerID-offset, currentPlayer);
		}
	}
	// Set player data for Civs
	if (args.Has("autostart-civ"))
	{
		if (mapDirectory != L"scenarios")
		{
			std::vector<CStr> civArgs = args.GetMultiple("autostart-civ");
			for (size_t i = 0; i < civArgs.size(); ++i)
			{
				int playerID = civArgs[i].BeforeFirst(":").ToInt();

				// Instead of overwriting existing player data, modify the array
				JS::RootedValue currentPlayer(rq.cx);
				if (!Script::GetPropertyInt(rq, playerData, playerID-offset, &currentPlayer) || currentPlayer.isUndefined())
					Script::CreateObject(rq, &currentPlayer);

				Script::SetProperty(rq, currentPlayer, "Civ", civArgs[i].AfterFirst(":"));
				Script::SetPropertyInt(rq, playerData, playerID-offset, currentPlayer);
			}
		}
		else
			LOGWARNING("Autostart: Option 'autostart-civ' is invalid for scenarios");
	}

	// Add additional scripts to the TriggerScripts property
	std::vector<CStrW> triggerScriptsVector;
	JS::RootedValue triggerScripts(rq.cx);

	if (Script::HasProperty(rq, settings, "TriggerScripts"))
	{
		Script::GetProperty(rq, settings, "TriggerScripts", &triggerScripts);
		Script::FromJSVal(rq, triggerScripts, triggerScriptsVector);
	}

	if (!CRenderer::IsInitialised())
	{
		CStr nonVisualScript = "scripts/NonVisualTrigger.js";
		triggerScriptsVector.push_back(nonVisualScript.FromUTF8());
	}

	Script::ToJSVal(rq, &triggerScripts, triggerScriptsVector);
	Script::SetProperty(rq, settings, "TriggerScripts", triggerScripts);

	std::vector<CStr> victoryConditions(1, "conquest");
	if (args.Has("autostart-victory"))
		victoryConditions = args.GetMultiple("autostart-victory");

	if (victoryConditions.size() == 1 && victoryConditions[0] == "endless")
		victoryConditions.clear();

	Script::SetProperty(rq, settings, "VictoryConditions", victoryConditions);

	int wonderDuration = 10;
	if (args.Has("autostart-wonderduration"))
		wonderDuration = args.Get("autostart-wonderduration").ToInt();
	Script::SetProperty(rq, settings, "WonderDuration", wonderDuration);

	int relicDuration = 10;
	if (args.Has("autostart-relicduration"))
		relicDuration = args.Get("autostart-relicduration").ToInt();
	Script::SetProperty(rq, settings, "RelicDuration", relicDuration);

	int relicCount = 2;
	if (args.Has("autostart-reliccount"))
		relicCount = args.Get("autostart-reliccount").ToInt();
	Script::SetProperty(rq, settings, "RelicCount", relicCount);

	// Add player data to map settings.
	Script::SetProperty(rq, settings, "PlayerData", playerData);

	// Add map settings to game attributes.
	Script::SetProperty(rq, attrs, "settings", settings);

	if (args.Has("autostart-host"))
	{
		int maxPlayers = 2;
		if (args.Has("autostart-host-players"))
			maxPlayers = args.Get("autostart-host-players").ToUInt();

		Script::CreateObject(
			rq,
			&sessionInitData,
			"attribs", attrs,
			"playerName", userName,
			"port", PS_DEFAULT_PORT,
			"maxPlayers", maxPlayers,
			"storeReplay", !args.Has("autostart-disable-replay"));

		JS::RootedValue global(rq.cx, rq.globalValue());
		if (!ScriptFunction::CallVoid(rq, global, "autostartHost", sessionInitData, true))
			return false;

		// In MP host mode, we need to wait until clients have loaded.
		bool shouldQuit = false;
		while (!shouldQuit)
		{
			g_NetClient->Poll();
			ScriptFunction::Call(rq, global, "onTick", shouldQuit);
			std::this_thread::sleep_for(std::chrono::microseconds(200));
		}
	}
	else
	{
		JS::RootedValue localPlayer(rq.cx);
		Script::CreateObject(
			rq,
			&localPlayer,
			"player", args.Has("autostart-player") ? args.Get("autostart-player").ToInt() : 1,
			"name", userName);

		JS::RootedValue playerAssignments(rq.cx);
		Script::CreateObject(rq, &playerAssignments);
		Script::SetProperty(rq, playerAssignments, "local", localPlayer);

		Script::CreateObject(
			rq,
			&sessionInitData,
			"attribs", attrs,
			"playerAssignments", playerAssignments,
			"storeReplay", !args.Has("autostart-disable-replay"));

		JS::RootedValue global(rq.cx, rq.globalValue());
		if (!ScriptFunction::CallVoid(rq, global, "autostartHost", sessionInitData, false))
			return false;
	}

	if (args.Has("autostart-nonvisual"))
	{
		LDR_NonprogressiveLoad();
		g_Game->ReallyStartGame();
	}

	return true;
}

bool AutostartVisualReplay(const std::string& replayFile)
{
	if (!FileExists(OsPath(replayFile)))
		return false;

	g_Game = new CGame(false);
	g_Game->SetPlayerID(-1);
	g_Game->StartVisualReplay(replayFile);

	ScriptInterface& scriptInterface = g_Game->GetSimulation2()->GetScriptInterface();
	ScriptRequest rq(scriptInterface);
	JS::RootedValue attrs(rq.cx, g_Game->GetSimulation2()->GetInitAttributes());

	JS::RootedValue playerAssignments(rq.cx);
	Script::CreateObject(rq, &playerAssignments);
	JS::RootedValue localPlayer(rq.cx);
	Script::CreateObject(rq, &localPlayer, "player", g_Game->GetPlayerID());
	Script::SetProperty(rq, playerAssignments, "local", localPlayer);

	JS::RootedValue sessionInitData(rq.cx);

	Script::CreateObject(
		rq,
		&sessionInitData,
		"attribs", attrs,
		"playerAssignments", playerAssignments);

	InitPs(true, L"page_loading.xml", &scriptInterface, sessionInitData);

	return true;
}

void CancelLoad(const CStrW& message)
{
	std::shared_ptr<ScriptInterface> pScriptInterface = g_GUI->GetActiveGUI()->GetScriptInterface();
	ScriptRequest rq(pScriptInterface);

	JS::RootedValue global(rq.cx, rq.globalValue());

	LDR_Cancel();

	if (g_GUI &&
	    g_GUI->GetPageCount() &&
		Script::HasProperty(rq, global, "cancelOnLoadGameError"))
		ScriptFunction::CallVoid(rq, global, "cancelOnLoadGameError", message);
}

bool InDevelopmentCopy()
{
	if (!g_CheckedIfInDevelopmentCopy)
	{
		g_InDevelopmentCopy = (g_VFS->GetFileInfo(L"config/dev.cfg", NULL) == INFO::OK);
		g_CheckedIfInDevelopmentCopy = true;
	}
	return g_InDevelopmentCopy;
}
