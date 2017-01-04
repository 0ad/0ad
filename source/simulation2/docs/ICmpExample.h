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

#ifndef INCLUDED_ICMPEXAMPLE
#define INCLUDED_ICMPEXAMPLE

#include "simulation2/system/Interface.h"

// ...any other forward declarations and includes you might need...

/**
 * Documentation to describe what this interface and its associated component types are
 * for, and roughly how they should be used.
 */
class ICmpExample : public IComponent
{
public:
	/**
	 * Documentation for each method.
	 */
	virtual int DoWhatever(int x, int y) = 0;

	// ...

	DECLARE_INTERFACE_TYPE(Example)
};

#endif // INCLUDED_ICMPEXAMPLE
