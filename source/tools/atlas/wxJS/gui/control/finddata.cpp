#include "precompiled.h"

/*
 * wxJavaScript - finddata.cpp
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
 * $Id: finddata.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
// finddata.cpp

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../../common/main.h"

#include "finddata.h"

using namespace wxjs;
using namespace wxjs::gui;

/***
 * <file>control/finddata</file>
 * <module>gui</module>
 * <class name="wxFindReplaceData">
 *   wxFindReplaceData holds the data for @wxFindReplaceDialog.
 *	 It is used to initialize the dialog with the default values and 
 *   will keep the last values from the dialog when it is closed.
 *   It is also updated each time a @wxFindDialogEvent is generated so 
 *   instead of using the @wxFindDialogEvent methods 
 *   you can also directly query this object.
 * </class>
 */
WXJS_INIT_CLASS(FindReplaceData, "wxFindReplaceData", 0)

/***
 * <properties>
 *  <property name="findString" type="String">
 *   Get/Set the string to find
 *  </property>
 *  <property name="flags" type="Integer">
 *   Get/Set the flags. 
 *  </property>
 *  <property name="replaceString" type="String">
 *   Get/Set the replacement string
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(FindReplaceData)
  WXJS_PROPERTY(P_FINDSTRING, "findString")
  WXJS_PROPERTY(P_REPLACESTRING, "replaceString")
  WXJS_PROPERTY(P_FLAGS, "flags")
WXJS_END_PROPERTY_MAP()

bool FindReplaceData::GetProperty(wxFindReplaceData *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    switch(id) 
	{
	case P_FLAGS:
		*vp = ToJS(cx, p->GetFlags());
		break;
	case P_FINDSTRING:
		*vp = ToJS(cx, p->GetFindString());
		break;
	case P_REPLACESTRING:
		*vp = ToJS(cx, p->GetReplaceString());
		break;
    }
    return true;
}

bool FindReplaceData::SetProperty(wxFindReplaceData *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    switch(id) 
	{
	case P_FLAGS:
		{
			int flag;
			if ( FromJS(cx, *vp, flag) )
				p->SetFlags(flag);
			break;
		}
	case P_FINDSTRING:
		{
			wxString str;
			FromJS(cx, *vp, str);
			p->SetFindString(str);
			break;
		}
	case P_REPLACESTRING:
		{
			wxString str;
			FromJS(cx, *vp, str);
			p->SetReplaceString(str);
			break;
		}
	}
    return true;
}

/***
 * <constants>
 *  <type name="Flags">
 *   <constant name="FR_DOWN" />
 *   <constant name="FR_WHOLEWORD" />
 *   <constant name="FR_MATCHCASE" />
 *  </type>
 * </constants>
 */
WXJS_BEGIN_CONSTANT_MAP(FindReplaceData)
  WXJS_CONSTANT(wx, FR_DOWN)
  WXJS_CONSTANT(wx, FR_WHOLEWORD)
  WXJS_CONSTANT(wx, FR_MATCHCASE)
WXJS_END_CONSTANT_MAP()

/***
 * <ctor>
 *  <function>
 *   <arg name="Flags" type="Integer" default="0" />
 *  </function>
 *  <desc>
 *   Constructs a new wxFindReplaceData object.
 *  </desc>
 * </ctor>
 */
wxFindReplaceData* FindReplaceData::Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing)
{
    if ( argc == 0 )
        return new wxFindReplaceData();
    else
    {
	    int flags = 0;
        if ( FromJS(cx, argv[0], flags) )
            return new wxFindReplaceData(flags);
    }
    
    return NULL;
}
