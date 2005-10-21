#include "precompiled.h"

#include "MessageHandler.h"
#include "../GameLoop.h"

#include "renderer/Renderer.h"
#include "gui/GUI.h"
#include "ps/CConsole.h"

#include "ps/GameSetup/Config.h"
#include "ps/GameSetup/GameSetup.h"


namespace AtlasMessage {


MESSAGEHANDLER_STR(init)
{
	oglInit();

	g_Quickstart = true;
	Init(g_GameLoop->argc, g_GameLoop->argv, INIT_HAVE_VMODE|INIT_NO_GUI);

#if OS_WIN
	// HACK (to stop things looking very ugly when scrolling) - should
	// use proper config system.
	if(oglHaveExtension("WGL_EXT_swap_control"))
		wglSwapIntervalEXT(1);
#endif
}


MESSAGEHANDLER_STR(shutdown)
{
	Shutdown();
	g_GameLoop->rendering = false;
}


MESSAGEHANDLER_STR(exit)
{
	g_GameLoop->running = false;
}


MESSAGEHANDLER_STR(render_enable)
{
	g_GameLoop->rendering = true;
}


MESSAGEHANDLER_STR(render_disable)
{
	g_GameLoop->rendering = false;
}

//////////////////////////////////////////////////////////////////////////

MESSAGEHANDLER(SetContext)
{
	g_GameLoop->glContext = msg->context;
	Atlas_GLSetCurrent((void*)g_GameLoop->glContext);
}


MESSAGEHANDLER(ResizeScreen)
{
	g_xres = msg->width;
	g_yres = msg->height;
	if (g_xres <= 2) g_xres = 2; // avoid GL errors caused by invalid sizes
	if (g_yres <= 2) g_yres = 2;

	SViewPort vp = { 0, 0, g_xres, g_yres };
	g_Renderer.SetViewport(vp);

	g_Renderer.Resize(g_xres, g_yres);

	g_GUI.UpdateResolution();

	g_Console->UpdateScreenSize(g_xres, g_yres);
}

//////////////////////////////////////////////////////////////////////////

MESSAGEHANDLER(RenderStyle)
{
	g_Renderer.SetTerrainRenderMode(msg->wireframe ? EDGED_FACES : SOLID);
	g_Renderer.SetModelRenderMode(msg->wireframe ? EDGED_FACES : SOLID);
}

}
