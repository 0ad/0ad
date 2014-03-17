/* Copyright (C) 2014 Wildfire Games.
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
GUI text
*/

#include "precompiled.h"

#include "GUI.h"
#include "graphics/FontMetrics.h"
#include "ps/CLogger.h"
#include "ps/Parser.h"
#include <algorithm>


static const wchar_t TagStart = '[';
static const wchar_t TagEnd   = ']';
// List of word demlimitor bounds
// The list contains ranges of word delimitors. The odd indexed chars are the start
// of a range, the even are the end of a range. The list must be sorted in INCREASING ORDER
static const int NUM_WORD_DELIMITORS = 4*2;
static const u16 WordDelimitors[NUM_WORD_DELIMITORS] = {
	' '   , ' ',    // spaces
	'-'   , '-',    // hyphens
	0x3000, 0x3002, // ideographic symbols (maybe this range should be wider)
	0x4E00, 0x9FFF  // characters in the Chinese unicode block
// TODO add unicode blocks of other languages that don't use spaces
};

void CGUIString::SFeedback::Reset()
{
	m_Images[Left].clear();
	m_Images[Right].clear();
	m_TextCalls.clear();
	m_SpriteCalls.clear();
	m_Size = CSize();
	m_NewLine=false;
}

void CGUIString::GenerateTextCall(const CGUI *pGUI,
								  SFeedback &Feedback,
								  CStrIntern DefaultFont,
								  const int &from, const int &to,
								  const bool FirstLine,
								  const IGUIObject *pObject) const
{
	// Reset width and height, because they will be determined with incrementation
	//  or comparisons.
	Feedback.Reset();

	// Check out which text chunk this is within.
	//bool match_found = false;
	std::vector<TextChunk>::const_iterator itTextChunk;
	for (itTextChunk=m_TextChunks.begin(); itTextChunk!=m_TextChunks.end(); ++itTextChunk)
	{
		// Get the area that is overlapped by both the TextChunk and
		//  by the from/to inputted.
		int _from, _to;
		_from = std::max(from, itTextChunk->m_From);
		_to = std::min(to, itTextChunk->m_To);

		// If from is larger than to, than they are not overlapping
		if (_to == _from && itTextChunk->m_From == itTextChunk->m_To)
		{
			// These should never be able to have more than one tag.
			ENSURE(itTextChunk->m_Tags.size()==1);

			// Now do second check
			//  because icons and images are placed on exactly one position
			//  in the words-list, it can be counted twice if placed on an
			//  edge. But there is always only one logical preference that
			//  we want. This check filters the unwanted.

			// it's in the end of one word, and the icon
			//  should really belong to the beginning of the next one
			if (_to == to && to >= 1)
			{
				if (GetRawString()[to-1] == ' ' ||
					GetRawString()[to-1] == '-' ||
					GetRawString()[to-1] == '\n')
					continue;
			}
			// This std::string is just a break
			if (_from == from && from >= 1)
			{
				if (GetRawString()[from] == '\n' &&
					GetRawString()[from-1] != '\n' &&
					GetRawString()[from-1] != ' ' &&
					GetRawString()[from-1] != '-')
					continue;
			}

			// Single tags
			if (itTextChunk->m_Tags[0].m_TagType == CGUIString::TextChunk::Tag::TAG_IMGLEFT)
			{
				// Only add the image if the icon exists.
				if (pGUI->IconExists(itTextChunk->m_Tags[0].m_TagValue))
				{
					Feedback.m_Images[SFeedback::Left].push_back(itTextChunk->m_Tags[0].m_TagValue);
				}
				else if (pObject)
				{
					LOGERROR(L"Trying to use an [imgleft]-tag with an undefined icon (\"%hs\").", itTextChunk->m_Tags[0].m_TagValue.c_str());
				}
			}
			else
			if (itTextChunk->m_Tags[0].m_TagType == CGUIString::TextChunk::Tag::TAG_IMGRIGHT)
			{
				// Only add the image if the icon exists.
				if (pGUI->IconExists(itTextChunk->m_Tags[0].m_TagValue))
				{
					Feedback.m_Images[SFeedback::Right].push_back(itTextChunk->m_Tags[0].m_TagValue);
				}
				else if (pObject)
				{
					LOGERROR(L"Trying to use an [imgright]-tag with an undefined icon (\"%hs\").", itTextChunk->m_Tags[0].m_TagValue.c_str());
				}
			}
			else
			if (itTextChunk->m_Tags[0].m_TagType == CGUIString::TextChunk::Tag::TAG_ICON)
			{
				// Only add the image if the icon exists.
				if (pGUI->IconExists(itTextChunk->m_Tags[0].m_TagValue))
				{
					// We'll need to setup a text-call that will point
					//  to the icon, this is to be able to iterate
					//  through the text-calls without having to
					//  complex the structure virtually for nothing more.
					SGUIText::STextCall TextCall;

					// Also add it to the sprites being rendered.
					SGUIText::SSpriteCall SpriteCall;

					// Get Icon from icon database in pGUI
					SGUIIcon icon = pGUI->GetIcon(itTextChunk->m_Tags[0].m_TagValue);

					CSize size = icon.m_Size;

					// append width, and make maximum height the height.
					Feedback.m_Size.cx += size.cx;
					Feedback.m_Size.cy = std::max(Feedback.m_Size.cy, size.cy);

					// These are also needed later
					TextCall.m_Size = size;
					SpriteCall.m_Area = size;

					// Handle additional attributes
					std::vector<TextChunk::Tag::TagAttribute>::const_iterator att_it;
					for(att_it = itTextChunk->m_Tags[0].m_TagAttributes.begin(); att_it != itTextChunk->m_Tags[0].m_TagAttributes.end(); ++att_it)
					{
						TextChunk::Tag::TagAttribute tagAttrib = (TextChunk::Tag::TagAttribute)(*att_it);

						if (tagAttrib.attrib == "displace" && !tagAttrib.value.empty())
						{	//Displace the sprite
							CSize displacement;
							// Parse the value
							if (!GUI<CSize>::ParseString(CStr(tagAttrib.value).FromUTF8(), displacement))
								LOGERROR(L"Error parsing 'displace' value for tag [ICON]");
							else
								SpriteCall.m_Area += displacement;

						}
						else if(tagAttrib.attrib == "tooltip")
						{
							SpriteCall.m_Tooltip = CStr(tagAttrib.value).FromUTF8();
						}
						else if(tagAttrib.attrib == "tooltip_style")
						{
							SpriteCall.m_TooltipStyle = CStr(tagAttrib.value).FromUTF8();
						}
					}

					SpriteCall.m_Sprite = icon.m_SpriteName;
					SpriteCall.m_CellID = icon.m_CellID;

					// Add sprite call
					Feedback.m_SpriteCalls.push_back(SpriteCall);

					// Finalize text call
					TextCall.m_pSpriteCall = &Feedback.m_SpriteCalls.back();

					// Add text call
					Feedback.m_TextCalls.push_back(TextCall);
				}
				else if (pObject)
				{
					LOGERROR(L"Trying to use an [icon]-tag with an undefined icon (\"%hs\").", itTextChunk->m_Tags[0].m_TagValue.c_str());
				}
			}
		}
		else
		if (_to > _from && !Feedback.m_NewLine)
		{
			SGUIText::STextCall TextCall;

			// Set defaults
			TextCall.m_Font = DefaultFont;
			TextCall.m_UseCustomColor = false;

			// Extract substd::string from RawString.
			TextCall.m_String = GetRawString().substr(_from, _to-_from);

			// Go through tags and apply changes.
			std::vector<CGUIString::TextChunk::Tag>::const_iterator it2;
			for (it2 = itTextChunk->m_Tags.begin(); it2 != itTextChunk->m_Tags.end(); ++it2)
			{
				if (it2->m_TagType == CGUIString::TextChunk::Tag::TAG_COLOR)
				{
					// Set custom color
					TextCall.m_UseCustomColor = true;

					// Try parsing the color std::string
					if (!GUI<CColor>::ParseString(CStr(it2->m_TagValue).FromUTF8(), TextCall.m_Color))
					{
						if (pObject)
							LOGERROR(L"Error parsing the value of a [color]-tag in GUI text when reading object \"%hs\".", pObject->GetPresentableName().c_str());
					}
				}
				else
				if (it2->m_TagType == CGUIString::TextChunk::Tag::TAG_FONT)
				{
					// TODO Gee: (2004-08-15) Check if Font exists?
					TextCall.m_Font = CStrIntern(it2->m_TagValue);
				}
			}

			// Calculate the size of the font
			CSize size;
			int cx, cy;
			CFontMetrics font (TextCall.m_Font);
			font.CalculateStringSize(TextCall.m_String.c_str(), cx, cy);
			// For anything other than the first line, the line spacing
			// needs to be considered rather than just the height of the text
			if (! FirstLine)
				cy = font.GetLineSpacing();

			size.cx = (float)cx;
			size.cy = (float)cy;

			// Append width, and make maximum height the height.
			Feedback.m_Size.cx += size.cx;
			Feedback.m_Size.cy = std::max(Feedback.m_Size.cy, size.cy);

			// These are also needed later
			TextCall.m_Size = size;

			if (! TextCall.m_String.empty())
			{
				if (TextCall.m_String[0] == '\n')
				{
					Feedback.m_NewLine = true;
				}
			}

			// Add text-chunk
			Feedback.m_TextCalls.push_back(TextCall);
		}
	}
}

