/* Copyright (C) 2017 Wildfire Games.
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

#include "View.h"

#include "ActorViewer.h"
#include "GameLoop.h"
#include "Messages.h"
#include "SimState.h"

#include "graphics/CinemaManager.h"
#include "graphics/GameView.h"
#include "graphics/ParticleManager.h"
#include "graphics/SColor.h"
#include "graphics/UnitManager.h"
#include "lib/timer.h"
#include "lib/utf8.h"
#include "maths/MathUtil.h"
#include "ps/Game.h"
#include "ps/GameSetup/GameSetup.h"
#include "ps/World.h"
#include "renderer/Renderer.h"
#include "simulation2/Simulation2.h"
#include "simulation2/components/ICmpObstructionManager.h"
#include "simulation2/components/ICmpParticleManager.h"
#include "simulation2/components/ICmpPathfinder.h"

extern void (*Atlas_GLSwapBuffers)(void* context);

extern int g_xres, g_yres;

//////////////////////////////////////////////////////////////////////////

void AtlasView::SetParam(const std::wstring& UNUSED(name), bool UNUSED(value))
{
}

void AtlasView::SetParam(const std::wstring& UNUSED(name), const AtlasMessage::Color& UNUSED(value))
{
}

void AtlasView::SetParam(const std::wstring& UNUSED(name), const std::wstring& UNUSED(value))
{
}

void AtlasView::SetParam(const std::wstring& UNUSED(name), int UNUSED(value))
{
}

//////////////////////////////////////////////////////////////////////////

AtlasViewActor::AtlasViewActor()
: m_SpeedMultiplier(1.f), m_ActorViewer(new ActorViewer())
{
}

AtlasViewActor::~AtlasViewActor()
{
	delete m_ActorViewer;
}

void AtlasViewActor::Update(float realFrameLength)
{
	m_ActorViewer->Update(realFrameLength * m_SpeedMultiplier, realFrameLength);
}

void AtlasViewActor::Render()
{
	SViewPort vp = { 0, 0, g_xres, g_yres };
	CCamera& camera = GetCamera();
	camera.SetViewPort(vp);
	camera.SetProjection(2.f, 512.f, DEGTORAD(20.f));
	camera.UpdateFrustum();

	m_ActorViewer->Render();
	Atlas_GLSwapBuffers((void*)g_AtlasGameLoop->glCanvas);
}

CCamera& AtlasViewActor::GetCamera()
{
	return m_Camera;
}

CSimulation2* AtlasViewActor::GetSimulation2()
{
	return m_ActorViewer->GetSimulation2();
}

entity_id_t AtlasViewActor::GetEntityId(AtlasMessage::ObjectID UNUSED(obj))
{
	return m_ActorViewer->GetEntity();
}

bool AtlasViewActor::WantsHighFramerate()
{
	if (m_SpeedMultiplier != 0.f)
		return true;

	return false;
}

void AtlasViewActor::SetEnabled(bool enabled)
{
	m_ActorViewer->SetEnabled(enabled);
}

void AtlasViewActor::SetSpeedMultiplier(float speedMultiplier)
{
	m_SpeedMultiplier = speedMultiplier;
}

ActorViewer& AtlasViewActor::GetActorViewer()
{
	return *m_ActorViewer;
}

void AtlasViewActor::SetParam(const std::wstring& name, bool value)
{
	if (name == L"wireframe")
		g_Renderer.SetModelRenderMode(value ? WIREFRAME : SOLID);
	else if (name == L"walk")
		m_ActorViewer->SetWalkEnabled(value);
	else if (name == L"ground")
		m_ActorViewer->SetGroundEnabled(value);
	// TODO: this causes corruption of WaterManager's global state
	//	which should be asociated with terrain or simulation instead
	//	see http://trac.wildfiregames.com/ticket/2692
	//else if (name == L"water")
		//m_ActorViewer->SetWaterEnabled(value);
	else if (name == L"shadows")
		m_ActorViewer->SetShadowsEnabled(value);
	else if (name == L"stats")
		m_ActorViewer->SetStatsEnabled(value);
	else if (name == L"bounding_box")
		m_ActorViewer->SetBoundingBoxesEnabled(value);
	else if (name == L"axes_marker")
		m_ActorViewer->SetAxesMarkerEnabled(value);
}

void AtlasViewActor::SetParam(const std::wstring& name, int value)
{
	if (name == L"prop_points")
		m_ActorViewer->SetPropPointsMode(value);
}

void AtlasViewActor::SetParam(const std::wstring& name, const AtlasMessage::Color& value)
{
	if (name == L"background")
	{
		m_ActorViewer->SetBackgroundColor(SColor4ub(value.r, value.g, value.b, 255));
	}
}


//////////////////////////////////////////////////////////////////////////

AtlasViewGame::AtlasViewGame()
	: m_SpeedMultiplier(0.f), m_IsTesting(false), m_DrawMoveTool(false)
{
	ENSURE(g_Game);
}

AtlasViewGame::~AtlasViewGame()
{
	for (const std::pair<std::wstring, SimState*>& p : m_SavedStates)
		delete p.second;
}

CSimulation2* AtlasViewGame::GetSimulation2()
{
	return g_Game->GetSimulation2();
}

void AtlasViewGame::Update(float realFrameLength)
{
	const float actualFrameLength = realFrameLength * m_SpeedMultiplier;

	// Clean up any entities destroyed during UI message processing
	g_Game->GetSimulation2()->FlushDestroyedEntities();

	if (m_SpeedMultiplier == 0.f)
	{
		// Update unit interpolation
		g_Game->Interpolate(0.0, realFrameLength);
	}
	else
	{
		// Update the whole world
		// (Tell the game update not to interpolate graphics - we'll do that
		// ourselves)
		g_Game->Update(actualFrameLength, false);

		// Interpolate the graphics - we only want to do this once per visual frame,
		// not in every call to g_Game->Update
		g_Game->Interpolate(actualFrameLength, realFrameLength);
	}

	// Cinematic motion should be independent of simulation update, so we can
	// preview the cinematics by themselves
	if (g_Game->GetView()->GetCinema()->IsPlaying())
		g_Game->GetView()->GetCinema()->Update(realFrameLength);
}

void AtlasViewGame::Render()
{
	SViewPort vp = { 0, 0, g_xres, g_yres };
	CCamera& camera = GetCamera();
	camera.SetViewPort(vp);
	camera.SetProjection(g_Game->GetView()->GetNear(), g_Game->GetView()->GetFar(), g_Game->GetView()->GetFOV());
	camera.UpdateFrustum();

	::Render();
	Atlas_GLSwapBuffers((void*)g_AtlasGameLoop->glCanvas);
}

void AtlasViewGame::DrawCinemaPathTool()
{
	if (!m_DrawMoveTool)
		return;

#if CONFIG2_GLES
	#warning TODO : implement Atlas cinema path tool for GLES
#else
	CVector3D focus = m_MoveTool;
	CVector3D camera = GetCamera().GetOrientation().GetTranslation();
	float scale = (focus - camera).Length() / 10.0;

	glDisable(GL_DEPTH_TEST);
	glLineWidth(1.6f);
	glEnable(GL_LINE_SMOOTH);

	glColor3f(1.0f, 0.0f, 0.0f);
	glBegin(GL_LINE_STRIP);
	glVertex3fv(focus.GetFloatArray());
	glVertex3fv((focus + CVector3D(scale, 0, 0)).GetFloatArray());
	glEnd();

	glColor3f(0.0f, 1.0f, 0.0f);
	glBegin(GL_LINE_STRIP);
	glVertex3fv(focus.GetFloatArray());
	glVertex3fv((focus + CVector3D(0, scale, 0)).GetFloatArray());
	glEnd();

	glColor3f(0.0f, 0.0f, 1.0f);
	glBegin(GL_LINE_STRIP);
	glVertex3fv(focus.GetFloatArray());
	glVertex3fv((focus + CVector3D(0, 0, scale)).GetFloatArray());
	glEnd();

	glDisable(GL_LINE_SMOOTH);
	glLineWidth(1.0f);
	glEnable(GL_DEPTH_TEST);
#endif
}

void AtlasViewGame::DrawOverlays()
{
#if CONFIG2_GLES
#warning TODO: implement Atlas game overlays for GLES
#else
	// Set up transform for overlays
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	CMatrix3D transform;
	transform.SetIdentity();
	transform.Scale(1.0f, -1.f, 1.0f);
	transform.Translate(0.0f, (float)g_yres, -1000.0f);
	CMatrix3D proj;
	proj.SetOrtho(0.f, (float)g_xres, 0.f, (float)g_yres, -1.f, 1000.f);
	transform = proj * transform;
	glLoadMatrixf(&transform._11);

	if (m_BandboxArray.size() > 0)
	{
		glEnableClientState(GL_COLOR_ARRAY);
		glEnableClientState(GL_VERTEX_ARRAY);

		// Render bandbox as array of lines
		glVertexPointer(2, GL_FLOAT, sizeof(SBandboxVertex), &m_BandboxArray[0].x);
		glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(SBandboxVertex), &m_BandboxArray[0].r);

		glDrawArrays(GL_LINES, 0, m_BandboxArray.size());

		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);
	}

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
#endif
}

void AtlasViewGame::SetParam(const std::wstring& name, bool value)
{
	if (name == L"priorities")
		g_Renderer.SetDisplayTerrainPriorities(value);
	else if (name == L"movetool")
		m_DrawMoveTool = value;
}

void AtlasViewGame::SetParam(const std::wstring& name, float value)
{
	if (name == L"movetool_x")
		m_MoveTool.X = value;
	else if (name == L"movetool_y")
		m_MoveTool.Y = value;
	else if (name == L"movetool_z")
		m_MoveTool.Z = value;
}

void AtlasViewGame::SetParam(const std::wstring& name, const std::wstring& value)
{
	if (name == L"passability")
	{
		m_DisplayPassability = CStrW(value).ToUTF8();

		CmpPtr<ICmpPathfinder> cmpPathfinder(*GetSimulation2(), SYSTEM_ENTITY);
		if (cmpPathfinder)
		{
			if (!value.empty())
				cmpPathfinder->SetAtlasOverlay(true, cmpPathfinder->GetPassabilityClass(m_DisplayPassability));
			else
				cmpPathfinder->SetAtlasOverlay(false);
		}
	}
	else if (name == L"renderpath")
	{
		g_Renderer.SetRenderPath(g_Renderer.GetRenderPathByName(CStrW(value).ToUTF8()));
	}
}

CCamera& AtlasViewGame::GetCamera()
{
	return *g_Game->GetView()->GetCamera();
}

bool AtlasViewGame::WantsHighFramerate()
{
	if (g_Game->GetView()->GetCinema()->IsPlaying())
		return true;

	if (m_SpeedMultiplier != 0.f)
		return true;

	return false;
}

void AtlasViewGame::SetSpeedMultiplier(float speed)
{
	m_SpeedMultiplier = speed;
}

void AtlasViewGame::SetTesting(bool testing)
{
	m_IsTesting = testing;
	// If we're testing, particles should freeze on pause (like in-game), otherwise they keep going
	CmpPtr<ICmpParticleManager> cmpParticleManager(*GetSimulation2(), SYSTEM_ENTITY);
	if (cmpParticleManager)
		cmpParticleManager->SetUseSimTime(m_IsTesting);
}

void AtlasViewGame::SaveState(const std::wstring& label)
{
	delete m_SavedStates[label]; // in case it already exists
	m_SavedStates[label] = SimState::Freeze();
}

void AtlasViewGame::RestoreState(const std::wstring& label)
{
	SimState* simState = m_SavedStates[label];
	if (! simState)
		return;

	simState->Thaw();
}

std::wstring AtlasViewGame::DumpState(bool binary)
{
	std::stringstream stream;
	if (binary)
	{
		if (! g_Game->GetSimulation2()->SerializeState(stream))
			return L"(internal error)";
		// We can't return raw binary data, because we want to handle it with wxJS which
		// doesn't like \0 bytes in strings, so return it as hex
		static const char digits[] = "0123456789abcdef";
		std::string str = stream.str();
		std::wstring ret;
		ret.reserve(str.length()*3);
		for (size_t i = 0; i < str.length(); ++i)
		{
			ret += digits[(unsigned char)str[i] >> 4];
			ret += digits[(unsigned char)str[i] & 0x0f];
			ret += ' ';
		}
		return ret;
	}
	else
	{
		if (! g_Game->GetSimulation2()->DumpDebugState(stream))
			return L"(internal error)";
		return wstring_from_utf8(stream.str());
	}
}

void AtlasViewGame::SetBandbox(bool visible, float x0, float y0, float x1, float y1)
{
	m_BandboxArray.clear();

	if (visible)
	{
		// Make sure corners are arranged in correct order
		if (x0 > x1)
			std::swap(x0, x1);
		if (y0 > y1)
			std::swap(y0, y1);

		// Bandbox is draw as lines comprising two rectangles
		SBandboxVertex vert[] = {
			// Black - outer rectangle
			SBandboxVertex(x0, y0, 0, 0, 0, 255), SBandboxVertex(x1, y0, 0, 0, 0, 255), SBandboxVertex(x1, y1, 0, 0, 0, 255), SBandboxVertex(x0, y1, 0, 0, 0, 255),
			// White - inner rectangle
			SBandboxVertex(x0+1.0f, y0+1.0f, 255, 255, 255, 255), SBandboxVertex(x1-1.0f, y0+1.0f, 255, 255, 255, 255), SBandboxVertex(x1-1.0f, y1-1.0f, 255, 255, 255, 255), SBandboxVertex(x0+1.0f, y1-1.0f, 255, 255, 255, 255)
		};

		for (size_t i = 0; i < 4; ++i)
		{
			m_BandboxArray.push_back(vert[i]);
			m_BandboxArray.push_back(vert[(i+1)%4]);
		}
		for (size_t i = 0; i < 4; ++i)
		{
			m_BandboxArray.push_back(vert[i+4]);
			m_BandboxArray.push_back(vert[(i+1)%4+4]);
		}
	}
}

//////////////////////////////////////////////////////////////////////////

AtlasViewNone* view_None = NULL;
AtlasViewGame* view_Game = NULL;
AtlasViewActor* view_Actor = NULL;

AtlasView::~AtlasView()
{
}

AtlasView* AtlasView::GetView(int /*eRenderView*/ view)
{
	switch (view)
	{
	case AtlasMessage::eRenderView::NONE:  return AtlasView::GetView_None();
	case AtlasMessage::eRenderView::GAME:  return AtlasView::GetView_Game();
	case AtlasMessage::eRenderView::ACTOR: return AtlasView::GetView_Actor();
	default:
		debug_warn(L"Invalid view type");
		return AtlasView::GetView_None();
	}
}

AtlasView* AtlasView::GetView_None()
{
	if (! view_None)
		view_None = new AtlasViewNone();
	return view_None;
}

AtlasViewGame* AtlasView::GetView_Game()
{
	if (! view_Game)
		view_Game = new AtlasViewGame();
	return view_Game;
}

AtlasViewActor* AtlasView::GetView_Actor()
{
	if (! view_Actor)
		view_Actor = new AtlasViewActor();
	return view_Actor;
}

void AtlasView::DestroyViews()
{
	delete view_None; view_None = NULL;
	delete view_Game; view_Game = NULL;
	delete view_Actor; view_Actor = NULL;
}
