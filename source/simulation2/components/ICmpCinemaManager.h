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

#ifndef INCLUDED_ICMPCINEMAMANAGER
#define INCLUDED_ICMPCINEMAMANAGER

#include "graphics/CinemaPath.h"
#include "simulation2/system/Interface.h"

#include "ps/CStr.h"

/**
 * Component for CCinemaManager class
 * TODO: write description
 */

class ICmpCinemaManager : public IComponent
{
public:
	/**
	* Adds the path to the path list
	* @param name path name
	* @param CCinemaPath path data
	*/
	virtual void AddPath(const CStrW& name, const CCinemaPath& path) = 0;

	/**
	* Adds the path to the playlist
	* @param name path name
	*/
	virtual void AddCinemaPathToQueue(const CStrW& name) = 0;

	virtual void Play() = 0;
	virtual void Stop() = 0;
	virtual void PlayQueue(const float deltaRealTime, CCamera* camera) = 0;

	/**
	* Checks the path name in the path list
	* @param name path name
	* @return true if path with that name exists, else false
	*/
	virtual bool HasPath(const CStrW& name) const = 0;

	/**
	* Clears the playlist
	*/
	virtual void ClearQueue() = 0;

	virtual const std::map<CStrW, CCinemaPath>& GetPaths() const = 0;
	virtual void SetPaths(const std::map<CStrW, CCinemaPath>& newPaths) = 0;
	virtual const std::list<CCinemaPath>& GetQueue() const = 0;

	virtual bool IsEnabled() const = 0;

	/**
	* Sets enable state of the cinema manager (shows/hide gui, show/hide rings, etc)
	* @param enable new state
	*/
	virtual void SetEnabled(bool enabled) = 0;

	DECLARE_INTERFACE_TYPE(CinemaManager)
};

#endif // INCLUDED_ICMPCINEMAMANAGER
