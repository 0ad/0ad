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

#include "graphics/Color.h"
#include "graphics/Texture.h"
#include "gui/ObjectBases/IGUIObject.h"
#include "maths/Vector2D.h"
#include "renderer/VertexArray.h"

#include <deque>
#include <vector>

class CMatrix3D;

class CMiniMap : public IGUIObject
{
	GUI_OBJECT(CMiniMap)

public:
	CMiniMap(CGUI& pGUI);
	virtual ~CMiniMap();

	bool Flare(const CVector2D& pos, const CStr& colorStr);

protected:
	struct MapFlare
	{
		CVector2D pos;
		CColor color;
		double time;
	};

	virtual void Draw(CCanvas2D& canvas);

	virtual void CreateJSObject();

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

	// Whether or not the mouse is currently down
	bool m_Clicking;

	std::deque<MapFlare> m_MapFlares;

	std::vector<CTexturePtr> m_FlareTextures;

	CGUISimpleSetting<u32> m_FlareTextureCount;
	CGUISimpleSetting<u32> m_FlareRenderSize;
	CGUISimpleSetting<bool> m_FlareInterleave;
	CGUISimpleSetting<float> m_FlareAnimationSpeed;
	CGUISimpleSetting<float> m_FlareLifetimeSeconds;

	// Whether to draw a black square around and under the minimap.
	CGUISimpleSetting<bool> m_Mask;

	// map size
	ssize_t m_MapSize;

	// 1.f if map is circular or 1.414f if square (to shrink it inside the circle)
	float m_MapScale;

	void RecreateFlareTextures();

	void DrawViewRect(CCanvas2D& canvas) const;

	void DrawFlare(CCanvas2D& canvas, const MapFlare& flare, double curentTime) const;

	void GetMouseWorldCoordinates(float& x, float& z) const;

	float GetAngle() const;
	CVector2D WorldSpaceToMiniMapSpace(const CVector3D& worldPosition) const;
};

#endif // INCLUDED_MINIMAP
