/*
 * wxJavaScript - jsevent.h
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
 * $Id: jsevent.h 783 2007-06-24 20:36:30Z fbraem $
 */

#ifndef wxJS_Event_H
#define wxJS_Event_H

#include "../../common/jsutil.h"

namespace wxjs
{
    namespace gui
    {
        template<class E>
        class JSEvent
        {
        public:
            JSEvent(E &event) : m_event(event), m_inScoop(true), m_clonedEvent(NULL)
            {
            }

            virtual ~JSEvent()
            {
                delete m_clonedEvent;
            }

            E* GetEvent()
            {
		        if ( InScoop() )
			        return &m_event;
		        else
			        return m_clonedEvent;
            }

            inline bool InScoop() const 
            {
                return m_inScoop;
            }

            E &m_event;
            bool m_inScoop;
            E *m_clonedEvent;

            inline void SetScoop(bool scoop)
            {
                m_inScoop = scoop;
                if ( ! scoop )
                    m_clonedEvent = (E *) m_event.Clone();
            }

	        template <class T>
            static void Fire(E &event, const wxString &property)
            {
                wxEvtHandler *evtHandler = dynamic_cast<wxEvtHandler*>(event.GetEventObject());
                JavaScriptClientData *obj = dynamic_cast<JavaScriptClientData *>(evtHandler->GetClientObject());

                JSContext *cx = obj->GetContext();
	            wxASSERT_MSG(cx != NULL, wxT("No application context"));
		        jsval fval;
		        if ( GetFunctionProperty(cx, obj->GetObject(), property.ToAscii(), &fval) == JS_TRUE )
	            {
		            jsval rval;

                    JSEvent *jsEvent = new JSEvent<E>(event);

			        jsval argv[] = { T::CreateObject(cx, jsEvent) };

		            JSBool result = JS_CallFunctionValue(cx, obj->GetObject(), fval, 1, argv, &rval);
		            jsEvent->SetScoop(false);

		            if ( result == JS_FALSE )
		            {
                        if ( JS_IsExceptionPending(cx) )
                        {
                            JS_ReportPendingException(cx);
                        }
		            }
	            }
            }
        };

        // Define the simple event classes: classes without properties and methods.
        // Remove the wx from the wxClass when using this macro
        #define WXJS_DECLARE_SIMPLE_EVENT(name, wxClass)                     \
	        typedef JSEvent<wx##wxClass> Priv##wxClass;                   \
            class name : public ApiWrapper<name, JSEvent<wx##wxClass> > \
            {                                                                \
            };

        WXJS_DECLARE_SIMPLE_EVENT(FocusEvent, FocusEvent)
        WXJS_DECLARE_SIMPLE_EVENT(SysColourChangedEvent, SysColourChangedEvent)
        WXJS_DECLARE_SIMPLE_EVENT(MaximizeEvent, MaximizeEvent)
        WXJS_DECLARE_SIMPLE_EVENT(InitDialogEvent, InitDialogEvent)

        // Initialize the event prototypes
        bool InitEventClasses(JSContext *cx, JSObject *global);
    }; // namespace gui
}; // namespace wxjs

#endif // wxJS_Event_H
