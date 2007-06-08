/*
 * wxJavaScript - toolbar.h
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
 * $Id: toolbar.h 698 2007-05-10 20:57:32Z fbraem $
 */
#ifndef _WXJSToolBar_H
#define _WXJSToolBar_H

namespace wxjs
{
    namespace gui
    {
      class ToolBar : public ApiWrapper<ToolBar, wxToolBar>
                    , public wxToolBar
      {
      public:

        ToolBar() : wxToolBar()
        {
        }

        virtual ~ToolBar()
        {
          ClearTools();
        }

        // Derived because the tool is rooted, and the tool must be unrooted
        // before it is destroyed
        virtual bool DoDeleteTool(size_t pos, wxToolBarToolBase *tool);

        static bool GetProperty(wxToolBar *p,
                                JSContext *cx,
                                JSObject *obj,
                                int id, 
                                jsval *vp);
        static bool SetProperty(wxToolBar *p,
                                JSContext *cx,
                                JSObject *obj,
                                int id,
                                jsval *vp);

        static wxToolBar* Construct(JSContext *cx,
                                  JSObject *obj,
                                  uintN argc,
                                  jsval *argv,
                                  bool constructing);

        WXJS_DECLARE_PROPERTY_MAP()
        enum
        {
            P_TOOL_SIZE
            , P_TOOL_BITMAP_SIZE
            , P_MARGINS
            , P_TOOL_PACKING
            , P_TOOL_SEPARATION
        };

        WXJS_DECLARE_CONSTANT_MAP()

        WXJS_DECLARE_METHOD_MAP()
        WXJS_DECLARE_METHOD(create)
        WXJS_DECLARE_METHOD(addControl)
        WXJS_DECLARE_METHOD(addSeparator)
        WXJS_DECLARE_METHOD(addTool)
        WXJS_DECLARE_METHOD(addCheckTool)
        WXJS_DECLARE_METHOD(addRadioTool)
        WXJS_DECLARE_METHOD(deleteTool)
        WXJS_DECLARE_METHOD(deleteToolByPos)
        WXJS_DECLARE_METHOD(enableTool)
        WXJS_DECLARE_METHOD(findControl)
        WXJS_DECLARE_METHOD(getToolEnabled)
        WXJS_DECLARE_METHOD(getToolLongHelp)
        WXJS_DECLARE_METHOD(getToolShortHelp)
        WXJS_DECLARE_METHOD(getToolState)
        WXJS_DECLARE_METHOD(insertControl)
        WXJS_DECLARE_METHOD(insertSeparator)
        WXJS_DECLARE_METHOD(insertTool)
        WXJS_DECLARE_METHOD(realize)
        WXJS_DECLARE_METHOD(setToolLongHelp)
        WXJS_DECLARE_METHOD(setToolShortHelp)
        WXJS_DECLARE_METHOD(toggleTool)

      };

      class ToolEventHandler : public EventConnector<wxToolBar>
                              , public wxEvtHandler
      {
      public:
        // Events
        void OnTool(wxCommandEvent &event);
      };
    }; // namespace gui
}; // namespace wxjs

#endif //_WXJSToolBar_H
