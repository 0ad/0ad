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

#include "precompiled.h"

#include "lib/app_hooks.h"
#include "lib/input.h"
#include "lib/lockfree.h"
#include "lib/ogl.h"
#include "lib/timer.h"
#include "lib/utf8.h"
#include "lib/external_libraries/sdl.h"
#include "lib/res/h_mgr.h"
#include "lib/res/graphics/cursor.h"
#include "lib/res/sound/snd_mgr.h"
#include "lib/sysdep/cpu.h"
#include "lib/sysdep/gfx.h"
#include "lib/tex/tex.h"

#include "ps/CConsole.h"
#include "ps/CLogger.h"
#include "ps/ConfigDB.h"
#include "ps/Filesystem.h"
#include "ps/Font.h"
#include "ps/Game.h"
#include "ps/Globals.h"
#include "ps/Hotkey.h"
#include "ps/Loader.h"
#include "ps/Overlay.h"
#include "ps/Profile.h"
#include "ps/ProfileViewer.h"
#include "ps/StringConvert.h"
#include "ps/Util.h"
#include "ps/VideoMode.h"
#include "ps/World.h"
#include "ps/i18n.h"

#include "graphics/CinemaTrack.h"
#include "graphics/GameView.h"
#include "graphics/LightEnv.h"
#include "graphics/MapReader.h"
#include "graphics/MaterialManager.h"
#include "graphics/ParticleEngine.h"
#include "graphics/TextureManager.h"

#include "renderer/Renderer.h"
#include "renderer/VertexBufferManager.h"

#include "maths/MathUtil.h"

#include "simulation2/Simulation2.h"

#include "scripting/ScriptableComplex.inl"
#include "scripting/ScriptingHost.h"
#include "scripting/ScriptGlue.h"
#include "scripting/DOMEvent.h"
#include "scripting/ScriptableComplex.h"

#include "scriptinterface/ScriptInterface.h"

#include "maths/scripting/JSInterface_Vector3D.h"

#include "graphics/scripting/JSInterface_Camera.h"
#include "graphics/scripting/JSInterface_LightEnv.h"

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

#include <iostream>

ERROR_GROUP(System);
ERROR_TYPE(System, SDLInitFailed);
ERROR_TYPE(System, VmodeFailed);
ERROR_TYPE(System, RequiredExtensionsMissing);

#define LOG_CATEGORY L"gamesetup"

bool g_DoRenderGui = true;

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
	CStrW i18n_description = I18n::translate(pending_task);
	JSString* js_desc = StringConvert::wstring_to_jsstring(g_ScriptingHost.getContext(), i18n_description);
	g_ScriptingHost.SetGlobal("g_Progress", INT_TO_JSVAL(percent));
	g_ScriptingHost.SetGlobal("g_LoadDescription", STRING_TO_JSVAL(js_desc));
	g_GUI->SendEventToAll("progress");
}



void Render()
{
	MICROLOG(L"begin frame");

	ogl_WarnIfError();

	CStr skystring = "61 193 255";
	CFG_GET_USER_VAL("skycolor", String, skystring);
	CColor skycol;
	GUI<CColor>::ParseString(skystring, skycol);
	g_Renderer.SetClearColor(skycol.AsSColor4ub());

	// start new frame
	g_Renderer.BeginFrame();

	ogl_WarnIfError();

	if (g_Game && g_Game->IsGameStarted())
	{
		g_Game->GetView()->Render();
	}

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

	// Temp GUI message GeeTODO
	PROFILE_START("render gui");
	if(g_DoRenderGui) g_GUI->Draw();
	PROFILE_END("render gui");

	ogl_WarnIfError();

	// Particle Engine Updating
	CParticleEngine::GetInstance()->UpdateEmitters();

	ogl_WarnIfError();

	// Text:

	// Use the GL_ALPHA texture as the alpha channel with a flat colouring
	glDisable(GL_ALPHA_TEST);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	// Added --
	glEnable(GL_TEXTURE_2D);
	// -- GL

	glLoadIdentity();

	PROFILE_START("render console");
	g_Console->Render();
	PROFILE_END("render console");

	ogl_WarnIfError();

	PROFILE_START("render logger");
	g_Logger->Render();
	PROFILE_END("render logger");

	ogl_WarnIfError();

	// Profile information

	PROFILE_START("render profiling");
	g_ProfileViewer.RenderProfile();
	PROFILE_END("render profiling");

	ogl_WarnIfError();

	// Draw the cursor (or set the Windows cursor, on Windows)
	CStrW cursorName = g_CursorName;
	if (cursorName.empty())
		cursor_draw(g_VFS, NULL, g_mouse_x, g_yres-g_mouse_y);
	else
		cursor_draw(g_VFS, cursorName.c_str(), g_mouse_x, g_yres-g_mouse_y);

	// restore
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glPopAttrib();

	MICROLOG(L"end frame");
	g_Renderer.EndFrame();

	ogl_WarnIfError();
}


