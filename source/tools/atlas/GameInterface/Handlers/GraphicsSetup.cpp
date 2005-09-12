#include "precompiled.h"

#include "MessageHandler.h"
#include "../GameLoop.h"

#include "renderer/Renderer.h"
#include "gui/GUI.h"
#include "ps/Game.h"
#include "ps/GameAttributes.h"
#include "ps/Loader.h"
#include "ps/CConsole.h"

extern int g_xres, g_yres;

extern void Init_(int argc, char** argv, bool setup_gfx);
extern void Shutdown_();


namespace AtlasMessage {


void fCommandString_init(IMessage*)
{
	oglInit();
	Init_(g_GameLoop->argc, g_GameLoop->argv, false);

	// HACK (to stop things looking very ugly when scrolling) - should
	// use proper config system.
	if(oglHaveExtension("WGL_EXT_swap_control"))
		wglSwapIntervalEXT(1);

	// Set attributes for the game:
	//  Start without a map
	g_GameAttributes.m_MapFile = L"";
	//  Make all players locally controlled
	for (int i=1; i<8; ++i) 
		g_GameAttributes.GetSlot(i)->AssignLocal();

	// Start the game:
	g_Game = new CGame();
	PSRETURN ret = g_Game->StartGame(&g_GameAttributes);
	assert(ret == PSRETURN_OK);
	LDR_NonprogressiveLoad();
	ret = g_Game->ReallyStartGame();
	assert(ret == PSRETURN_OK);
}
REGISTER(CommandString_init);


void fCommandString_shutdown(IMessage*)
{
	Shutdown_();
	g_GameLoop->rendering = false;
}
REGISTER(CommandString_shutdown);


void fCommandString_exit(IMessage*)
{
	g_GameLoop->running = false;
}
REGISTER(CommandString_exit);


void fCommandString_render_enable(IMessage*)
{
	g_GameLoop->rendering = true;
}
REGISTER(CommandString_render_enable);


void fCommandString_render_disable(IMessage*)
{
	g_GameLoop->rendering = false;
}
REGISTER(CommandString_render_disable);

//////////////////////////////////////////////////////////////////////////

void fSetContext(IMessage* msg)
{
	mSetContext* cmd = static_cast<mSetContext*>(msg);
	// TODO: portability
	wglMakeCurrent((HDC)cmd->hdc, (HGLRC)cmd->hglrc);
	g_GameLoop->currentDC = cmd->hdc;
}
REGISTER(SetContext);


void fResizeScreen(IMessage* msg)
{
	mResizeScreen* cmd = static_cast<mResizeScreen*>(msg);
	g_xres = cmd->width;
	g_yres = cmd->height;
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
REGISTER(ResizeScreen);

//////////////////////////////////////////////////////////////////////////

void fRenderStyle(IMessage* msg)
{
	mRenderStyle* cmd = static_cast<mRenderStyle*>(msg);

	g_Renderer.SetTerrainRenderMode(cmd->wireframe ? EDGED_FACES : SOLID);
	g_Renderer.SetModelRenderMode(cmd->wireframe ? EDGED_FACES : SOLID);
}
REGISTER(RenderStyle);

}
