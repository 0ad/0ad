/* Copyright (C) 2015 Wildfire Games.
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

#ifndef INCLUDED_ICMPOVERLAYRENDERER
#define INCLUDED_ICMPOVERLAYRENDERER

#include "simulation2/system/Interface.h"

#include "simulation2/helpers/Position.h"

#include "lib/file/vfs/vfs_path.h"
#include "maths/FixedVector2D.h"
#include "maths/FixedVector3D.h"

/**
 * Interface for rendering 'overlay' objects (typically sprites), automatically
 * positioned relative to the entity.
 * Usually driven by the StatusBars component.
 *
 * (TODO: maybe we should add a "category" argument to Reset/AddSprite/etc,
 * so different components can each maintain independent sets of overlays here?)
 */
class ICmpOverlayRenderer : public IComponent
{
public:
	/**
	 * Delete all sprites that have been previously added.
	 */
	virtual void Reset() = 0;

	/**
	 * Add a new textured billboard sprite to be rendered.
	 * @param textureName filename of texture to render.
	 * @param corner0,corner1 coordinates of sprite's corners, in world-space units oriented with the camera plane,
	 *        relative to the sprite position.
	 * @param offset world-space offset of sprite position from the entity's base position.
	 * @param color multiply color of texture
	 */
	virtual void AddSprite(VfsPath textureName, CFixedVector2D corner0, CFixedVector2D corner1, CFixedVector3D offset, std::string color = "255 255 255 255") = 0;

	/**
	* Enables or disables rendering of all sprites.
	* @param visible Whether the selectable should be visible.
	*/
	static void SetOverrideVisibility(bool visible)
	{
		ICmpOverlayRenderer::m_OverrideVisible = visible;
	}

	DECLARE_INTERFACE_TYPE(OverlayRenderer)

protected:
	static bool m_OverrideVisible;
};

#endif // INCLUDED_ICMPOVERLAYRENDERER
