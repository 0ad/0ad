/* Copyright (C) 2022 Wildfire Games.
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

#include "CGUIText.h"

#include "graphics/Canvas2D.h"
#include "graphics/FontMetrics.h"
#include "graphics/TextRenderer.h"
#include "gui/CGUI.h"
#include "gui/ObjectBases/IGUIObject.h"
#include "gui/SettingTypes/CGUIString.h"
#include "ps/CStrInternStatic.h"
#include "ps/VideoMode.h"
#include "renderer/backend/gl/DeviceCommandContext.h"
#include "renderer/Renderer.h"

#include <math.h>

extern int g_xres, g_yres;

// TODO Gee: CRect => CPoint ?
void SGenerateTextImage::SetupSpriteCall(
	const bool Left, CGUIText::SSpriteCall& SpriteCall, const float width, const float y,
	const CSize2D& Size, const CStr& TextureName, const float BufferZone)
{
	// TODO Gee: Temp hardcoded values
	SpriteCall.m_Area.top = y + BufferZone;
	SpriteCall.m_Area.bottom = y + BufferZone + Size.Height;

	if (Left)
	{
		SpriteCall.m_Area.left = BufferZone;
		SpriteCall.m_Area.right = Size.Width + BufferZone;
	}
	else
	{
		SpriteCall.m_Area.left = width-BufferZone - Size.Width;
		SpriteCall.m_Area.right = width-BufferZone;
	}

	SpriteCall.m_Sprite = TextureName;

	m_YFrom = SpriteCall.m_Area.top - BufferZone;
	m_YTo = SpriteCall.m_Area.bottom + BufferZone;
	m_Indentation = Size.Width + BufferZone * 2;
}

CGUIText::CGUIText(const CGUI& pGUI, const CGUIString& string, const CStrW& FontW, const float Width, const float BufferZone, const EAlign align, const IGUIObject* pObject)
{
	if (string.m_Words.empty())
		return;

	CStrIntern Font(FontW.ToUTF8());
	float x = BufferZone, y = BufferZone; // drawing pointer
	int from = 0;

	bool FirstLine = true;	// Necessary because text in the first line is shorter
							// (it doesn't count the line spacing)

	// Images on the left or the right side.
	SGenerateTextImages Images;
	int pos_last_img = -1;	// Position in the string where last img (either left or right) were encountered.
							//  in order to avoid duplicate processing.

	// Go through string word by word
	for (int i = 0; i < static_cast<int>(string.m_Words.size()) - 1; ++i)
	{
		// Pre-process each line one time, so we know which floating images
		//  will be added for that line.

		// Generated stuff is stored in Feedback.
		CGUIString::SFeedback Feedback;

		// Preliminary line height, used for word-wrapping with floating images.
		float prelim_line_height = 0.f;

		// Width and height of all text calls generated.
		string.GenerateTextCall(pGUI, Feedback, Font, string.m_Words[i], string.m_Words[i+1], FirstLine);

		SetupSpriteCalls(pGUI, Feedback.m_Images, y, Width, BufferZone, i, pos_last_img, Images);

		pos_last_img = std::max(pos_last_img, i);

		x += Feedback.m_Size.Width;
		prelim_line_height = std::max(prelim_line_height, Feedback.m_Size.Height);

		// If Width is 0, then there's no word-wrapping, disable NewLine.
		if (((Width != 0 && (x > Width - BufferZone || Feedback.m_NewLine)) || i == static_cast<int>(string.m_Words.size()) - 2) &&
		    ProcessLine(pGUI, string, Font, pObject, Images, align, prelim_line_height, Width, BufferZone, FirstLine, x, y, i, from))
			return;
	}
}

// Loop through our images queues, to see if images have been added.
void CGUIText::SetupSpriteCalls(
	const CGUI& pGUI,
	const std::array<std::vector<CStr>, 2>& FeedbackImages,
	const float y,
	const float Width,
	const float BufferZone,
	const int i,
	const int pos_last_img,
	SGenerateTextImages& Images)
{
	// Check if this has already been processed.
	//  Also, floating images are only applicable if Word-Wrapping is on
	if (Width == 0 || i <= pos_last_img)
		return;

	// Loop left/right
	for (int j = 0; j < 2; ++j)
		for (const CStr& imgname : FeedbackImages[j])
		{
			SSpriteCall SpriteCall;
			SGenerateTextImage Image;

			// Y is if no other floating images is above, y. Else it is placed
			//  after the last image, like a stack downwards.
			float _y;
			if (!Images[j].empty())
				_y = std::max(y, Images[j].back().m_YTo);
			else
				_y = y;

			const SGUIIcon& icon = pGUI.GetIcon(imgname);
			Image.SetupSpriteCall(j == CGUIString::SFeedback::Left, SpriteCall, Width, _y, icon.m_Size, icon.m_SpriteName, BufferZone);

			// Check if image is the lowest thing.
			m_Size.Height = std::max(m_Size.Height, Image.m_YTo);

			Images[j].emplace_back(Image);
			m_SpriteCalls.emplace_back(std::move(SpriteCall));
		}
}

// Now we'll do another loop to figure out the height and width of
//  the line (the height of the largest character and the width is
//  the sum of all of the individual widths). This
//  couldn't be determined in the first loop (main loop)
//  because it didn't regard images, so we don't know
//  if all characters processed, will actually be involved
//  in that line.
void CGUIText::ComputeLineSize(
	const CGUI& pGUI,
	const CGUIString& string,
	const CStrIntern& Font,
	const bool FirstLine,
	const float Width,
	const float width_range_to,
	const int i,
	const int temp_from,
	float& x,
	CSize2D& line_size) const
{
	for (int j = temp_from; j <= i; ++j)
	{
		// We don't want to use Feedback now, so we'll have to use another one.
		CGUIString::SFeedback Feedback2;

		// Don't attach object, it'll suppress the errors
		//  we want them to be reported in the final GenerateTextCall()
		//  so that we don't get duplicates.
		string.GenerateTextCall(pGUI, Feedback2, Font, string.m_Words[j], string.m_Words[j+1], FirstLine);

		// Append X value.
		x += Feedback2.m_Size.Width;

		if (Width != 0 && x > width_range_to && j != temp_from && !Feedback2.m_NewLine)
		{
			// The calculated width of each word includes the space between the current
			// word and the next. When we're wrapping, we need subtract the width of the
			// space after the last word on the line before the wrap.
			CFontMetrics currentFont(Font);
			line_size.Width -= currentFont.GetCharacterWidth(*L" ");
			break;
		}

		// Let line_size.cy be the maximum m_Height we encounter.
		line_size.Height = std::max(line_size.Height, Feedback2.m_Size.Height);

		// If the current word is an explicit new line ("\n"),
		// break now before adding the width of this character.
		// ("\n" doesn't have a glyph, thus is given the same width as
		// the "missing glyph" character by CFont::GetCharacterWidth().)
		if (Width != 0 && Feedback2.m_NewLine)
			break;

		line_size.Width += Feedback2.m_Size.Width;
	}
}

bool CGUIText::ProcessLine(
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
	int& from)
{
	// Change 'from' to 'i', but first keep a copy of its value.
	int temp_from = from;
	from = i;

	float width_range_from = BufferZone;
	float width_range_to = Width - BufferZone;
	ComputeLineRange(Images, y, Width, prelim_line_height, width_range_from, width_range_to);

	// Reset X for the next loop
	x = width_range_from;

	CSize2D line_size;
	ComputeLineSize(pGUI, string, Font, FirstLine, Width, width_range_to, i, temp_from, x, line_size);

	// Reset x once more
	x = width_range_from;

	// Move down, because font drawing starts from the baseline
	y += line_size.Height;

	const float dx = GetLineOffset(align, width_range_from, width_range_to, line_size);

	// Do the real processing now
	const bool done = AssembleCalls(pGUI, string, Font, pObject, FirstLine, Width, width_range_to, dx, y, temp_from, i, x, from);

	// Reset X
	x = BufferZone;

	// Update dimensions
	m_Size.Width = std::max(m_Size.Width, line_size.Width + BufferZone * 2);
	m_Size.Height = std::max(m_Size.Height, y + BufferZone);

	FirstLine = false;

	// Now if we entered as from = i, then we want
	//  i being one minus that, so that it will become
	//  the same i in the next loop. The difference is that
	//  we're on a new line now.
	i = from - 1;

	return done;
}

// Decide width of the line. We need to iterate our floating images.
//  this won't be exact because we're assuming the line_size.cy
//  will be as our preliminary calculation said. But that may change,
//  although we'd have to add a couple of more loops to try straightening
//  this problem out, and it is very unlikely to happen noticeably if one
//  structures his text in a stylistically pure fashion. Even if not, it
//  is still quite unlikely it will happen.
// Loop through left and right side, from and to.
void CGUIText::ComputeLineRange(
	const SGenerateTextImages& Images,
	const float y,
	const float Width,
	const float prelim_line_height,
	float& width_range_from,
	float& width_range_to) const
{
	// Floating images are only applicable if word-wrapping is enabled.
	if (Width == 0)
		return;

	for (int j = 0; j < 2; ++j)
		for (const SGenerateTextImage& img : Images[j])
		{
			// We're working with two intervals here, the image's and the line height's.
			//  let's find the union of these two.
			float union_from, union_to;

			union_from = std::max(y, img.m_YFrom);
			union_to = std::min(y + prelim_line_height, img.m_YTo);

			// The union is not empty
			if (union_to > union_from)
			{
				if (j == 0)
					width_range_from = std::max(width_range_from, img.m_Indentation);
				else
					width_range_to = std::min(width_range_to, Width - img.m_Indentation);
			}
		}
}

// compute offset based on what kind of alignment
float CGUIText::GetLineOffset(
	const EAlign align,
	const float width_range_from,
	const float width_range_to,
	const CSize2D& line_size) const
{
	switch (align)
	{
	case EAlign::LEFT:
		// don't add an offset
		return 0.f;

	case EAlign::CENTER:
		return ((width_range_to - width_range_from) - line_size.Width) / 2;

	case EAlign::RIGHT:
		return width_range_to - line_size.Width;

	default:
		debug_warn(L"Broken EAlign in CGUIText()");
		return 0.f;
	}
}

bool CGUIText::AssembleCalls(
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
	int& from)
{
	bool done = false;

	for (int j = temp_from; j <= i; ++j)
	{
		// We don't want to use Feedback now, so we'll have to use another one.
		CGUIString::SFeedback Feedback2;

		// Defaults
		string.GenerateTextCall(pGUI, Feedback2, Font, string.m_Words[j], string.m_Words[j+1], FirstLine, pObject);

		// Iterate all and set X/Y values
		// Since X values are not set, we need to make an internal
		//  iteration with an increment that will append the internal
		//  x, that is what x_pointer is for.
		float x_pointer = 0.f;

		for (STextCall& tc : Feedback2.m_TextCalls)
		{
			tc.m_Pos = CVector2D(dx + x + x_pointer, y);

			x_pointer += tc.m_Size.Width;

			if (tc.m_pSpriteCall)
				tc.m_pSpriteCall->m_Area += tc.m_Pos - CSize2D(0, tc.m_pSpriteCall->m_Area.GetHeight());
		}

		// Append X value.
		x += Feedback2.m_Size.Width;

		// The first word overrides the width limit, what we
		//  do, in those cases, are just drawing that word even
		//  though it'll extend the object.
		if (Width != 0) // only if word-wrapping is applicable
		{
			if (Feedback2.m_NewLine)
			{
				from = j + 1;

				// Sprite call can exist within only a newline segment,
				//  therefore we need this.
				if (!Feedback2.m_SpriteCalls.empty())
				{
					auto newEnd = std::remove_if(Feedback2.m_TextCalls.begin(), Feedback2.m_TextCalls.end(), [](const STextCall& call) { return !call.m_pSpriteCall; });
					m_TextCalls.insert(
						m_TextCalls.end(),
						std::make_move_iterator(Feedback2.m_TextCalls.begin()),
						std::make_move_iterator(newEnd));
					m_SpriteCalls.insert(
						m_SpriteCalls.end(),
						std::make_move_iterator(Feedback2.m_SpriteCalls.begin()),
						std::make_move_iterator(Feedback2.m_SpriteCalls.end()));
				}
				break;
			}
			else if (x > width_range_to && j == temp_from)
			{
				from = j+1;
				// do not break, since we want it to be added to m_TextCalls
			}
			else if (x > width_range_to)
			{
				from = j;
				break;
			}
		}

		// Add the whole Feedback2.m_TextCalls to our m_TextCalls.
		m_TextCalls.insert(
			m_TextCalls.end(),
			std::make_move_iterator(Feedback2.m_TextCalls.begin()),
			std::make_move_iterator(Feedback2.m_TextCalls.end()));

		m_SpriteCalls.insert(
			m_SpriteCalls.end(),
			std::make_move_iterator(Feedback2.m_SpriteCalls.begin()),
			std::make_move_iterator(Feedback2.m_SpriteCalls.end()));

		if (j == static_cast<int>(string.m_Words.size()) - 2)
			done = true;
	}

	return done;
}

void CGUIText::Draw(CGUI& pGUI, CCanvas2D& canvas, const CGUIColor& DefaultColor, const CVector2D& pos, CRect clipping) const
{
	Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext =
		g_Renderer.GetDeviceCommandContext();

	bool isClipped = clipping != CRect();
	if (isClipped)
	{
		// Make clipping rect as small as possible to prevent rounding errors
		clipping.top = std::ceil(clipping.top);
		clipping.bottom = std::floor(clipping.bottom);
		clipping.left = std::ceil(clipping.left);
		clipping.right = std::floor(clipping.right);

		float scale = g_VideoMode.GetScale();
		Renderer::Backend::GL::CDeviceCommandContext::ScissorRect scissorRect;
		scissorRect.x = std::ceil(clipping.left * scale);
		scissorRect.y = std::ceil(g_yres - clipping.bottom * scale);
		scissorRect.width = std::floor(clipping.GetWidth() * scale);
		scissorRect.height = std::floor(clipping.GetHeight() * scale);
		// TODO: move scissors to CCanvas2D.
		deviceCommandContext->SetScissors(1, &scissorRect);
	}

	CTextRenderer textRenderer;
	textRenderer.SetClippingRect(clipping);
	textRenderer.Translate(0.0f, 0.0f);

	for (const STextCall& tc : m_TextCalls)
	{
		// If this is just a placeholder for a sprite call, continue
		if (tc.m_pSpriteCall)
			continue;

		textRenderer.SetCurrentColor(tc.m_UseCustomColor ? tc.m_Color : DefaultColor);
		textRenderer.SetCurrentFont(tc.m_Font);
		textRenderer.Put(floorf(pos.X + tc.m_Pos.X), floorf(pos.Y + tc.m_Pos.Y), &tc.m_String);
	}

	canvas.DrawText(textRenderer);

	for (const SSpriteCall& sc : m_SpriteCalls)
		pGUI.DrawSprite(sc.m_Sprite, canvas, sc.m_Area + pos);

	if (isClipped)
		deviceCommandContext->SetScissors(0, nullptr);
}
