/* Copyright (C) 2012 Wildfire Games.
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

#include "lib/app_hooks.h"
#include "lib/input.h"
#include "lib/ogl.h"
#include "lib/timer.h"
#include "lib/external_libraries/libsdl.h"
#include "lib/file/common/file_stats.h"
#include "lib/res/h_mgr.h"
#include "lib/res/graphics/cursor.h"
#include "lib/res/sound/snd_mgr.h"
#include "lib/sysdep/cursor.h"
#include "lib/sysdep/cpu.h"
#include "lib/sysdep/gfx.h"
#include "lib/tex/tex.h"
#if OS_WIN
#include "lib/sysdep/os/win/wversion.h"
#endif

#include "ps/CConsole.h"
#include "ps/CLogger.h"
#include "ps/ConfigDB.h"
#include "ps/Filesystem.h"
#include "ps/Font.h"
#include "ps/Game.h"
#include "ps/Globals.h"
#include "ps/Hotkey.h"
#include "ps/Joystick.h"
#include "ps/Loader.h"
#include "ps/Overlay.h"
#include "ps/Profile.h"
#include "ps/ProfileViewer.h"
#include "ps/Profiler2.h"
#include "ps/UserReport.h"
#include "ps/Util.h"
#include "ps/VideoMode.h"
#include "ps/World.h"

#include "graphics/CinemaTrack.h"
#include "graphics/GameView.h"
#include "graphics/LightEnv.h"
#include "graphics/MapReader.h"
#include "graphics/MaterialManager.h"
#include "graphics/TerrainTextureManager.h"

#include "renderer/Renderer.h"
#include "renderer/VertexBufferManager.h"
#include "renderer/ModelRenderer.h"

#include "maths/MathUtil.h"

#include "simulation2/Simulation2.h"

#include "scripting/ScriptingHost.h"
#include "scripting/ScriptGlue.h"

#include "scriptinterface/ScriptInterface.h"
#include "scriptinterface/ScriptStats.h"

#include "maths/scripting/JSInterface_Vector3D.h"

#include "ps/scripting/JSInterface_Console.h"

#include "gui/GUI.h"
#include "gui/GUIManager.h"
#include "gui/scripting/JSInterface_IGUIObject.h"
#include "gui/scripting/JSInterface_GUITypes.h"
#include "gui/scripting/ScriptFunctions.h"

#include "sound/JSI_Sound.h"

#include "network/NetServer.h"
#include "network/NetClient.h"

#include "ps/Pyrogenesis.h"	// psSetLogDir
#include "ps/GameSetup/Atlas.h"
#include "ps/GameSetup/GameSetup.h"
#include "ps/GameSetup/Paths.h"
#include "ps/GameSetup/Config.h"
#include "ps/GameSetup/CmdLineArgs.h"
#include "ps/GameSetup/HWDetect.h"

#if !(OS_WIN || OS_MACOSX) // assume all other platforms use X11 for wxWidgets
#define MUST_INIT_X11 1
#include <X11/Xlib.h>
#else
#define MUST_INIT_X11 0
#endif

#if OS_WIN
extern void wmi_Shutdown();
#endif

#include <iostream>

ERROR_GROUP(System);
ERROR_TYPE(System, SDLInitFailed);
ERROR_TYPE(System, VmodeFailed);
ERROR_TYPE(System, RequiredExtensionsMissing);

bool g_DoRenderGui = true;
bool g_DoRenderLogger = true;
bool g_DoRenderCursor = true;

static const int SANE_TEX_QUALITY_DEFAULT = 5;	// keep in sync with code

static void SetTextureQuality(int quality)
{
	int q_flags;
	GLint filter;

retry:
	// keep this in sync with SANE_TEX_QUALITY_DEFAULT
	switch(quality)
	{
		// worst quality
	case 0:
		q_flags = OGL_TEX_HALF_RES|OGL_TEX_HALF_BPP;
		filter = GL_NEAREST;
		break;
		// [perf] add bilinear filtering
	case 1:
		q_flags = OGL_TEX_HALF_RES|OGL_TEX_HALF_BPP;
		filter = GL_LINEAR;
		break;
		// [vmem] no longer reduce resolution
	case 2:
		q_flags = OGL_TEX_HALF_BPP;
		filter = GL_LINEAR;
		break;
		// [vmem] add mipmaps
	case 3:
		q_flags = OGL_TEX_HALF_BPP;
		filter = GL_NEAREST_MIPMAP_LINEAR;
		break;
		// [perf] better filtering
	case 4:
		q_flags = OGL_TEX_HALF_BPP;
		filter = GL_LINEAR_MIPMAP_LINEAR;
		break;
		// [vmem] no longer reduce bpp
	case SANE_TEX_QUALITY_DEFAULT:
		q_flags = OGL_TEX_FULL_QUALITY;
		filter = GL_LINEAR_MIPMAP_LINEAR;
		break;
		// [perf] add anisotropy
	case 6:
		// TODO: add anisotropic filtering
		q_flags = OGL_TEX_FULL_QUALITY;
		filter = GL_LINEAR_MIPMAP_LINEAR;
		break;
		// invalid
	default:
		debug_warn(L"SetTextureQuality: invalid quality");
		quality = SANE_TEX_QUALITY_DEFAULT;
		// careful: recursion doesn't work and we don't want to duplicate
		// the "sane" default values.
		goto retry;
	}

	ogl_tex_set_defaults(q_flags, filter);
}


//----------------------------------------------------------------------------
// GUI integration
//----------------------------------------------------------------------------

// display progress / description in loading screen
void GUI_DisplayLoadProgress(int percent, const wchar_t* pending_task)
{
	g_ScriptingHost.GetScriptInterface().SetGlobal("g_Progress", percent, true);
	g_ScriptingHost.GetScriptInterface().SetGlobal("g_LoadDescription", pending_task, true);
	g_GUI->SendEventToAll("progress");
}



void Render()
{
	PROFILE3("render");

	ogl_WarnIfError();

	g_Profiler2.RecordGPUFrameStart();

	ogl_WarnIfError();

	CStr skystring = "255 0 255";
	CFG_GET_USER_VAL("skycolor", String, skystring);
	CColor skycol;
	GUI<CColor>::ParseString(skystring.FromUTF8(), skycol);
	g_Renderer.SetClearColor(skycol.AsSColor4ub());

	// prepare before starting the renderer frame
	if (g_Game && g_Game->IsGameStarted())
		g_Game->GetView()->BeginFrame();

	// start new frame
	g_Renderer.BeginFrame();

	ogl_WarnIfError();

	if (g_Game && g_Game->IsGameStarted())
		g_Game->GetView()->Render();

	ogl_WarnIfError();

	// set up overlay mode
	glPushAttrib(GL_ENABLE_BIT);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_CULL_FACE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0.f, (float)g_xres, 0.f, (float)g_yres, -1.f, 1000.f);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	ogl_WarnIfError();

	g_Renderer.RenderTextOverlays();

	if (g_DoRenderGui)
		g_GUI->Draw();

	ogl_WarnIfError();

	// Text:

	// Use the GL_ALPHA texture as the alpha channel with a flat colouring
	glDisable(GL_ALPHA_TEST);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	// Added --
	glEnable(GL_TEXTURE_2D);
	// -- GL

	glLoadIdentity();

	g_Console->Render();

	ogl_WarnIfError();

	if (g_DoRenderLogger)
		g_Logger->Render();

	ogl_WarnIfError();

	// Profile information

	g_ProfileViewer.RenderProfile();

	ogl_WarnIfError();

	// Draw the cursor (or set the Windows cursor, on Windows)
	if (g_DoRenderCursor)
	{
		PROFILE3_GPU("cursor");
		CStrW cursorName = g_CursorName;
		if (cursorName.empty())
		{
			cursor_draw(g_VFS, NULL, g_mouse_x, g_yres-g_mouse_y);
		}
		else
		{
			if (cursor_draw(g_VFS, cursorName.c_str(), g_mouse_x, g_yres-g_mouse_y) < 0)
				LOGWARNING(L"Failed to draw cursor '%ls'", cursorName.c_str());
		}
	}

	// restore
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glPopAttrib();

	g_Renderer.EndFrame();

	PROFILE2_ATTR("draw calls: %d", (int)g_Renderer.GetStats().m_DrawCalls);
	PROFILE2_ATTR("terrain tris: %d", (int)g_Renderer.GetStats().m_TerrainTris);
	PROFILE2_ATTR("water tris: %d", (int)g_Renderer.GetStats().m_WaterTris);
	PROFILE2_ATTR("model tris: %d", (int)g_Renderer.GetStats().m_ModelTris);
	PROFILE2_ATTR("overlay tris: %d", (int)g_Renderer.GetStats().m_OverlayTris);
	PROFILE2_ATTR("blend splats: %d", (int)g_Renderer.GetStats().m_BlendSplats);
	PROFILE2_ATTR("particles: %d", (int)g_Renderer.GetStats().m_Particles);

	ogl_WarnIfError();

	g_Profiler2.RecordGPUFrameEnd();

	ogl_WarnIfError();
}


static void RegisterJavascriptInterfaces()
{
	// maths
	JSI_Vector3D::init();

	// graphics
	CGameView::ScriptingInit();

	// renderer
	CRenderer::ScriptingInit();

	// sound
	JSI_Sound::ScriptingInit();

	// ps
	JSI_Console::init();

	// GUI
	CGUI::ScriptingInit();

	GuiScriptingInit(g_ScriptingHost.GetScriptInterface());
}


static void InitScripting()
{
	TIMER(L"InitScripting");

	// Create the scripting host.  This needs to be done before the GUI is created.
	// [7ms]
	new ScriptingHost;
	
	RegisterJavascriptInterfaces();
}


static size_t OperatingSystemFootprint()
{
#if OS_WIN
	switch(wversion_Number())
	{
	case WVERSION_2K:
	case WVERSION_XP:
		return 150;
	case WVERSION_XP64:
		return 200;
	default:	// newer Windows version: assume the worst, and don't warn
	case WVERSION_VISTA:
		return 300;
	case WVERSION_7:
		return 250;
	}
#else
	return 200;
#endif
}

static size_t ChooseCacheSize()
{
	// (all sizes in MiB and signed to allow temporarily negative computations)

	const ssize_t total = (ssize_t)os_cpu_MemorySize();
	// (NB: os_cpu_MemoryAvailable is useless on Linux because free memory
	// is marked as "in use" by OS caches.)
	const ssize_t os = (ssize_t)OperatingSystemFootprint();
	const ssize_t game = 300;	// estimated working set

	ssize_t cache = 500;	// upper bound: total size of our data

	// the cache reserves contiguous address space, which is a precious
	// resource on 32-bit systems, so don't use too much:
	if(ARCH_IA32 || sizeof(void*) == 4)
		cache = std::min(cache, (ssize_t)200);

	// try to leave over enough memory for the OS and game
	cache = std::min(cache, total-os-game);

	// always provide at least this much to ensure correct operation
	cache = std::max(cache, (ssize_t)64);

	debug_printf(L"Cache: %d (total: %d) MiB\n", (int)cache, (int)total);
	return size_t(cache)*MiB;
}


ErrorReactionInternal psDisplayError(const wchar_t* UNUSED(text), size_t UNUSED(flags))
{
	// If we're fullscreen, then sometimes (at least on some particular drivers on Linux)
	// displaying the error dialog hangs the desktop since the dialog box is behind the
	// fullscreen window. So we just force the game to windowed mode before displaying the dialog.
	// (But only if we're in the main thread, and not if we're being reentrant.)
	if (ThreadUtil::IsMainThread())
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

static void InitVfs(const CmdLineArgs& args)
{
	TIMER(L"InitVfs");

	const Paths paths(args);

	OsPath logs(paths.Logs());
	CreateDirectories(logs, 0700);

	psSetLogDir(logs);
	// desired location for crashlog is now known. update AppHooks ASAP
	// (particularly before the following error-prone operations):
	AppHooks hooks = {0};
	hooks.bundle_logs = psBundleLogs;
	hooks.get_log_dir = psLogDir;
	hooks.display_error = psDisplayError;
	app_hooks_update(&hooks);

	const size_t cacheSize = ChooseCacheSize();
	g_VFS = CreateVfs(cacheSize);

	g_VFS->Mount(L"screenshots/", paths.Data()/"screenshots"/"");
	g_VFS->Mount(L"saves/", paths.Data()/"saves"/"");
	const OsPath readonlyConfig = paths.RData()/"config"/"";
	g_VFS->Mount(L"config/", readonlyConfig);
	if(readonlyConfig != paths.Config())
		g_VFS->Mount(L"config/", paths.Config());
	g_VFS->Mount(L"cache/", paths.Cache(), VFS_MOUNT_ARCHIVABLE);	// (adding XMBs to archive speeds up subsequent reads)

	std::vector<CStr> mods = args.GetMultiple("mod");
	mods.push_back("public");
	if(!args.Has("onlyPublicFiles"))
		mods.push_back("internal");

	OsPath modArchivePath = paths.Cache()/"mods";
	OsPath modLoosePath = paths.RData()/"mods";
	for (size_t i = 0; i < mods.size(); ++i)
	{
		size_t priority = i+1;	// mods are higher priority than regular mountings, which default to priority 0
		size_t flags = VFS_MOUNT_WATCH|VFS_MOUNT_ARCHIVABLE|VFS_MOUNT_MUST_EXIST;
		OsPath modName(mods[i]);
		g_VFS->Mount(L"", modLoosePath / modName/"", flags, priority);
		g_VFS->Mount(L"", modArchivePath / modName/"", flags, priority);
	}

	// note: don't bother with g_VFS->TextRepresentation - directories
	// haven't yet been populated and are empty.
}


static void InitPs(bool setup_gui, const CStrW& gui_page, CScriptVal initData)
{
	{
		// console
		TIMER(L"ps_console");

		g_Console->UpdateScreenSize(g_xres, g_yres);

		// Calculate and store the line spacing
		CFont font(CONSOLE_FONT);
		g_Console->m_iFontHeight = font.GetLineSpacing();
		g_Console->m_iFontWidth = font.GetCharacterWidth(L'C');
		g_Console->m_charsPerPage = (size_t)(g_xres / g_Console->m_iFontWidth);
		// Offset by an arbitrary amount, to make it fit more nicely
		g_Console->m_iFontOffset = 7;
	}

	// hotkeys
	{
		TIMER(L"ps_lang_hotkeys");
		LoadHotkeys();
	}

	if (!setup_gui)
	{
		// We do actually need *some* kind of GUI loaded, so use the
		// (currently empty) Atlas one
		g_GUI->SwitchPage(L"page_atlas.xml", initData);
		return;
	}

	// GUI uses VFS, so this must come after VFS init.
	g_GUI->SwitchPage(gui_page, initData);
}


static void InitInput()
{
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

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

	in_add_handler(conInputHandler);

	in_add_handler(HotkeyInputHandler);

	// gui_handler needs to be registered after (i.e. called before!) the
	// hotkey handler so that input boxes can be typed in without
	// setting off hotkeys.
	in_add_handler(gui_handler);

	// must be registered after (called before) the GUI which relies on these globals
	in_add_handler(GlobalsInputHandler);
}


static void ShutdownPs()
{
	SAFE_DELETE(g_GUI);

	SAFE_DELETE(g_Console);

	// disable the special Windows cursor, or free textures for OGL cursors
	cursor_draw(g_VFS, 0, g_mouse_x, g_yres-g_mouse_y);
}


static void InitRenderer()
{
	TIMER(L"InitRenderer");

	if(g_NoGLS3TC)
		ogl_tex_override(OGL_TEX_S3TC, OGL_TEX_DISABLE);
	if(g_NoGLAutoMipmap)
		ogl_tex_override(OGL_TEX_AUTO_MIPMAP_GEN, OGL_TEX_DISABLE);

	// create renderer
	new CRenderer;

	// set renderer options from command line options - NOVBO must be set before opening the renderer
	g_Renderer.SetOptionBool(CRenderer::OPT_NOVBO,g_NoGLVBO);
	g_Renderer.SetOptionBool(CRenderer::OPT_SHADOWS,g_Shadows);
	g_Renderer.SetOptionBool(CRenderer::OPT_FANCYWATER,g_FancyWater);
	g_Renderer.SetRenderPath(CRenderer::GetRenderPathByName(g_RenderPath));
	g_Renderer.SetOptionFloat(CRenderer::OPT_LODBIAS, g_LodBias);
	g_Renderer.SetOptionBool(CRenderer::OPT_SHADOWPCF, g_ShadowPCF);

	// create terrain related stuff
	new CTerrainTextureManager;

	// create the material manager
	new CMaterialManager;

	g_Renderer.Open(g_xres,g_yres);

	// Setup lighting environment. Since the Renderer accesses the
	// lighting environment through a pointer, this has to be done before
	// the first Frame.
	g_Renderer.SetLightEnv(&g_LightEnv);

	// I haven't seen the camera affecting GUI rendering and such, but the
	// viewport has to be updated according to the video mode
	SViewPort vp;
	vp.m_X=0;
	vp.m_Y=0;
	vp.m_Width=g_xres;
	vp.m_Height=g_yres;
	g_Renderer.SetViewport(vp);

	ColorActivateFastImpl();
	ModelRenderer::Init();
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
		LOGERROR(L"SDL library initialization failed: %hs", SDL_GetError());
		throw PSERROR_System_SDLInitFailed();
	}
	atexit(SDL_Quit);
	SDL_EnableUNICODE(1);
}

static void ShutdownSDL()
{
	SDL_Quit();
	sys_cursor_reset();
}


void EndGame()
{
	SAFE_DELETE(g_NetServer);
	SAFE_DELETE(g_NetClient);
	SAFE_DELETE(g_Game);
}


void Shutdown(int UNUSED(flags))
{
	EndGame();

	ShutdownPs(); // Must delete g_GUI before g_ScriptingHost

	in_reset_handlers();

	// destroy actor related stuff
	TIMER_BEGIN(L"shutdown actor stuff");
	delete &g_MaterialManager;
	TIMER_END(L"shutdown actor stuff");

	// destroy terrain related stuff
	TIMER_BEGIN(L"shutdown TexMan");
	delete &g_TexMan;
	TIMER_END(L"shutdown TexMan");

	// destroy renderer
	TIMER_BEGIN(L"shutdown Renderer");
	delete &g_Renderer;
	g_VBMan.Shutdown();
	TIMER_END(L"shutdown Renderer");

	tex_codec_unregister_all();

	g_Profiler2.ShutdownGPU();

	TIMER_BEGIN(L"shutdown SDL");
	ShutdownSDL();
	TIMER_END(L"shutdown SDL");

	g_VideoMode.Shutdown();

	TIMER_BEGIN(L"shutdown UserReporter");
	g_UserReporter.Deinitialize();
	TIMER_END(L"shutdown UserReporter");

	TIMER_BEGIN(L"shutdown ScriptingHost");
	delete &g_ScriptingHost;
	TIMER_END(L"shutdown ScriptingHost");

	TIMER_BEGIN(L"shutdown ConfigDB");
	delete &g_ConfigDB;
	TIMER_END(L"shutdown ConfigDB");

	// resource
	// first shut down all resource owners, and then the handle manager.
	TIMER_BEGIN(L"resource modules");
		snd_shutdown();

		g_VFS.reset();

		// this forcibly frees all open handles (thus preventing real leaks),
		// and makes further access to h_mgr impossible.
		h_mgr_shutdown();

		file_stats_dump();

	TIMER_END(L"resource modules");

	TIMER_BEGIN(L"shutdown misc");
		timer_DisplayClientTotals();

		CNetHost::Deinitialize();

		SAFE_DELETE(g_ScriptStatsTable);

		// should be last, since the above use them
		SAFE_DELETE(g_Logger);
		delete &g_Profiler;
		delete &g_ProfileViewer;
	TIMER_END(L"shutdown misc");

#if OS_WIN
	TIMER_BEGIN(L"shutdown wmi");
	wmi_Shutdown();
	TIMER_END(L"shutdown wmi");
#endif
}

#if OS_UNIX
static void FixLocales()
{
#if OS_MACOSX
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
		LOGWARNING(L"Invalid locale settings");

		for (size_t i = 0; i < ARRAY_SIZE(LocaleEnvVars); i++)
		{
			if (char* envval = getenv(LocaleEnvVars[i]))
				LOGWARNING(L"  %hs=\"%hs\"", LocaleEnvVars[i], envval);
			else
				LOGWARNING(L"  %hs=\"(unset)\"", LocaleEnvVars[i]);
		}

		// We should set LC_ALL since it overrides LANG
		if (setenv("LC_ALL", std::locale::classic().name().c_str(), 1))
			debug_warn(L"Invalid locale settings, and unable to set LC_ALL env variable.");
		else
			LOGWARNING(L"Setting LC_ALL env variable to: %hs", getenv("LC_ALL"));
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

	ThreadUtil::SetMainThread();

	debug_SetThreadName("main");
	// add all debug_printf "tags" that we are interested in:
	debug_filter_add(L"TIMER");

	timer_LatchStartTime();

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
		debug_printf(L"Error enabling thread-safety via XInitThreads\n");
#endif

	// Initialise the low-quality rand function
	srand(time(NULL));	// NOTE: this rand should *not* be used for simulation!
}

bool Autostart(const CmdLineArgs& args);

void Init(const CmdLineArgs& args, int UNUSED(flags))
{
	h_mgr_init();

	// Do this as soon as possible, because it chdirs
	// and will mess up the error reporting if anything
	// crashes before the working directory is set.
	InitVfs(args);

	// This must come after VFS init, which sets the current directory
	// (required for finding our output log files).
	g_Logger = new CLogger;

	// Special command-line mode to dump the entity schemas instead of running the game.
	// (This must be done after loading VFS etc, but should be done before wasting time
	// on anything else.)
	if (args.Has("dumpSchema"))
	{
		CSimulation2 sim(NULL, NULL);
		sim.LoadDefaultScripts();
		std::ofstream f("entity.rng", std::ios_base::out | std::ios_base::trunc);
		f << sim.GenerateSchema();
		std::cout << "Generated entity.rng\n";
		exit(0);
	}

	// override ah_translate with our i18n code.
	AppHooks hooks = {0};
	hooks.translate = psTranslate;
	hooks.translate_free = psTranslateFree;
	app_hooks_update(&hooks);

	// Set up the console early, so that debugging
	// messages can be logged to it. (The console's size
	// and fonts are set later in InitPs())
	g_Console = new CConsole();

	CNetHost::Initialize();

	new CProfileViewer;
	new CProfileManager;	// before any script code

	g_ScriptStatsTable = new CScriptStatsTable;
	g_ProfileViewer.AddRootTable(g_ScriptStatsTable);

	InitScripting();	// before GUI

	// g_ConfigDB, command line args, globals
	CONFIG_Init(args);

	// Optionally start profiler HTTP output automatically
	// (By default it's only enabled by a hotkey, for security/performance)
	bool profilerHTTPEnable = false;
	CFG_GET_USER_VAL("profiler2.http.autoenable", Bool, profilerHTTPEnable);
	if (profilerHTTPEnable)
		g_Profiler2.EnableHTTP();

	if (!g_Quickstart)
		g_UserReporter.Initialize(); // after config

	PROFILE2_EVENT("Init finished");
}

void InitGraphics(const CmdLineArgs& args, int flags)
{
	const bool setup_vmode = (flags & INIT_HAVE_VMODE) == 0;

	if(setup_vmode)
	{
		InitSDL();

		if (!g_VideoMode.InitSDL())
			throw PSERROR_System_VmodeFailed(); // abort startup

		SDL_WM_SetCaption("0 A.D.", "0 A.D.");
	}

	RunHardwareDetection();

	tex_codec_register_all();

	const int quality = SANE_TEX_QUALITY_DEFAULT;	// TODO: set value from config file
	SetTextureQuality(quality);

	ogl_WarnIfError();

	// Optionally start profiler GPU timings automatically
	// (By default it's only enabled by a hotkey, for performance/compatibility)
	bool profilerGPUEnable = false;
	CFG_GET_USER_VAL("profiler2.gpu.autoenable", Bool, profilerGPUEnable);
	if (profilerGPUEnable)
		g_Profiler2.EnableGPU();

	if(!g_Quickstart)
	{
		WriteSystemInfo();
		// note: no longer vfs_display here. it's dog-slow due to unbuffered
		// file output and very rarely needed.
	}

	if(g_DisableAudio)
	{
		// speed up startup by disabling all sound
		// (OpenAL init will be skipped).
		// must be called before first snd_open.
		snd_disable(true);
	}

	g_GUI = new CGUIManager(g_ScriptingHost.GetScriptInterface());

	// (must come after SetVideoMode, since it calls ogl_Init)
	const char* missing = ogl_HaveExtensions(0,
		"GL_ARB_multitexture",
		"GL_EXT_draw_range_elements",
		"GL_ARB_texture_env_combine",
		"GL_ARB_texture_env_dot3",
		NULL);
	if(missing)
	{
		wchar_t buf[500];
		swprintf_s(buf, ARRAY_SIZE(buf),
			L"The %hs extension doesn't appear to be available on your computer."
			L" The game may still work, though - you are welcome to try at your own risk."
			L" If not or it doesn't look right, upgrade your graphics card.",
			missing
		);
		DEBUG_DISPLAY_ERROR(buf);
		// TODO: i18n
	}

	if (!ogl_HaveExtension("GL_ARB_texture_env_crossbar"))
	{
		DEBUG_DISPLAY_ERROR(
			L"The GL_ARB_texture_env_crossbar extension doesn't appear to be available on your computer."
			L" Shadows are not available and overall graphics quality might suffer."
			L" You are advised to try installing newer drivers and/or upgrade your graphics card.");
		g_Shadows = false;
	}

	ogl_WarnIfError();
	InitRenderer();

	InitInput();

	ogl_WarnIfError();

	try
	{
		if (!Autostart(args))
		{
			const bool setup_gui = ((flags & INIT_NO_GUI) == 0);
			InitPs(setup_gui, L"page_pregame.xml", JSVAL_VOID);
		}
	}
	catch (PSERROR_Game_World_MapLoadFailed e)
	{
		// Map Loading failed

		// Start the engine so we have a GUI
		InitPs(true, L"page_pregame.xml", JSVAL_VOID);

		// Call script function to do the actual work
		//	(delete game data, switch GUI page, show error, etc.)
		CancelLoad(CStr(e.what()).FromUTF8());
	}
}

void RenderGui(bool RenderingState)
{
	g_DoRenderGui = RenderingState;
}

void RenderLogger(bool RenderingState)
{
	g_DoRenderLogger = RenderingState;
}

void RenderCursor(bool RenderingState)
{
	g_DoRenderCursor = RenderingState;
}

bool Autostart(const CmdLineArgs& args)
{
	/*
	 * Handle various command-line options, for quick testing of various features:
	 * -autostart=name					-- map name for scenario, or rms name for random map
	 * -autostart-ai=1:dummybot			-- adds the dummybot AI to player 1
	 * -autostart-playername=name		-- multiplayer player name
	 * -autostart-host					-- multiplayer host mode
	 * -autostart-players=2				-- number of players
	 * -autostart-client				-- multiplayer client mode
	 * -autostart-ip=127.0.0.1			-- multiplayer connect to 127.0.0.1
	 * -autostart-random=104			-- random map, optional seed value = 104 (default is 0, random is -1)
	 * -autostart-size=192				-- random map size in tiles = 192 (default is 192)
	 *
	 * Examples:
	 * -autostart=Acropolis -autostart-host -autostart-players=2		-- Host game on Acropolis map, 2 players
	 * -autostart=latium -autostart-random=-1							-- Start single player game on latium random map, random rng seed
	 */

	CStr autoStartName = args.Get("autostart");
	if (autoStartName.empty())
	{
		return false;
	}

	g_Game = new CGame();

	ScriptInterface& scriptInterface = g_Game->GetSimulation2()->GetScriptInterface();

	CScriptValRooted attrs;
	scriptInterface.Eval("({})", attrs);
	CScriptVal settings;
	scriptInterface.Eval("({})", settings);
	CScriptVal playerData;
	scriptInterface.Eval("([])", playerData);

	// Set different attributes for random or scenario game
	if (args.Has("autostart-random"))
	{
		CStr seedArg = args.Get("autostart-random");

		// Default seed is 0
		uint32 seed = 0;
		if (!seedArg.empty())
		{
			if (seedArg.compare("-1") == 0)
			{	// Random seed value
				seed = rand();
			}
			else
			{
				seed = seedArg.ToULong();
			}
		}
		
		// Random map definition will be loaded from JSON file, so we need to parse it
		std::wstring mapPath = L"maps/random/";
		std::wstring scriptPath = mapPath + autoStartName.FromUTF8() + L".json";
		CScriptValRooted scriptData = scriptInterface.ReadJSONFile(scriptPath);
		if (!scriptData.undefined() && scriptInterface.GetProperty(scriptData.get(), "settings", settings))
		{
			// JSON loaded ok - copy script name over to game attributes
			std::wstring scriptFile;
			scriptInterface.GetProperty(settings.get(), "Script", scriptFile);
			scriptInterface.SetProperty(attrs.get(), "script", scriptFile);				// RMS filename
		}
		else
		{
			// Problem with JSON file
			LOGERROR(L"Error reading random map script '%ls'", scriptPath.c_str());
			throw PSERROR_Game_World_MapLoadFailed("Error reading random map script.\nCheck application log for details.");
		}

		// Get optional map size argument (default 192)
		uint mapSize = 192;
		if (args.Has("autostart-size"))
		{
			CStr size = args.Get("autostart-size");
			mapSize = size.ToUInt();
		}

		scriptInterface.SetProperty(attrs.get(), "map", std::string(autoStartName));
		scriptInterface.SetProperty(attrs.get(), "mapPath", mapPath);
		scriptInterface.SetProperty(attrs.get(), "mapType", std::string("random"));
		scriptInterface.SetProperty(settings.get(), "Seed", seed);									// Random seed
		scriptInterface.SetProperty(settings.get(), "Size", mapSize);								// Random map size (in patches)

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
			CScriptVal player;
			scriptInterface.Eval("({})", player);

			scriptInterface.SetProperty(player.get(), "Civ", std::string("hele"));
			scriptInterface.SetPropertyInt(playerData.get(), i, player);
		}
	}
	else
	{
		scriptInterface.SetProperty(attrs.get(), "map", std::string(autoStartName));
		scriptInterface.SetProperty(attrs.get(), "mapType", std::string("scenario"));
	}

	// Set player data for AIs
	//		attrs.settings = { PlayerData: [ { AI: ... }, ... ] }:
	if (args.Has("autostart-ai"))
	{
		std::vector<CStr> aiArgs = args.GetMultiple("autostart-ai");
		for (size_t i = 0; i < aiArgs.size(); ++i)
		{
			// Instead of overwriting existing player data, modify the array
			CScriptVal player;
			if (!scriptInterface.GetPropertyInt(playerData.get(), i, player) || player.undefined())
			{
				scriptInterface.Eval("({})", player);
			}

			int playerID = aiArgs[i].BeforeFirst(":").ToInt();
			CStr name = aiArgs[i].AfterFirst(":");

			scriptInterface.SetProperty(player.get(), "AI", std::string(name));
			scriptInterface.SetPropertyInt(playerData.get(), playerID-1, player);
		}
	}

	// Add player data to map settings
	scriptInterface.SetProperty(settings.get(), "PlayerData", playerData);

	// Add map settings to game attributes
	scriptInterface.SetProperty(attrs.get(), "settings", settings);

	CScriptVal mpInitData;
	g_GUI->GetScriptInterface().Eval("({isNetworked:true, playerAssignments:{}})", mpInitData);
	g_GUI->GetScriptInterface().SetProperty(mpInitData.get(), "attribs",
			CScriptVal(g_GUI->GetScriptInterface().CloneValueFromOtherContext(scriptInterface, attrs.get())));

	// Get optional playername
	CStrW userName = L"anonymous";
	if (args.Has("autostart-playername"))
	{
		userName = args.Get("autostart-playername").FromUTF8();
	}

	if (args.Has("autostart-host"))
	{
		InitPs(true, L"page_loading.xml", mpInitData.get());

		size_t maxPlayers = 2;
		if (args.Has("autostart-players"))
		{
			maxPlayers = args.Get("autostart-players").ToUInt();
		}

		g_NetServer = new CNetServer(maxPlayers);

		g_NetServer->UpdateGameAttributes(attrs.get(), scriptInterface);

		bool ok = g_NetServer->SetupConnection();
		ENSURE(ok);

		g_NetClient = new CNetClient(g_Game);
		g_NetClient->SetUserName(userName);
		g_NetClient->SetupConnection("127.0.0.1");
	}
	else if (args.Has("autostart-client"))
	{
		InitPs(true, L"page_loading.xml", mpInitData.get());

		g_NetClient = new CNetClient(g_Game);
		g_NetClient->SetUserName(userName);

		CStr ip = "127.0.0.1";
		if (args.Has("autostart-ip"))
		{
			ip = args.Get("autostart-ip");
		}

		bool ok = g_NetClient->SetupConnection(ip);
		ENSURE(ok);
	}
	else
	{
		g_Game->SetPlayerID(1);
		g_Game->StartGame(attrs, "");

		LDR_NonprogressiveLoad();

		PSRETURN ret = g_Game->ReallyStartGame();
		ENSURE(ret == PSRETURN_OK);

		InitPs(true, L"page_session.xml", JSVAL_VOID);
	}

	return true;
}

void CancelLoad(const CStrW& message)
{
	// Cancel loader
	LDR_Cancel();

	// Call the cancelOnError GUI function, defined in ..gui/common/functions_utility_error.js
	// So all GUI pages that load games should include this script
	if (g_GUI && g_GUI->HasPages())
	{
		JSContext* cx = g_ScriptingHost.getContext();
		jsval fval, rval;
		JSBool ok = JS_GetProperty(cx, g_GUI->GetScriptObject(), "cancelOnError", &fval);
		ENSURE(ok);

		jsval msgval = ToJSVal(message);

		if (ok && !JSVAL_IS_VOID(fval))
			ok = JS_CallFunctionValue(cx, g_GUI->GetScriptObject(), fval, 1, &msgval, &rval);
	}
}
