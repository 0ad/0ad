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

/*
	A GUI Sprite, which is actually a collage of several
	sprites.
*/

#ifndef INCLUDED_CGUISPRITE
#define INCLUDED_CGUISPRITE

#include "gui/GUIRenderer.h"
#include "gui/SettingTypes/CGUISize.h"
#include "gui/SettingTypes/CGUIColor.h"
#include "lib/file/vfs/vfs_path.h"
#include "ps/CStr.h"
#include "renderer/backend/Sampler.h"

#include <map>
#include <memory>
#include <vector>

class CCanvas2D;

struct SGUIImageEffects
{
	SGUIImageEffects() : m_Greyscale(false) {}
	CGUIColor m_AddColor;
	CGUIColor m_SolidColor;
	bool m_Greyscale;
};

/**
 * A CGUISprite is actually a collage of several <b>real</b>
 * sprites, this struct represents is such real sprite.
 */
struct SGUIImage
{
	NONCOPYABLE(SGUIImage);
public:
	SGUIImage() :
		m_FixedHAspectRatio(0.f),
		m_RoundCoordinates(true),
		m_AddressMode(Renderer::Backend::Sampler::AddressMode::REPEAT),
		m_Effects(),
		m_Size(CGUISize::Full()),
		m_TextureSize(CGUISize::Full())
	{
	}

	// Filename of the texture
	VfsPath m_TextureName;

	// Image placement (relative to object)
	CGUISize m_Size;

	// Texture placement (relative to image placement)
	CGUISize m_TextureSize;

	// Because OpenGL wants textures in squares with a power of 2 (64x64, 256x256)
	//  it's sometimes tedious to adjust this. So this value simulates which area
	//  is the real texture
	CRect m_TexturePlacementInFile;

	/**
	 * If non-zero, then the image's width will be adjusted when rendering so that
	 * the width:height ratio equals this value.
	 */
	float m_FixedHAspectRatio;

	/**
	 * If true, the image's coordinates will be rounded to integer pixels when
	 * rendering, to avoid blurry filtering.
	 */
	bool m_RoundCoordinates;

	/**
	 * Texture address mode (REPEAT, CLAMP_TO_EDGE, etc).
	 */
	Renderer::Backend::Sampler::AddressMode m_AddressMode;

	// Visual effects (e.g. color modulation)
	std::shared_ptr<SGUIImageEffects> m_Effects;

	// Color
	CGUIColor m_BackColor;
};

/**
 * The GUI sprite, is actually several real sprites (images)
 * like a collage. View the section \<sprites\> in the GUI
 * TDD for more information.
 *
 * Drawing routine is located in CGUI
 *
 * @see CGUI#DrawSprite
 */
class CGUISprite
{
	NONCOPYABLE(CGUISprite);
public:
	CGUISprite() {}
	virtual ~CGUISprite();

	/**
	 * Adds an image to the sprite collage.
	 *
	 * @param image Adds this image to the sprite collage.
	 */
	void AddImage(std::unique_ptr<SGUIImage> image);

	/// List of images
	std::vector<std::unique_ptr<SGUIImage>> m_Images;
};

// An instance of a sprite, usually stored in IGUIObjects - basically a string
// giving the sprite's name, but with some extra data to cache rendering
// calculations between draw calls.
class CGUISpriteInstance
{
public:
	NONCOPYABLE(CGUISpriteInstance);
	MOVABLE(CGUISpriteInstance);

	CGUISpriteInstance();
	CGUISpriteInstance(const CStr& SpriteName);

	void Draw(CGUI& pGUI, CCanvas2D& canvas, const CRect& Size, std::map<CStr, std::unique_ptr<const CGUISprite>>& Sprites) const;

	/**
	 * Whether this Sprite has no texture name set.
	 */
	operator bool() const { return !m_SpriteName.empty(); };

	/**
	 * Returns the sprite texture name.
	 */
	const CStr& GetName() const { return m_SpriteName; }

	/**
	 * Changes the texture name.
	 * Use as rarely as possible, because it clears the draw cache.
	 */
	void SetName(const CStr& SpriteName);

private:
	CStr m_SpriteName;

	// Stored drawing calls, for more efficient rendering
	mutable GUIRenderer::DrawCalls m_DrawCallCache;
	// Relevant details of previously rendered sprite; the cache is invalidated
	// whenever any of these values changes.
	mutable CRect m_CachedSize;
};

#endif // INCLUDED_CGUISPRITE
