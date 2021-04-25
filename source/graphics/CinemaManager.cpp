/* Copyright (C) 2021 Wildfire Games.
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
#include "renderer/DebugRenderer.h"
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
	if (!IsEnabled() && m_DrawPaths)
		DrawPaths();
}

void CCinemaManager::DrawPaths() const
{
	CmpPtr<ICmpCinemaManager> cmpCinemaManager(g_Game->GetSimulation2()->GetSimContext().GetSystemEntity());
	if (!cmpCinemaManager)
		return;

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);

	for (const std::pair<const CStrW, CCinemaPath>& p : cmpCinemaManager->GetPaths())
	{
		DrawSpline(p.second, CColor(0.2f, 0.2f, 1.f, 0.9f), 128);
		DrawNodes(p.second, CColor(0.1f, 1.f, 0.f, 1.f));

		if (p.second.GetTargetSpline().GetAllNodes().empty())
			continue;

		DrawSpline(p.second.GetTargetSpline(), CColor(1.f, 0.3f, 0.4f, 0.9f), 128);
		DrawNodes(p.second.GetTargetSpline(), CColor(1.f, 0.1f, 0.f, 1.f));
	}

	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
}

void CCinemaManager::DrawSpline(const RNSpline& spline, const CColor& splineColor, int smoothness) const
{
	if (spline.GetAllNodes().size() < 2)
		return;
	if (spline.GetAllNodes().size() == 2)
		smoothness = 2;

	const float start = spline.MaxDistance.ToFloat() / smoothness;

	std::vector<CVector3D> line;
	for (int i = 0; i <= smoothness; ++i)
	{
		const float time = start * i / spline.MaxDistance.ToFloat();
		line.emplace_back(spline.GetPosition(time));
	}
	g_Renderer.GetDebugRenderer().DrawLine(line, splineColor, 0.2f);

	// Height indicator
	if (g_Game && g_Game->GetWorld() && g_Game->GetWorld()->GetTerrain())
	{
		for (int i = 0; i <= smoothness; ++i)
		{
			const float time = start * i / spline.MaxDistance.ToFloat();
			const CVector3D tmp = spline.GetPosition(time);
			const float groundY = g_Game->GetWorld()->GetTerrain()->GetExactGroundLevel(tmp.X, tmp.Z);
			g_Renderer.GetDebugRenderer().DrawLine(tmp, CVector3D(tmp.X, groundY, tmp.Z), splineColor, 0.1f);
		}
	}
}

void CCinemaManager::DrawNodes(const RNSpline& spline, const CColor& nodeColor) const
{
	for (const SplineData& node : spline.GetAllNodes())
	{
		g_Renderer.GetDebugRenderer().DrawCircle(
			CVector3D(node.Position.X.ToFloat(), node.Position.Y.ToFloat(), node.Position.Z.ToFloat()),
			0.5f, nodeColor);
	}
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
