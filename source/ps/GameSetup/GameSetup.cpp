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
#include "ps/Interact.h"
#include "ps/Loader.h"
#include "ps/Overlay.h"
#include "ps/Profile.h"
#include "ps/ProfileViewer.h"
#include "ps/StringConvert.h"
#include "ps/Util.h"
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

#include "simulation/Entity.h"
#include "simulation/EntityManager.h"
#include "simulation/EntityTemplateCollection.h"
#include "simulation/Scheduler.h"

#include "simulation2/Simulation2.h"

#include "scripting/ScriptableComplex.inl"
#include "scripting/ScriptingHost.h"
#include "scripting/ScriptGlue.h"
#include "scripting/DOMEvent.h"
#include "scripting/GameEvents.h"
#include "scripting/ScriptableComplex.h"

#include "maths/scripting/JSInterface_Vector3D.h"

#include "graphics/scripting/JSInterface_Camera.h"
#include "graphics/scripting/JSInterface_LightEnv.h"

#include "ps/scripting/JSInterface_Selection.h"
#include "ps/scripting/JSInterface_Console.h"
#include "ps/scripting/JSCollection.h"

#include "simulation/scripting/SimulationScriptInit.h"

#include "gui/GUI.h"
#include "gui/GUIManager.h"
#include "gui/scripting/JSInterface_IGUIObject.h"
#include "gui/scripting/JSInterface_GUITypes.h"
#include "gui/scripting/ScriptFunctions.h"

#include "sound/JSI_Sound.h"

#include "network/NetLog.h"
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

