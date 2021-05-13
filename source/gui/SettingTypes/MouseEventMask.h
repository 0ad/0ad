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

#ifndef INCLUDED_GUI_MOUSE_EVENT_MASK
#define INCLUDED_GUI_MOUSE_EVENT_MASK

#include "gui/CGUISetting.h"
#include "ps/CStr.h"

#include <string>
#include <memory>

class CRect;
class CVector2D;
class IGUIObject;
class ScriptRequest;

/**
 * A custom shape that changes the object's "over-ability", and thus where one can click on it.
 * Supported:
 *  - "texture:[path]" loads a texture and uses either the alpha or the red channel. Any non-0 is clickable.
 *    The texture is always 'stretched' in sprite terminology.
 *
 * TODO:
 *  - the minimap circular shape should be moved here.
 */
class CGUIMouseEventMask : public IGUISetting
{
public:
	CGUIMouseEventMask(IGUIObject* owner);
	~CGUIMouseEventMask();

	/**
	 * @return true if the mask is initialised <=> its spec is not ""
	 */
	explicit operator bool() const { return !!m_Impl; }

	/**
	 * @return true if the mouse pointer is over the mask. False if the mask is not initialised.
	 */
	bool IsMouseOver(const CVector2D& mousePos, const CRect& objectSize) const;

	void ToJSVal(const ScriptRequest& rq, JS::MutableHandleValue value) override;

	class Impl;
protected:
	bool DoFromString(const CStrW& value) override;
	bool DoFromJSVal(const ScriptRequest& rq, JS::HandleValue value) override;
	CStr GetName() const override;

	std::string m_Spec;
	std::unique_ptr<Impl> m_Impl;
};

#endif // INCLUDED_GUI_MOUSE_EVENT_MASK
