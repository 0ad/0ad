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

#include "precompiled.h"

#include "CImage.h"

#include "GUI.h"

#include "lib/ogl.h"

CImage::CImage(CGUI* pGUI)
	: IGUIObject(pGUI)
{
	AddSetting<CGUISpriteInstance>("sprite");
	AddSetting<int>("cell_id");
	AddSetting<CStrW>("tooltip");
	AddSetting<CStr>("tooltip_style");
}

CImage::~CImage()
{
}

void CImage::Draw()
{
	if (!GetGUI())
		return;

	float bz = GetBufferedZ();

	CGUISpriteInstance* sprite;
	int cell_id;
	GUI<CGUISpriteInstance>::GetSettingPointer(this, "sprite", sprite);
	GUI<int>::GetSetting(this, "cell_id", cell_id);

	GetGUI()->DrawSprite(*sprite, cell_id, bz, m_CachedActualSize);
}
