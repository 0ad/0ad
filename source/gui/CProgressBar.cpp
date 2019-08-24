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

#include "GUI.h"
#include "CProgressBar.h"

#include "lib/ogl.h"

CProgressBar::CProgressBar(CGUI& pGUI)
	: IGUIObject(pGUI)
{
	AddSetting<CGUISpriteInstance>("sprite_background");
	AddSetting<CGUISpriteInstance>("sprite_bar");
	AddSetting<float>("caption"); // aka value from 0 to 100
	AddSetting<CStrW>("tooltip");
	AddSetting<CStr>("tooltip_style");
}

CProgressBar::~CProgressBar()
{
}

void CProgressBar::HandleMessage(SGUIMessage& Message)
{
	// Important
	IGUIObject::HandleMessage(Message);

	switch (Message.type)
	{
	case GUIM_SETTINGS_UPDATED:
		// Update scroll-bar
		// TODO Gee: (2004-09-01) Is this really updated each time it should?
		if (Message.value == CStr("caption"))
		{
			const float value = GUI<float>::GetSetting(this, "caption");
			if (value > 100.f)
				GUI<float>::SetSetting(this, "caption", 100.f);
			else if (value < 0.f)
				GUI<float>::SetSetting(this, "caption", 0.f);
		}
		break;
	default:
		break;
	}
}

void CProgressBar::Draw()
{
	CGUISpriteInstance& sprite_bar = GUI<CGUISpriteInstance>::GetSetting(this, "sprite_bar");
	CGUISpriteInstance& sprite_background = GUI<CGUISpriteInstance>::GetSetting(this, "sprite_background");

	float bz = GetBufferedZ();

	int cell_id = 0;
	const float value = GUI<float>::GetSetting(this, "caption");

	m_pGUI.DrawSprite(sprite_background, cell_id, bz, m_CachedActualSize);

	// Get size of bar (notice it is drawn slightly closer, to appear above the background)
	CRect bar_size(m_CachedActualSize.left, m_CachedActualSize.top,
				   m_CachedActualSize.left+m_CachedActualSize.GetWidth()*(value/100.f), m_CachedActualSize.bottom);
	m_pGUI.DrawSprite(sprite_bar, cell_id, bz+0.01f, bar_size);
}
