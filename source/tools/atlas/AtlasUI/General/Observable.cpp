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

#include "Observable.h"

void ObservableScopedConnections::Add(const ObservableConnection& conn)
{
	// Clean up any disconnected connections that might be left in here
	m_Conns.erase(
		remove_if(m_Conns.begin(), m_Conns.end(),
			not1(std::mem_fun_ref(&ObservableConnection::connected))),
		m_Conns.end()
	);

	// Add the new connection
	m_Conns.push_back(conn);
}

ObservableScopedConnections::~ObservableScopedConnections()
{
	// Disconnect all connections that we hold
	for_each(m_Conns.begin(), m_Conns.end(), std::mem_fun_ref(&ObservableConnection::disconnect));
}
