/* Copyright (C) 2022 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef INCLUDED_GUITEXT
#define INCLUDED_GUITEXT

#include "gui/CGUISprite.h"
#include "gui/SettingTypes/CGUIColor.h"
#include "gui/SettingTypes/EAlign.h"
#include "maths/Rect.h"
#include "maths/Size2D.h"
#include "maths/Vector2D.h"
#include "ps/CStrIntern.h"

#include <array>
#include <list>
#include <vector>

class CCanvas2D;
class CGUI;
class CGUIString;
class IGUIObject;
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

		SSpriteCall() {}

		/**
		 * Size and position of sprite
		 */
		CRect m_Area;

		/**
		 * Sprite from global GUI sprite database.
		 */
		CGUISpriteInstance m_Sprite;
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
		CVector2D m_Pos;

		/**
		 * Size
		 */
		CSize2D m_Size;

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
		 * Tooltip text
		 */
		CStrW m_Tooltip;

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
	 * @param string Text to generate CGUIText object from.
	 * @param font Default font, notice both Default color and default font
	 *		  can be changed by tags.
	 * @param width Width, 0 if no word-wrapping.
	 * @param bufferZone Space between text and edge, and space between text and images.
	 * @param align Horizontal alignment (left / center / right).
	 * @param pObject Optional parameter for error output. Used *only* if error parsing fails,
	 *		  and we need to be able to output which object the error occurred in to aid the user.
	 */
	CGUIText(const CGUI& pGUI, const CGUIString& string, const CStrW& fontW, const float width, const float bufferZone, const EAlign align, const IGUIObject* pObject);

	/**
	 * Draw this CGUIText object
	 */
	void Draw(CGUI& pGUI, CCanvas2D& canvas, const CGUIColor& DefaultColor, const CVector2D& pos, CRect clipping) const;

	const CSize2D& GetSize() const { return m_Size; }

	const std::list<SSpriteCall>& GetSpriteCalls() const { return m_SpriteCalls; }

	const std::vector<STextCall>& GetTextCalls() const { return m_TextCalls; }

	// Helper functions of the constructor
	bool ProcessLine(
		const CGUI& pGUI,
		const CGUIString& string,
		const CStrIntern& font,
		const IGUIObject* pObject,
		const SGenerateTextImages& images,
		const EAlign align,
		const float prelimLineHeight,
		const float width,
		const float bufferZone,
		bool& firstLine,
		float& y,
		int& i,
		int& from);

	void SetupSpriteCalls(
		const CGUI& pGUI,
		const std::array<std::vector<CStr>, 2>& feedbackImages,
		const float y,
		const float width,
		const float bufferZone,
		const int i,
		const int posLastImage,
		SGenerateTextImages& images);

	float GetLineOffset(
		const EAlign align,
		const float widthRangeFrom,
		const float widthRangeTo,
		const CSize2D& lineSize) const;

	void ComputeLineRange(
		const SGenerateTextImages& images,
		const float y,
		const float width,
		const float prelimLineHeight,
		float& widthRangeFrom,
		float& widthRangeTo) const;

	void ComputeLineSize(
		const CGUI& pGUI,
		const CGUIString& string,
		const CStrIntern& font,
		const bool firstLine,
		const float width,
		const float widthRangeFrom,
		const float widthRangeTo,
		const int i,
		const int tempFrom,
		CSize2D& lineSize) const;

	bool AssembleCalls(
		const CGUI& pGUI,
		const CGUIString& string,
		const CStrIntern& font,
		const IGUIObject* pObject,
		const bool firstLine,
		const float width,
		const float widthRangeTo,
		const float dx,
		const float y,
		const int tempFrom,
		const int i,
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
	CSize2D m_Size;
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
		const bool left, CGUIText::SSpriteCall& spriteCall, const float width, const float y,
		const CSize2D& size, const CStr& textureName, const float bufferZone);
};

#endif // INCLUDED_GUITEXT
