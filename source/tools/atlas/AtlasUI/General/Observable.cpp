#include "stdafx.h"

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