static void RegisterJavascriptInterfaces()
{
	// maths
	JSI_Vector3D::init();

	// graphics
	JSI_Camera::init();
	JSI_LightEnv::init();
	CGameView::ScriptingInit();

	// renderer
	CRenderer::ScriptingInit();

	// sound
	JSI_Sound::ScriptingInit();

	// scripting
	CScriptEvent::ScriptingInit();

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

#define REG_JS_CONSTANT(_name) g_ScriptingHost.DefineConstant(#_name, _name)
	REG_JS_CONSTANT(SDL_BUTTON_LEFT);
	REG_JS_CONSTANT(SDL_BUTTON_MIDDLE);
	REG_JS_CONSTANT(SDL_BUTTON_RIGHT);
	REG_JS_CONSTANT(SDL_BUTTON_WHEELUP);
	REG_JS_CONSTANT(SDL_BUTTON_WHEELDOWN);
#undef REG_JS_CONSTANT
}


static size_t ChooseCacheSize()
{
#if OS_WIN
	//const size_t overheadKiB = (wutil_WindowsVersion() >= WUTIL_VERSION_VISTA)? 1024 : 512;
#endif
	return 96*MiB;
}


static void InitVfs(const CmdLineArgs& args)
{
	TIMER(L"InitVfs");

	const Paths paths(args);

	fs::wpath logs(paths.Logs());
	CreateDirectories(logs, 0700);

	psSetLogDir(logs);
	// desired location for crashlog is now known. update AppHooks ASAP
	// (particularly before the following error-prone operations):
	AppHooks hooks = {0};
	hooks.bundle_logs = psBundleLogs;
	hooks.get_log_dir = psLogDir;
	app_hooks_update(&hooks);

	const size_t cacheSize = ChooseCacheSize();
	g_VFS = CreateVfs(cacheSize);

	g_VFS->Mount(L"screenshots/", paths.Data()/L"screenshots/");
	const fs::wpath readonlyConfig = paths.RData()/L"config/";
	g_VFS->Mount(L"config/", readonlyConfig);
	if(readonlyConfig != paths.Config())
		g_VFS->Mount(L"config/", paths.Config());
	g_VFS->Mount(L"cache/", paths.Cache(), VFS_MOUNT_ARCHIVABLE);	// (adding XMBs to archive speeds up subsequent reads)

	std::vector<CStr> mods = args.GetMultiple("mod");
	mods.push_back("public");
	if(!args.Has("onlyPublicFiles"))
		mods.push_back("internal");

	fs::wpath modArchivePath(paths.Cache()/L"mods");
	fs::wpath modLoosePath(paths.RData()/L"mods");
	for (size_t i = 0; i < mods.size(); ++i)
	{
		size_t priority = i;
		size_t flags = VFS_MOUNT_WATCH|VFS_MOUNT_ARCHIVABLE|VFS_MOUNT_MUST_EXIST;
		std::wstring modName (wstring_from_utf8(mods[i]));
		g_VFS->Mount(L"", AddSlash(modLoosePath/modName), flags, priority);
		g_VFS->Mount(L"", AddSlash(modArchivePath/modName), flags, priority);
	}

	// note: don't bother with g_VFS->TextRepresentation - directories
	// haven't yet been populated and are empty.
}


static void InitPs(bool setup_gui, const CStrW& gui_page)
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

	// language and hotkeys
	{
		TIMER(L"ps_lang_hotkeys");

		std::string lang = "english";
		CFG_GET_SYS_VAL("language", String, lang);
		I18n::LoadLanguage(lang.c_str());

		LoadHotkeys();
	}

	if (!setup_gui)
	{
		// We do actually need *some* kind of GUI loaded, so use the
		// (currently empty) Atlas one
		g_GUI->SwitchPage(L"page_atlas.xml", JSVAL_VOID);
		return;
	}

	// GUI uses VFS, so this must come after VFS init.
	g_GUI->SwitchPage(gui_page, JSVAL_VOID);

	// Warn nicely about missing S3TC support
	if (!ogl_tex_has_s3tc())
	{
		g_GUI->DisplayMessageBox(600, 350, L"Warning",
			L"Performance warning:\n\n"
			L"Your graphics drivers do not support S3TC compressed textures. This will significantly reduce performance and increase memory usage.\n\n"
#if !(OS_WIN || OS_MACOSX)
			L"See http://dri.freedesktop.org/wiki/S3TC for details. "
			L"Installing the libtxc_dxtn library will fix these problems. "
			L"Alternatively, running 'driconf' and setting force_s3tc_enable will fix the performance but may cause rendering bugs."
#else
			L"Please try updating your graphics drivers to ensure you have full hardware acceleration."
#endif
		);
	}
}


