/* Copyright (C) 2015 Wildfire Games.
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

#ifndef INCLUDED_SCRIPTVAL
#define INCLUDED_SCRIPTVAL

#include "ScriptTypes.h"

/**
 * A default constructible wrapper around JS::PersistentRootedValue
 *
 * It's a very common case that we need to store JS::Values on the heap as
 * class members and only need them conditionally or want to initialize
 * them after the constructor because we don't have the runtime available yet.
 * Use it in these cases, but prefer to use JS::PersistentRootedValue directly
 * if initializing it with a runtime/context in the constructor isn't a problem.
 */
 template <typename T>
class DefPersistentRooted
{
public:
	DefPersistentRooted()
	{
	}

	DefPersistentRooted(JSRuntime* rt)
	{
		m_Val.reset(new JS::PersistentRooted<T>(rt));
	}

	DefPersistentRooted(JSRuntime* rt, JS::HandleValue val)
	{
		m_Val.reset(new JS::PersistentRooted<T>(rt, val));
	}

	DefPersistentRooted(JSContext* cx, JS::Handle<T> val)
	{
		m_Val.reset(new JS::PersistentRooted<T>(cx, val));
	}

	void clear()
	{
		m_Val = nullptr;
	}

	inline bool uninitialized()
	{
		return m_Val == nullptr;
	}

	inline JS::PersistentRooted<T>& get() const
	{
		ENSURE(m_Val);
		return *m_Val;
	}

	inline void set(JSRuntime* rt, T val)
	{
		m_Val.reset(new JS::PersistentRooted<T>(rt, val));
	}

	inline void set(JSContext* cx, T val)
	{
		m_Val.reset(new JS::PersistentRooted<T>(cx, val));
	}

	// TODO: Move assignment operator and move constructor only have to be
	// explicitly defined for Visual Studio. VS2013 is still behind on C++11 support
	// What's missing is what they call "Rvalue references v3.0", see
	// https://msdn.microsoft.com/en-us/library/hh567368.aspx#rvref
	DefPersistentRooted<T>& operator=(DefPersistentRooted<T>&& other)
	{
		m_Val = std::move(other.m_Val);
		return *this;
	}

	DefPersistentRooted<T>(DefPersistentRooted<T>&& other)
	{
		m_Val = std::move(other.m_Val);
	}

private:
	std::unique_ptr<JS::PersistentRooted<T> > m_Val;
};

#endif // INCLUDED_SCRIPTVAL
