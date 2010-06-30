/* Copyright (C) 2010 Wildfire Games.
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

#ifndef INCLUDED_ICMPPLAYER
#define INCLUDED_ICMPPLAYER

#include "simulation2/system/Interface.h"

struct CColor;

/**
 * Player data.
 * (This interface only includes the functions needed by native code for loading maps,
 * and for minimap rendering; most player interaction is handled by scripts instead.)
 */
class ICmpPlayer : public IComponent
{
public:
	virtual void SetName(const std::wstring& name) = 0;
	virtual void SetCiv(const std::wstring& civcode) = 0;
	virtual void SetColour(u8 r, u8 g, u8 b) = 0;

	virtual CColor GetColour() = 0;

	DECLARE_INTERFACE_TYPE(Player)
};

#endif // INCLUDED_ICMPPLAYER
