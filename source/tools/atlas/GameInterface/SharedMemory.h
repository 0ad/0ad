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

#ifndef INCLUDED_SHAREDMEMORY
#define INCLUDED_SHAREDMEMORY

// we want to use placement new without grief
// (Duplicated in Shareable.h)
#undef new

namespace AtlasMessage
{

// Shared pointers need to be allocated and freed from the same heap.
// So, both sides of the Shareable interface should set these function pointers
// to point to the same function. (The game will have to dynamically load them
// from the DLL.)
extern void* (*ShareableMallocFptr) (size_t n);
extern void (*ShareableFreeFptr) (void* p);

// Implement shared new/delete on top of those
template<typename T> T* ShareableNew()
{
	T* p = (T*)ShareableMallocFptr(sizeof(T));
	new (p) T;
	return p;
}
template<typename T> void ShareableDelete(T* p)
{
	p->~T();
	ShareableFreeFptr(p);
}
// Or maybe we want to use a non-default constructor
#define SHAREABLE_NEW(T, data) (new ( (T*)AtlasMessage::ShareableMallocFptr(sizeof(T)) ) T data)

}

#endif // INCLUDED_SHAREDMEMORY
