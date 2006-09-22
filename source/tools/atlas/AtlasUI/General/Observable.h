#ifndef OBSERVABLE_H__
#define OBSERVABLE_H__

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

#include <boost/signals.hpp>
#include <boost/bind.hpp>

typedef boost::signals::connection ObservableConnection;
typedef boost::signals::scoped_connection ObservableScopedConnection;

template <typename T> class Observable : public T
{
public:
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
			conn.block();
			NotifyObservers();
			conn.unblock();
		}
	}

	Observable<T>* operator=(const T& rhs)
	{
		*dynamic_cast<T*>(this) = rhs;
		return this;
	}

private:
 	boost::signal<void (const T&)> m_Signal;
};

class ObservableScopedConnections
{
public:
	void Add(const ObservableConnection& conn);
	~ObservableScopedConnections();
private:
	std::vector<ObservableConnection> m_Conns;
};

#endif // OBSERVABLE_H__
