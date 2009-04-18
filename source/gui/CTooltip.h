/* Copyright (C) 2009 Wildfire Games.
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

/*
GUI Object - Tooltip

--Overview--

Mostly like CText, but intended for dynamic tooltips

*/

#ifndef INCLUDED_CTOOLTIP
#define INCLUDED_CTOOLTIP

#include "IGUITextOwner.h"

class CTooltip : public IGUITextOwner
{
	GUI_OBJECT(CTooltip)

public:
	CTooltip();
	virtual ~CTooltip();

protected:
	void SetupText();

	virtual void HandleMessage(const SGUIMessage &Message);

	virtual void Draw();
};

#endif
