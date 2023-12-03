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

#include "precompiled.h"

/* Disabled (and should be removed if it turns out to be unnecessary)
- see MessagePasserImpl.cpp for information

#include "AtlasEventLoop.h"

AtlasEventLoop::AtlasEventLoop()
: m_NeedsPaint(false)
{
}

void AtlasEventLoop::AddMessage(MSG* msg)
{
	m_Messages.push_back(msg);
}

void AtlasEventLoop::NeedsPaint()
{
	m_NeedsPaint = true;
}

bool AtlasEventLoop::Dispatch()
{
	// Process the messages that QueryCallback collected
	for (size_t i = 0; i < m_Messages.size(); ++i)
	{
		MSG* pMsg = m_Messages[i];
		wxEventLoop::GetActive()->ProcessMessage(pMsg);
		delete pMsg;
	}
	m_Messages.clear();

	if (m_NeedsPaint && wxTheApp && wxTheApp->GetTopWindow())
	{
		wxTheApp->GetTopWindow()->Refresh();
		m_NeedsPaint = false;
	}

	return wxEventLoop::Dispatch();
}
*/
