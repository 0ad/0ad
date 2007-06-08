/*
 * wxJavaScript - validate.h
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
 * $Id: validate.h 739 2007-06-08 20:51:47Z fbraem $
 */
#ifndef _WXJSValidator_H
#define _WXJSValidator_H

namespace wxjs
{
    namespace gui
    {
        class Validator : public wxValidator
                        , public ApiWrapper<Validator, wxValidator>
        {
        public:

            Validator();

            Validator(const Validator &copy);

            virtual wxObject *Clone() const
	        {
		        return new Validator(*this);
	        }

	        virtual ~Validator()
	        {
	        }

	        /**
	         * Validate forwards the validation to the validate method of the JavaScript object.
	         */
	        virtual bool Validate(wxWindow *parent);

	        /**
	         * TransferToWindow forwards this to the transferToWindow method of the JavaScript object.
	         */
	        virtual bool TransferToWindow();

	        /**
	         * TransferFromWindow forwards this to the transferFromWindow method of the JavaScript object.
	         */
	        virtual bool TransferFromWindow();

	        static bool GetProperty(wxValidator *p,
                                    JSContext *cx, 
                                    JSObject *obj, 
                                    int id, 
                                    jsval *vp);
	        static bool SetProperty(wxValidator *p,
                                    JSContext *cx,
                                    JSObject *obj,
                                    int id,
                                    jsval *vp);

	        static wxValidator* Construct(JSContext *cx,
                                          JSObject *obj,
                                          uintN argc,
                                          jsval *argv,
                                          bool constructing);

	        WXJS_DECLARE_PROPERTY_MAP()

	        /**
	         * Property Ids.
	         */
	        enum
	        {
		        P_WINDOW
		        , P_BELL_ON_ERROR
	        };
        };
    }; // namespace gui
}; // namespace wxjs

#endif //_WXJSValidator_H
