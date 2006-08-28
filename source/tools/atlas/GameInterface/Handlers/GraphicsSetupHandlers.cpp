#include "precompiled.h"

#include "MessageHandler.h"
#include "../GameLoop.h"
#include "../CommandProc.h"
#include "../ActorViewer.h"
#include "../View.h"

#include "renderer/Renderer.h"
#include "graphics/GameView.h"
#include "gui/GUIbase.h"
#include "gui/CGUI.h"
#include "ps/CConsole.h"
#include "ps/Game.h"
#include "maths/MathUtil.h"

#include "ps/GameSetup/Config.h"
#include "ps/GameSetup/GameSetup.h"

namespace AtlasMessage {

MESSAGEHANDLER(Init)
{
	UNUSED2(msg);
	
	oglInit();

	g_Quickstart = true;
	Init(g_GameLoop->argc, g_GameLoop->argv, INIT_HAVE_VMODE|INIT_NO_GUI);

#if OS_WIN
	// HACK (to stop things looking very ugly when scrolling) - should
	// use proper config system.
	if(oglHaveExtension("WGL_EXT_swap_control"))
		pwglSwapIntervalEXT(1);
#endif
}


MESSAGEHANDLER(Shutdown)
{
	UNUSED2(msg);
	
	// Empty the CommandProc, to get rid of its references to entities before
	// we kill the EntityManager
	GetCommandProc().Destroy();

	View::DestroyViews();
	g_GameLoop->view = View::GetView_None();

	Shutdown();
}


QUERYHANDLER(Exit)
{
	UNUSED2(msg);
	g_GameLoop->running = false;
}


MESSAGEHANDLER(RenderEnable)
{
	if      (msg->view == eRenderView::NONE)  g_GameLoop->view = View::GetView_None();
	else if (msg->view == eRenderView::GAME)  g_GameLoop->view = View::GetView_Game();
	else if (msg->view == eRenderView::ACTOR) g_GameLoop->view = View::GetView_Actor();
	else debug_warn("Invalid view type");
}

MESSAGEHANDLER(SetActorViewer)
{
	View::GetView_Actor()->SetSpeedMultiplier(msg->speed);
	View::GetView_Actor()->GetActorViewer().SetActor(*msg->id, *msg->animation);
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
