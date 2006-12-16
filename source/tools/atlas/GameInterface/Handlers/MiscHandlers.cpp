#include "precompiled.h"

#include "MessageHandler.h"
#include "../MessagePasserImpl.h"

#include "ps/GameSetup/Config.h"
#include "ps/Util.h"

#include "ps/Game.h"
#include "graphics/GameView.h"
#include "graphics/CinemaTrack.h"
#include "renderer/Renderer.h"
#include "ps/GameSetup/GameSetup.h"
#include "../GameLoop.h"

extern void (*Atlas_GLSwapBuffers)(void* context);

namespace AtlasMessage {

MESSAGEHANDLER(MessageTrace)
{
	((MessagePasserImpl*)g_MessagePasser)->SetTrace(msg->enable);
}

MESSAGEHANDLER(Screenshot)
{
	// TODO: allow non-big screenshots too
	WriteBigScreenshot("bmp", msg->tiles);
}

QUERYHANDLER(CinemaRecord)
{
	CCinemaManager* manager = g_Game->GetView()->GetCinema();
	manager->SetCurrentTrack(*msg->track, false, false, false);

	const int w = 640, h = 480;
	{
		g_Renderer.Resize(w, h);
		SViewPort vp = { 0, 0, w, h };
		g_Game->GetView()->GetCamera()->SetViewPort(&vp);
		g_Game->GetView()->GetCamera()->SetProjection(CGameView::defaultNear, CGameView::defaultFar, CGameView::defaultFOV);
	}

	unsigned char* img = new unsigned char [w*h*3];

	int num_frames = msg->framerate * msg->duration;

	for (int frame = 0; frame < num_frames; ++frame)
	{
		manager->MoveToPointAbsolute((float)frame/msg->framerate);
		Render();
		Atlas_GLSwapBuffers((void*)g_GameLoop->glContext);
		glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, img);

		// Swap the rows around, else the image will be upside down
		char temp[w*3];
		for (int y = 0; y < h/2; ++y)
		{
			memcpy2(temp, &img[y*w*3], w*3);
			memcpy2(&img[y*w*3], &img[(h-1-y)*w*3], w*3);
			memcpy2(&img[(h-1-y)*w*3], temp, w*3);
		}

		// Call the user-supplied function with this data, so they can
		// store it as a video
		sCinemaRecordCB cbdata = { img };
		msg->cb.Call(cbdata);
	}

	delete[] img;

	// Restore viewport
	{
		g_Renderer.Resize(g_xres, g_yres);
		SViewPort vp = { 0, 0, g_xres, g_yres };
		g_Game->GetView()->GetCamera()->SetViewPort(&vp);
		g_Game->GetView()->GetCamera()->SetProjection(CGameView::defaultNear, CGameView::defaultFar, CGameView::defaultFOV);
	}

}

QUERYHANDLER(Ping)
{
	UNUSED2(msg);
}

}
