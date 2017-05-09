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

/*
 * GUI text, handles text stuff
 *
 * --Overview--
 * Mainly contains struct SGUIText and friends.
 * Actual text processing is made in CGUI::GenerateText()
 *
 * --More info--
 * Check GUI.h
 *
 */

#ifndef INCLUDED_GUITEXT
#define INCLUDED_GUITEXT

#include <list>

#include "CGUISprite.h"
#include "ps/CStrIntern.h"

class CGUI;

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
		CColor m_Color;

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

/**
 * String class, substitute for CStr, but that parses
 * the tags and builds up a list of all text that will
 * be different when outputted.
 *
 * The difference between CGUIString and SGUIText is that
 * CGUIString is a string-class that parses the tags
 * when the value is set. The SGUIText is just a container
 * which stores the positions and settings of all text-calls
 * that will have to be made to the Renderer.
 */
class CGUIString
{
public:
	/**
	 * A chunk of text that represents one call to the renderer.
	 * In other words, all text in one chunk, will be drawn
	 * exactly with the same settings.
	 */
	struct TextChunk
	{
		/**
		 * A tag looks like this "Hello [b]there[/b] little"
		 */
		struct Tag
		{
			/**
			 * Tag Type
			 */
			enum TagType
			{
				TAG_B,
				TAG_I,
				TAG_FONT,
				TAG_SIZE,
				TAG_COLOR,
				TAG_IMGLEFT,
				TAG_IMGRIGHT,
				TAG_ICON,
				TAG_INVALID
			};

			struct TagAttribute
			{
				std::wstring attrib;
				std::wstring value;
			};

			/**
			 * Set tag from string
			 *
			 * @param tagtype TagType by string, like 'img' for [img]
			 * @return True if m_TagType was set.
			 */
			bool SetTagType(const CStrW& tagtype);
			TagType GetTagType(const CStrW& tagtype) const;


			/**
			 * In [b="Hello"][/b]
			 * m_TagType is TAG_B
			 */
			TagType m_TagType;

			/**
			 * In [b="Hello"][/b]
			 * m_TagValue is 'Hello'
			 */
			std::wstring m_TagValue;

			/**
			 * Some tags need an additional attributes
			 */
			std::vector<TagAttribute> m_TagAttributes;
		};

		/**
		 * m_From and m_To is the range of the string
		 */
		int m_From, m_To;

		/**
		 * Tags that are present. [a][b]
		 */
		std::vector<Tag> m_Tags;
	};

	/**
	 * All data generated in GenerateTextCall()
	 */
	struct SFeedback
	{
		// Constants
		static const int Left = 0;
		static const int Right = 1;

		/**
		 * Reset all member data.
		 */
		void Reset();

		/**
		 * Image stacks, for left and right floating images.
		 */
		std::vector<CStr> m_Images[2]; // left and right

		/**
		 * Text and Sprite Calls.
		 */
		std::vector<SGUIText::STextCall> m_TextCalls;
		std::list<SGUIText::SSpriteCall> m_SpriteCalls; // list for consistent mem addresses
														//  so that we can point to elements.

		/**
		 * Width and Height *feedback*
		 */
		CSize m_Size;

		/**
		 * If the word inputted was a new line.
		 */
		bool m_NewLine;
	};

	/**
	 * Set the value, the string will automatically
	 * be parsed when set.
	 */
	void SetValue(const CStrW& str);

	/**
	 * Get String, with tags
	 */
	const CStrW& GetOriginalString() const { return m_OriginalString; }

	/**
	 * Get String, stripped of tags
	 */
	const CStrW& GetRawString() const { return m_RawString; }

	/**
	 * Generate Text Call from specified range. The range
	 * must span only within ONE TextChunk though. Otherwise
	 * it can't be fit into a single Text Call
	 *
	 * Notice it won't make it complete, you will have to add
	 * X/Y values and such.
	 *
	 * @param pGUI Pointer to CGUI object making this call, for e.g. icon retrieval.
	 * @param Feedback contains all info that is generated.
	 * @param DefaultFont Default Font
	 * @param from From character n,
	 * @param to to character n.
	 * @param FirstLine Whether this is the first line of text, to calculate its height correctly
	 * @param pObject Only for Error outputting, optional! If NULL
	 *		  then no Errors will be reported! Useful when you need
	 *		  to make several GenerateTextCall in different phases,
	 *		  it avoids duplicates.
	 */
	void GenerateTextCall(const CGUI* pGUI, SFeedback& Feedback, CStrIntern DefaultFont, const int& from, const int& to, const bool FirstLine, const IGUIObject* pObject = NULL) const;

	/**
	 * Words
	 */
	std::vector<int> m_Words;

private:
	/**
	 * TextChunks
	 */
	std::vector<TextChunk> m_TextChunks;

	/**
	 * The full raw string. Stripped of tags.
	 */
	CStrW m_RawString;

	/**
	 * The original string value passed to SetValue.
	 */
	CStrW m_OriginalString;
};

#endif // INCLUDED_GUITEXT
