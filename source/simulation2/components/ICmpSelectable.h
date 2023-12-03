/* Copyright (C) 2018 Wildfire Games.
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

#ifndef INCLUDED_ICMPSELECTABLE
#define INCLUDED_ICMPSELECTABLE

#include "simulation2/system/Interface.h"

struct CColor;

class ICmpSelectable : public IComponent
{
public:
	/**
	 * Returns true if the entity is only selectable in Atlas editor, e.g. a decorative visual actor.
	 */
	virtual bool IsEditorOnly() const = 0;

	/**
	 * Sets the selection highlight state.
	 * The highlight is typically a circle/square overlay around the unit.
	 * @param color color and alpha of the selection highlight. Set color.a = 0 to hide the highlight.
	 * @param selected whether the entity is selected; affects desaturation for always visible highlights.
	 */
	virtual void SetSelectionHighlight(const CColor& color, bool selected) = 0;

	/**
	 * Enables or disables rendering of an entity's selectable.
	 * @param visible Whether the selectable should be visible.
	 */
	virtual void SetVisibility(bool visible) = 0;

	/**
	* Enables or disables rendering of all entities selectable.
	* @param visible Whether the selectable should be visible.
	*/
	static void SetOverrideVisibility(bool visible)
	{
		ICmpSelectable::m_OverrideVisible = visible;
	}

	/**
	 * Updates the selection color to match the current owner.
	 */
	virtual void UpdateColor() = 0;

	/**
	 * Sets the alpha of the selection highlight. Set to 0 to hide the highlight.
	 */
	virtual void SetSelectionHighlightAlpha(float alpha) = 0;

	DECLARE_INTERFACE_TYPE(Selectable)

	// TODO: this is slightly ugly design; it would be nice to change the component system to support per-component-type data
	// and methods, where we can keep settings like these. Note that any such data store would need to be per-component-manager
	// and not entirely global, to support multiple simulation instances.
	static bool ms_EnableDebugOverlays; // ms for member static

protected:
	static bool m_OverrideVisible;
};

#endif // INCLUDED_ICMPSELECTABLE
