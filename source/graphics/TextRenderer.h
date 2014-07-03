/* Copyright (C) 2012 Wildfire Games.
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

#ifndef INCLUDED_TEXTRENDERER
#define INCLUDED_TEXTRENDERER

#include "graphics/ShaderProgramPtr.h"
#include "maths/Matrix3D.h"
#include "ps/CStrIntern.h"
#include "ps/Overlay.h"

class CFont;

class CTextRenderer
{
public:
	CTextRenderer(const CShaderProgramPtr& shader);

	/**
	 * Reset the text transform to the default, with (0,0) in the top-left of the screen.
	 */
	void ResetTransform();

	CMatrix3D GetTransform();
	
	void SetTransform(const CMatrix3D& transform);

	void Translate(float x, float y, float z);

	/**
	 * Set clipping rectangle, in pre-transform coordinates (i.e. text is clipped against
	 * this rect based purely on the x,y values passed into Put()). Text fully outside the
	 * clipping rectangle may not be rendered. Should be used in conjunction with glScissor
	 * for precise clipping - this is just an optimisation.
	 */
	void SetClippingRect(const CRect& rect);

	/**
	 * Set the color for subsequent print calls.
	 */
	void Color(const CColor& color);
	
	/**
	 * Set the color for subsequent print calls.
	 */
	void Color(float r, float g, float b, float a = 1.0);
	
	/**
	 * Set the font for subsequent print calls.
	 */
	void Font(CStrIntern font);

	/**
	 * Print formatted text at (0,0) under the current transform,
	 * and advance the transform by the width of the text.
	 */
	void PrintfAdvance(const wchar_t* fmt, ...);

	/**
	 * Print formatted text at (x,y) under the current transform.
	 * Does not alter the current transform.
	 */
	void PrintfAt(float x, float y, const wchar_t* fmt, ...);

	/**
	 * Print text at (0,0) under the current transform,
	 * and advance the transform by the width of the text.
	 */
	void PutAdvance(const wchar_t* buf);

	/**
	 * Print text at (x,y) under the current transform.
	 * Does not alter the current transform.
	 */
	void Put(float x, float y, const wchar_t* buf);

	/**
	 * Print text at (x,y) under the current transform.
	 * Does not alter the current transform.
	 * @p buf must remain valid until Render() is called.
	 * (This should be used to minimise memory copies when possible.)
	 */
	void Put(float x, float y, const std::wstring* buf);

	/**
	 * Render all of the previously printed text calls.
	 */
	void Render();

private:
	friend struct SBatchCompare;

	/**
	 * A string (optionally owned by this object, or else pointing to an
	 * externally-owned string) with a position.
	 */
	struct SBatchRun
	{
	private:
		SBatchRun& operator=(const SBatchRun&);
	public:
		SBatchRun()
			: text(NULL), owned(false)
		{
		}

		SBatchRun(const SBatchRun& str)
			: x(str.x), y(str.y), owned(str.owned)
		{
			if (owned)
				text = new std::wstring(*str.text);
			else
				text = str.text;
		}

		~SBatchRun()
		{
			if (owned)
				delete text;
		}

		float x, y;

		const std::wstring* text;
		bool owned;
	};

	/**
	 * A list of SBatchRuns, with a single font/color/transform,
	 * to be rendered in a single GL call.
	 */
	struct SBatch
	{
		size_t chars; // sum of runs[i].text->size()
		CMatrix3D transform;
		CColor color;
		shared_ptr<CFont> font;
		std::list<SBatchRun> runs;
	};

	void PutString(float x, float y, const std::wstring* buf, bool owned);

	CShaderProgramPtr m_Shader;

	CMatrix3D m_Transform;
	CRect m_Clipping;

	CColor m_Color;
	CStrIntern m_FontName;
	shared_ptr<CFont> m_Font;

	bool m_Dirty;

	std::list<SBatch> m_Batches;
};

#endif // INCLUDED_TEXTRENDERER
