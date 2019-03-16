/* Copyright (C) 2019 Wildfire Games.
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
#include "lib/config2.h"
#include "lib/input.h"
#include "lib/ogl.h"
#include "lib/timer.h"
#include "lib/external_libraries/libsdl.h"
#include "lib/file/common/file_stats.h"
#include "lib/res/h_mgr.h"
#include "lib/res/graphics/cursor.h"
#include "lib/sysdep/cursor.h"

#include "graphics/CinemaManager.h"
#include "graphics/FontMetrics.h"
#include "graphics/GameView.h"
#include "graphics/LightEnv.h"
#include "graphics/MapReader.h"
#include "graphics/MaterialManager.h"
#include "graphics/TerrainTextureManager.h"
#include "gui/GUI.h"
#include "gui/GUIManager.h"
#include "gui/scripting/ScriptFunctions.h"
#include "i18n/L10n.h"
#include "maths/MathUtil.h"
#include "network/NetServer.h"
#include "network/NetClient.h"
#include "network/NetMessage.h"
#include "network/NetMessages.h"

#include "ps/CConsole.h"
#include "ps/CLogger.h"
#include "ps/ConfigDB.h"
#include "ps/Filesystem.h"
#include "ps/Game.h"
#include "ps/GameSetup/Atlas.h"
#include "ps/GameSetup/GameSetup.h"
#include "ps/GameSetup/Paths.h"
#include "ps/GameSetup/Config.h"
#include "ps/GameSetup/CmdLineArgs.h"
#include "ps/GameSetup/HWDetect.h"
#include "ps/Globals.h"
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
#include "ps/TouchInput.h"
#include "ps/UserReport.h"
#include "ps/Util.h"
#include "ps/VideoMode.h"
#include "ps/VisualReplay.h"
#include "ps/World.h"

#include "renderer/Renderer.h"
#include "renderer/VertexBufferManager.h"
#include "renderer/ModelRenderer.h"
#include "scriptinterface/ScriptInterface.h"
#include "scriptinterface/ScriptStats.h"
#include "scriptinterface/ScriptConversions.h"
#include "scriptinterface/ScriptRuntime.h"
#include "simulation2/Simulation2.h"
#include "lobby/IXmppClient.h"
#include "soundmanager/scripting/JSInterface_Sound.h"
#include "soundmanager/ISoundManager.h"
#include "tools/atlas/GameInterface/GameLoop.h"
#include "tools/atlas/GameInterface/View.h"

#if !(OS_WIN || OS_MACOSX || OS_ANDROID) // assume all other platforms use X11 for wxWidgets
#define MUST_INIT_X11 1
#include <X11/Xlib.h>
#else
#define MUST_INIT_X11 0
#endif

extern void RestartEngine();

#include <iostream>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

ERROR_GROUP(System);
ERROR_TYPE(System, SDLInitFailed);
ERROR_TYPE(System, VmodeFailed);
ERROR_TYPE(System, RequiredExtensionsMissing);

bool g_DoRenderGui = true;
bool g_DoRenderLogger = true;
bool g_DoRenderCursor = true;

shared_ptr<ScriptRuntime> g_ScriptRuntime;

static const int SANE_TEX_QUALITY_DEFAULT = 5;	// keep in sync with code

bool g_InDevelopmentCopy;
bool g_CheckedIfInDevelopmentCopy = false;

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
	g_GUI->GetActiveGUI()->GetScriptInterface()->SetGlobal("g_Progress", percent, true, false, true);
	g_GUI->GetActiveGUI()->GetScriptInterface()->SetGlobal("g_LoadDescription", pending_task, true, false, true);
	g_GUI->GetActiveGUI()->SendEventToAll("progress");
}



void Render()
{
	PROFILE3("render");

	if (g_SoundManager)
		g_SoundManager->IdleTask();

	ogl_WarnIfError();

	g_Profiler2.RecordGPUFrameStart();

	ogl_WarnIfError();

	// prepare before starting the renderer frame
	if (g_Game && g_Game->IsGameStarted())
		g_Game->GetView()->BeginFrame();

	if (g_Game)
		g_Renderer.SetSimulation(g_Game->GetSimulation2());

	// start new frame
	g_Renderer.BeginFrame();

	ogl_WarnIfError();

	if (g_Game && g_Game->IsGameStarted())
		g_Game->GetView()->Render();

	ogl_WarnIfError();

	g_Renderer.RenderTextOverlays();

	// If we're in Atlas game view, render special tools
	if (g_AtlasGameLoop && g_AtlasGameLoop->view)
	{
		g_AtlasGameLoop->view->DrawCinemaPathTool();
		ogl_WarnIfError();
	}

	if (g_Game && g_Game->IsGameStarted())
		g_Game->GetView()->GetCinema()->Render();

	ogl_WarnIfError();

	if (g_DoRenderGui)
		g_GUI->Draw();

	ogl_WarnIfError();

	// If we're in Atlas game view, render special overlays (e.g. editor bandbox)
	if (g_AtlasGameLoop && g_AtlasGameLoop->view)
	{
		g_AtlasGameLoop->view->DrawOverlays();
		ogl_WarnIfError();
	}

	// Text:

 	glDisable(GL_DEPTH_TEST);

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
			cursor_draw(g_VFS, NULL, g_mouse_x, g_yres-g_mouse_y, g_GuiScale, false);
		}
		else
		{
			bool forceGL = false;
			CFG_GET_VAL("nohwcursor", forceGL);

#if CONFIG2_GLES
#warning TODO: implement cursors for GLES
#else
			// set up transform for GL cursor
			glMatrixMode(GL_PROJECTION);
			glPushMatrix();
			glLoadIdentity();
			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
			glLoadIdentity();
			CMatrix3D transform;
			transform.SetOrtho(0.f, (float)g_xres, 0.f, (float)g_yres, -1.f, 1000.f);
			glLoadMatrixf(&transform._11);
#endif

#if OS_ANDROID
#warning TODO: cursors for Android
#else
			if (cursor_draw(g_VFS, cursorName.c_str(), g_mouse_x, g_yres-g_mouse_y, g_GuiScale, forceGL) < 0)
				LOGWARNING("Failed to draw cursor '%s'", utf8_from_wstring(cursorName));
#endif

#if CONFIG2_GLES
#warning TODO: implement cursors for GLES
#else
			// restore transform
			glMatrixMode(GL_PROJECTION);
			glPopMatrix();
			glMatrixMode(GL_MODELVIEW);
			glPopMatrix();
#endif
		}
	}

	glEnable(GL_DEPTH_TEST);

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

const std::vector<CStr>& GetMods(const CmdLineArgs& args, int flags)
{
	const bool init_mods = (flags & INIT_MODS) == INIT_MODS;
	const bool add_user = !InDevelopmentCopy() && !args.Has("noUserMod");
	const bool add_public = (flags & INIT_MODS_PUBLIC) == INIT_MODS_PUBLIC;

	if (!init_mods)
	{
		// Add the user mod if it should be present
		if (add_user && (g_modsLoaded.empty() || g_modsLoaded.back() != "user"))
			g_modsLoaded.push_back("user");

		return g_modsLoaded;
	}

	g_modsLoaded = args.GetMultiple("mod");

	if (add_public)
		g_modsLoaded.insert(g_modsLoaded.begin(), "public");

	g_modsLoaded.insert(g_modsLoaded.begin(), "mod");

	// Add the user mod if not explicitly disabled or we have a dev copy so
	// that saved files end up in version control and not in the user mod.
	if (add_user)
		g_modsLoaded.push_back("user");

	return g_modsLoaded;
}

void MountMods(const Paths& paths, const std::vector<CStr>& mods)
{
	OsPath modPath = paths.RData()/"mods";
	OsPath modUserPath = paths.UserData()/"mods";
	for (size_t i = 0; i < mods.size(); ++i)
	{
		size_t priority = (i+1)*2;	// mods are higher priority than regular mountings, which default to priority 0
		size_t userFlags = VFS_MOUNT_WATCH|VFS_MOUNT_ARCHIVABLE|VFS_MOUNT_REPLACEABLE;
		size_t baseFlags = userFlags|VFS_MOUNT_MUST_EXIST;

		OsPath modName(mods[i]);
		if (InDevelopmentCopy())
		{
			// We are running a dev copy, so only mount mods in the user mod path
			// if the mod does not exist in the data path.
			if (DirectoryExists(modPath / modName/""))
				g_VFS->Mount(L"", modPath / modName/"", baseFlags, priority);
			else
				g_VFS->Mount(L"", modUserPath / modName/"", userFlags, priority);
		}
		else
		{
			g_VFS->Mount(L"", modPath / modName/"", baseFlags, priority);
			// Ensure that user modified files are loaded, if they are present
			g_VFS->Mount(L"", modUserPath / modName/"", userFlags, priority+1);
		}
	}
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
	g_VFS->Mount(L"config/", readonlyConfig);

	// Engine localization files.
	g_VFS->Mount(L"l10n/", paths.RData()/"l10n"/"");

	MountMods(paths, GetMods(args, flags));

	// We mount these dirs last as otherwise writing could result in files being placed in a mod's dir.
	g_VFS->Mount(L"screenshots/", paths.UserData()/"screenshots"/"");
	g_VFS->Mount(L"saves/", paths.UserData()/"saves"/"", VFS_MOUNT_WATCH);
	// Mounting with highest priority, so that a mod supplied user.cfg is harmless
	g_VFS->Mount(L"config/", readonlyConfig, 0, (size_t)-1);
	if(readonlyConfig != paths.Config())
		g_VFS->Mount(L"config/", paths.Config(), 0, (size_t)-1);

	g_VFS->Mount(L"cache/", paths.Cache(), VFS_MOUNT_ARCHIVABLE);	// (adding XMBs to archive speeds up subsequent reads)

	// note: don't bother with g_VFS->TextRepresentation - directories
	// haven't yet been populated and are empty.
}


static void InitPs(bool setup_gui, const CStrW& gui_page, ScriptInterface* srcScriptInterface, JS::HandleValue initData)
{
	{
		// console
		TIMER(L"ps_console");

		g_Console->UpdateScreenSize(g_xres, g_yres);

		// Calculate and store the line spacing
		CFontMetrics font(CStrIntern(CONSOLE_FONT));
		g_Console->m_iFontHeight = font.GetLineSpacing();
		g_Console->m_iFontWidth = font.GetCharacterWidth(L'C');
		g_Console->m_charsPerPage = (size_t)(g_xres / g_Console->m_iFontWidth);
		// Offset by an arbitrary amount, to make it fit more nicely
		g_Console->m_iFontOffset = 7;

		double blinkRate = 0.5;
		CFG_GET_VAL("gui.cursorblinkrate", blinkRate);
		g_Console->SetCursorBlinkRate(blinkRate);
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
		g_GUI->SwitchPage(L"page_atlas.xml", srcScriptInterface, initData);
		return;
	}

	// GUI uses VFS, so this must come after VFS init.
	g_GUI->SwitchPage(gui_page, srcScriptInterface, initData);
}

void InitPsAutostart(bool networked, JS::HandleValue attrs)
{
	// The GUI has not been initialized yet, so use the simulation scriptinterface for this variable
	ScriptInterface& scriptInterface = g_Game->GetSimulation2()->GetScriptInterface();
	JSContext* cx = scriptInterface.GetContext();
	JSAutoRequest rq(cx);

	JS::RootedValue playerAssignments(cx);
	scriptInterface.Eval("({})", &playerAssignments);

	if (!networked)
	{
		JS::RootedValue localPlayer(cx);
		scriptInterface.Eval("({})", &localPlayer);
		scriptInterface.SetProperty(localPlayer, "player", g_Game->GetPlayerID());
		scriptInterface.SetProperty(playerAssignments, "local", localPlayer);
	}

	JS::RootedValue sessionInitData(cx);
	scriptInterface.Eval("({})", &sessionInitData);
	scriptInterface.SetProperty(sessionInitData, "attribs", attrs);
	scriptInterface.SetProperty(sessionInitData, "playerAssignments", playerAssignments);

	InitPs(true, L"page_loading.xml", &scriptInterface, sessionInitData);
}


static void InitInput()
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

	in_add_handler(conInputHandler);

	in_add_handler(HotkeyInputHandler);

	// gui_handler needs to be registered after (i.e. called before!) the
	// hotkey handler so that input boxes can be typed in without
	// setting off hotkeys.
	in_add_handler(gui_handler);

	in_add_handler(touch_input_handler);

	// must be registered after (called before) the GUI which relies on these globals
	in_add_handler(GlobalsInputHandler);
}


static void ShutdownPs()
{
	SAFE_DELETE(g_GUI);

	UnloadHotkeys();

	// disable the special Windows cursor, or free textures for OGL cursors
	cursor_draw(g_VFS, 0, g_mouse_x, g_yres-g_mouse_y, 1.0, false);
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
	// and init them in the ConfigDB when needed
	g_Renderer.SetOptionBool(CRenderer::OPT_NOVBO, g_NoGLVBO);
	g_Renderer.SetOptionBool(CRenderer::OPT_SHADOWS, g_Shadows);
	g_ConfigDB.SetValueBool(CFG_SYSTEM, "shadows", g_Shadows);

	g_Renderer.SetOptionBool(CRenderer::OPT_WATEREFFECTS, g_WaterEffects);
	g_ConfigDB.SetValueBool(CFG_SYSTEM, "watereffects", g_WaterEffects);
	g_Renderer.SetOptionBool(CRenderer::OPT_WATERFANCYEFFECTS, g_WaterFancyEffects);
	g_ConfigDB.SetValueBool(CFG_SYSTEM, "waterfancyeffects", g_WaterFancyEffects);
	g_Renderer.SetOptionBool(CRenderer::OPT_WATERREALDEPTH, g_WaterRealDepth);
	g_ConfigDB.SetValueBool(CFG_SYSTEM, "waterrealdepth", g_WaterRealDepth);
	g_Renderer.SetOptionBool(CRenderer::OPT_WATERREFLECTION, g_WaterReflection);
	g_ConfigDB.SetValueBool(CFG_SYSTEM, "waterreflection", g_WaterReflection);
	g_Renderer.SetOptionBool(CRenderer::OPT_WATERREFRACTION, g_WaterRefraction);
	g_ConfigDB.SetValueBool(CFG_SYSTEM, "waterrefraction", g_WaterRefraction);
	g_Renderer.SetOptionBool(CRenderer::OPT_SHADOWSONWATER, g_WaterShadows);
	g_ConfigDB.SetValueBool(CFG_SYSTEM, "watershadows", g_WaterShadows);

	g_Renderer.SetRenderPath(CRenderer::GetRenderPathByName(g_RenderPath));
	g_Renderer.SetOptionBool(CRenderer::OPT_SHADOWPCF, g_ShadowPCF);
	g_ConfigDB.SetValueBool(CFG_SYSTEM, "shadowpcf", g_ShadowPCF);
	g_Renderer.SetOptionBool(CRenderer::OPT_PARTICLES, g_Particles);
	g_ConfigDB.SetValueBool(CFG_SYSTEM, "particles", g_Particles);
	g_Renderer.SetOptionBool(CRenderer::OPT_FOG, g_Fog);
	g_ConfigDB.SetValueBool(CFG_SYSTEM, "fog", g_Fog);
	g_Renderer.SetOptionBool(CRenderer::OPT_SILHOUETTES, g_Silhouettes);
	g_ConfigDB.SetValueBool(CFG_SYSTEM, "silhouettes", g_Silhouettes);
	g_Renderer.SetOptionBool(CRenderer::OPT_SHOWSKY, g_ShowSky);
	g_ConfigDB.SetValueBool(CFG_SYSTEM, "showsky", g_ShowSky);
	g_Renderer.SetOptionBool(CRenderer::OPT_PREFERGLSL, g_PreferGLSL);
	g_ConfigDB.SetValueBool(CFG_SYSTEM, "preferglsl", g_PreferGLSL);
	g_Renderer.SetOptionBool(CRenderer::OPT_POSTPROC, g_PostProc);
	g_ConfigDB.SetValueBool(CFG_SYSTEM, "postproc", g_PostProc);
	g_Renderer.SetOptionBool(CRenderer::OPT_SMOOTHLOS, g_SmoothLOS);
	g_ConfigDB.SetValueBool(CFG_SYSTEM, "smoothlos", g_SmoothLOS);

	// create terrain related stuff
	new CTerrainTextureManager;

	g_Renderer.Open(g_xres, g_yres);

	// Setup lighting environment. Since the Renderer accesses the
	// lighting environment through a pointer, this has to be done before
	// the first Frame.
	g_Renderer.SetLightEnv(&g_LightEnv);

	// I haven't seen the camera affecting GUI rendering and such, but the
	// viewport has to be updated according to the video mode
	SViewPort vp;
	vp.m_X = 0;
	vp.m_Y = 0;
	vp.m_Width = g_xres;
	vp.m_Height = g_yres;
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
		LOGERROR("SDL library initialization failed: %s", SDL_GetError());
		throw PSERROR_System_SDLInitFailed();
	}
	atexit(SDL_Quit);

	// Text input is active by default, disable it until it is actually needed.
	SDL_StopTextInput();

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
	sys_cursor_reset();
}


void EndGame()
{
	const bool nonVisual = g_Game && g_Game->IsGraphicsDisabled();

	if (g_Game && g_Game->IsGameStarted() && !g_Game->IsVisualReplay() &&
	    g_AtlasGameLoop && !g_AtlasGameLoop->running && !nonVisual)
		VisualReplay::SaveReplayMetadata(g_GUI->GetActiveGUI()->GetScriptInterface().get());

	SAFE_DELETE(g_NetClient);
	SAFE_DELETE(g_NetServer);
	SAFE_DELETE(g_Game);

	if (!nonVisual)
	{
		ISoundManager::CloseGame();
		g_Renderer.ResetState();
	}
}

void Shutdown(int flags)
{
	const bool nonVisual = g_Game && g_Game->IsGraphicsDisabled();

	if ((flags & SHUTDOWN_FROM_CONFIG))
		goto from_config;

	EndGame();

	SAFE_DELETE(g_XmppClient);

	SAFE_DELETE(g_ModIo);

	ShutdownPs();

	TIMER_BEGIN(L"shutdown TexMan");
	delete &g_TexMan;
	TIMER_END(L"shutdown TexMan");

	// destroy renderer if it was initialised
	if (!nonVisual)
	{
		TIMER_BEGIN(L"shutdown Renderer");
		delete &g_Renderer;
		g_VBMan.Shutdown();
		TIMER_END(L"shutdown Renderer");
	}

	g_Profiler2.ShutdownGPU();

	// Free cursors before shutting down SDL, as they may depend on SDL.
	cursor_shutdown();

	TIMER_BEGIN(L"shutdown SDL");
	ShutdownSDL();
	TIMER_END(L"shutdown SDL");

	if (!nonVisual)
		g_VideoMode.Shutdown();

	TIMER_BEGIN(L"shutdown UserReporter");
	g_UserReporter.Deinitialize();
	TIMER_END(L"shutdown UserReporter");

	// Cleanup curl now that g_ModIo and g_UserReporter have been shutdown.
	curl_global_cleanup();

	delete &g_L10n;

from_config:
	TIMER_BEGIN(L"shutdown ConfigDB");
	delete &g_ConfigDB;
	TIMER_END(L"shutdown ConfigDB");

	SAFE_DELETE(g_Console);

	// This is needed to ensure that no callbacks from the JSAPI try to use
	// the profiler when it's already destructed
	g_ScriptRuntime.reset();

	// resource
	// first shut down all resource owners, and then the handle manager.
	TIMER_BEGIN(L"resource modules");

		ISoundManager::SetEnabled(false);

		g_VFS.reset();

		// this forcibly frees all open handles (thus preventing real leaks),
		// and makes further access to h_mgr impossible.
		h_mgr_shutdown();

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

	ThreadUtil::SetMainThread();

	debug_SetThreadName("main");
	// add all debug_printf "tags" that we are interested in:
	debug_filter_add("TIMER");

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
	h_mgr_init();

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

	// Using a global object for the runtime is a workaround until Simulation and AI use
	// their own threads and also their own runtimes.
	const int runtimeSize = 384 * 1024 * 1024;
	const int heapGrowthBytesGCTrigger = 20 * 1024 * 1024;
	g_ScriptRuntime = ScriptInterface::CreateRuntime(shared_ptr<ScriptRuntime>(), runtimeSize, heapGrowthBytesGCTrigger);

	Mod::CacheEnabledModVersions(g_ScriptRuntime);

	// Special command-line mode to dump the entity schemas instead of running the game.
	// (This must be done after loading VFS etc, but should be done before wasting time
	// on anything else.)
	if (args.Has("dumpSchema"))
	{
		CSimulation2 sim(NULL, g_ScriptRuntime, NULL);
		sim.LoadDefaultScripts();
		std::ofstream f("entity.rng", std::ios_base::out | std::ios_base::trunc);
		f << sim.GenerateSchema();
		std::cout << "Generated entity.rng\n";
		exit(0);
	}

	CNetHost::Initialize();

#if CONFIG2_AUDIO
	if (!args.Has("autostart-nonvisual"))
		ISoundManager::CreateSoundManager();
#endif

	// Check if there are mods specified on the command line,
	// or if we already set the mods (~INIT_MODS),
	// else check if there are mods that should be loaded specified
	// in the config and load those (by aborting init and restarting
	// the engine).
	if (!args.Has("mod") && (flags & INIT_MODS) == INIT_MODS)
	{
		CStr modstring;
		CFG_GET_VAL("mod.enabledmods", modstring);
		if (!modstring.empty())
		{
			std::vector<CStr> mods;
			boost::split(mods, modstring, boost::is_any_of(" "), boost::token_compress_on);
			std::swap(g_modsLoaded, mods);

			// Abort init and restart
			RestartEngine();
			return false;
		}
	}

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

	const int quality = SANE_TEX_QUALITY_DEFAULT;	// TODO: set value from config file
	SetTextureQuality(quality);

	ogl_WarnIfError();

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

	// (must come after SetVideoMode, since it calls ogl_Init)
	if (ogl_HaveExtensions(0, "GL_ARB_vertex_program", "GL_ARB_fragment_program", NULL) != 0 // ARB
		&& ogl_HaveExtensions(0, "GL_ARB_vertex_shader", "GL_ARB_fragment_shader", NULL) != 0) // GLSL
	{
		DEBUG_DISPLAY_ERROR(
			L"Your graphics card doesn't appear to be fully compatible with OpenGL shaders."
			L" In the future, the game will not support pre-shader graphics cards."
			L" You are advised to try installing newer drivers and/or upgrade your graphics card."
			L" For more information, please see http://www.wildfiregames.com/forum/index.php?showtopic=16734"
		);
		// TODO: actually quit once fixed function support is dropped
	}

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

	// TODO: Is this the best place for this?
	if (VfsDirectoryExists(L"maps/"))
		CXeromyces::AddValidator(g_VFS, "map", "maps/scenario.rng");

	try
	{
		if (!AutostartVisualReplay(args.Get("replay-visual")) && !Autostart(args))
		{
			const bool setup_gui = ((flags & INIT_NO_GUI) == 0);
			// We only want to display the splash screen at startup
			shared_ptr<ScriptInterface> scriptInterface = g_GUI->GetScriptInterface();
			JSContext* cx = scriptInterface->GetContext();
			JSAutoRequest rq(cx);
			JS::RootedValue data(cx);
			if (g_GUI)
			{
				scriptInterface->Eval("({})", &data);
				scriptInterface->SetProperty(data, "isStartup", true);
				if (!installedMods.empty())
					scriptInterface->SetProperty(data, "installedMods", installedMods);
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

void InitNonVisual(const CmdLineArgs& args)
{
	// Need some stuff for terrain movement costs:
	// (TODO: this ought to be independent of any graphics code)
	new CTerrainTextureManager;
	g_TexMan.LoadTerrainTextures();
	Autostart(args);
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

/*
 * Command line options for autostart
 * (keep synchronized with binaries/system/readme.txt):
 *
 * -autostart="TYPEDIR/MAPNAME"    enables autostart and sets MAPNAME;
 *                                 TYPEDIR is skirmishes, scenarios, or random
 * -autostart-seed=SEED            sets randomization seed value (default 0, use -1 for random)
 * -autostart-ai=PLAYER:AI         sets the AI for PLAYER (e.g. 2:petra)
 * -autostart-aidiff=PLAYER:DIFF   sets the DIFFiculty of PLAYER's AI
 *                                 (0: sandbox, 5: very hard)
 * -autostart-aiseed=AISEED        sets the seed used for the AI random
 *                                 generator (default 0, use -1 for random)
 * -autostart-player=NUMBER        sets the playerID in non-networked games (default 1, use -1 for observer)
 * -autostart-civ=PLAYER:CIV       sets PLAYER's civilisation to CIV
 *                                 (skirmish and random maps only)
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
 * -autostart="scenarios/Arcadia" -autostart-host -autostart-host-players=2 -autostart-playername="Bob"
 *  "Alice" joins the match as player 2:
 * -autostart="scenarios/Arcadia" -autostart-client=127.0.0.1 -autostart-playername="Alice"
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
	CStr autoStartName = args.Get("autostart");

	if (autoStartName.empty())
		return false;

	const bool nonVisual = args.Has("autostart-nonvisual");
	g_Game = new CGame(nonVisual, !nonVisual);

	ScriptInterface& scriptInterface = g_Game->GetSimulation2()->GetScriptInterface();
	JSContext* cx = scriptInterface.GetContext();
	JSAutoRequest rq(cx);

	JS::RootedValue attrs(cx);
	scriptInterface.Eval("({})", &attrs);
	JS::RootedValue settings(cx);
	scriptInterface.Eval("({})", &settings);
	JS::RootedValue playerData(cx);
	scriptInterface.Eval("([])", &playerData);

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
		// Random map definition will be loaded from JSON file, so we need to parse it
		std::wstring scriptPath = L"maps/" + autoStartName.FromUTF8() + L".json";
		JS::RootedValue scriptData(cx);
		scriptInterface.ReadJSONFile(scriptPath, &scriptData);
		if (!scriptData.isUndefined() && scriptInterface.GetProperty(scriptData, "settings", &settings))
		{
			// JSON loaded ok - copy script name over to game attributes
			std::wstring scriptFile;
			scriptInterface.GetProperty(settings, "Script", scriptFile);
			scriptInterface.SetProperty(attrs, "script", scriptFile);				// RMS filename
		}
		else
		{
			// Problem with JSON file
			LOGERROR("Autostart: Error reading random map script '%s'", utf8_from_wstring(scriptPath));
			throw PSERROR_Game_World_MapLoadFailed("Error reading random map script.\nCheck application log for details.");
		}

		// Get optional map size argument (default 192)
		uint mapSize = 192;
		if (args.Has("autostart-size"))
		{
			CStr size = args.Get("autostart-size");
			mapSize = size.ToUInt();
		}

		scriptInterface.SetProperty(settings, "Size", mapSize);		// Random map size (in patches)

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
			JS::RootedValue player(cx);
			scriptInterface.Eval("({})", &player);

			// We could load player_defaults.json here, but that would complicate the logic
			// even more and autostart is only intended for developers anyway
			scriptInterface.SetProperty(player, "Civ", std::string("athen"));
			scriptInterface.SetPropertyInt(playerData, i, player);
		}
		mapType = "random";
	}
	else if (mapDirectory == L"scenarios" || mapDirectory == L"skirmishes")
	{
		// Initialize general settings from the map data so some values
		// (e.g. name of map) are always present, even when autostart is
		// partially configured
		CStr8 mapSettingsJSON = LoadSettingsOfScenarioMap("maps/" + autoStartName + ".xml");
		scriptInterface.ParseJSON(mapSettingsJSON, &settings);

		// Initialize the playerData array being modified by autostart
		// with the real map data, so sensible values are present:
		scriptInterface.GetProperty(settings, "PlayerData", &playerData);

		if (mapDirectory == L"scenarios")
			mapType = "scenario";
		else
			mapType = "skirmish";
	}
	else
	{
		LOGERROR("Autostart: Unrecognized map type '%s'", utf8_from_wstring(mapDirectory));
		throw PSERROR_Game_World_MapLoadFailed("Unrecognized map type.\nConsult readme.txt for the currently supported types.");
	}

	scriptInterface.SetProperty(attrs, "mapType", mapType);
	scriptInterface.SetProperty(attrs, "map", std::string("maps/" + autoStartName));
	scriptInterface.SetProperty(settings, "mapType", mapType);
	scriptInterface.SetProperty(settings, "CheatsEnabled", true);

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
	scriptInterface.SetProperty(settings, "Seed", seed);

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
	scriptInterface.SetProperty(settings, "AISeed", aiseed);

	// Set player data for AIs
	//		attrs.settings = { PlayerData: [ { AI: ... }, ... ] }
	//		            or = { PlayerData: [ null, { AI: ... }, ... ] } when gaia set
	int offset = 1;
	JS::RootedValue player(cx);
	if (scriptInterface.GetPropertyInt(playerData, 0, &player) && player.isNull())
		offset = 0;

	// Set teams
	if (args.Has("autostart-team"))
	{
		std::vector<CStr> civArgs = args.GetMultiple("autostart-team");
		for (size_t i = 0; i < civArgs.size(); ++i)
		{
			int playerID = civArgs[i].BeforeFirst(":").ToInt();

			// Instead of overwriting existing player data, modify the array
			JS::RootedValue player(cx);
			if (!scriptInterface.GetPropertyInt(playerData, playerID-offset, &player) || player.isUndefined())
			{
				if (mapDirectory == L"skirmishes")
				{
					// playerID is certainly bigger than this map player number
					LOGWARNING("Autostart: Invalid player %d in autostart-team option", playerID);
					continue;
				}
				scriptInterface.Eval("({})", &player);
			}

			int teamID = civArgs[i].AfterFirst(":").ToInt() - 1;
			scriptInterface.SetProperty(player, "Team", teamID);
			scriptInterface.SetPropertyInt(playerData, playerID-offset, player);
		}
	}

	int ceasefire = 0;
	if (args.Has("autostart-ceasefire"))
		ceasefire = args.Get("autostart-ceasefire").ToInt();
	scriptInterface.SetProperty(settings, "Ceasefire", ceasefire);

	if (args.Has("autostart-ai"))
	{
		std::vector<CStr> aiArgs = args.GetMultiple("autostart-ai");
		for (size_t i = 0; i < aiArgs.size(); ++i)
		{
			int playerID = aiArgs[i].BeforeFirst(":").ToInt();

			// Instead of overwriting existing player data, modify the array
			JS::RootedValue player(cx);
			if (!scriptInterface.GetPropertyInt(playerData, playerID-offset, &player) || player.isUndefined())
			{
				if (mapDirectory == L"scenarios" || mapDirectory == L"skirmishes")
				{
					// playerID is certainly bigger than this map player number
					LOGWARNING("Autostart: Invalid player %d in autostart-ai option", playerID);
					continue;
				}
				scriptInterface.Eval("({})", &player);
			}

			CStr name = aiArgs[i].AfterFirst(":");
			scriptInterface.SetProperty(player, "AI", std::string(name));
			scriptInterface.SetProperty(player, "AIDiff", 3);
			scriptInterface.SetProperty(player, "AIBehavior", std::string("balanced"));
			scriptInterface.SetPropertyInt(playerData, playerID-offset, player);
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
			JS::RootedValue player(cx);
			if (!scriptInterface.GetPropertyInt(playerData, playerID-offset, &player) || player.isUndefined())
			{
				if (mapDirectory == L"scenarios" || mapDirectory == L"skirmishes")
				{
					// playerID is certainly bigger than this map player number
					LOGWARNING("Autostart: Invalid player %d in autostart-aidiff option", playerID);
					continue;
				}
				scriptInterface.Eval("({})", &player);
			}

			int difficulty = civArgs[i].AfterFirst(":").ToInt();
			scriptInterface.SetProperty(player, "AIDiff", difficulty);
			scriptInterface.SetPropertyInt(playerData, playerID-offset, player);
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
				JS::RootedValue player(cx);
				if (!scriptInterface.GetPropertyInt(playerData, playerID-offset, &player) || player.isUndefined())
				{
					if (mapDirectory == L"skirmishes")
					{
						// playerID is certainly bigger than this map player number
						LOGWARNING("Autostart: Invalid player %d in autostart-civ option", playerID);
						continue;
					}
					scriptInterface.Eval("({})", &player);
				}

				CStr name = civArgs[i].AfterFirst(":");
				scriptInterface.SetProperty(player, "Civ", std::string(name));
				scriptInterface.SetPropertyInt(playerData, playerID-offset, player);
			}
		}
		else
			LOGWARNING("Autostart: Option 'autostart-civ' is invalid for scenarios");
	}

	// Add player data to map settings
	scriptInterface.SetProperty(settings, "PlayerData", playerData);

	// Add map settings to game attributes
	scriptInterface.SetProperty(attrs, "settings", settings);

	// Get optional playername
	CStrW userName = L"anonymous";
	if (args.Has("autostart-playername"))
		userName = args.Get("autostart-playername").FromUTF8();

	// Add additional scripts to the TriggerScripts property
	std::vector<CStrW> triggerScriptsVector;
	JS::RootedValue triggerScripts(cx);

	if (scriptInterface.HasProperty(settings, "TriggerScripts"))
	{
		scriptInterface.GetProperty(settings, "TriggerScripts", &triggerScripts);
		FromJSVal_vector(cx, triggerScripts, triggerScriptsVector);
	}

	if (nonVisual)
	{
		CStr nonVisualScript = "scripts/NonVisualTrigger.js";
		triggerScriptsVector.push_back(nonVisualScript.FromUTF8());
	}

	std::vector<CStr> victoryConditions(1, "conquest");
	if (args.Has("autostart-victory"))
		victoryConditions = args.GetMultiple("autostart-victory");

	if (victoryConditions.size() == 1 && victoryConditions[0] == "endless")
		victoryConditions.clear();

	scriptInterface.SetProperty(settings, "VictoryConditions", victoryConditions);

	for (const CStr& victory : victoryConditions)
	{
		JS::RootedValue scriptData(cx);
		JS::RootedValue data(cx);
		JS::RootedValue victoryScripts(cx);

		CStrW scriptPath = L"simulation/data/settings/victory_conditions/" + victory.FromUTF8() + L".json";
		scriptInterface.ReadJSONFile(scriptPath, &scriptData);
		if (!scriptData.isUndefined() && scriptInterface.GetProperty(scriptData, "Data", &data) && !data.isUndefined()
			&& scriptInterface.GetProperty(data, "Scripts", &victoryScripts) && !victoryScripts.isUndefined())
		{
			std::vector<CStrW> victoryScriptsVector;
			FromJSVal_vector(cx, victoryScripts, victoryScriptsVector);
			triggerScriptsVector.insert(triggerScriptsVector.end(), victoryScriptsVector.begin(), victoryScriptsVector.end());
		}
		else
		{
			LOGERROR("Autostart: Error reading victory script '%s'", utf8_from_wstring(scriptPath));
			throw PSERROR_Game_World_MapLoadFailed("Error reading victory script.\nCheck application log for details.");
		}
	}

	ToJSVal_vector(cx, &triggerScripts, triggerScriptsVector);
	scriptInterface.SetProperty(settings, "TriggerScripts", triggerScripts);

	int wonderDuration = 10;
	if (args.Has("autostart-wonderduration"))
		wonderDuration = args.Get("autostart-wonderduration").ToInt();
	scriptInterface.SetProperty(settings, "WonderDuration", wonderDuration);

	int relicDuration = 10;
	if (args.Has("autostart-relicduration"))
		relicDuration = args.Get("autostart-relicduration").ToInt();
	scriptInterface.SetProperty(settings, "RelicDuration", relicDuration);

	int relicCount = 2;
	if (args.Has("autostart-reliccount"))
		relicCount = args.Get("autostart-reliccount").ToInt();
	scriptInterface.SetProperty(settings, "RelicCount", relicCount);

	if (args.Has("autostart-host"))
	{
		InitPsAutostart(true, attrs);

		size_t maxPlayers = 2;
		if (args.Has("autostart-host-players"))
			maxPlayers = args.Get("autostart-host-players").ToUInt();

		g_NetServer = new CNetServer(false, maxPlayers);

		g_NetServer->UpdateGameAttributes(&attrs, scriptInterface);

		bool ok = g_NetServer->SetupConnection(PS_DEFAULT_PORT);
		ENSURE(ok);

		g_NetClient = new CNetClient(g_Game, true);
		g_NetClient->SetUserName(userName);
		g_NetClient->SetupConnection("127.0.0.1", PS_DEFAULT_PORT);
	}
	else if (args.Has("autostart-client"))
	{
		InitPsAutostart(true, attrs);

		g_NetClient = new CNetClient(g_Game, false);
		g_NetClient->SetUserName(userName);

		CStr ip = args.Get("autostart-client");
		if (ip.empty())
			ip = "127.0.0.1";

		bool ok = g_NetClient->SetupConnection(ip, PS_DEFAULT_PORT);
		ENSURE(ok);
	}
	else
	{
		g_Game->SetPlayerID(args.Has("autostart-player") ? args.Get("autostart-player").ToInt() : 1);

		g_Game->StartGame(&attrs, "");

		if (nonVisual)
		{
			// TODO: Non progressive load can fail - need a decent way to handle this
			LDR_NonprogressiveLoad();
			ENSURE(g_Game->ReallyStartGame() == PSRETURN_OK);
		}
		else
			InitPsAutostart(false, attrs);
	}

	return true;
}

bool AutostartVisualReplay(const std::string& replayFile)
{
	if (!FileExists(OsPath(replayFile)))
		return false;

	g_Game = new CGame(false, false);
	g_Game->SetPlayerID(-1);
	g_Game->StartVisualReplay(replayFile);

	ScriptInterface& scriptInterface = g_Game->GetSimulation2()->GetScriptInterface();
	JSContext* cx = scriptInterface.GetContext();
	JSAutoRequest rq(cx);
	JS::RootedValue attrs(cx, g_Game->GetSimulation2()->GetInitAttributes());

	InitPsAutostart(false, attrs);

	return true;
}

void CancelLoad(const CStrW& message)
{
	shared_ptr<ScriptInterface> pScriptInterface = g_GUI->GetActiveGUI()->GetScriptInterface();
	JSContext* cx = pScriptInterface->GetContext();
	JSAutoRequest rq(cx);

	JS::RootedValue global(cx, pScriptInterface->GetGlobalObject());

	LDR_Cancel();

	if (g_GUI &&
	    g_GUI->HasPages() &&
	    pScriptInterface->HasProperty(global, "cancelOnLoadGameError"))
		pScriptInterface->CallFunctionVoid(global, "cancelOnLoadGameError", message);
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
