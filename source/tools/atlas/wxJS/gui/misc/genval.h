/*
 * wxJavaScript - genval.h
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
 * $Id: genval.h 598 2007-03-07 20:13:28Z fbraem $
 */
#include <wx/valgen.h>

/////////////////////////////////////////////////////////////////////////////
// Name:        genval.h
// Purpose:		GenericValidator ports wxGenericValidator to JavaScript 
// Author:      Franky Braem
// Modified by:
// Created:     29.01.02
// Copyright:   (c) 2001-2002 Franky Braem
// Licence:     LGPL
/////////////////////////////////////////////////////////////////////////////

namespace wxjs
{
    namespace gui
    {
        class GenericValidator : public wxGenericValidator
                                   , public ApiWrapper<GenericValidator, GenericValidator>
                                   , public Object
        {
	        DECLARE_CLASS(GenericValidator)
        public:

            GenericValidator(JSContext *cx, JSObject *obj, bool val);
            GenericValidator(JSContext *cx, JSObject *obj, int val);
            GenericValidator(JSContext *cx, JSObject *obj, wxString val);
            GenericValidator(JSContext *cx, JSObject *obj, wxArrayInt val);

	        GenericValidator(const GenericValidator &copy);
	        virtual wxObject* Clone() const;
	        virtual ~GenericValidator()
	        {
	        }

	        /**
	         * Validate forwards the validation to the validate method of the JavaScript object.
	         */
	        virtual bool Validate(wxWindow *parent);

	        static bool GetProperty(wxGenericValidator *p, JSContext *cx, JSObject *obj, int id, jsval *vp);

	        static GenericValidator* Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing);

	        WXJS_DECLARE_PROPERTY_MAP()

	        /**
	         * Property Ids.
	         */
	        enum
	        {
		        P_VALUE
	        };

        private:
          bool       m_boolValue;
          int        m_intValue;
          wxString   m_stringValue;
          wxArrayInt m_arrayIntValue;
        };
    }; // namespace gui
}; // namespace wxjs