static void InitInput()
{
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

	// register input handlers
	// This stack is constructed so the first added, will be the last
	//  one called. This is important, because each of the handlers
	//  has the potential to block events to go further down
	//  in the chain. I.e. the last one in the list added, is the
	//  only handler that can block all messages before they are
	//  processed.
	in_add_handler(game_view_handler);

	in_add_handler(conInputHandler);

	in_add_handler(CProfileViewer::InputThunk);

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

	// Unload the real language (since it depends on the scripting engine,
	// which is going to be killed later) and use the English fallback messages
	I18n::LoadLanguage(NULL);
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
	g_Renderer.SetOptionBool(CRenderer::OPT_NOFRAMEBUFFEROBJECT,g_NoGLFramebufferObject);
	g_Renderer.SetOptionBool(CRenderer::OPT_SHADOWS,g_Shadows);
	g_Renderer.SetOptionBool(CRenderer::OPT_FANCYWATER,g_FancyWater);
	g_Renderer.SetRenderPath(CRenderer::GetRenderPathByName(g_RenderPath));
	g_Renderer.SetOptionFloat(CRenderer::OPT_LODBIAS, g_LodBias);

	// create terrain related stuff
	new CTextureManager;

	// create the material manager
	new CMaterialManager;

	MICROLOG(L"init renderer");
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
}

static void InitSDL()
{
	MICROLOG(L"init sdl");
	if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_NOPARACHUTE) < 0)
	{
		LOG(CLogger::Error, LOG_CATEGORY, L"SDL library initialization failed: %hs", SDL_GetError());
		throw PSERROR_System_SDLInitFailed();
	}
	atexit(SDL_Quit);
	SDL_EnableUNICODE(1);
}


void EndGame()
{
	SAFE_DELETE(g_NetServer);
	SAFE_DELETE(g_NetClient);
	SAFE_DELETE(g_Game);
}


void Shutdown(int UNUSED(flags))
{
	MICROLOG(L"Shutdown");

	EndGame();

	ShutdownPs(); // Must delete g_GUI before g_ScriptingHost

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

	TIMER_BEGIN(L"shutdown ScriptingHost");
	delete &g_ScriptingHost;
	TIMER_END(L"shutdown ScriptingHost");

	TIMER_BEGIN(L"shutdown ConfigDB");
	delete &g_ConfigDB;
	TIMER_END(L"shutdown ConfigDB");

	// Really shut down the i18n system. Any future calls
	// to translate() will crash.
	TIMER_BEGIN(L"shutdown I18N");
	I18n::Shutdown();
	TIMER_END(L"shutdown I18N");

	// resource
	// first shut down all resource owners, and then the handle manager.
	TIMER_BEGIN(L"resource modules");
		snd_shutdown();

		g_VFS.reset();

		// this forcibly frees all open handles (thus preventing real leaks),
		// and makes further access to h_mgr impossible.
		h_mgr_shutdown();

	TIMER_END(L"resource modules");

	TIMER_BEGIN(L"shutdown misc");
		timer_DisplayClientTotals();

		CNetHost::Deinitialize();

		// should be last, since the above use them
		SAFE_DELETE(g_Logger);
		delete &g_Profiler;
		delete &g_ProfileViewer;
	TIMER_END(L"shutdown misc");
}

void EarlyInit()
{
	MICROLOG(L"EarlyInit");

	// If you ever want to catch a particular allocation:
	//_CrtSetBreakAlloc(232647);

	debug_SetThreadName("main");
	// add all debug_printf "tags" that we are interested in:
	debug_filter_add(L"TIMER");
	debug_filter_add(L"HRT");

	cpu_ConfigureFloatingPoint();

	timer_LatchStartTime();

	// Initialise the low-quality rand function
	srand(time(NULL));	// NOTE: this rand should *not* be used for simulation!
}

static bool Autostart(const CmdLineArgs& args);

