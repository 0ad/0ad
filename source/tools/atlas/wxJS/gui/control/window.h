/*
 * wxJavaScript - window.h
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
 * $Id: window.h 745 2007-06-11 20:16:54Z fbraem $
 */
#ifndef _WXJSWINDOW_H
#define _WXJSWINDOW_H

#include "../../common/evtconn.h"

namespace wxjs
{
  namespace gui
  {
    class Window : public ApiWrapper<Window, wxWindow>
    {
    public:
      static void InitClass(JSContext* cx,
                            JSObject* obj,
                            JSObject* proto);

      static bool GetProperty(wxWindow *p,
                              JSContext *cx,
                              JSObject *obj,
                              int id,
                              jsval *vp);
      static bool SetProperty(wxWindow *p,
                              JSContext *cx,
                              JSObject *obj,
                              int id,
                              jsval *vp);
      static bool GetStaticProperty(JSContext *cx,
                                    int id,
                                    jsval *vp);

      WXJS_DECLARE_METHOD_MAP()
      WXJS_DECLARE_METHOD(captureMouse)
      WXJS_DECLARE_METHOD(centre)
      WXJS_DECLARE_METHOD(clearBackground)
      WXJS_DECLARE_METHOD(clientToScreen)
      WXJS_DECLARE_METHOD(close)
      WXJS_DECLARE_METHOD(move)
      WXJS_DECLARE_METHOD(convertDialogToPixels)
      WXJS_DECLARE_METHOD(convertPixelsToDialog)
      WXJS_DECLARE_METHOD(destroy)
      WXJS_DECLARE_METHOD(destroyChildren)
      WXJS_DECLARE_METHOD(releaseMouse)
      WXJS_DECLARE_METHOD(layout)
      WXJS_DECLARE_METHOD(setSize)
      WXJS_DECLARE_METHOD(raise)
      WXJS_DECLARE_METHOD(lower)
      WXJS_DECLARE_METHOD(centreOnParent)
      WXJS_DECLARE_METHOD(fit)
      WXJS_DECLARE_METHOD(setSizeHints)
      WXJS_DECLARE_METHOD(show)
      WXJS_DECLARE_METHOD(refresh)
      WXJS_DECLARE_METHOD(setFocus)
      WXJS_DECLARE_METHOD(findFocus)
      WXJS_DECLARE_METHOD(findWindow)
      WXJS_DECLARE_METHOD(initDialog)
      WXJS_DECLARE_METHOD(transferDataToWindow)
      WXJS_DECLARE_METHOD(transferDataFromWindow)
      WXJS_DECLARE_METHOD(validate)
      WXJS_DECLARE_METHOD(makeModal)
      WXJS_DECLARE_METHOD(warpPointer)
      WXJS_DECLARE_METHOD(update)
      WXJS_DECLARE_METHOD(freeze)
      WXJS_DECLARE_METHOD(thaw)
      WXJS_DECLARE_METHOD(findWindowById)
      WXJS_DECLARE_METHOD(findWindowByLabel)

      WXJS_DECLARE_STATIC_METHOD_MAP()
      WXJS_DECLARE_STATIC_PROPERTY_MAP()
      WXJS_DECLARE_CONSTANT_MAP()

      WXJS_DECLARE_PROPERTY_MAP()
      enum
      {
          P_CLIENT_HEIGHT = -128
          , P_AUTO_LAYOUT
          , P_CLIENT_WIDTH
          , P_CLIENT_ORIGIN
          , P_ENABLE
          , P_HEIGHT
          , P_TITLE 
          , P_VISIBLE
          , P_WIDTH
          , P_POSITION
          , P_SIZE
          , P_SIZER
          , P_CLIENT_SIZE
          , P_ID
          , P_RECT
          , P_CLIENT_RECT
          , P_BEST_SIZE
          , P_WINDOW_STYLE
          , P_EXTRA_STYLE
          , P_SHOWN
          , P_TOP_LEVEL
          , P_ACCEPTS_FOCUS
          , P_ACCEPTS_FOCUS_KEYBOARD
          , P_DEFAULT_ITEM
          , P_CHILDREN
          , P_PARENT
          , P_VALIDATOR
          , P_ACCELERATOR_TABLE
          , P_CAPTURE
          , P_HAS_CAPTURE
          , P_UPDATE_REGION
          , P_BACKGROUND_COLOUR
          , P_FOREGROUND_COLOUR
          , P_FONT
          , P_TOOL_TIP
      };
    };

