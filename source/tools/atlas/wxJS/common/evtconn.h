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
 * $Id: evtconn.h 680 2007-04-20 10:13:02Z jupeters $
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
        typedef void (*ConnectEventFn)(T_Priv *p, bool connect);
        typedef std::map<wxString, ConnectEventFn> ConnectEventMap; 

        static void AddConnector(const wxString &event, ConnectEventFn fun)
        {
            m_eventMap[event] = fun;
        }

        static bool ConnectEvent(T_Priv *p, const wxString &name, bool connect)
        {
		#if (__GNUC__ >= 4)
            typename ConnectEventMap::iterator it = m_eventMap.find(name);
		#else
			ConnectEventMap::iterator it = m_eventMap.find(name);
		#endif
            if ( it != m_eventMap.end() )
            {
                it->second(p, connect);
                return true;
            } 
            return false;
        }

        static ConnectEventMap m_eventMap;
    };

#if (__GNUC__ >= 4)
    #define WXJS_INIT_EVENT_MAP(class) \
        namespace wxjs { \
            template<> EventConnector<class>::ConnectEventMap \
            EventConnector<class>::m_eventMap = EventConnector<class>::ConnectEventMap(); \
        }
#else
    #define WXJS_INIT_EVENT_MAP(class) \
        namespace wxjs { \
            EventConnector<class>::ConnectEventMap EventConnector<class>::m_eventMap; \
        }
#endif

} // end namespace wxjs

#endif //  _wxjs_evt_conn_h
