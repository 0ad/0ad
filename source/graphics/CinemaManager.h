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

#ifndef INCLUDED_CINEMAMANAGER
#define INCLUDED_CINEMAMANAGER

#include "lib/input.h" // InReaction - can't forward-declare enum
#include "graphics/Color.h"
#include "ps/CStr.h"
#include "simulation2/helpers/CinemaPath.h"


/**
 * Class for in game playing of cinematics. Should only be instantiated in CGameView.
 */
class CCinemaManager
{
public:
	CCinemaManager();
	~CCinemaManager() {}

	/**
	 * Renders black bars and paths (if enabled)
	 */
	void Render() const;
	void DrawBars() const;
	void DrawPaths() const;
	void DrawSpline(const RNSpline& spline, const CColor& splineColor, int smoothness, bool lines) const;
	void DrawNodes(const RNSpline& spline, const CColor& nodesColor) const;

	bool IsPlaying() const;
	bool IsEnabled() const;

	/**
	 * Updates CCinemManager and current path
	 * @param deltaRealTime Elapsed real time since the last frame.
	 */
	void Update(const float deltaRealTime) const;

	bool GetPathsDrawing() const;
	void SetPathsDrawing(const bool drawPath);

private:
	bool m_DrawPaths;
};

#endif
