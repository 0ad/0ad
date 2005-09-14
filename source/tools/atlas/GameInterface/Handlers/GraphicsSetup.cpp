#include "precompiled.h"

#include "MessageHandler.h"
#include "../GameLoop.h"

#include "renderer/Renderer.h"
#include "gui/GUI.h"
#include "ps/CConsole.h"

extern int g_xres, g_yres;

extern void Init_(int argc, char** argv, bool setup_gfx);
extern void Shutdown_();


namespace AtlasMessage {


MESSAGEHANDLER_STR(init)
{
	oglInit();
	Init_(g_GameLoop->argc, g_GameLoop->argv, false);

#if OS_WIN
	// HACK (to stop things looking very ugly when scrolling) - should
	// use proper config system.
	if(oglHaveExtension("WGL_EXT_swap_control"))
		wglSwapIntervalEXT(1);
#endif
}


MESSAGEHANDLER_STR(shutdown)
{
	Shutdown_();
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
//	SViewPort vp;
//	vp.m_X = vp.m_Y = 0;
//	vp.m_Width = g_xres;
//	vp.m_Height = g_yres;
//	g_Renderer.SetViewport(vp); // TODO: what does this do?
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