static int SetVideoMode(int w, int h, int bpp, bool fullscreen)
{
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	Uint32 flags = SDL_OPENGL;
	if(fullscreen)
		flags |= SDL_FULLSCREEN;
	if(!SDL_SetVideoMode(w, h, bpp, flags))
		return -1;

	// Work around a bug in the proprietary Linux ATI driver (at least versions 8.16.20 and 8.14.13).
	// The driver appears to register its own atexit hook on context creation.
	// If this atexit hook is called before SDL_Quit destroys the OpenGL context,
	// some kind of double-free problem causes a crash and lockup in the driver.
	// Calling SDL_Quit twice appears to be harmless, though, and avoids the problem
	// by destroying the context *before* the driver's atexit hook is called.
	// (Note that atexit hooks are guaranteed to be called in reverse order of their registration.)
	atexit(SDL_Quit);
	// End work around.

	glViewport(0, 0, w, h);

	ogl_Init();	// required after each mode change

	if(SDL_SetGamma(g_Gamma, g_Gamma, g_Gamma) < 0)
		LOGWARNING(L"SDL_SetGamma failed");

	return 0;
}

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

	if (g_Game && g_Game->IsGameStarted() && g_UseSimulation2)
	{
		g_Game->GetView()->Render();
	}
	else if (g_Game && g_Game->IsGameStarted() && !g_UseSimulation2)
	{
		g_Game->GetView()->Render();

		glPushAttrib( GL_ENABLE_BIT );
		glDisable( GL_LIGHTING );
		glDisable( GL_TEXTURE_2D );
		glDisable( GL_DEPTH_TEST );

		if( g_EntGraph )
		{
			PROFILE( "render entity overlays" );
			glColor3f( 1.0f, 0.0f, 1.0f );
			g_EntityManager.RenderAll(); // <-- collision outlines, pathing routes
		}

		glEnable( GL_DEPTH_TEST );

		PROFILE_START( "render entity outlines" );
		g_Mouseover.RenderSelectionOutlines();
		g_Selection.RenderSelectionOutlines();
		PROFILE_END( "render entity outlines" );

		PROFILE_START( "render entity auras" );
		g_Mouseover.RenderAuras();
		g_Selection.RenderAuras();
		PROFILE_END( "render entity auras" );

		glDisable(GL_DEPTH_TEST);

		PROFILE_START( "render entity bars" );
		pglActiveTextureARB(GL_TEXTURE1_ARB);
		glDisable(GL_TEXTURE_2D);
		pglActiveTextureARB(GL_TEXTURE0_ARB);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_REPLACE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);
		glTexEnvf(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, g_Renderer.m_Options.m_LodBias);
		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		/*glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(0.f, (float)g_xres, 0.f, (float)g_yres, -1.0f, 1000.f);
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();*/
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		g_Mouseover.RenderBars();
		g_Selection.RenderBars();
		
		glDisable(GL_BLEND);
		/*glPopMatrix();
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);*/
		PROFILE_END( "render entity bars" );

		glPopAttrib();
		glMatrixMode(GL_MODELVIEW);

		// Depth test is now enabled
		PROFILE_START( "render rally points" );
		g_Selection.RenderRallyPoints();
		g_Mouseover.RenderRallyPoints();
		PROFILE_END( "render rally points" );
		
		PROFILE_START( "render cinematic splines" );
		//Sets/resets renderering properties itself
		g_Game->GetView()->GetCinema()->DrawSpline();
		PROFILE_END( "render cinematic splines" );
	}

	ogl_WarnIfError();

	PROFILE_START( "render fonts" );
	MICROLOG(L"render fonts");
	// overlay mode
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

	PROFILE_END( "render fonts" );

	ogl_WarnIfError();

	// Temp GUI message GeeTODO
	MICROLOG(L"render GUI");
	PROFILE_START( "render gui" );
	if(g_DoRenderGui) g_GUI->Draw();
	PROFILE_END( "render gui" );

	ogl_WarnIfError();

	// Particle Engine Updating
	CParticleEngine::GetInstance()->UpdateEmitters();

	// Text:

	// Use the GL_ALPHA texture as the alpha channel with a flat colouring
	glDisable(GL_ALPHA_TEST);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	// Added --
	glEnable(GL_TEXTURE_2D);
	// -- GL

	ogl_WarnIfError();

	{
		PROFILE( "render console" );
		glLoadIdentity();

		MICROLOG(L"render console");
		CFont font(L"console");
		font.Bind();
		g_Console->Render();
	}

	ogl_WarnIfError();


	// Profile information

	PROFILE_START( "render profiling" );
	g_ProfileViewer.RenderProfile();
	PROFILE_END( "render profiling" );

	ogl_WarnIfError();

	if (g_Game && g_Game->IsGameStarted() && !g_UseSimulation2)
	{
		PROFILE( "render selection overlays" );
		g_Mouseover.RenderOverlays();
		g_Selection.RenderOverlays();
	}

	ogl_WarnIfError();

	// Draw the cursor (or set the Windows cursor, on Windows)
	CStrW cursorName = (!g_UseSimulation2 && g_BuildingPlacer.m_active) ? L"action-build" : g_CursorName;
	if (cursorName.empty())
		cursor_draw(NULL, g_mouse_x, g_mouse_y);
	else
		cursor_draw(cursorName.c_str(), g_mouse_x, g_mouse_y);

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
	SColour::ScriptingInit();
	CScriptEvent::ScriptingInit();
	// call CJSComplexPropertyAccessor's ScriptingInit. doesn't really
	// matter which <T> we use, but we know CJSPropertyAccessor<T> is
	// already being compiled for T = CEntity.
	ScriptableComplex_InitComplexPropertyAccessor<CEntity>();

	// ps
	JSI_Console::init();
	CGameAttributes::ScriptingInit();
	CPlayerSlot::ScriptingInit();
	CPlayer::ScriptingInit();
	PlayerCollection::Init( "PlayerCollection" );

	// network
	CNetClient::ScriptingInit();
	CNetServer::ScriptingInit();
	CNetSession::ScriptingInit();
	CServerPlayer::ScriptingInit();

	// simulation
	SimulationScriptInit();

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
	
	// It would be nice for onLoad code to be able to access the setTimeout() calls.
	new CScheduler;

	RegisterJavascriptInterfaces();

#define REG_JS_CONSTANT(_name) g_ScriptingHost.DefineConstant(#_name, _name)
	REG_JS_CONSTANT(SDL_BUTTON_LEFT);
	REG_JS_CONSTANT(SDL_BUTTON_MIDDLE);
	REG_JS_CONSTANT(SDL_BUTTON_RIGHT);
	REG_JS_CONSTANT(SDL_BUTTON_WHEELUP);
	REG_JS_CONSTANT(SDL_BUTTON_WHEELDOWN);
