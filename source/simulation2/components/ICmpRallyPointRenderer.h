/* Copyright (C) 2014 Wildfire Games.
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

#ifndef INCLUDED_ICMPRALLYPOINT
#define INCLUDED_ICMPRALLYPOINT

#include "maths/FixedVector2D.h"
#include "simulation2/helpers/Position.h"
#include "simulation2/system/Interface.h"

/**
 * Rally Point.
 * Holds the position of a unit's rally points, and renders them to screen.
 */
class ICmpRallyPointRenderer : public IComponent
{
public:

	/// Sets whether the rally point marker and line should be displayed.
	virtual void SetDisplayed(bool displayed) = 0;

	/// Sets the position at which the rally point marker should be displayed.
	/// Discards all previous positions
	virtual void SetPosition(CFixedVector2D position) = 0;

	/// Updates the position of one given rally point marker.
	virtual void UpdatePosition(u32 rallyPointId, CFixedVector2D position) = 0;

	/// Add another position at which a marker should be displayed, connected
	/// to the previous one.
	virtual void AddPosition_wrapper(CFixedVector2D position) = 0;

	/// Reset the positions of this rally point marker
	virtual void Reset() = 0;

	/// Returns true if at least one display rally point is set
	virtual bool IsSet() = 0;

	DECLARE_INTERFACE_TYPE(RallyPointRenderer)
};

#endif // INCLUDED_ICMPRALLYPOINT
