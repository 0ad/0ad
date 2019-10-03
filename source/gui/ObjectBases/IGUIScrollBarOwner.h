/* Copyright (C) 2019 Wildfire Games.
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

#ifndef INCLUDED_IGUISCROLLBAROWNER
#define INCLUDED_IGUISCROLLBAROWNER

#include "gui/ObjectBases/IGUIObject.h"

#include <vector>

struct SGUIScrollBarStyle;
class IGUIScrollBar;

/**
 * Base-class this if you want an object to contain
 * one, or several, scroll-bars.
 */
class IGUIScrollBarOwner
{
	NONCOPYABLE(IGUIScrollBarOwner);

	friend class IGUIScrollBar;

public:
	IGUIScrollBarOwner(IGUIObject& m_pObject);
	virtual ~IGUIScrollBarOwner();

	virtual void Draw();

	/**
	 * @see IGUIObject#HandleMessage()
	 */
	virtual void HandleMessage(SGUIMessage& Message);

	/**
	 * @see IGUIObject#ResetStates()
	 */
	virtual void ResetStates();

	/**
	 * Interface for the m_ScrollBar to use.
	 */
	virtual const SGUIScrollBarStyle* GetScrollBarStyle(const CStr& style) const;

	/**
	 * Add a scroll-bar
	 */
	virtual void AddScrollBar(IGUIScrollBar* scrollbar);

	/**
	 * Get Scroll Bar reference (it should be transparent it's actually
	 * pointers).
	 */
	virtual IGUIScrollBar& GetScrollBar(const int& index)
	{
		return *m_ScrollBars[index];
	}

	/**
	 * Get the position of the scroll bar at @param index.
	 * Equivalent to GetScrollbar(index).GetPos().
	 */
	virtual float GetScrollBarPos(const int index) const;

protected:
	/**
	 * Predominately you will only have one, but you can have
	 * as many as you like.
	 */
	std::vector<IGUIScrollBar*> m_ScrollBars;

private:
	/**
	 * Reference to the IGUIObject.
	 * Private, because we don't want to inherit it in multiple classes.
	 */
	IGUIObject& m_pObject;
};

#endif // INCLUDED_IGUISCROLLBAROWNER
