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
#include "renderer/backend/IDeviceCommandContext.h"
#include "renderer/Renderer.h"

#include <math.h>

extern int g_xres, g_yres;

// TODO Gee: CRect => CPoint ?
void SGenerateTextImage::SetupSpriteCall(
	const bool left, CGUIText::SSpriteCall& spriteCall, const float width, const float y,
	const CSize2D& size, const CStr& textureName, const float bufferZone)
{
	// TODO Gee: Temp hardcoded values
	spriteCall.m_Area.top = y + bufferZone;
	spriteCall.m_Area.bottom = y + bufferZone + size.Height;

	if (left)
	{
		spriteCall.m_Area.left = bufferZone;
		spriteCall.m_Area.right = size.Width + bufferZone;
	}
	else
	{
		spriteCall.m_Area.left = width - bufferZone - size.Width;
		spriteCall.m_Area.right = width - bufferZone;
	}

	spriteCall.m_Sprite = textureName;

	m_YFrom = spriteCall.m_Area.top - bufferZone;
	m_YTo = spriteCall.m_Area.bottom + bufferZone;
	m_Indentation = size.Width + bufferZone * 2;
}

CGUIText::CGUIText(const CGUI& pGUI, const CGUIString& string, const CStrW& fontW, const float width, const float bufferZone, const EAlign align, const IGUIObject* pObject)
{
	if (string.m_Words.empty())
		return;

	CStrIntern font(fontW.ToUTF8());
	float y = bufferZone; // drawing pointer
	int from = 0;

	bool firstLine = true;	// Necessary because text in the first line is shorter
							// (it doesn't count the line spacing)

	// Images on the left or the right side.
	SGenerateTextImages images;
	int posLastImage = -1;	// Position in the string where last img (either left or right) were encountered.
							//  in order to avoid duplicate processing.

	// Go through string word by word
	for (int i = 0; i < static_cast<int>(string.m_Words.size()) - 1; ++i)
	{
		// Pre-process each line one time, so we know which floating images
		//  will be added for that line.

		// Generated stuff is stored in feedback.
		CGUIString::SFeedback feedback;

		// Preliminary line height, used for word-wrapping with floating images.
		float prelimLineHeight = 0.f;

		// Width and height of all text calls generated.
		string.GenerateTextCall(pGUI, feedback, font, string.m_Words[i], string.m_Words[i+1], firstLine);

		SetupSpriteCalls(pGUI, feedback.m_Images, y, width, bufferZone, i, posLastImage, images);

		posLastImage = std::max(posLastImage, i);

		prelimLineHeight = std::max(prelimLineHeight, feedback.m_Size.Height);

		// If width is 0, then there's no word-wrapping, disable NewLine.
		if (((width != 0 && (feedback.m_Size.Width + 2 * bufferZone > width || feedback.m_NewLine)) || i == static_cast<int>(string.m_Words.size()) - 2) &&
		    ProcessLine(pGUI, string, font, pObject, images, align, prelimLineHeight, width, bufferZone, firstLine, y, i, from))
			return;
	}
}

