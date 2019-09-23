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

#include "gui/CGUIColor.h"
#include "gui/CGUISprite.h"
#include "ps/CStrIntern.h"
#include "ps/Shapes.h"

#include <array>
#include <list>
#include <vector>

class CGUI;
class CGUIString;
struct SGenerateTextImage;
using SGenerateTextImages = std::array<std::vector<SGenerateTextImage>, 2>;

/**
 * An CGUIText object is a parsed string, divided into
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
class CGUIText
{
public:
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
			m_pSpriteCall(nullptr) {}

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
		 * *IF* an icon, then this is not nullptr.
		 */
		std::list<SSpriteCall>::pointer m_pSpriteCall;
	};

	// The SSpriteCall CGUISpriteInstance makes this uncopyable to avoid invalidating its draw cache.
	// Also take advantage of exchanging the containers directly with move semantics.
	NONCOPYABLE(CGUIText);
	MOVABLE(CGUIText);

	/**
	 * Generates empty text.
	 */
	CGUIText() = default;

	/**
	 * Generate a CGUIText object from the inputted string.
	 * The function will break down the string and its
	 * tags to calculate exactly which rendering queries
	 * will be sent to the Renderer. Also, horizontal alignment
	 * is taken into acount in this method but NOT vertical alignment.
	 *
	 * @param Text Text to generate CGUIText object from
	 * @param Font Default font, notice both Default color and default font
	 *		  can be changed by tags.
	 * @param Width Width, 0 if no word-wrapping.
	 * @param BufferZone space between text and edge, and space between text and images.
	 * @param pObject Optional parameter for error output. Used *only* if error parsing fails,
	 *		  and we need to be able to output which object the error occurred in to aid the user.
	 */
	CGUIText(const CGUI& pGUI, const CGUIString& string, const CStrW& FontW, const float Width, const float BufferZone, const IGUIObject* pObject = nullptr);

	/**
	 * Draw this CGUIText object
	 */
	void Draw(CGUI& pGUI, const CGUIColor& DefaultColor, const CPos& pos, const float z, const CRect& clipping) const;

	const CSize& GetSize() const { return m_Size; }

	const std::list<SSpriteCall>& GetSpriteCalls() const { return m_SpriteCalls; }

	const std::vector<STextCall>& GetTextCalls() const { return m_TextCalls; }

	// Helper functions of the constructor
	bool ProcessLine(
		const CGUI& pGUI,
		const CGUIString& string,
		const CStrIntern& Font,
		const IGUIObject* pObject,
		const SGenerateTextImages& Images,
		const EAlign align,
		const float prelim_line_height,
		const float Width,
		const float BufferZone,
		bool& FirstLine,
		float& x,
		float& y,
		int& i,
		int& from);

	void SetupSpriteCalls(
		const CGUI& pGUI,
		const std::array<std::vector<CStr>, 2>& FeedbackImages,
		const float y,
		const float Width,
		const float BufferZone,
		const int i,
		const int pos_last_img,
		SGenerateTextImages& Images);

	float GetLineOffset(
		const EAlign align,
		const float width_range_from,
		const float width_range_to,
		const CSize& line_size) const;

	void ComputeLineRange(
		const SGenerateTextImages& Images,
		const float y,
		const float Width,
		const float prelim_line_height,
		float& width_range_from,
		float& width_range_to) const;

	void ComputeLineSize(
		const CGUI& pGUI,
		const CGUIString& string,
		const CStrIntern& Font,
		const bool FirstLine,
		const float Width,
		const float width_range_to,
		const int i,
		const int temp_from,
		float& x,
		CSize& line_size) const;

	bool AssembleCalls(
		const CGUI& pGUI,
		const CGUIString& string,
		const CStrIntern& Font,
		const IGUIObject* pObject,
		const bool FirstLine,
		const float Width,
		const float width_range_to,
		const float dx,
		const float y,
		const int temp_from,
		const int i,
		float& x,
		int& from);

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

struct SGenerateTextImage
{
	// The image's starting location in Y
	float m_YFrom;

	// The image's end location in Y
	float m_YTo;

	// The image width in other words
	float m_Indentation;

	void SetupSpriteCall(
		const bool Left, CGUIText::SSpriteCall& SpriteCall, const float width, const float y,
		const CSize& Size, const CStr& TextureName, const float BufferZone, const int CellID);
};

#endif // INCLUDED_GUITEXT
