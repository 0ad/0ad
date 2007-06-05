#include "precompiled.h"

/*
 * wxJavaScript - textval.cpp
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
 * $Id: textval.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
// TextValidator.cpp

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../../common/main.h"
#include "../../common/type.h"

#include "validate.h"
#include "textval.h"
using namespace wxjs;
using namespace wxjs::gui;

TextValidator::TextValidator(long style,
                                     const wxString &value) : wxTextValidator(style)
{
    m_value = value;
    m_stringValue = &m_value;
}

/***
 * <file>misc/textval</file>
 * <module>gui</module>
 * <class name="wxTextValidator" prototype="@wxValidator">
 *  wxTextValidator validates text controls, providing a variety of filtering behaviours.
 *  The following example shows a dialog with a textfield. It's only allowed to enter numeric
 *  characters. The textfield is initialised using the validator.
 *  <pre><code class="whjs">
 *   function init()
 *   {
 *     var dlg = new wxDialog(null, -1);
 *     var txtField = new wxTextCtrl(dlg, -1);
 *
 *     txtField.validator = new wxTextValidator(wxFilter.NUMERIC, "10");
 *
 *     dlg.fit();
 *     dlg.showModal();
 *
 *     wxMessageBox("You have entered: " + txtField.value);
 *
 *     return false;
 *   }
 *
 *   wxTheApp.onInit = init;
 *   wxTheApp.mainLoop();
 *  </code></pre>
 * </class>
 */
WXJS_INIT_CLASS(TextValidator, "wxTextValidator", 0)

/***
 * <properties>
 *  <property name="excludes" type="Array">
 *   Get/Set an array of invalid values.
 *  </property>
 *  <property name="includes" type="Array">
 *   Get/Set an array of valid values.
 *  </property>
 *  <property name="style" type="Integer">
 *   Get/Set the filter style. See @wxFilter
 *  </property>
 *  <property name="value" type="String" readonly="Y">
 *   Get the value entered in the textfield.
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(TextValidator)
    WXJS_PROPERTY(P_EXCLUDE_LIST, "excludes")
    WXJS_PROPERTY(P_INCLUDE_LIST, "includes")
    WXJS_PROPERTY(P_STYLE, "style")
    WXJS_READONLY_PROPERTY(P_STYLE, "value")
WXJS_END_PROPERTY_MAP()

bool TextValidator::GetProperty(wxTextValidator *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    switch (id)
    {
    case P_EXCLUDE_LIST:
        *vp = ToJS(cx, p->GetExcludes());
        break;
    case P_INCLUDE_LIST:
        *vp = ToJS(cx, p->GetIncludes());
        break;
    case P_STYLE:
        *vp = ToJS(cx, p->GetStyle());
        break;
    case P_VALUE:
        {
            TextValidator *val = dynamic_cast<TextValidator *>(p);
            if ( val )
                *vp = ToJS(cx, val->m_value);
        }
        break;
    }
    return true;
}

bool TextValidator::SetProperty(wxTextValidator *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    switch (id)
    {
    case P_EXCLUDE_LIST:
        {
			wxArrayString list;
            if ( FromJS(cx, *vp, list) )
                p->SetExcludes(list);
        }
        break;
    case P_INCLUDE_LIST:
        {
            wxArrayString list;
            if ( FromJS(cx, *vp, list) )
                p->SetIncludes(list);
        }
        break;
    case P_STYLE:
        {
            long style;
            if ( FromJS(cx, *vp, style) )
                p->SetStyle(style);
        }
        break;
    }
    return true;
}

/***
 * <ctor>
 *  <function>
 *   <arg name="Style" type="Integer" default="wxFilter.NONE" />
 *   <arg name="Value" type="String" default="">
 *    Value used to initialize the textfield.
 *   </arg>
 *  </function>
 *  <desc>
 *   Constructs a new wxTextValidator object.
 *  </desc>
 * </ctor>
 */
wxTextValidator* TextValidator::Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing)
{
    long filter = wxFILTER_NONE;

    wxTextValidator *p = NULL;
    switch(argc)
    {
    case 0:
        p = new TextValidator();
        break;
    case 1:
        if ( FromJS(cx, argv[0], filter) )
        {
            p = new TextValidator(filter);
        }
        break;
    case 2:
        if ( FromJS(cx, argv[0], filter) )
        {
            wxString value;
            FromJS(cx, argv[1], value);
            p = new TextValidator(filter, value);
        }
        break;
    }

    return p;
}
