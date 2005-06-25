#include "precompiled.h"

#include "MessageHandlerImpl.h"
#include "Messages.h"

#include "lib/sdl.h"
#include "lib/ogl.h"
#include "ps/CLogger.h"
#include "gui/GUI.h"
#include "renderer/Renderer.h"

using namespace AtlasMessage;

extern __declspec(dllimport) void Atlas_StartWindow(wchar_t* type);
extern __declspec(dllimport) void Atlas_SetMessageHandler(MessageHandler*);

static MessageHandlerImpl msgHandler;

static void* LaunchWindow(void*)
{
	Atlas_StartWindow(L"ScenarioEditor");
	return NULL;
}

extern void Init_(int argc, char** argv, bool setup_gfx);
extern void Shutdown_();
extern void Render_();

extern int g_xres, g_yres;

extern "C" { __declspec(dllimport) int __stdcall SwapBuffers(void*); }
	// HACK (and not exactly portable)
	//
	// (Er, actually that's what most of this file is. Oh well.)


void BeginAtlas(int argc, char** argv) 
{
	Atlas_SetMessageHandler(&msgHandler);

	pthread_t gameThread;
	pthread_create(&gameThread, NULL, LaunchWindow, NULL);

	bool running = true;
	bool rendering = false;
	HDC currentDC = NULL;

	while (running)
	{
		IMessage* msg;
		while (msg = msgHandler.Retrieve())
		{
			switch (msg->GetType())
			{
			case CommandString:
				{
					mCommandString* cmd = static_cast<mCommandString*>(msg);

					if (cmd->name == "init")
					{
						oglInit();
						Init_(argc, argv, false);
					}
					else if (cmd->name == "render_enable")
					{
						rendering = true;
					}
					else if (cmd->name == "render_disable")
					{
						rendering = false;
					}
					else if (cmd->name == "shutdown")
					{
						Shutdown_();
					}
					else if (cmd->name == "exit")
					{
						running = false;
					}
					else
					{
						LOG(ERROR, "atlas", "Unrecognised command string (%s)", cmd->name.c_str());
					}
				}
				break;

			case SetContext:
				{
					mSetContext* cmd = static_cast<mSetContext*>(msg);
					// TODO: portability
					wglMakeCurrent(cmd->hdc, cmd->hglrc);
					currentDC = cmd->hdc;
				}
				break;

			case ResizeScreen:
				{
					mResizeScreen* cmd = static_cast<mResizeScreen*>(msg);
					g_xres = cmd->width;
					g_yres = cmd->height;
					if (g_xres == 0) g_xres = 160;
					if (g_yres == 0) g_yres = 120;
					SViewPort vp;
					vp.m_X = vp.m_Y = 0;
					vp.m_Width = g_xres;
					vp.m_Height = g_yres;
					g_Renderer.SetViewport(vp);
					g_GUI.UpdateResolution();
				}
				break;

			default:
				LOG(ERROR, "atlas", "Unrecognised message (%d)", msg->GetType());
				break;
			}

			delete msg;
		}

		if (! running)
			break;

		if (rendering)
		{
			Render_();
			SwapBuffers(currentDC);
		}

		SDL_Delay(100);
	}

	pthread_join(gameThread, NULL);
}
