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

#include <algorithm>

#include "gui/GUI.h"
#include "lib/utf8.h"
#include "graphics/FontMetrics.h"
#include "ps/CLogger.h"


// List of word delimiter bounds
// The list contains ranges of word delimiters. The odd indexed chars are the start
// of a range, the even are the end of a range. The list must be sorted in INCREASING ORDER
static const int NUM_WORD_DELIMITERS = 4*2;
static const u16 WordDelimiters[NUM_WORD_DELIMITERS] = {
	' '   , ' ',    // spaces
	'-'   , '-',    // hyphens
	0x3000, 0x31FF, // ideographic symbols
	0x3400, 0x9FFF
// TODO add unicode blocks of other languages that don't use spaces
};

void CGUIString::SFeedback::Reset()
{
	m_Images[Left].clear();
	m_Images[Right].clear();
	m_TextCalls.clear();
	m_SpriteCalls.clear();
	m_Size = CSize();
	m_NewLine = false;
}

void CGUIString::GenerateTextCall(const CGUI* pGUI, SFeedback& Feedback, CStrIntern DefaultFont, const int& from, const int& to, const bool FirstLine, const IGUIObject* pObject) const
{
	// Reset width and height, because they will be determined with incrementation
	//  or comparisons.
	Feedback.Reset();

	// Check out which text chunk this is within.
	for (const TextChunk& textChunk : m_TextChunks)
	{
		// Get the area that is overlapped by both the TextChunk and
		//  by the from/to inputted.
		int _from = std::max(from, textChunk.m_From);
		int _to = std::min(to, textChunk.m_To);

		// If from is larger than to, then they are not overlapping
		if (_to == _from && textChunk.m_From == textChunk.m_To)
		{
			// These should never be able to have more than one tag.
			ENSURE(textChunk.m_Tags.size() == 1);

			// Icons and images are placed on exactly one position
			//  in the words-list, and they can be counted twice if placed
			//  on an edge. But there is always only one logical preference
			//  that we want. This check filters the unwanted.

			// it's in the end of one word, and the icon
			//  should really belong to the beginning of the next one
			if (_to == to && to >= 1 && to < (int)m_RawString.length())
			{
				if (m_RawString[to-1] == ' ' ||
				    m_RawString[to-1] == '-' ||
				    m_RawString[to-1] == '\n')
					continue;
			}
			// This std::string is just a break
			if (_from == from && from >= 1)
			{
				if (m_RawString[from] == '\n' &&
				    m_RawString[from-1] != '\n' &&
				    m_RawString[from-1] != ' ' &&
				    m_RawString[from-1] != '-')
					continue;
			}

			const TextChunk::Tag& tag = textChunk.m_Tags[0];
			ENSURE(tag.m_TagType == TextChunk::Tag::TAG_IMGLEFT ||
			       tag.m_TagType == TextChunk::Tag::TAG_IMGRIGHT ||
			       tag.m_TagType == TextChunk::Tag::TAG_ICON);

			const std::string& path = utf8_from_wstring(tag.m_TagValue);
			if (!pGUI->HasIcon(path))
			{
				if (pObject)
					LOGERROR("Trying to use an icon, imgleft or imgright-tag with an undefined icon (\"%s\").", path.c_str());
				continue;
			}

			switch (tag.m_TagType)
			{
			case TextChunk::Tag::TAG_IMGLEFT:
				Feedback.m_Images[SFeedback::Left].push_back(path);
				break;
			case TextChunk::Tag::TAG_IMGRIGHT:
				Feedback.m_Images[SFeedback::Right].push_back(path);
				break;
			case TextChunk::Tag::TAG_ICON:
			{
				// We'll need to setup a text-call that will point
				//  to the icon, this is to be able to iterate
				//  through the text-calls without having to
				//  complex the structure virtually for nothing more.
				SGUIText::STextCall TextCall;

				// Also add it to the sprites being rendered.
				SGUIText::SSpriteCall SpriteCall;

				// Get Icon from icon database in pGUI
				const SGUIIcon& icon = pGUI->GetIcon(path);

				const CSize& size = icon.m_Size;

				// append width, and make maximum height the height.
				Feedback.m_Size.cx += size.cx;
				Feedback.m_Size.cy = std::max(Feedback.m_Size.cy, size.cy);

				// These are also needed later
				TextCall.m_Size = size;
				SpriteCall.m_Area = size;

				// Handle additional attributes
				for (const TextChunk::Tag::TagAttribute& tagAttrib : tag.m_TagAttributes)
				{
					if (tagAttrib.attrib == L"displace" && !tagAttrib.value.empty())
					{
						// Displace the sprite
						CSize displacement;
						// Parse the value
						if (!GUI<CSize>::ParseString(pGUI, tagAttrib.value, displacement))
							LOGERROR("Error parsing 'displace' value for tag [ICON]");
						else
							SpriteCall.m_Area += displacement;
					}
					else if (tagAttrib.attrib == L"tooltip")
						SpriteCall.m_Tooltip = tagAttrib.value;
					else if (tagAttrib.attrib == L"tooltip_style")
						SpriteCall.m_TooltipStyle = tagAttrib.value;
				}

				SpriteCall.m_Sprite = icon.m_SpriteName;
				SpriteCall.m_CellID = icon.m_CellID;

				// Add sprite call
				Feedback.m_SpriteCalls.push_back(std::move(SpriteCall));

				// Finalize text call
				TextCall.m_pSpriteCall = &Feedback.m_SpriteCalls.back();

				// Add text call
				Feedback.m_TextCalls.emplace_back(std::move(TextCall));

				break;
			}
			NODEFAULT;
			}
		}
		else if (_to > _from && !Feedback.m_NewLine)
		{
			SGUIText::STextCall TextCall;

			// Set defaults
			TextCall.m_Font = DefaultFont;
			TextCall.m_UseCustomColor = false;

			TextCall.m_String = m_RawString.substr(_from, _to-_from);

			// Go through tags and apply changes.
			for (const TextChunk::Tag& tag : textChunk.m_Tags)
			{
				switch (tag.m_TagType)
				{
				case TextChunk::Tag::TAG_COLOR:
					TextCall.m_UseCustomColor = true;

					if (!GUI<CGUIColor>::ParseString(pGUI, tag.m_TagValue, TextCall.m_Color) && pObject)
						LOGERROR("Error parsing the value of a [color]-tag in GUI text when reading object \"%s\".", pObject->GetPresentableName().c_str());
					break;
				case TextChunk::Tag::TAG_FONT:
					// TODO Gee: (2004-08-15) Check if Font exists?
					TextCall.m_Font = CStrIntern(utf8_from_wstring(tag.m_TagValue));
					break;
				default:
					LOGERROR("Encountered unexpected tag applied to text");
					break;
				}
			}

			// Calculate the size of the font
			CSize size;
			int cx, cy;
			CFontMetrics font (TextCall.m_Font);
			font.CalculateStringSize(TextCall.m_String.c_str(), cx, cy);
			// For anything other than the first line, the line spacing
			// needs to be considered rather than just the height of the text
			if (!FirstLine)
				cy = font.GetLineSpacing();

			size.cx = (float)cx;
			size.cy = (float)cy;

			// Append width, and make maximum height the height.
			Feedback.m_Size.cx += size.cx;
			Feedback.m_Size.cy = std::max(Feedback.m_Size.cy, size.cy);

			// These are also needed later
			TextCall.m_Size = size;

			if (!TextCall.m_String.empty() && TextCall.m_String[0] == '\n')
				Feedback.m_NewLine = true;

			// Add text-chunk
			Feedback.m_TextCalls.emplace_back(std::move(TextCall));
		}
	}
}