#undef REG_JS_CONSTANT

	new CGameEvents;
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
	if (!setup_gui)
	{
		// We do actually need *some* kind of GUI loaded, so use the
		// (currently empty) Atlas one
		g_GUI->SwitchPage(L"page_atlas.xml", JSVAL_VOID);
		return;
	}

	// The things here aren't strictly GUI, but they're unnecessary when in Atlas
	// because the game doesn't draw any text or handle keys or anything

	{
		// console
		TIMER(L"ps_console");

		g_Console->UpdateScreenSize(g_xres, g_yres);

		// Calculate and store the line spacing
		CFont font(L"console");
		g_Console->m_iFontHeight = font.GetLineSpacing();
		g_Console->m_iFontWidth = font.GetCharacterWidth(L'C');
		g_Console->m_charsPerPage = (size_t)(g_xres / g_Console->m_iFontWidth);
		// Offset by an arbitrary amount, to make it fit more nicely
		g_Console->m_iFontOffset = 9;
	}

	// language and hotkeys
	{
		TIMER(L"ps_lang_hotkeys");

		std::string lang = "english";
		CFG_GET_SYS_VAL("language", String, lang);
		I18n::LoadLanguage(lang.c_str());

		LoadHotkeys();
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

	in_add_handler(InteractInputHandler);

	in_add_handler(conInputHandler);

	in_add_handler(CProfileViewer::InputThunk);

	in_add_handler(HotkeyInputHandler);

	in_add_handler(GlobalsInputHandler);
}


