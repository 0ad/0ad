/*
GUI text
*/

#include "precompiled.h"

#include "GUI.h"
#include "ps/CLogger.h"
#include "ps/Parser.h"
#include <algorithm>

#define LOG_CATEGORY "gui"

#include "ps/Font.h"


static const wchar_t TagStart = '[';
static const wchar_t TagEnd   = ']';

void CGUIString::SFeedback::Reset()
{
	m_Images[Left].clear();
	m_Images[Right].clear();
	m_TextCalls.clear();
	m_SpriteCalls.clear();
	m_Size = CSize();
	m_NewLine=false;
}

void CGUIString::GenerateTextCall(SFeedback &Feedback,
								  const CStr& DefaultFont,
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
		// - GL - Temp
		TextChunk tc = *itTextChunk;
		// -- GL

		// Get the area that is overlapped by both the TextChunk and
		//  by the from/to inputted.
		int _from, _to;
		_from = std::max(from, itTextChunk->m_From);
		_to = std::min(to, itTextChunk->m_To);
		
		// If from is larger than to, than they are not overlapping
		if (_to == _from && itTextChunk->m_From == itTextChunk->m_To)
		{
			// These should never be able to have more than one tag.
			debug_assert(itTextChunk->m_Tags.size()==1);

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
				if (g_GUI.IconExists(itTextChunk->m_Tags[0].m_TagValue))
				{
					Feedback.m_Images[SFeedback::Left].push_back(itTextChunk->m_Tags[0].m_TagValue);
				}
				else if (pObject)
				{
					LOG(CLogger::Error, LOG_CATEGORY, "Trying to use an [imgleft]-tag with an undefined icon (\"%s\").", itTextChunk->m_Tags[0].m_TagValue.c_str());
				}
			}
			else
			if (itTextChunk->m_Tags[0].m_TagType == CGUIString::TextChunk::Tag::TAG_IMGRIGHT)
			{
				// Only add the image if the icon exists.
				if (g_GUI.IconExists(itTextChunk->m_Tags[0].m_TagValue))
				{
					Feedback.m_Images[SFeedback::Right].push_back(itTextChunk->m_Tags[0].m_TagValue);
				}
				else if (pObject)
				{
					LOG(CLogger::Error, LOG_CATEGORY, "Trying to use an [imgright]-tag with an undefined icon (\"%s\").", itTextChunk->m_Tags[0].m_TagValue.c_str());
				}
			}
			else
			if (itTextChunk->m_Tags[0].m_TagType == CGUIString::TextChunk::Tag::TAG_ICON)
			{
				// Only add the image if the icon exists.
				if (g_GUI.IconExists(itTextChunk->m_Tags[0].m_TagValue))
				{
					// We'll need to setup a text-call that will point
					//  to the icon, this is to be able to iterate
					//  through the text-calls without having to
					//  complex the structure virtually for nothing more.
					SGUIText::STextCall TextCall;

					// Also add it to the sprites being rendered.
					SGUIText::SSpriteCall SpriteCall;

					// Get Icon from icon database in g_GUI
					SGUIIcon icon = g_GUI.GetIcon(itTextChunk->m_Tags[0].m_TagValue);

					CSize size = icon.m_Size;

					// append width, and make maximum height the height.
					Feedback.m_Size.cx += size.cx;
					Feedback.m_Size.cy = std::max(Feedback.m_Size.cy, size.cy);

					// These are also needed later
					TextCall.m_Size = size;
					SpriteCall.m_Area = size;

					// Now displace the sprite if the additional value in the tag
					//  exists
					if (itTextChunk->m_Tags[0].m_TagAdditionalValue != std::string())
					{
						CSize displacement;
						// Parse the value
						if (!GUI<CSize>::ParseString(itTextChunk->m_Tags[0].m_TagAdditionalValue, displacement))
							LOG(CLogger::Error, LOG_CATEGORY, "Error parsing 'displace' value for tag [ICON]");
						else
							SpriteCall.m_Area += displacement;
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
					LOG(CLogger::Error, LOG_CATEGORY, "Trying to use an [icon]-tag with an undefined icon (\"%s\").", itTextChunk->m_Tags[0].m_TagValue.c_str());
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
					if (!GUI<CColor>::ParseString(it2->m_TagValue, TextCall.m_Color))
					{
						if (pObject)
							LOG(CLogger::Error, LOG_CATEGORY, "Error parsing the value of a [color]-tag in GUI text when reading object \"%s\".", pObject->GetPresentableName().c_str());
					}
				}
				else
				if (it2->m_TagType == CGUIString::TextChunk::Tag::TAG_FONT)
				{
					// TODO Gee: (2004-08-15) Check if Font exists?
					TextCall.m_Font = it2->m_TagValue;
				}
			}

			// Calculate the size of the font
			CSize size;
			int cx, cy;
			CFont font (TextCall.m_Font);
			font.CalculateStringSize(TextCall.m_String, cx, cy);
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

	if (_tagtype == CStr("B"))
	{
		m_TagType = TAG_B;
		return true;
	}
	else
	if (_tagtype == CStr("I"))
	{
		m_TagType = TAG_I;
		return true;
	}
	else
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
				CStr tagstr (str.substr(curpos+1, pos_right-curpos-1));

				CParserLine Line;
				Line.ParseString(Parser, (const char*)tagstr);

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

							// Check if an additional parameter was specified
							if (Line.GetArgCount() == 4)
							{
								std::string str;
								Line.GetArgString(2, str);

								if (tag.m_TagType == TextChunk::Tag::TAG_ICON && 
									str == "displace")
								{
									Line.GetArgString(3, tag.m_TagAdditionalValue);
								}
								else
								{
									LOG(CLogger::Warning, LOG_CATEGORY, "Trying to declare an additional attribute ('%s') in a [%s]-tag, which the tag isn't accepting", str.c_str(), Str_TagType.c_str());
								}
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
	// We'll sort later.
	m_Words.push_back(0);
	m_Words.push_back((int)m_RawString.length());

	// Space: ' '
	for (position=0, curpos=0;;position = curpos+1)
	{
		// Find the next word-delimiter.
		long dl = m_RawString.Find(position, ' ');

		if (dl == -1)
			break;

		curpos = dl;
		m_Words.push_back((int)dl+1);
	}

	// Dash: '-'
	for (position=0, curpos=0;;position = curpos+1)
	{
		// Find the next word-delimiter.
		long dl = m_RawString.Find(position, '-');

		if (dl == -1)
			break;

		curpos = dl;
		m_Words.push_back((int)dl+1);
	}

	// New Line: '\n'
	for (position=0, curpos=0;;position = curpos+1)
	{
		// Find the next word-delimiter.
		long dl = m_RawString.Find(position, '\n');

		if (dl == -1)
			break;

		curpos = dl;

		// Add before and 
		m_Words.push_back((int)dl);
		m_Words.push_back((int)dl+1);
	}

	sort(m_Words.begin(), m_Words.end());

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
		LOG(CLogger::Normal,  LOG_CATEGORY, "m_Words[%d] = %d", i, m_Words[i]);
	}

	for (int i=0; i<(int)m_TextChunks.size(); ++i)
	{
		LOG(CLogger::Normal,  LOG_CATEGORY, "m_TextChunk[%d] = [%d,%d]", i, m_TextChunks[i].m_From, m_TextChunks[i].m_To);
		for (int j=0; j<(int)m_TextChunks[i].m_Tags.size(); ++j)
		{
			LOG(CLogger::Normal,  LOG_CATEGORY, "--Tag: %d \"%s\"", (int)m_TextChunks[i].m_Tags[j].m_TagType, m_TextChunks[i].m_Tags[j].m_TagValue.c_str());
		}
	}
#endif
}
