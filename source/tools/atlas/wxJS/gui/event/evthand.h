/*
 * wxJavaScript - evthand.h
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
 * $Id: evthand.h 598 2007-03-07 20:13:28Z fbraem $
 */
/**
 * (c) 2001-2002 Franky Braem (S.A.W.)
 *
 * This file is part of wxJS. wxJS ports wxWindows to JavaScript
 * 
 * File      : EventHandler.h
 * Desc.     : EventHandler will try to handle the normal control events
 *             received from all wxJSxxx classes.
 * Created   : 16-12-2001
 * L. Update :
 */

#ifndef _wxJS_EventHandler_H
#define _wxJS_EventHandler_H

namespace wxjs
{
    namespace gui
    {
        class EventHandler : public wxEvtHandler
        {
        public:

	        /**
	         * Constructors
	         */
	        EventHandler(Object *obj) : wxEvtHandler(), m_obj(obj) 
	        {
	        }

	        virtual ~EventHandler()
	        {
	        }

        protected:
	        DECLARE_EVENT_TABLE()

	        /**
	         * Handles the OnChar event. Executes the function 
	         * in the onChar property.
	         */
	        void OnChar(wxKeyEvent &event);
        	
	        /**
	         * Handles the OnKeyDown event. Executes the function 
	         * in the onKeyDown property.
	         */
	        void OnKeyDown(wxKeyEvent &event);

	        /**
	         * Handles the OnKeyUp event. Executes the function 
	         * in the onKeyUp property.
	         */
	        void OnKeyUp(wxKeyEvent &event);
        	
	        /**
	         * Handles the OnCharHook event. Executes the function 
	         * in the onCharHook property.
	         */
	        void OnCharHook(wxKeyEvent &event);

	        /**
	         * Handles the OnActivate event. Executes the function 
	         * in the onActivate property.
	         */
	        void OnActivate(wxActivateEvent &event);

	        /**
	         * Handles the OnSetFocus event. Executes the function 
	         * in the onSetFocus property.
	         */
	        void OnSetFocus(wxFocusEvent &event);

	        /**
	         * Handles the OnKillFocus event. Executes the function 
	         * in the onKillFocus property.
	         */
	        void OnKillFocus(wxFocusEvent &event);

	        /**
	         * Handles the OnInitDialog event. Executes the function
	         * in the onInitDialog property.
	         */
	        void OnInitDialog(wxInitDialogEvent &event);

	        /**
	         * Handles all the mouse events.
	         */
	        void OnMouseEvents(wxMouseEvent &event);

	        /**
	         * Handles the move event
	         */
	        void OnMove(wxMoveEvent &event);

	        void OnSize(wxSizeEvent &event);

	        void OnScroll(wxScrollWinEvent& event);
	        void OnScrollTop(wxScrollWinEvent& event);
	        void OnScrollBottom(wxScrollWinEvent& event);
	        void OnScrollLineUp(wxScrollWinEvent& event);
	        void OnScrollLineDown(wxScrollWinEvent& event);
	        void OnScrollPageUp(wxScrollWinEvent& event);
	        void OnScrollPageDown(wxScrollWinEvent& event);
	        void OnScrollThumbTrack(wxScrollWinEvent& event);
	        void OnScrollThumbRelease(wxScrollWinEvent& event);

            void OnHelp(wxHelpEvent &event);

        private:

            // No need to root it. It's alive as long as the window is available.
            // And when the window is destroyed, this is also destroyed.
	        Object *m_obj;
        };
    }; // namespace gui
}; // namespace wxjs
#endif // _wxJS_EventHandler_H
