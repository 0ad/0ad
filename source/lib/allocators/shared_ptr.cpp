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
#include "shared_ptr.h"

#include "allocators.h"	// AllocatorChecker


#ifndef NDEBUG
static AllocatorChecker s_allocatorChecker;
#endif

class CheckedArrayDeleter
{
public:
	CheckedArrayDeleter(size_t size)
		: m_size(size)
	{
	}

	void operator()(u8* p)
	{
		debug_assert(m_size != 0);
#ifndef NDEBUG
		s_allocatorChecker.OnDeallocate(p, m_size);
#endif
		delete[] p;
		m_size = 0;
	}

private:
	size_t m_size;
};

shared_ptr<u8> Allocate(size_t size)
{
	debug_assert(size != 0);

	u8* p = new u8[size];
#ifndef NDEBUG
	s_allocatorChecker.OnAllocate(p, size);
#endif

	return shared_ptr<u8>(p, CheckedArrayDeleter(size));
}
