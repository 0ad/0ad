/*
 * wxJavaScript - evtconn.h
 *
 * Copyright (c) 2002-2007 Franky Braem and the wxJavaScript project
 *
 * Project Info: http://www.wxjavascript.net or http://wxjs.sourceforge.net
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 *
 * $Id: evtconn.h 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef _wxjs_evt_conn_h
#define _wxjs_evt_conn_h

// A helper class to connect an event to a method of a class
// This is used to avoid a big if-statement for selecting
// the correct event connector
// An event connector is a function that connects an event

#include <map>

namespace wxjs
{
    template<class T_Priv>
    class EventConnector
    {
    public:
        typedef void (*ConnectEventFn)(T_Priv *p);
        typedef std::map<wxString, ConnectEventFn> ConnectEventMap; 
    protected:
        static bool ConnectEvent(T_Priv *p, const wxString &name)
        {
		#if (__GNUC__ >= 4)
            typename ConnectEventMap::iterator it = m_eventMap.find(name);
		#else
			ConnectEventMap::iterator it = m_eventMap.find(name);
		#endif
            if ( it != m_eventMap.end() )
            {
                it->second(p);
                return true;
            } 
            return false;
        }

        static ConnectEventMap m_eventMap;
    };

} // end namespace wxjs

// Some macro's
#if (__GNUC__ >= 4)
	#define WXJS_INIT_EVT_CONNECTOR_MAP(priv) template <> EventConnector<priv>::ConnectEventMap EventConnector<priv>::m_eventMap = EventConnector< priv >::ConnectEventMap();
#else
	#define WXJS_INIT_EVT_CONNECTOR_MAP(priv) EventConnector<priv>::ConnectEventMap EventConnector<priv>::m_eventMap;
#endif

#define WXJS_BEGIN_EVT_CONNECTOR_MAP(cl) \
    void cl::InitConnectEventMap() \
    { 
#define WXJS_EVT_CONNECTOR(name, fun) m_eventMap[name] = fun;

#define WXJS_END_EVT_CONNECTOR_MAP() }

#endif //  _wxjs_evt_conn_h