bool CGUIString::TextChunk::Tag::SetTagType(const CStr& tagtype)
{
	CStr _tagtype = tagtype.UpperCase();

	if (_tagtype == CStr("COLOR"))
	{
		m_TagType = TAG_COLOR;
		return true;
	}
	else
	if (_tagtype == CStr("FONT"))
	{
		m_TagType = TAG_FONT;
		return true;
	}
	else
	if (_tagtype == CStr("ICON"))
	{
		m_TagType = TAG_ICON;
		return true;
	}
	else
	if (_tagtype == CStr("IMGLEFT"))
	{
		m_TagType = TAG_IMGLEFT;
		return true;
	}
	else
	if (_tagtype == CStr("IMGRIGHT"))
	{
		m_TagType = TAG_IMGRIGHT;
		return true;
	}

	return false;
}

void CGUIString::SetValue(const CStrW& str)
{
	m_OriginalString = str;

	// clear
	m_TextChunks.clear();
	m_Words.clear();
	m_RawString = CStrW();

	// Setup parser
	// TODO Gee: (2004-08-16) Create and store this parser object somewhere to save loading time.
	// TODO PT: Extended CParserCache so that the above is possible (since it currently only
	// likes one-task parsers)
	CParser Parser;
	// I've added the option of an additional parameter. Only used for icons when writing this.
	Parser.InputTaskType("start", "$ident[_=_$value_[$ident_=_$value_]]");
	Parser.InputTaskType("end", "/$ident");

	long position = 0;
	long from=0;		// the position in the raw std::string where the last tag ended
	long from_nonraw=0;	// like from only in position of the REAL std::string, with tags.
	long curpos = 0;

	// Current Text Chunk
	CGUIString::TextChunk CurrentTextChunk;

	for (;;position = curpos+1)
	{
		// Find next TagStart character
		curpos = str.Find(position, TagStart);

		if (curpos == -1)
		{
			m_RawString += str.substr(position);

			if (from != (long)m_RawString.length())
			{
				CurrentTextChunk.m_From = from;
				CurrentTextChunk.m_To = (int)m_RawString.length();
				m_TextChunks.push_back(CurrentTextChunk);
			}

			break;
		}
		else
		{
			// First check if there is another TagStart before a TagEnd,
			//  in that case it's just a regular TagStart and we can continue.
			long pos_left = str.Find(curpos+1, TagStart);
			long pos_right = str.Find(curpos+1, TagEnd);

			if (pos_right == -1)
			{
				m_RawString += str.substr(position, curpos-position+1);
				continue;
			}
			else
			if (pos_left != -1 && pos_left < pos_right)
			{
				m_RawString += str.substr(position, pos_left-position);
				continue;
			}
			else
			{
				m_RawString += str.substr(position, curpos-position);

				// Okay we've found a TagStart and TagEnd, positioned
				//  at pos and pos_right. Now let's extract the
				//  interior and try parsing.
				CStrW tagstr (str.substr(curpos+1, pos_right-curpos-1));

				CParserLine Line;
				Line.ParseString(Parser, tagstr.ToUTF8());

				// Set to true if the tag is just text.
				bool justtext = false;

				if (Line.m_ParseOK)
				{
					if (Line.m_TaskTypeName == "start")
					{
 						// The tag
						TextChunk::Tag tag;
						std::string Str_TagType;

						Line.GetArgString(0, Str_TagType);

						if (!tag.SetTagType(Str_TagType))
						{
							justtext = true;
						}
						else
						{
							// Check for possible value-std::strings
							if (Line.GetArgCount() >= 2)
								Line.GetArgString(1, tag.m_TagValue);

							//Handle arbitrary number of additional parameters
							size_t argn;
							for(argn = 2; argn < Line.GetArgCount(); argn += 2)
							{
								TextChunk::Tag::TagAttribute a;

								Line.GetArgString(argn, a.attrib);
								Line.GetArgString(argn+1, a.value);

								tag.m_TagAttributes.push_back(a);
							}

							// Finalize last
							if (curpos != from_nonraw)
							{
								CurrentTextChunk.m_From = from;
								CurrentTextChunk.m_To = from + curpos - from_nonraw;
								m_TextChunks.push_back(CurrentTextChunk);
								from = CurrentTextChunk.m_To;
							}
							from_nonraw = pos_right+1;

							// Some tags does not have a closure, and should be
							//  stored without text. Like a <tag /> in XML.
							if (tag.m_TagType == TextChunk::Tag::TAG_IMGLEFT ||
								tag.m_TagType == TextChunk::Tag::TAG_IMGRIGHT ||
								tag.m_TagType == TextChunk::Tag::TAG_ICON)
							{
								// We need to use a fresh text chunk
								//  because 'tag' should be the *only* tag.
								TextChunk FreshTextChunk;

								// They does not work with the text system.
								FreshTextChunk.m_From = from + pos_right+1 - from_nonraw;
								FreshTextChunk.m_To = from + pos_right+1 - from_nonraw;

								FreshTextChunk.m_Tags.push_back(tag);

								m_TextChunks.push_back(FreshTextChunk);
							}
							else
							{
								// Add that tag, but first, erase previous occurences of the
								//  same tag.
								std::vector<TextChunk::Tag>::iterator it;
								for (it = CurrentTextChunk.m_Tags.begin(); it != CurrentTextChunk.m_Tags.end(); ++it)
								{
									if (it->m_TagType == tag.m_TagType)
									{
										CurrentTextChunk.m_Tags.erase(it);
										break;
									}
								}

								// Add!
								CurrentTextChunk.m_Tags.push_back(tag);
							}
						}
					}
					else
					if (Line.m_TaskTypeName == "end")
					{
						// The tag
						TextChunk::Tag tag;
						std::string Str_TagType;

						Line.GetArgString(0, Str_TagType);

						if (!tag.SetTagType(Str_TagType))
						{
							justtext = true;
						}
						else
						{
							// Finalize the previous chunk
							if (curpos != from_nonraw)
							{
								CurrentTextChunk.m_From = from;
								CurrentTextChunk.m_To = from + curpos - from_nonraw;
								m_TextChunks.push_back(CurrentTextChunk);
								from = CurrentTextChunk.m_To;
							}
							from_nonraw = pos_right+1;

							// Search for the tag, if it's not added, then
							//  pass it as plain text.
							std::vector<TextChunk::Tag>::iterator it;
							for (it = CurrentTextChunk.m_Tags.begin(); it != CurrentTextChunk.m_Tags.end(); ++it)
							{
								if (it->m_TagType == tag.m_TagType)
								{
									CurrentTextChunk.m_Tags.erase(it);
									break;
								}
							}
						}
					}
				}
				else justtext = true;

				if (justtext)
				{
					// What was within the tags could not be interpreted
					//  so we'll assume it's just text.
					m_RawString += str.substr(curpos, pos_right-curpos+1);
				}

				curpos = pos_right;

				continue;
			}
		}
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
		for (int n = 0; n < NUM_WORD_DELIMITORS; n += 2)
		{
			if (c <= WordDelimitors[n+1])
			{
				if (c >= WordDelimitors[n])
					m_Words.push_back((int)i+1);
				// assume the WordDelimitors list is stored in increasing order
				break;
			}
		}
	}

	m_Words.push_back((int)m_RawString.length());

	// Remove duplicates (only if larger than 2)
	if (m_Words.size() > 2)
	{
		std::vector<int>::iterator it;
		int last_word = -1;
		for (it = m_Words.begin(); it != m_Words.end(); )
		{
			if (last_word == *it)
			{
				it = m_Words.erase(it);
			}
			else
			{
				last_word = *it;
				++it;
			}
		}
	}

#if 0
	for (int i=0; i<(int)m_Words.size(); ++i)
	{
		LOGMESSAGE(L"m_Words[%d] = %d", i, m_Words[i]);
	}

	for (int i=0; i<(int)m_TextChunks.size(); ++i)
	{
		LOGMESSAGE(L"m_TextChunk[%d] = [%d,%d]", i, m_TextChunks[i].m_From, m_TextChunks[i].m_To);
		for (int j=0; j<(int)m_TextChunks[i].m_Tags.size(); ++j)
		{
			LOGMESSAGE(L"--Tag: %d \"%hs\"", (int)m_TextChunks[i].m_Tags[j].m_TagType, m_TextChunks[i].m_Tags[j].m_TagValue.c_str());
		}
	}
#endif
}
