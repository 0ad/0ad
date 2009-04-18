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

/*
IGUIScrollBarOwner
*/

#include "precompiled.h"
#include "GUI.h"


//-------------------------------------------------------------------
//  Constructor / Destructor
//-------------------------------------------------------------------
IGUIScrollBarOwner::IGUIScrollBarOwner()
{
}

IGUIScrollBarOwner::~IGUIScrollBarOwner()
{
	// Delete scroll-bars
	std::vector<IGUIScrollBar*>::iterator it;
	for (it=m_ScrollBars.begin(); it!=m_ScrollBars.end(); ++it)
	{
		delete *it;
	}
}

void IGUIScrollBarOwner::ResetStates()
{
	IGUIObject::ResetStates();
	
	std::vector<IGUIScrollBar*>::iterator it;
	for (it=m_ScrollBars.begin(); it!=m_ScrollBars.end(); ++it)
	{
		(*it)->SetBarPressed(false);
	}
}

void IGUIScrollBarOwner::AddScrollBar(IGUIScrollBar * scrollbar)
{
	scrollbar->SetHostObject(this);
	scrollbar->SetGUI(GetGUI());
	m_ScrollBars.push_back(scrollbar);
}

SGUIScrollBarStyle * IGUIScrollBarOwner::GetScrollBarStyle(const CStr& style) const
{
	if (!GetGUI())
	{
		// TODO Gee: Output in log
		return NULL;
	}
	
	if (GetGUI()->m_ScrollBarStyles.count(style) == 0)
	{
		// TODO Gee: Output in log
		return NULL;
	}

 	return &((CGUI*)GetGUI())->m_ScrollBarStyles.find(style)->second;
}

void IGUIScrollBarOwner::HandleMessage(const SGUIMessage &Message)
{
	std::vector<IGUIScrollBar*>::iterator it;
	for (it=m_ScrollBars.begin(); it!=m_ScrollBars.end(); ++it)
	{
		(*it)->HandleMessage(Message);
	}
}

void IGUIScrollBarOwner::Draw() 
{
	std::vector<IGUIScrollBar*>::iterator it;
	for (it=m_ScrollBars.begin(); it!=m_ScrollBars.end(); ++it)
	{
		(*it)->Draw();
	}
}
