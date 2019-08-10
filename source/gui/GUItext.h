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

#ifndef INCLUDED_GUITEXT
#define INCLUDED_GUITEXT

#include <list>

#include "gui/CGUIColor.h"
#include "gui/CGUISprite.h"
#include "ps/CStrIntern.h"
#include "ps/Shapes.h"

/**
 * An SGUIText object is a parsed string, divided into
 * text-rendering components. Each component, being a
 * call to the Renderer. For instance, if you by tags
 * change the color, then the GUI will have to make
 * individual calls saying it want that color on the
 * text.
 *
 * For instance:
 * "Hello [b]there[/b] bunny!"
 *
 * That without word-wrapping would mean 3 components.
 * i.e. 3 calls to CRenderer. One drawing "Hello",
 * one drawing "there" in bold, and one drawing "bunny!".
 */
struct SGUIText
{
	/**
	 * A sprite call to the CRenderer
	 */
	struct SSpriteCall
	{
		// The CGUISpriteInstance makes this uncopyable to avoid invalidating its draw cache
		NONCOPYABLE(SSpriteCall);
		MOVABLE(SSpriteCall);

		SSpriteCall() : m_CellID(0) {}

		/**
		 * Size and position of sprite
		 */
		CRect m_Area;

		/**
		 * Sprite from global GUI sprite database.
		 */
		CGUISpriteInstance m_Sprite;

		int m_CellID;

		/**
		 * Tooltip text
		 */
		CStrW m_Tooltip;

		/**
		 * Tooltip style
		 */
		CStrW m_TooltipStyle;
	};

	/**
	 * A text call to the CRenderer
	 */
	struct STextCall
	{
		NONCOPYABLE(STextCall);
		MOVABLE(STextCall);

		STextCall() :
			m_UseCustomColor(false),
			m_Bold(false), m_Italic(false), m_Underlined(false),
			m_pSpriteCall(NULL) {}

		/**
		 * Position
		 */
		CPos m_Pos;

		/**
		 * Size
		 */
		CSize m_Size;

		/**
		 * The string that is suppose to be rendered.
		 */
		CStrW m_String;

		/**
		 * Use custom color? If true then m_Color is used,
		 * else the color inputted will be used.
		 */
		bool m_UseCustomColor;

		/**
		 * Color setup
		 */
		CGUIColor m_Color;

		/**
		 * Font name
		 */
		CStrIntern m_Font;

		/**
		 * Settings
		 */
		bool m_Bold, m_Italic, m_Underlined;

		/**
		 * *IF* an icon, then this is not NULL.
		 */
		std::list<SSpriteCall>::pointer m_pSpriteCall;
	};

	/**
	 * List of TextCalls, for instance "Hello", "there!"
	 */
	std::vector<STextCall> m_TextCalls;

	/**
	 * List of sprites, or "icons" that should be rendered
	 * along with the text.
	 */
	std::list<SSpriteCall> m_SpriteCalls; // list for consistent mem addresses
										  // so that we can point to elements.

	/**
	 * Width and height of the whole output, used when setting up
	 * scrollbars and such.
	 */
	CSize m_Size;
};

#endif // INCLUDED_GUITEXT
