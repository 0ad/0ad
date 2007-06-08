/*
 * wxJavaScript - fontenum.h
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
 * $Id: fontenum.h 715 2007-05-18 20:38:04Z fbraem $
 */
#ifndef _WXJSFontEnumerator_H
#define _WXJSFontEnumerator_H

#include <wx/fontenum.h>

namespace wxjs
{
    namespace gui
    {

        class FontEnumerator : public wxFontEnumerator
                             , public ApiWrapper<FontEnumerator, FontEnumerator>
        {
        public:
	        /**
	         * Constructor
	         */
	        FontEnumerator(JSContext *cx, JSObject *obj);

	        /**
	         * Destructor
	         */
	        virtual ~FontEnumerator();

            static bool GetStaticProperty(JSContext *cx, int id, jsval *vp);

	        static FontEnumerator *Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing);
        	
	        static JSBool enumerateFacenames(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool enumerateEncodings(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

	        bool OnFacename(const wxString &faceName);

	        bool OnFontencoding(const wxString &faceName, const wxString &encoding);

            WXJS_DECLARE_STATIC_PROPERTY_MAP()
	        WXJS_DECLARE_METHOD_MAP()

	        /**
	         * Property Ids.
	         */
	        enum
	        {
		        P_FACENAMES
		        , P_ENCODINGS
	        };
        private:
          JavaScriptClientData *m_data;
        };
    }; // namespace gui
}; // namespace wxjs

#endif //_WXJSFontEnumerator_H
