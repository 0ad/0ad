/* Copyright (C) 2021 Wildfire Games.
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

#ifndef INCLUDED_MINIMAP
#define INCLUDED_MINIMAP

#include "gui/ObjectBases/IGUIObject.h"
#include "renderer/VertexArray.h"

class CCamera;
class CMatrix3D;
class CTerrain;

class CMiniMap : public IGUIObject
{
	GUI_OBJECT(CMiniMap)
public:
	CMiniMap(CGUI& pGUI);
	virtual ~CMiniMap();

protected:
	virtual void Draw(CCanvas2D& canvas);

	/**
	 * @see IGUIObject#HandleMessage()
	 */
	virtual void HandleMessage(SGUIMessage& Message);

	/**
	 * @see IGUIObject#IsMouseOver()
	 */
	virtual bool IsMouseOver() const;

private:
	void SetCameraPositionFromMousePosition();

	bool FireWorldClickEvent(int button, int clicks);

	static const CStr EventNameWorldClick;

	const CCamera* m_Camera;

	// Whether or not the mouse is currently down
	bool m_Clicking;

	// Whether to draw a black square around and under the minimap.
	CGUISimpleSetting<bool> m_Mask;

	// map size
	ssize_t m_MapSize;

	// 1.f if map is circular or 1.414f if square (to shrink it inside the circle)
	float m_MapScale;

	void DrawViewRect(const CMatrix3D& transform) const;

	void GetMouseWorldCoordinates(float& x, float& z) const;

	float GetAngle() const;

	VertexIndexArray m_IndexArray;
	VertexArray m_VertexArray;
	VertexArray::Attribute m_AttributePos;
	VertexArray::Attribute m_AttributeColor;

	size_t m_EntitiesDrawn;

	double m_PingDuration;
	double m_HalfBlinkDuration;
	double m_NextBlinkTime;
	bool m_BlinkState;
};

#endif // INCLUDED_MINIMAP
