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

#ifndef INCLUDED_ICMPRANGEOVERLAYRENDERER
#define INCLUDED_ICMPRANGEOVERLAYRENDERER

#include "ps/CStrIntern.h"
#include "simulation2/system/Interface.h"

class ICmpRangeOverlayRenderer : public IComponent
{
public:
	/**
	 * Add a range overlay to this entity, for example for an aura or attack.
	 */
	virtual void AddRangeOverlay(float radius, const std::string& texture, const std::string& textureMask, float thickness) = 0;

	/**
	 * Delete all range overlays.
	 */
	virtual void ResetRangeOverlays() = 0;

	DECLARE_INTERFACE_TYPE(RangeOverlayRenderer)
};

#endif // INCLUDED_ICMPRANGEOVERLAYRENDERER
