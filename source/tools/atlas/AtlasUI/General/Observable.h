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
	void RemoveObserver(ObservableConnection handle)
	{
		handle.disconnect();
	}
	void NotifyObservers()
	{
 		m_Signal(*this);
	}
private:
 	boost::signal<void (const T&)> m_Signal;
};

#endif // OBSERVABLE_H__
