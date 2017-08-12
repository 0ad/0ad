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

#include <sstream>
#include <string>

#include "graphics/CinemaManager.h"

#include "graphics/Camera.h"
#include "graphics/GameView.h"
#include "gui/CGUI.h"
#include "gui/GUIutil.h"
#include "gui/GUIManager.h"
#include "gui/IGUIObject.h"
#include "lib/ogl.h"
#include "maths/MathUtil.h"
#include "maths/Quaternion.h"
#include "maths/Vector3D.h"
#include "maths/Vector4D.h"
#include "ps/CLogger.h"
#include "ps/ConfigDB.h"
#include "ps/CStr.h"
#include "ps/Game.h"
#include "ps/GameSetup/Config.h"
#include "ps/Hotkey.h"
#include "ps/World.h"
#include "simulation2/components/ICmpCinemaManager.h"
#include "simulation2/components/ICmpOverlayRenderer.h"
#include "simulation2/components/ICmpRangeManager.h"
#include "simulation2/components/ICmpSelectable.h"
#include "simulation2/components/ICmpTerritoryManager.h"
#include "simulation2/MessageTypes.h"
#include "simulation2/system/ComponentManager.h"
#include "simulation2/Simulation2.h"
#include "renderer/Renderer.h"


CCinemaManager::CCinemaManager()
	: m_DrawPaths(false)
{
}

void CCinemaManager::Update(const float deltaRealTime) const
{
	CmpPtr<ICmpCinemaManager> cmpCinemaManager(g_Game->GetSimulation2()->GetSimContext().GetSystemEntity());
	if (!cmpCinemaManager)
		return;

	if (IsPlaying())
		cmpCinemaManager->PlayQueue(deltaRealTime, g_Game->GetView()->GetCamera());
}

void CCinemaManager::Render() const
{
	if (IsEnabled())
		DrawBars();
	else if (m_DrawPaths)
		DrawPaths();
}

void CCinemaManager::DrawPaths() const
{
	CmpPtr<ICmpCinemaManager> cmpCinemaManager(g_Game->GetSimulation2()->GetSimContext().GetSystemEntity());
	if (!cmpCinemaManager)
		return;

	for (const std::pair<CStrW, CCinemaPath>& p : cmpCinemaManager->GetPaths())
	{
		DrawSpline(p.second, CColor(0.2f, 0.2f, 1.f, 0.9f), 128, true);
		DrawNodes(p.second, CColor(0.1f, 1.f, 0.f, 1.f));

		if (p.second.GetTargetSpline().GetAllNodes().empty())
			continue;

		DrawSpline(p.second.GetTargetSpline(), CColor(1.f, 0.3f, 0.4f, 0.9f), 128, true);
		DrawNodes(p.second.GetTargetSpline(), CColor(1.f, 0.1f, 0.f, 1.f));
	}
}

void CCinemaManager::DrawSpline(const RNSpline& spline, const CColor& splineColor, int smoothness, bool lines) const
{
	if (spline.GetAllNodes().size() < 2)
		return;
	if (spline.GetAllNodes().size() == 2 && lines)
		smoothness = 2;

	float start = spline.MaxDistance.ToFloat() / smoothness;
	float time = 0;

#if CONFIG2_GLES
	#warning TODO : implement CCinemaPath on GLES
#else

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glColor4f(splineColor.r, splineColor.g, splineColor.b, splineColor.a);
	if (lines)
	{
		glLineWidth(1.8f);
		glEnable(GL_LINE_SMOOTH);
		glBegin(GL_LINE_STRIP);
		for (int i = 0; i <= smoothness; ++i)
		{
			time = start * i / spline.MaxDistance.ToFloat();
			CVector3D tmp = spline.GetPosition(time);
			glVertex3f(tmp.X, tmp.Y, tmp.Z);
		}
		glEnd();

		// Height indicator
		if (g_Game && g_Game->GetWorld() && g_Game->GetWorld()->GetTerrain())
		{
			glLineWidth(1.1f);
			glBegin(GL_LINES);
			for (int i = 0; i <= smoothness; ++i)
			{
				time = start * i / spline.MaxDistance.ToFloat();
				CVector3D tmp = spline.GetPosition(time);
				float groundY = g_Game->GetWorld()->GetTerrain()->GetExactGroundLevel(tmp.X, tmp.Z);
				glVertex3f(tmp.X, tmp.Y, tmp.Z);
				glVertex3f(tmp.X, groundY, tmp.Z);
			}
			glEnd();
		}

		glDisable(GL_LINE_SMOOTH);
		glLineWidth(1.0f);
	}
	else
	{
		smoothness /= 2;
		start = spline.MaxDistance.ToFloat() / smoothness;
		glEnable(GL_POINT_SMOOTH);
		glPointSize(3.0f);
		glBegin(GL_POINTS);
		for (int i = 0; i <= smoothness; ++i)
		{
			time = start * i / spline.MaxDistance.ToFloat();
			CVector3D tmp = spline.GetPosition(time);
			glVertex3f(tmp.X, tmp.Y, tmp.Z);
		}
		glEnd();
		glPointSize(1.0f);
		glDisable(GL_POINT_SMOOTH);
	}
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);

#endif
}

void CCinemaManager::DrawNodes(const RNSpline& spline, const CColor& nodeColor) const
{
#if CONFIG2_GLES
	#warning TODO : implement CCinemaPath on GLES
#else

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_POINT_SMOOTH);
	glPointSize(7.0f);
	glColor4f(nodeColor.r, nodeColor.g, nodeColor.b, nodeColor.a);
	glBegin(GL_POINTS);

	for (const SplineData& node : spline.GetAllNodes())
		glVertex3f(node.Position.X.ToFloat(), node.Position.Y.ToFloat(), node.Position.Z.ToFloat());

	glEnd();
	glPointSize(1.0f);
	glDisable(GL_POINT_SMOOTH);
	glEnable(GL_DEPTH_TEST);
#endif
}

void CCinemaManager::DrawBars() const
{
	int height = (float)g_xres / 2.39f;
	int shift = (g_yres - height) / 2;
	if (shift <= 0)
		return;

#if CONFIG2_GLES
	#warning TODO : implement bars for GLES
#else
	// Set up transform for GL bars
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	CMatrix3D transform;
	transform.SetOrtho(0.f, (float)g_xres, 0.f, (float)g_yres, -1.f, 1000.f);
	glLoadMatrixf(&transform._11);

	glColor4f(0.0f, 0.0f, 0.0f, 1.0f);

	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);

	glBegin(GL_QUADS);
	glVertex2i(0, 0);
	glVertex2i(g_xres, 0);
	glVertex2i(g_xres, shift);
	glVertex2i(0, shift);
	glEnd();

	glBegin(GL_QUADS);
	glVertex2i(0, g_yres - shift);
	glVertex2i(g_xres, g_yres - shift);
	glVertex2i(g_xres, g_yres);
	glVertex2i(0, g_yres);
	glEnd();

	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);

	// Restore transform
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
#endif
}

bool CCinemaManager::IsEnabled() const
{
	CmpPtr<ICmpCinemaManager> cmpCinemaManager(g_Game->GetSimulation2()->GetSimContext().GetSystemEntity());
	return cmpCinemaManager && cmpCinemaManager->IsEnabled();
}

bool CCinemaManager::IsPlaying() const
{
	return IsEnabled() && g_Game && !g_Game->m_Paused;
}

bool CCinemaManager::GetPathsDrawing() const
{
	return m_DrawPaths;
}

void CCinemaManager::SetPathsDrawing(const bool drawPath)
{
	m_DrawPaths = drawPath;
}