static void ShutdownPs()
{
	SAFE_DELETE(g_GUI);

	SAFE_DELETE(g_Console);

	// disable the special Windows cursor, or free textures for OGL cursors
	cursor_draw(0, g_mouse_x, g_mouse_y);

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
	g_Renderer.Open(g_xres,g_yres,g_bpp);

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


void Shutdown(int flags)
{
	MICROLOG(L"Shutdown");

	if (g_Game)
		EndGame();

	ShutdownPs(); // Must delete g_GUI before g_ScriptingHost

	TIMER_BEGIN(L"shutdown Scheduler");
	delete &g_Scheduler;
	TIMER_END(L"shutdown Scheduler");

	delete &g_JSGameEvents;

	if (! (flags & INIT_NO_SIM))
	{
		if (g_UseSimulation2)
		{
			delete &g_GameAttributes;
			delete &g_EntityTemplateCollection;
		}
		else
		{
			TIMER_BEGIN(L"shutdown mouse stuff");
			delete &g_Mouseover;
			delete &g_Selection;
			delete &g_BuildingPlacer;
			TIMER_END(L"shutdown mouse stuff");

			TIMER_BEGIN(L"shutdown game scripting stuff");
			delete &g_GameAttributes;
			SimulationShutdown();
			TIMER_END(L"shutdown game scripting stuff");
		}
	}

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

	TIMER_BEGIN(L"shutdown CNetLogManager");
	CNetLogManager::Shutdown();
	TIMER_END(L"shutdown CNetLogManager");

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

	if(setup_vmode)
		InitSDL();

	// preferred video mode = current desktop settings
	// (command line params may override these)
	gfx_get_video_mode(&g_xres, &g_yres, &g_bpp, &g_freq);

	new CProfileViewer;
	new CProfileManager;	// before any script code

	MICROLOG(L"init scripting");
	InitScripting();	// before GUI

	// g_ConfigDB, command line args, globals
	CONFIG_Init(args);

	// setup_gui must be set after CONFIG_Init, so command-line parameters can disable it
	const bool setup_gui = ((flags & INIT_NO_GUI) == 0);

	// GUI is notified in SetVideoMode, so this must come before that.
	g_GUI = new CGUIManager(g_ScriptingHost.GetScriptInterface());

	// default to windowed, so users don't get stuck if e.g. half the
	// filesystem is missing and the config files aren't loaded
	bool windowed = true;
	CFG_GET_SYS_VAL("windowed", Bool, windowed);

	if(setup_vmode)
	{
		MICROLOG(L"SetVideoMode");
		if(SetVideoMode(g_xres, g_yres, 32, !windowed) < 0)
		{
			LOG(CLogger::Error, LOG_CATEGORY, L"Could not set %dx%d graphics mode: %hs", g_xres, g_yres, SDL_GetError());
			throw PSERROR_System_VmodeFailed();
		}

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
	else
	{
		// speed up startup by disabling all sound
		// (OpenAL init will be skipped).
		// must be called before first snd_open.
		snd_disable(true);
	}

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

	if (! (flags & INIT_NO_SIM))
	{
		// This needs to be done after the renderer has loaded all its actors...
		if (g_UseSimulation2)
		{
			new CEntityTemplateCollection; // this is just needed for loading old maps
			new CGameAttributes;
		}
		else
		{
			SimulationInit();

			{
				TIMER(L"Init_miscgamesection");
				new CSelectedEntities;
				new CMouseoverEntities;
				new CBuildingPlacer;
				new CGameAttributes;
			}
		}

		// Register a few Game/Network JS globals
		g_ScriptingHost.SetGlobal("g_GameAttributes", OBJECT_TO_JSVAL(g_GameAttributes.GetScript()));
	}

	InitInput();

	ogl_WarnIfError();

	if (!Autostart(args))
	{
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
	AutostartNetServer(CGame *pGame, CGameAttributes *pGameAttributes, int maxPlayers) :
		CNetServer(pGame, pGameAttributes), m_NumPlayers(1), m_MaxPlayers(maxPlayers)
	{
	}
protected:
	virtual void OnPlayerJoin(CNetSession* pSession)
	{
		for (size_t slot = 0; slot < g_GameAttributes.GetSlotCount(); ++slot)
		{
			if (g_GameAttributes.GetSlot(slot)->GetAssignment() == SLOT_OPEN)
			{
				g_GameAttributes.GetSlot(slot)->AssignToSession(pSession);
				break;
			}
		}

		m_NumPlayers++;

		debug_printf(L"# player joined (got %d, need %d)\n", (int)m_NumPlayers, (int)m_MaxPlayers);

		if (m_NumPlayers >= m_MaxPlayers)
		{
			g_GUI->SwitchPage(L"page_loading.xml", JSVAL_VOID);
			int ret = StartGame();
			debug_assert(ret == 0);
		}
	}

	virtual void OnPlayerLeave(CNetSession* UNUSED(pSession))
	{
		debug_warn(L"client left?!");
		m_NumPlayers--;
	}

private:
	size_t m_NumPlayers;
	size_t m_MaxPlayers;
};

class AutostartNetClient : public CNetClient
{
public:
	AutostartNetClient(CGame *pGame, CGameAttributes *pGameAttributes) :
		CNetClient(pGame, pGameAttributes)
	{
	}
protected:
	virtual void OnConnectComplete()
	{
		debug_printf(L"# connect complete\n");
	}

	virtual void OnStartGame()
	{
		g_GUI->SwitchPage(L"page_loading.xml", JSVAL_VOID);
		int ret = StartGame();
		debug_assert(ret == 0);
	}
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

	g_GameAttributes.m_MapFile = autostartMap + ".pmp";

	// Make the whole world visible
	g_GameAttributes.m_LOSSetting = LOS_SETTING_ALL_VISIBLE;
	g_GameAttributes.m_FogOfWar = false;

	if (args.Has("autostart-host"))
	{
		InitPs(true, L"page_loading.xml");

		size_t maxPlayers = 2;
		if (args.Has("autostart-players"))
			maxPlayers = args.Get("autostart-players").ToUInt();

		g_NetServer = new AutostartNetServer(g_Game, &g_GameAttributes, maxPlayers);
		// TODO: player name, etc
		bool ok = g_NetServer->Start(NULL, 0, NULL);
		debug_assert(ok);
	}
	else if (args.Has("autostart-client"))
	{
		InitPs(true, L"page_loading.xml");

		bool ok;
		g_NetClient = new AutostartNetClient(g_Game, &g_GameAttributes);
		// TODO: player name, etc
		ok = g_NetClient->Create();
		debug_assert(ok);
		ok = g_NetClient->Connect(args.Get("autostart-ip"), DEFAULT_HOST_PORT);
		debug_assert(ok);
	}
	else
	{
		for (int i = 1; i < 8; ++i)
			g_GameAttributes.GetSlot(i)->AssignLocal();

		PSRETURN ret = g_Game->StartGame(&g_GameAttributes);
		debug_assert(ret == PSRETURN_OK);
		LDR_NonprogressiveLoad();
		ret = g_Game->ReallyStartGame();
		debug_assert(ret == PSRETURN_OK);

		InitPs(true, g_UseSimulation2 ? L"page_session_new.xml" : L"page_session.xml");
	}

	return true;
}
