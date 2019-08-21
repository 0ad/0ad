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

#ifndef INCLUDED_GUITOOLTIP
#define INCLUDED_GUITOOLTIP

class IGUIObject;
class CGUI;

#include "ps/CStr.h"
#include "ps/Shapes.h"

class GUITooltip
{
public:
	NONCOPYABLE(GUITooltip);

	GUITooltip();
	void Update(IGUIObject* Nearest, const CPos& MousePos, CGUI& GUI);

private:

	void ShowTooltip(IGUIObject* obj, const CPos& pos, const CStr& style, CGUI& pGUI);
	void HideTooltip(const CStr& style, CGUI& pGUI);
	bool GetTooltip(IGUIObject* obj, CStr& style);

	int m_State;

	IGUIObject* m_PreviousObject;
	CStr m_PreviousTooltipName;
	CPos m_PreviousMousePos;
	double m_Time;
	bool m_IsIconTooltip;
};

#endif // INCLUDED_GUITOOLTIP
