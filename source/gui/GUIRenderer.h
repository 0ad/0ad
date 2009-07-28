/* Copyright (C) 2009 Wildfire Games.
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

#ifndef GUIRenderer_h
#define GUIRenderer_h

#include "lib/res/handle.h"
#include "ps/Overlay.h"
#include "ps/CStr.h"
#include <vector>

struct SGUIImageEffects;

namespace GUIRenderer
{
	class IGLState
	{
	public:
		virtual ~IGLState() {};
		virtual void Set(Handle tex)=0;
		virtual void Unset()=0;
	};

	struct SDrawCall
	{
		SDrawCall() : m_TexHandle(0), m_Effects(NULL) {}

		Handle m_TexHandle;

		bool m_EnableBlending;

		IGLState* m_Effects;

		CRect m_Vertices;
		CRect m_TexCoords;
		float m_DeltaZ;

		CColor m_BorderColor; // == CColor() for no border
		CColor m_BackColor;
	};

	class DrawCalls : public std::vector<SDrawCall>
	{
	public:
		void clear();
		DrawCalls();
		~DrawCalls();
		// Copy/assignment results in an empty list, not an actual copy
		DrawCalls(const DrawCalls&);
		const DrawCalls& operator=(const DrawCalls&);
	};
}

#include "gui/CGUISprite.h"

namespace GUIRenderer
{
	void UpdateDrawCallCache(DrawCalls &Calls, CStr&SpriteName, CRect& Size, int CellID, std::map<CStr, CGUISprite> &Sprites);

	void Draw(DrawCalls &Calls);
}

#endif // GUIRenderer_h
