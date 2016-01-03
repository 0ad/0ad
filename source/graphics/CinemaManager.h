/* Copyright (C) 2015 Wildfire Games.
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

#include <list>
#include <map>

#include "lib/input.h" // InReaction - can't forward-declare enum

#include "graphics/CinemaPath.h"
#include "ps/CStr.h"

/*
	desc: contains various functions used for cinematic camera paths
	See also: CinemaHandler.cpp, CinemaPath.cpp
*/

// Cinematic structure for data accessable from the simulation
struct CinematicSimulationData
{
	bool m_Enabled;
	bool m_Paused;
	std::map<CStrW, CCinemaPath> m_Paths;
	std::list<CCinemaPath> m_PathQueue;

	// States before playing
	bool m_MapRevealed;

	fixed m_ElapsedTime, m_TotalTime, m_CurrentPathElapsedTime;

	CinematicSimulationData()
		:	m_Enabled(false),
			m_Paused(true),
			m_MapRevealed(false),
			m_ElapsedTime(fixed::Zero()),
			m_TotalTime(fixed::Zero()),
			m_CurrentPathElapsedTime(fixed::Zero())
	{}
};

/**
 * Class for in game playing of cinematics. Should only be instantiated in CGameView. 
 */

class CCinemaManager
{
public:
	CCinemaManager();
	~CCinemaManager() {}

	/**
	 * Adds the path to the path list
	 * @param name path name
	 * @param CCinemaPath path data
	 */
	void AddPath(const CStrW& name, const CCinemaPath& path);

	/**
	 * Adds the path to the playlist 
	 * @param name path name 
	 */
	void AddPathToQueue(const CStrW& name);

	/**
	 * Clears the playlist
	 */
	void ClearQueue();

	/**
	 * Checks the path name in the path list
	 * @param name path name
	 * @return true if path with that name exists, else false
	 */
	bool HasPath(const CStrW& name) const;
	
	const std::map<CStrW, CCinemaPath>& GetAllPaths();
	void SetAllPaths( const std::map<CStrW, CCinemaPath>& tracks);

	/**
	 * Starts play paths
	 */
	void Play();
	void Stop();
	bool IsPlaying() const;

	/**
	 * Renders black bars and paths (if enabled)
	 */
	void Render();
	void DrawBars() const;

	/**
	 * Get current enable state of the cinema manager
	 */
	bool GetEnabled() const;

	/**
	 * Sets enable state of the cinema manager (shows/hide gui, show/hide rings, etc)
	 * @enable new state
	 */
	void SetEnabled(bool enabled);

	/**
	 * Updates CCinemManager and current path
	 * @param deltaRealTime Elapsed real time since the last frame.
	 */
	void Update(const float deltaRealTime);

	InReaction HandleEvent(const SDL_Event_* ev);

	CinematicSimulationData* GetCinematicSimulationData();

	bool GetPathsDrawing() const;
	void SetPathsDrawing(const bool drawPath);

private:    
	bool m_DrawPaths;

	// Cinematic data is accessed from the simulation
	CinematicSimulationData m_CinematicSimulationData;
};

extern InReaction cinema_manager_handler(const SDL_Event_* ev);

#endif
