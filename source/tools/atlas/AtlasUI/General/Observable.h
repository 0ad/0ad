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

#ifndef INCLUDED_OBSERVABLE
#define INCLUDED_OBSERVABLE

/*
Wrapper around Boost.Signals to make watching objects for changes more convenient.

General usage:

Observable<int> variable_to_be_watched;
...
class Thing {
	ObservableScopedConnection m_Conn;
	Thing() {
		m_Conn = variable_to_be_watched.RegisterObserver(OnChange);
	}
	void OnChange(const int& var) {
		do_something_with(var);
	}
}
...
variable_to_be_watched.NotifyObservers();

*/

#include <boost/bind.hpp>
#include <boost/version.hpp>
#if BOOST_VERSION >= 104000
# include <boost/signals2.hpp>
typedef boost::signals2::connection ObservableConnection;
typedef boost::signals2::scoped_connection ObservableScopedConnection;
#else
# error Atlas requires Boost 1.40 or later
#endif

template <typename T> class Observable : public T
{
public:
	Observable() {}

	template <typename T1>
		explicit Observable(const T1& a1) : T(a1) {}
	template <typename T1, typename T2>
		explicit Observable(T1& a1, T2& a2) : T(a1, a2) {}
	template <typename T1, typename T2>
		explicit Observable(T1& a1, T2 a2) : T(a1, a2) {}

	template<typename C> ObservableConnection RegisterObserver(int order, void (C::*callback) (const T&), C* obj)
	{
		return m_Signal.connect(order, boost::bind(std::mem_fun(callback), obj, _1));
	}

	ObservableConnection RegisterObserver(int order, void (*callback) (const T&))
	{
		return m_Signal.connect(order, callback);
	}

	void RemoveObserver(const ObservableConnection& conn)
	{
		conn.disconnect();
	}

	void NotifyObservers()
	{
		m_Signal(*this);
	}

	// Use when an object is changing something that it's also observing,
	// because it already knows about the change and doesn't need to be notified
	// again (particularly since that may cause infinite loops).
	void NotifyObserversExcept(ObservableConnection& conn)
	{
		if (conn.blocked())
		{
			// conn is already blocked and won't see anything
			NotifyObservers();
		}
		else
		{
			// Temporarily disable conn
			boost::signals2::shared_connection_block blocker(conn);
			NotifyObservers();
		}
	}

	Observable<T>& operator=(const T& rhs)
	{
		*dynamic_cast<T*>(this) = rhs;
		return *this;
	}

private:
	boost::signals2::signal<void (const T&)> m_Signal;
};

// A similar thing, but for wrapping pointers instead of objects
template <typename T> class ObservablePtr
{
public:
	ObservablePtr() : m_Ptr(NULL) {}

	ObservablePtr(T* p) : m_Ptr(p) {}

	ObservablePtr& operator=(T* p)
	{
		m_Ptr = p;
		return *this;
	}

	T* operator->()
	{
		return m_Ptr;
	}

	T* operator*()
	{
		return m_Ptr;
	}

	template<typename C> ObservableConnection RegisterObserver(int order, void (C::*callback) (T*), C* obj)
	{
		return m_Signal.connect(order, boost::bind(std::mem_fun(callback), obj, _1));
	}

	ObservableConnection RegisterObserver(int order, void (*callback) (T*))
	{
		return m_Signal.connect(order, callback);
	}

	void RemoveObserver(const ObservableConnection& conn)
	{
		conn.disconnect();
	}

	void NotifyObservers()
	{
		m_Signal(m_Ptr);
	}

	// Use when an object is changing something that it's also observing,
	// because it already knows about the change and doesn't need to be notified
	// again (particularly since that may cause infinite loops).
	void NotifyObserversExcept(ObservableConnection& conn)
	{
		if (conn.blocked())
		{
			// conn is already blocked and won't see anything
			NotifyObservers();
		}
		else
		{
			// Temporarily disable conn
			boost::signals2::shared_connection_block blocker(conn);
			NotifyObservers();
		}
	}

private:
	T* m_Ptr;
	boost::signals2::signal<void (T*)> m_Signal;
};

class ObservableScopedConnections
{
public:
	void Add(const ObservableConnection& conn);
	~ObservableScopedConnections();
private:
	std::vector<ObservableConnection> m_Conns;
};

#endif // INCLUDED_OBSERVABLE
