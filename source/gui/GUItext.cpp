/*
GUI text
by Gustav Larsson
gee@pyro.nu
*/

#include "precompiled.h"

#include "GUI.h"
#include "Parser.h"
#include "OverlayText.h"
#include <algorithm>

// TODO Gee: Remove, just for temp-output
#include <fstream>

using namespace std;

static const TCHAR TagStart =	'[';
static const TCHAR TagEnd =		']';

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
								  const CStr& DefaultFont, /*const CColor &DefaultColor,*/
								  const int &from, const int &to) const
{
	// Reset width and height, because they will be determined with incrementation
	//  or comparisons.
	Feedback.Reset();

	// Check out which text chunk this is within.
	//bool match_found = false;
	vector<TextChunk>::const_iterator itTextChunk;
	for (itTextChunk=m_TextChunks.begin(); itTextChunk!=m_TextChunks.end(); ++itTextChunk)
	{
		// Get the area that is overlapped by both the TextChunk and
		//  by the from/to inputted.
		int _from, _to;
		_from = max(from, itTextChunk->m_From);
		_to = min(to, itTextChunk->m_To);
		
		// If from is larger than to, than they are not overlapping
		if (_to == _from && itTextChunk->m_From == itTextChunk->m_To && _from > from)
		{
			// These should never be able to have more than one tag.
			assert(itTextChunk->m_Tags.size()==1);

			// Single tags
			if (itTextChunk->m_Tags[0].m_TagType == CGUIString::TextChunk::Tag::TAG_IMGLEFT)
			{
				//if (_from > from)
					Feedback.m_Images[SFeedback::Left].push_back(itTextChunk->m_Tags[0].m_TagValue);
			}
			else
			if (itTextChunk->m_Tags[0].m_TagType == CGUIString::TextChunk::Tag::TAG_IMGRIGHT)
			{
				//if (_from > from)
					Feedback.m_Images[SFeedback::Right].push_back(itTextChunk->m_Tags[0].m_TagValue);
			}
			else
			if (itTextChunk->m_Tags[0].m_TagType == CGUIString::TextChunk::Tag::TAG_ICON)
			{
				//if (_from <= from)continue;;
				// We'll need to setup a text-call that will point
				//  to the icon, this is to be able to iterate
				//  through the text-calls without having to
				//  complex the structure ultimately for nothing more.
				SGUIText::STextCall TextCall;

				// Also add it to the sprites being rendered.
				SGUIText::SSpriteCall SpriteCall;

				CSize size(20,20);
				// Query size of icon
				// TODO Gee: Temp

				// append width, and make maximum height the height.
				Feedback.m_Size.cx += size.cx;
				Feedback.m_Size.cy = max(Feedback.m_Size.cy, size.cy);

				// These are also needed later
				TextCall.m_Size = size;
				SpriteCall.m_Area = size;

				SpriteCall.m_TextureName = CStr("scroll");

				// Add sprite call
				Feedback.m_SpriteCalls.push_back(SpriteCall);

				// Finalize text call
				TextCall.m_pSpriteCall = &Feedback.m_SpriteCalls.back();

				// Add text call
				Feedback.m_TextCalls.push_back(TextCall);
			}
		}
		else
		if (_to > _from && !Feedback.m_NewLine)
		{
			SGUIText::STextCall TextCall;

			// Set defaults
			TextCall.m_Font = DefaultFont;
			TextCall.m_UseCustomColor = false;

			// Extract substring from RawString.
			TextCall.m_String = GetRawString().GetSubstring(_from, _to-_from);
			
			// Go through tags and apply changes.
			vector<CGUIString::TextChunk::Tag>::const_iterator it2;
			for (it2 = itTextChunk->m_Tags.begin(); it2 != itTextChunk->m_Tags.end(); ++it2)
			{
				if (it2->m_TagType == CGUIString::TextChunk::Tag::TAG_COLOR)
				{
					// Set custom color
					TextCall.m_UseCustomColor = true;
					GUI<CColor>::ParseString(it2->m_TagValue, TextCall.m_Color);
				}
				else
				if (it2->m_TagType == CGUIString::TextChunk::Tag::TAG_FONT)
				{
					TextCall.m_Font = it2->m_TagValue;
				}
			}

			CSize size;

			COverlayText txt(0, 0, 0, TextCall.m_Font, TextCall.m_String, TextCall.m_Color);
  			// TODO Gee: Ask Rich to change to (size);
			txt.GetOutputStringSize((int&)size.cx, (int&)size.cy);

			// append width, and make maximum height the height.
			Feedback.m_Size.cx += size.cx;
			Feedback.m_Size.cy = max(Feedback.m_Size.cy, size.cy);

			// These are also needed later
			TextCall.m_Size = size;

			if (TextCall.m_String.Length() >= 1)
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

void CGUIString::SetValue(const CStr& str)
{
	// clear
	m_TextChunks.clear();
	m_Words.clear();
	m_RawString = CStr();

	// Setup parser
	CParser Parser;
	Parser.InputTaskType("start", "$ident[_=_$value]");
	Parser.InputTaskType("end", "/$ident");

	long position = 0;
	long from=0;			// the position in the raw string where the last tag ended
	long from_nonraw=0;	// like from only in position of the REAL string, with tags.
	long curpos = 0;

	// Current Text Chunk
	CGUIString::TextChunk CurrentTextChunk;

	for (;;position = curpos+1)
	{
		// Find next TagStart character
		curpos = str.Find(position, TagStart);

		if (curpos == -1)
		{
			m_RawString += str.GetSubstring(position, str.Length()-position);

			if (from != m_RawString.Length())
			{
				CurrentTextChunk.m_From = from;
				CurrentTextChunk.m_To = (int)m_RawString.Length();
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
				m_RawString += str.GetSubstring(position, curpos-position+1);
				continue;
			}
			else
			if (pos_left != -1 && pos_left < pos_right)
			{
				m_RawString += str.GetSubstring(position, pos_left-position);
				continue;
			}
			else
			{
				m_RawString += str.GetSubstring(position, curpos-position);

				// Okay we've found a TagStart and TagEnd, positioned
				//  at pos and pos_right. Now let's extract the
				//  interior and try parsing.
				CStr tagstr = str.GetSubstring(curpos+1, pos_right-curpos-1);

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
						CStr Str_TagType;

						Line.GetArgString(0, (std::string &)Str_TagType);

						if (!tag.SetTagType(Str_TagType))
						{
							justtext = true;
						}
						else
						{
							// Check for possible value-strings
							if (Line.GetArgCount() == 2)
								Line.GetArgString(1, (std::string &)tag.m_TagValue);

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
								vector<TextChunk::Tag>::iterator it;
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
						CStr Str_TagType;

						Line.GetArgString(0, (std::string &)Str_TagType);

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
							vector<TextChunk::Tag>::iterator it;
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
					m_RawString += str.GetSubstring(curpos, pos_right-curpos+1);
				}

				curpos = pos_right;

				continue;
			}
		}
	}

#if 1

	ofstream fout("output1.txt");

	for (int i=0; i<(int)m_TextChunks.size(); ++i)	
	{
		fout << "{\"";
		fout << m_TextChunks[i].m_From << " " << m_TextChunks[i].m_To << "\",";
		for (int j=0; j<(int)m_TextChunks[i].m_Tags.size(); ++j)
		{
			fout << "(" << m_TextChunks[i].m_Tags[j].m_TagType << " " << m_TextChunks[i].m_Tags[j].m_TagValue << ")";
		}
		fout << "}\n";
	}

	fout.close();

#endif


	// Add a delimiter at start and at end, it helps when
	//  processing later, because we don't have make exceptions for
	//  those cases.
	// We'll sort later.
	m_Words.push_back(0);
	m_Words.push_back((int)m_RawString.Length());

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

	// Remove duplicates
	vector<int>::iterator it;
	int last_word = -1;
	for (it = m_Words.begin(); it != m_Words.end(); ++it)
	{
		if (last_word == *it)
			m_Words.erase(it);

		last_word = *it;
	}
}
