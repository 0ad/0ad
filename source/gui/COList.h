/* Copyright (C) 2014 Wildfire Games.
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
#ifndef INCLUDED_COLIST
#define INCLUDED_COLIST

//--------------------------------------------------------
//  Includes / Compiler directives
//--------------------------------------------------------
#include "GUI.h"
#include "CList.h"

//--------------------------------------------------------
//  Macros
//--------------------------------------------------------

//--------------------------------------------------------
//  Types
//--------------------------------------------------------

//--------------------------------------------------------
//  Declarations
//--------------------------------------------------------

struct ObjectDef
{
  CColor m_TextColor;
  CStr m_Id;
  float m_Width;
  CStrW m_Heading;

};

/**
 *  Todo : add description
 *
 */
class COList : public CList
{
	GUI_OBJECT(COList)

public:
	COList();

protected:
	void SetupText();
	void HandleMessage(SGUIMessage &Message);

	/**
	 * Handle the \<item\> tag.
	 */
	virtual bool HandleAdditionalChildren(const XMBElement& child, CXeromyces* pFile);

	void DrawList(const int &selected, const CStr& _sprite,
					const CStr& _sprite_selected, const CStr& _textcolor);

	virtual CRect GetListRect() const;

	std::vector<ObjectDef> m_ObjectsDefs;

private:
	float m_HeadingHeight;
	// Width of space avalible for columns
	float m_TotalAvalibleColumnWidth;
};

#endif // INCLUDED_COLIST
