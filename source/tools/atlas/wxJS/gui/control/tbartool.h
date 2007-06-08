/*
 * wxJavaScript - toolbart.h
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
 * $Id$
 */
#ifndef _wxjs_toolbart_h
#define _wxjs_toolbart_h

namespace wxjs
{
  namespace gui
  {
    class ToolBarToolBase : public ApiWrapper<ToolBarToolBase,
                                              wxToolBarToolBase>
    {
    public:
      //static bool GetProperty(wxToolBarToolBase *p,
      //                        JSContext *cx,
      //                        JSObject *obj,
      //                        int id, 
      //                        jsval *vp);
      //static bool SetProperty(wxToolBarToolBase *p,
      //                        JSContext *cx,
      //                        JSObject *obj,
      //                        int id,
      //                        jsval *vp);

      WXJS_DECLARE_METHOD_MAP()
      WXJS_DECLARE_METHOD(enable)
      WXJS_DECLARE_METHOD(toggle)
    };

    class ToolBarToolData : public wxObject
    {
    public:
      ToolBarToolData(JSContext *cx, JSObject *obj) : wxObject()
      {
        data = new JavaScriptClientData(cx, obj, true, false);
      }

      virtual ~ToolBarToolData()
      {
        delete data;
      }

      JSContext* GetContext()
      {
        return data->GetContext();
      }

      JSObject* GetObject()
      {
        return data->GetObject();
      }

    private:
      JavaScriptClientData *data;
    };
  };
};

#endif // _wxjs_toolbart_h