    class WindowEventHandler : public EventConnector<wxWindow>
                             , public wxEvtHandler
    {
    public:
      void OnChar(wxKeyEvent &event);
      void OnKeyDown(wxKeyEvent &event);
      void OnKeyUp(wxKeyEvent &event);
      void OnCharHook(wxKeyEvent &event);
      void OnActivate(wxActivateEvent &event);
      void OnSetFocus(wxFocusEvent &event);
      void OnKillFocus(wxFocusEvent &event);
      void OnMouseEvents(wxMouseEvent &event);
      void OnMove(wxMoveEvent &event);
      void OnSize(wxSizeEvent &event);
      void OnScrollTop(wxScrollWinEvent& event);
      void OnScrollBottom(wxScrollWinEvent& event);
      void OnScrollLineUp(wxScrollWinEvent& event);
      void OnScrollLineDown(wxScrollWinEvent& event);
      void OnScrollPageUp(wxScrollWinEvent& event);
      void OnScrollPageDown(wxScrollWinEvent& event);
      void OnScrollThumbTrack(wxScrollWinEvent& event);
      void OnScrollThumbRelease(wxScrollWinEvent& event);
      void OnHelp(wxHelpEvent &event);
      void OnLeftDown(wxMouseEvent &event);
      void OnLeftUp(wxMouseEvent &event);
      void OnLeftDClick(wxMouseEvent &event);
      void OnMiddleDown(wxMouseEvent &event);
      void OnMiddleUp(wxMouseEvent &event);
      void OnMiddleDClick(wxMouseEvent &event);
      void OnRightDown(wxMouseEvent &event);
      void OnRightUp(wxMouseEvent &event);
      void OnRightDClick(wxMouseEvent &event);
      void OnMotion(wxMouseEvent &event);
      void OnEnterWindow(wxMouseEvent &event);
      void OnLeaveWindow(wxMouseEvent &event);
      void OnMouseWheel(wxMouseEvent &event);
      static void InitConnectEventMap(void);

    private:
      static void ConnectChar(wxWindow *p, bool connect);
      static void ConnectKeyDown(wxWindow *p, bool connect);
      static void ConnectKeyUp(wxWindow *p, bool connect);
      static void ConnectCharHook(wxWindow *p, bool connect);
      static void ConnectActivate(wxWindow *p, bool connect);
      static void ConnectSetFocus(wxWindow *p, bool connect);
      static void ConnectKillFocus(wxWindow *p, bool connect);
      static void ConnectMouseEvents(wxWindow *p, bool connect);
      static void ConnectMove(wxWindow *p, bool connect);
      static void ConnectSize(wxWindow *p, bool connect);
      static void ConnectScrollTop(wxWindow *p, bool connect);
      static void ConnectScrollBottom(wxWindow *p, bool connect);
      static void ConnectScrollLineUp(wxWindow *p, bool connect);
      static void ConnectScrollLineDown(wxWindow *p, bool connect);
      static void ConnectScrollPageUp(wxWindow *p, bool connect);
      static void ConnectScrollPageDown(wxWindow *p, bool connect);
      static void ConnectScrollThumbTrack(wxWindow *p, bool connect);
      static void ConnectScrollThumbRelease(wxWindow *p, bool connect);
      static void ConnectHelp(wxWindow *p, bool connect);
      static void ConnectLeftDown(wxWindow *p, bool connect);
      static void ConnectLeftUp(wxWindow *p, bool connect);
      static void ConnectLeftDClick(wxWindow *p, bool connect);
      static void ConnectMiddleDown(wxWindow *p, bool connect);
      static void ConnectMiddleUp(wxWindow *p, bool connect);
      static void ConnectMiddleDClick(wxWindow *p, bool connect);
      static void ConnectRightDown(wxWindow *p, bool connect);
      static void ConnectRightUp(wxWindow *p, bool connect);
      static void ConnectRightDClick(wxWindow *p, bool connect);
      static void ConnectMotion(wxWindow *p, bool connect);
      static void ConnectEnterWindow(wxWindow *p, bool connect);
      static void ConnectLeaveWindow(wxWindow *p, bool connect);
      static void ConnectMouseWheel(wxWindow *p, bool connect);
    };
  }; // namespace gui
}; // namespace wxjs

#endif // _WXJSWINDOW_H
