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

#include "precompiled.h"

#include "DynamicSubscription.h"

void CDynamicSubscription::Add(IComponent* cmp)
{
	m_Removed.erase(cmp);
	m_Added.insert(cmp);
}

void CDynamicSubscription::Remove(IComponent* cmp)
{
	m_Added.erase(cmp);
	m_Removed.insert(cmp);
}

void CDynamicSubscription::Flatten()
{
	if (m_Added.empty() && m_Removed.empty())
		return;

	std::vector<IComponent*> tmp;
	tmp.reserve(m_Components.size() + m_Added.size());

	// tmp = m_Components - m_Removed
	std::set_difference(
			m_Components.begin(), m_Components.end(),
			m_Removed.begin(), m_Removed.end(),
			std::back_inserter(tmp),
			CompareIComponent());

	m_Components.clear();

	// m_Components = tmp + m_Added
	std::set_union(
			tmp.begin(), tmp.end(),
			m_Added.begin(), m_Added.end(),
			std::back_inserter(m_Components),
			CompareIComponent());

	m_Added.clear();
	m_Removed.clear();
}

const std::vector<IComponent*>& CDynamicSubscription::GetComponents()
{
	// Must be flattened before calling this function
	ENSURE(m_Added.empty() && m_Removed.empty());

	return m_Components;
}

void CDynamicSubscription::DebugDump()
{
	std::set<IComponent*, CompareIComponent>::iterator it;

	debug_printf(L"components:");
	for (size_t i = 0; i < m_Components.size(); i++)
		debug_printf(L" %p", m_Components[i]);
	debug_printf(L"\n");

	debug_printf(L"added:");
	for (it = m_Added.begin(); it != m_Added.end(); ++it)
		debug_printf(L" %p", *it);
	debug_printf(L"\n");

	debug_printf(L"removed:");
	for (it = m_Removed.begin(); it != m_Removed.end(); ++it)
		debug_printf(L" %p", *it);
	debug_printf(L"\n");
}
