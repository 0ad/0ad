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

#ifndef INCLUDED_CGUISTRING
#define INCLUDED_CGUISTRING

#include "gui/CGUIText.h"
#include "ps/CStrIntern.h"

#include <array>
#include <list>
#include <vector>

class CGUI;

/**
 * String class, substitute for CStr, but that parses
 * the tags and builds up a list of all text that will
 * be different when outputted.
 *
 * The difference between CGUIString and CGUIText is that
 * CGUIString is a string-class that parses the tags
 * when the value is set. The CGUIText is just a container
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
		// Avoid copying the vector and list containers.
		NONCOPYABLE(SFeedback);
		MOVABLE(SFeedback);
		SFeedback() = default;

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
		std::array<std::vector<CStr>, 2> m_Images; // left and right

		/**
		 * Text and Sprite Calls.
		 */
		std::vector<CGUIText::STextCall> m_TextCalls;

		// list for consistent mem addresses so that we can point to elements.
		std::list<CGUIText::SSpriteCall> m_SpriteCalls;

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
	 * @param pObject Only for Error outputting, optional! If nullptr
	 *		  then no Errors will be reported! Useful when you need
	 *		  to make several GenerateTextCall in different phases,
	 *		  it avoids duplicates.
	 */
	void GenerateTextCall(const CGUI& pGUI, SFeedback& Feedback, CStrIntern DefaultFont, const int& from, const int& to, const bool FirstLine, const IGUIObject* pObject = nullptr) const;

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

#endif // INCLUDED_CGUISTRING
