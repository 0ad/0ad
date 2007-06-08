#include "precompiled.h"

/*
 * wxJavaScript - cmnconst.cpp
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
 * $Id: cmnconst.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
// cmnconst.cpp

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif
#include <wx/treectrl.h>

#include "../../common/main.h"
#include "cmnconst.h"

/***
 * <file>misc/cmnconst</file>
 * <module>gui</module>
 * <class name="Common constants" />
 * <constants>
 *  <type name="wxTreeItemIcon">
 *   <desc>
 *    wxTreeItemIcon is used to associated different kind of images
 *    to a treectrl item. wxTreeItemIcon contains the following constants:
 *    See @wxTreeCtrl and @wxTreeItem.
 *   </desc>
 *   <constant name="Normal">Not selected, Not expanded</constant>
 *   <constant name="Selected">Selected, Not expanded</constant>
 *   <constant name="Expanded">Not selected, Expanded</constant>
 *   <constant name="SelectedExpanded">Selected, Expanded</constant>
 *  </type>
 * </constants>
 */
JSConstDoubleSpec wxTreeItemIconMap[] = 
{
    WXJS_CONSTANT(wxTreeItemIcon_, Normal)
    WXJS_CONSTANT(wxTreeItemIcon_, Selected)        
    WXJS_CONSTANT(wxTreeItemIcon_, Expanded)        
    WXJS_CONSTANT(wxTreeItemIcon_, SelectedExpanded)
	{ 0 }
};

void InitCommonConst(JSContext *cx, JSObject *obj)
{
    JSObject *constObj = JS_DefineObject(cx, obj, "wxTreeItemIcon", 
							   NULL, NULL,
							   JSPROP_READONLY | JSPROP_PERMANENT);
    JS_DefineConstDoubles(cx, constObj, wxTreeItemIconMap);
}

