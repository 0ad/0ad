#ifndef OBSERVABLE_H__
#define OBSERVABLE_H__

#include <boost/signals.hpp>
#include <boost/bind.hpp>

typedef boost::signals::connection ObservableConnection;

template <typename T> class Observable : public T
{
public:
	template<typename C> ObservableConnection RegisterObserver(int order, void (C::*callback) (const T&), C* obj)
	{
		return m_Signal.connect(order, boost::bind(std::mem_fun(callback), obj, _1));
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

private:
 	boost::signal<void (const T&)> m_Signal;
};

#endif // OBSERVABLE_H__