// Loop through our images queues, to see if images have been added.
void CGUIText::SetupSpriteCalls(
	const CGUI& pGUI,
	const std::array<std::vector<CStr>, 2>& feedbackImages,
	const float y,
	const float width,
	const float bufferZone,
	const int i,
	const int posLastImage,
	SGenerateTextImages& images)
{
	// Check if this has already been processed.
	//  Also, floating images are only applicable if Word-Wrapping is on
	if (width == 0 || i <= posLastImage)
		return;

	// Loop left/right
	for (int j = 0; j < 2; ++j)
		for (const CStr& imgname : feedbackImages[j])
		{
			SSpriteCall spriteCall;
			SGenerateTextImage image;

			// Y is if no other floating images is above, y. Else it is placed
			//  after the last image, like a stack downwards.
			float _y;
			if (!images[j].empty())
				_y = std::max(y, images[j].back().m_YTo);
			else
				_y = y;

			const SGUIIcon& icon = pGUI.GetIcon(imgname);
			image.SetupSpriteCall(j == CGUIString::SFeedback::Left, spriteCall, width, _y, icon.m_Size, icon.m_SpriteName, bufferZone);

			// Check if image is the lowest thing.
			m_Size.Height = std::max(m_Size.Height, image.m_YTo);

			images[j].emplace_back(image);
			m_SpriteCalls.emplace_back(std::move(spriteCall));
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
	const CStrIntern& font,
	const bool firstLine,
	const float width,
	const float widthRangeFrom,
	const float widthRangeTo,
	const int i,
	const int tempFrom,
	CSize2D& lineSize) const
{
	float x = widthRangeFrom;
	for (int j = tempFrom; j <= i; ++j)
	{
		// We don't want to use feedback now, so we'll have to use another one.
		CGUIString::SFeedback feedback2;

		// Don't attach object, it'll suppress the errors
		//  we want them to be reported in the final GenerateTextCall()
		//  so that we don't get duplicates.
		string.GenerateTextCall(pGUI, feedback2, font, string.m_Words[j], string.m_Words[j+1], firstLine);

		// Append X value.
		x += feedback2.m_Size.Width;

		if (width != 0 && x > widthRangeTo && j != tempFrom && !feedback2.m_NewLine)
		{
			// The calculated width of each word includes the space between the current
			// word and the next. When we're wrapping, we need subtract the width of the
			// space after the last word on the line before the wrap.
			CFontMetrics currentFont(font);
			lineSize.Width -= currentFont.GetCharacterWidth(*L" ");
			break;
		}

		// Let lineSize.cy be the maximum m_Height we encounter.
		lineSize.Height = std::max(lineSize.Height, feedback2.m_Size.Height);

		// If the current word is an explicit new line ("\n"),
		// break now before adding the width of this character.
		// ("\n" doesn't have a glyph, thus is given the same width as
		// the "missing glyph" character by CFont::GetCharacterWidth().)
		if (width != 0 && feedback2.m_NewLine)
			break;

		lineSize.Width += feedback2.m_Size.Width;
	}
}

bool CGUIText::ProcessLine(
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
	int& from)
{
	// Change 'from' to 'i', but first keep a copy of its value.
	int tempFrom = from;
	from = i;

	float widthRangeFrom = bufferZone;
	float widthRangeTo = width - bufferZone;
	ComputeLineRange(images, y, width, prelimLineHeight, widthRangeFrom, widthRangeTo);

	CSize2D lineSize;
	ComputeLineSize(pGUI, string, font, firstLine, width, widthRangeFrom, widthRangeTo, i, tempFrom, lineSize);

	// Move down, because font drawing starts from the baseline
	y += lineSize.Height;

	// Do the real processing now
	const bool done = AssembleCalls(pGUI, string, font, pObject, firstLine, width, widthRangeTo, GetLineOffset(align, widthRangeFrom, widthRangeTo, lineSize), y, tempFrom, i, from);

	// Update dimensions
	m_Size.Width = std::max(m_Size.Width, lineSize.Width + bufferZone * 2);
	m_Size.Height = std::max(m_Size.Height, y + bufferZone);

	firstLine = false;

	// Now if we entered as from = i, then we want
	//  i being one minus that, so that it will become
	//  the same i in the next loop. The difference is that
	//  we're on a new line now.
	i = from - 1;

	return done;
}

// Decide width of the line. We need to iterate our floating images.
//  this won't be exact because we're assuming the lineSize.cy
//  will be as our preliminary calculation said. But that may change,
//  although we'd have to add a couple of more loops to try straightening
//  this problem out, and it is very unlikely to happen noticeably if one
//  structures his text in a stylistically pure fashion. Even if not, it
//  is still quite unlikely it will happen.
// Loop through left and right side, from and to.
void CGUIText::ComputeLineRange(
	const SGenerateTextImages& images,
	const float y,
	const float width,
	const float prelimLineHeight,
	float& widthRangeFrom,
	float& widthRangeTo) const
{
	// Floating images are only applicable if word-wrapping is enabled.
	if (width == 0)
		return;

	for (int j = 0; j < 2; ++j)
		for (const SGenerateTextImage& img : images[j])
		{
			// We're working with two intervals here, the image's and the line height's.
			//  let's find the union of these two.
			float unionFrom, unionTo;

			unionFrom = std::max(y, img.m_YFrom);
			unionTo = std::min(y + prelimLineHeight, img.m_YTo);

			// The union is not empty
			if (unionTo > unionFrom)
			{
				if (j == 0)
					widthRangeFrom = std::max(widthRangeFrom, img.m_Indentation);
				else
					widthRangeTo = std::min(widthRangeTo, width - img.m_Indentation);
			}
		}
}

// compute offset based on what kind of alignment
float CGUIText::GetLineOffset(
	const EAlign align,
	const float widthRangeFrom,
	const float widthRangeTo,
	const CSize2D& lineSize) const
{
	switch (align)
	{
	case EAlign::LEFT:
		return widthRangeFrom;

	case EAlign::CENTER:
		return (widthRangeTo + widthRangeFrom - lineSize.Width) / 2;

	case EAlign::RIGHT:
		return widthRangeTo - lineSize.Width;

	default:
		debug_warn(L"Broken EAlign in CGUIText()");
		return 0.f;
	}
}

bool CGUIText::AssembleCalls(
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
	int& from)
{
	bool done = false;
	float x = 0.f;

	for (int j = tempFrom; j <= i; ++j)
	{
		// We don't want to use feedback now, so we'll have to use another one.
		CGUIString::SFeedback feedback2;

		// Defaults
		string.GenerateTextCall(pGUI, feedback2, font, string.m_Words[j], string.m_Words[j+1], firstLine, pObject);

		// Iterate all and set X/Y values
		// Since X values are not set, we need to make an internal
		//  iteration with an increment that will append the internal
		//  x, that is what xPointer is for.
		float xPointer = 0.f;

		for (STextCall& tc : feedback2.m_TextCalls)
		{
			tc.m_Pos = CVector2D(dx + x + xPointer, y);

			xPointer += tc.m_Size.Width;

			if (tc.m_pSpriteCall)
				tc.m_pSpriteCall->m_Area += tc.m_Pos - CSize2D(0, tc.m_pSpriteCall->m_Area.GetHeight());
		}

		// Append X value.
		x += feedback2.m_Size.Width;

		// The first word overrides the width limit, what we
		//  do, in those cases, are just drawing that word even
		//  though it'll extend the object.
		if (width != 0) // only if word-wrapping is applicable
		{
			if (feedback2.m_NewLine)
			{
				from = j + 1;

				// Sprite call can exist within only a newline segment,
				//  therefore we need this.
				if (!feedback2.m_SpriteCalls.empty())
				{
					auto newEnd = std::remove_if(feedback2.m_TextCalls.begin(), feedback2.m_TextCalls.end(), [](const STextCall& call) { return !call.m_pSpriteCall; });
					m_TextCalls.insert(
						m_TextCalls.end(),
						std::make_move_iterator(feedback2.m_TextCalls.begin()),
						std::make_move_iterator(newEnd));
					m_SpriteCalls.insert(
						m_SpriteCalls.end(),
						std::make_move_iterator(feedback2.m_SpriteCalls.begin()),
						std::make_move_iterator(feedback2.m_SpriteCalls.end()));
				}
				break;
			}
			else if (x > widthRangeTo && j == tempFrom)
			{
				from = j+1;
				// do not break, since we want it to be added to m_TextCalls
			}
			else if (x > widthRangeTo)
			{
				from = j;
				break;
			}
		}

		// Add the whole feedback2.m_TextCalls to our m_TextCalls.
		m_TextCalls.insert(
			m_TextCalls.end(),
			std::make_move_iterator(feedback2.m_TextCalls.begin()),
			std::make_move_iterator(feedback2.m_TextCalls.end()));

		m_SpriteCalls.insert(
			m_SpriteCalls.end(),
			std::make_move_iterator(feedback2.m_SpriteCalls.begin()),
			std::make_move_iterator(feedback2.m_SpriteCalls.end()));

		if (j == static_cast<int>(string.m_Words.size()) - 2)
			done = true;
	}

	return done;
}

void CGUIText::Draw(CGUI& pGUI, CCanvas2D& canvas, const CGUIColor& DefaultColor, const CVector2D& pos, CRect clipping) const
{
	Renderer::Backend::IDeviceCommandContext* deviceCommandContext =
		g_Renderer.GetDeviceCommandContext();

	bool isClipped = clipping != CRect();
	if (isClipped)
	{
		// Make clipping rect as small as possible to prevent rounding errors
		clipping.top = std::ceil(clipping.top);
		clipping.bottom = std::floor(clipping.bottom);
		clipping.left = std::ceil(clipping.left);
		clipping.right = std::floor(clipping.right);

		const float scale = g_VideoMode.GetScale();
		Renderer::Backend::IDeviceCommandContext::Rect scissorRect;
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