bool CGUIString::TextChunk::Tag::SetTagType(const CStrW& tagtype)
{
	TagType t = GetTagType(tagtype);
	if (t == TAG_INVALID)
		return false;

	m_TagType = t;
	return true;
}

CGUIString::TextChunk::Tag::TagType CGUIString::TextChunk::Tag::GetTagType(const CStrW& tagtype) const
{
	if (tagtype == L"color")
		return TAG_COLOR;
	if (tagtype == L"font")
		return TAG_FONT;
	if (tagtype == L"icon")
		return TAG_ICON;
	if (tagtype == L"imgleft")
		return TAG_IMGLEFT;
	if (tagtype == L"imgright")
		return TAG_IMGRIGHT;

	return TAG_INVALID;
}

void CGUIString::SetValue(const CStrW& str)
{
	m_OriginalString = str;

	m_TextChunks.clear();
	m_Words.clear();
	m_RawString.clear();

	// Current Text Chunk
	CGUIString::TextChunk CurrentTextChunk;
	CurrentTextChunk.m_From = 0;

	int l = str.length();
	int rawpos = 0;
	CStrW tag;
	std::vector<CStrW> tags;
	bool closing = false;
	for (int p = 0; p < l; ++p)
	{
		TextChunk::Tag tag_;
		switch (str[p])
		{
		case L'[':
			CurrentTextChunk.m_To = rawpos;
			// Add the current chunks if it is not empty
			if (CurrentTextChunk.m_From != rawpos)
				m_TextChunks.push_back(CurrentTextChunk);
			CurrentTextChunk.m_From = rawpos;

			closing = false;
			if (++p == l)
			{
				LOGERROR("Partial tag at end of string '%s'", utf8_from_wstring(str));
				break;
			}
			if (str[p] == L'/')
			{
				closing = true;
				if (tags.empty())
				{
					LOGERROR("Encountered closing tag without having any open tags. At %d in '%s'", p, utf8_from_wstring(str));
					break;
				}
				if (++p == l)
				{
					LOGERROR("Partial closing tag at end of string '%s'", utf8_from_wstring(str));
					break;
				}
			}
			tag.clear();
			// Parse tag
			for (; p < l && str[p] != L']'; ++p)
			{
				CStrW name, param;
				switch (str[p])
				{
				case L' ':
					if (closing) // We still parse them to make error handling cleaner
						LOGERROR("Closing tags do not support parameters (at pos %d '%s')", p, utf8_from_wstring(str));

					// parse something="something else"
					for (++p; p < l && str[p] != L'='; ++p)
						name.push_back(str[p]);

					if (p == l)
					{
						LOGERROR("Parameter without value at pos %d '%s'", p, utf8_from_wstring(str));
						break;
					}
					FALLTHROUGH;
				case L'=':
					// parse a quoted parameter
					if (closing) // We still parse them to make error handling cleaner
						LOGERROR("Closing tags do not support parameters (at pos %d '%s')", p, utf8_from_wstring(str));

					if (++p == l)
					{
						LOGERROR("Expected parameter, got end of string '%s'", utf8_from_wstring(str));
						break;
					}
					if (str[p] != L'"')
					{
						LOGERROR("Unquoted parameters are not supported (at pos %d '%s')", p, utf8_from_wstring(str));
						break;
					}
					for (++p; p < l && str[p] != L'"'; ++p)
					{
						switch (str[p])
						{
						case L'\\':
							if (++p == l)
							{
								LOGERROR("Escape character at end of string '%s'", utf8_from_wstring(str));
								break;
							}
							// NOTE: We do not support \n in tag parameters
							FALLTHROUGH;
						default:
							param.push_back(str[p]);
						}
					}

					if (!name.empty())
					{
						TextChunk::Tag::TagAttribute a = {name, param};
						tag_.m_TagAttributes.push_back(a);
					}
					else
						tag_.m_TagValue = param;
					break;
				default:
					tag.push_back(str[p]);
					break;
				}
			}

			if (!tag_.SetTagType(tag))
			{
				LOGERROR("Invalid tag '%s' at %d in '%s'", utf8_from_wstring(tag), p, utf8_from_wstring(str));
				break;
			}
			if (!closing)
			{
				if (tag_.m_TagType == TextChunk::Tag::TAG_IMGRIGHT
					|| tag_.m_TagType == TextChunk::Tag::TAG_IMGLEFT
					|| tag_.m_TagType == TextChunk::Tag::TAG_ICON)
				{
					TextChunk FreshTextChunk = { rawpos, rawpos };
					FreshTextChunk.m_Tags.push_back(tag_);
					m_TextChunks.push_back(FreshTextChunk);
				}
				else
				{
					tags.push_back(tag);
					CurrentTextChunk.m_Tags.push_back(tag_);
				}
			}
			else
			{
				if (tag != tags.back())
				{
					LOGERROR("Closing tag '%s' does not match last opened tag '%s' at %d in '%s'", utf8_from_wstring(tag), utf8_from_wstring(tags.back()), p, utf8_from_wstring(str));
					break;
				}

				tags.pop_back();
				CurrentTextChunk.m_Tags.pop_back();
			}
			break;
		case L'\\':
			if (++p == l)
			{
				LOGERROR("Escape character at end of string '%s'", utf8_from_wstring(str));
				break;
			}
			if (str[p] == L'n')
			{
				++rawpos;
				m_RawString.push_back(L'\n');
				break;
			}
			FALLTHROUGH;
		default:
			++rawpos;
			m_RawString.push_back(str[p]);
			break;
		}
	}

	// Add the chunk after the last tag
	if (CurrentTextChunk.m_From != rawpos)
	{
		CurrentTextChunk.m_To = rawpos;
		m_TextChunks.push_back(CurrentTextChunk);
	}


	// Add a delimiter at start and at end, it helps when
	//  processing later, because we don't have make exceptions for
	//  those cases.
	m_Words.push_back(0);

	// Add word boundaries in increasing order
	for (u32 i = 0; i < m_RawString.length(); ++i)
	{
		wchar_t c = m_RawString[i];
		if (c == '\n')
		{
			m_Words.push_back((int)i);
			m_Words.push_back((int)i+1);
			continue;
		}
		for (int n = 0; n < NUM_WORD_DELIMITERS; n += 2)
		{
			if (c <= WordDelimiters[n+1])
			{
				if (c >= WordDelimiters[n])
					m_Words.push_back((int)i+1);
				// assume the WordDelimiters list is stored in increasing order
				break;
			}
		}
	}

	m_Words.push_back((int)m_RawString.length());

	// Remove duplicates (only if larger than 2)
	if (m_Words.size() <= 2)
		return;

	m_Words.erase(std::unique(m_Words.begin(), m_Words.end()), m_Words.end());
}
