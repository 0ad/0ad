/*
 * wxJavaScript - gauge.h
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
 * $Id: gauge.h 682 2007-04-24 20:38:18Z fbraem $
 */
#ifndef _WXJSGauge_H
#define _WXJSGauge_H

namespace wxjs
{
    namespace gui
    {
      class Gauge: public ApiWrapper<Gauge, wxGauge>
      {
        public:

          static bool GetProperty(wxGauge *p,
                                  JSContext *cx,
                                  JSObject *obj,
                                  int id,
                                  jsval *vp);
          static bool SetProperty(wxGauge *p,
                                  JSContext *cx,
                                  JSObject *obj,
                                  int id,
                                  jsval *vp);
          static bool AddProperty(wxGauge *p, 
                                  JSContext *cx, 
                                  JSObject *obj, 
                                  const wxString &prop, 
                                  jsval *vp);

          static bool DeleteProperty(wxGauge *p, 
                                     JSContext* cx, 
                                     JSObject* obj, 
                                     const wxString &prop);
	        static wxGauge* Construct(JSContext *cx,
                                      JSObject *obj,
                                      uintN argc,
                                      jsval *argv,
                                      bool constructing);
        	
	        WXJS_DECLARE_CONSTANT_MAP()

            WXJS_DECLARE_PROPERTY_MAP()
	        enum
	        {
		        P_BEZEL_FACE
		        , P_RANGE
		        , P_SHADOW_WIDTH
		        , P_VALUE
	        };

            WXJS_DECLARE_METHOD_MAP()
            WXJS_DECLARE_METHOD(create)
        };
    }; // namespace gui
}; // namespace wxjs
#endif //_WXJSGauge_H
