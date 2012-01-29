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

#include "maths/Matrix3D.h"
#include "ps/CStr.h"
#include "ps/Overlay.h"

class CFont;

class CTextRenderer
{
public:
	CTextRenderer();

	void ResetTransform();
	CMatrix3D GetTransform();
	void SetTransform(const CMatrix3D& transform);

	void Translate(float x, float y, float z);

	void Color(const CColor& color);
	void Font(const CStrW& font);

	void Printf(const wchar_t* fmt, ...);

	void Render();

private:
	struct SBatch
	{
		CMatrix3D transform;
		CColor color;
		shared_ptr<CFont> font;
		std::wstring text;
	};

	CMatrix3D m_Transform;

	CColor m_Color;
	shared_ptr<CFont> m_Font;

	std::map<CStrW, shared_ptr<CFont> > m_Fonts;

	std::vector<SBatch> m_Batches;
};

#endif // INCLUDED_TEXTRENDERER