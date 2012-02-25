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

#include "graphics/ShaderProgram.h"
#include "maths/Matrix3D.h"
#include "ps/CStr.h"
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
	void Font(const CStrW& font);

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
	 * Print text at (x,y) under the current transform,
	 * and advance the transform by the width of the text.
	 */
	void Put(float x, float y, const wchar_t* buf);

	/**
	 * Render all of the previously printed text calls.
	 */
	void Render();

private:
	struct SBatch
	{
		CMatrix3D transform;
		CColor color;
		shared_ptr<CFont> font;
		std::wstring text;
	};

	CShaderProgramPtr m_Shader;

	CMatrix3D m_Transform;

	CColor m_Color;
	shared_ptr<CFont> m_Font;

	std::map<CStrW, shared_ptr<CFont> > m_Fonts;

	std::vector<SBatch> m_Batches;
};

#endif // INCLUDED_TEXTRENDERER
