/* Copyright (C) 2011 Wildfire Games.
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

#ifndef INCLUDED_ICMPAIINTERFACE
#define INCLUDED_ICMPAIINTERFACE

#include "simulation2/system/Interface.h"

class ICmpAIInterface : public IComponent
{
public:
	/**
	 * Returns a script object that represents the current world state,
	 * to be passed to AI scripts.
	 */
	virtual void GetRepresentation(JS::MutableHandleValue ret) = 0;
	/**
	 * Returns a script object that represents the current world state,
	 * to be passed to AI scripts. No caching for initialization
	 */
	virtual void GetFullRepresentation(JS::MutableHandleValue ret, bool flushEvents) = 0;

	DECLARE_INTERFACE_TYPE(AIInterface)
};

#endif // INCLUDED_ICMPAIINTERFACE
