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

#include "graphics/CinemaPath.h"
#include "ps/CStr.h"

//Class for in game playing of cinematics. Should only be instantiated in CGameView. 
class CCinemaManager
{
public:
	CCinemaManager();
	~CCinemaManager() {}

	void AddPath(CCinemaPath path, const CStrW& name);

	//Adds track to list of being played. 
	void QueuePath(const CStrW& name, bool queue);
	void OverridePath(const CStrW& name);	//clears track queue and replaces with 'name'

	/**
	 * @param deltaRealTime Elapsed real time since the last frame.
	 */
	bool Update(const float deltaRealTime);
	
	//These stop track play, and accept time, not ratio of time
	void MoveToPointAt(float time);

	inline void StopPlaying() { m_PathQueue.clear(); }
	void DrawSpline() const;
	
	inline bool IsPlaying() const { return !m_PathQueue.empty(); }
	bool HasTrack(const CStrW& name) const; 
	inline bool IsActive() const { return m_Active; }
	inline void SetActive(bool active) { m_Active=active; }

	inline const std::map<CStrW, CCinemaPath>& GetAllPaths() { return m_Paths; }
	void SetAllPaths( const std::map<CStrW, CCinemaPath>& tracks);
	void SetCurrentPath(const CStrW& name, bool current, bool lines);

private:
	
	bool m_Active, m_DrawCurrentSpline, m_DrawLines, m_ValidCurrent;
	std::map<CStrW, CCinemaPath>::iterator m_CurrentPath;
	std::map<CStrW, CCinemaPath> m_Paths;
	std::list<CCinemaPath> m_PathQueue;
};

#endif