void Init(const CmdLineArgs& args, int flags)
{
	const bool setup_vmode = (flags & INIT_HAVE_VMODE) == 0;

	MICROLOG(L"Init");

	h_mgr_init();

	// Do this as soon as possible, because it chdirs
	// and will mess up the error reporting if anything
	// crashes before the working directory is set.
	MICROLOG(L"init vfs");
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

	// Call LoadLanguage(NULL) to initialize the I18n system, but
	// without loading an actual language file - translate() will
	// just show the English key text, which is better than crashing
	// from a null pointer when attempting to translate e.g. error messages.
	// Real languages can only be loaded when the scripting system has
	// been initialised.
	//
	// this uses LOG and must therefore come after CLogger init.
	MICROLOG(L"init i18n");
	I18n::LoadLanguage(NULL);

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

	if(setup_vmode)
		InitSDL();

	new CProfileViewer;
	new CProfileManager;	// before any script code

	MICROLOG(L"init scripting");
	InitScripting();	// before GUI

	// g_ConfigDB, command line args, globals
	CONFIG_Init(args);

	if (setup_vmode)
	{
		if (!g_VideoMode.InitSDL())
			throw PSERROR_System_VmodeFailed(); // abort startup

		SDL_WM_SetCaption("0 A.D.", "0 A.D.");
	}

	tex_codec_register_all();

	const int quality = SANE_TEX_QUALITY_DEFAULT;	// TODO: set value from config file
	SetTextureQuality(quality);

	// needed by ogl_tex to detect broken gfx card/driver combos,
	// but takes a while due to WMI startup, so make it optional.
	if(!g_Quickstart)
		gfx_detect();

	ogl_WarnIfError();

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

	// enable/disable VSync
	// note: "GL_EXT_SWAP_CONTROL" is "historical" according to dox.
#if OS_WIN
	if(ogl_HaveExtension("WGL_EXT_swap_control"))
		pwglSwapIntervalEXT(g_VSync? 1 : 0);
#endif

	ogl_WarnIfError();
	InitRenderer();

	InitInput();

	ogl_WarnIfError();

	if (!Autostart(args))
	{
		const bool setup_gui = ((flags & INIT_NO_GUI) == 0);
		InitPs(setup_gui, L"page_pregame.xml");
	}
}

void RenderGui(bool RenderingState)
{
	g_DoRenderGui = RenderingState;
}


// Network autostart:

class AutostartNetServer : public CNetServer
{
public:
	AutostartNetServer(const CStr& map, int maxPlayers) :
		CNetServer(), m_NeedsStart(false), m_NumPlayers(0), m_MaxPlayers(maxPlayers)
	{
		CScriptValRooted attrs;
		GetScriptInterface().Eval("({})", attrs);
		GetScriptInterface().SetProperty(attrs.get(), "map", std::string(map), false);
		UpdateGameAttributes(attrs);
	}

protected:
	virtual void OnAddPlayer()
	{
		m_NumPlayers++;

		debug_printf(L"# player joined (got %d, need %d)\n", (int)m_NumPlayers, (int)m_MaxPlayers);

		if (m_NumPlayers >= m_MaxPlayers)
			m_NeedsStart = true; // delay until next Poll, so the new player has been fully processed
	}

	virtual void OnRemovePlayer()
	{
		debug_warn(L"client left?!");
		m_NumPlayers--;
	}

	virtual void Poll()
	{
		if (m_NeedsStart)
		{
			StartGame();
			m_NeedsStart = false;
		}

		CNetServer::Poll();
	}

private:
	bool m_NeedsStart;
	size_t m_NumPlayers;
	size_t m_MaxPlayers;
};

static bool Autostart(const CmdLineArgs& args)
{
	/*
	 * Handle various command-line options, for quick testing of various features:
	 *  -autostart=mapname   -- single-player
	 *  -autostart=mapname -autostart-playername=Player -autostart-host -autostart-players=2        -- multiplayer host, wait for 2 players
	 *  -autostart=mapname -autostart-playername=Player -autostart-client -autostart-ip=127.0.0.1   -- multiplayer client, connect to 127.0.0.1
	 */

	CStr autostartMap = args.Get("autostart");
	if (autostartMap.empty())
		return false;

	g_Game = new CGame();

	if (args.Has("autostart-host"))
	{
		InitPs(true, L"page_loading.xml");

		size_t maxPlayers = 2;
		if (args.Has("autostart-players"))
			maxPlayers = args.Get("autostart-players").ToUInt();

		g_NetServer = new AutostartNetServer(autostartMap, maxPlayers);
		bool ok = g_NetServer->SetupConnection();
		debug_assert(ok);

		g_NetClient = new CNetClient(g_Game);
		// TODO: player name, etc
		g_NetClient->SetupLocalConnection(*g_NetServer);
	}
	else if (args.Has("autostart-client"))
	{
		InitPs(true, L"page_loading.xml");

		g_NetClient = new CNetClient(g_Game);
		// TODO: player name, etc
		bool ok = g_NetClient->SetupConnection(args.Get("autostart-ip"));
		debug_assert(ok);
	}
	else
	{
		CScriptValRooted attrs;
		g_Game->GetSimulation2()->GetScriptInterface().Eval("({})", attrs);
		g_Game->GetSimulation2()->GetScriptInterface().SetProperty(attrs.get(), "map", std::string(autostartMap), false);

		g_Game->SetPlayerID(1);
		g_Game->StartGame(attrs);
		LDR_NonprogressiveLoad();
		PSRETURN ret = g_Game->ReallyStartGame();
		debug_assert(ret == PSRETURN_OK);

		InitPs(true, L"page_session_new.xml");
	}

	return true;
}
