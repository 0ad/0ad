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

#include "precompiled.h"

#include "IGUIScrollBarOwner.h"

#include "gui/CGUI.h"
#include "gui/IGUIScrollBar.h"

IGUIScrollBarOwner::IGUIScrollBarOwner(IGUIObject& pObject)
	: m_pObject(pObject)
{
}

IGUIScrollBarOwner::~IGUIScrollBarOwner()
{
	for (IGUIScrollBar* const& sb : m_ScrollBars)
		delete sb;
}

void IGUIScrollBarOwner::ResetStates()
{
	for (IGUIScrollBar* const& sb : m_ScrollBars)
		sb->SetBarPressed(false);
}

void IGUIScrollBarOwner::AddScrollBar(IGUIScrollBar* scrollbar)
{
	scrollbar->SetHostObject(this);
	m_ScrollBars.push_back(scrollbar);
}

const SGUIScrollBarStyle* IGUIScrollBarOwner::GetScrollBarStyle(const CStr& style) const
{
	return m_pObject.GetGUI().GetScrollBarStyle(style);
}

void IGUIScrollBarOwner::HandleMessage(SGUIMessage& msg)
{
	for (IGUIScrollBar* const& sb : m_ScrollBars)
		sb->HandleMessage(msg);
}

void IGUIScrollBarOwner::Draw()
{
	for (IGUIScrollBar* const& sb : m_ScrollBars)
		sb->Draw();
}

float IGUIScrollBarOwner::GetScrollBarPos(const int index) const
{
	return m_ScrollBars[index]->GetPos();
}
