#include "precompiled.h"

/*
 * wxJavaScript - treehit.cpp
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
 * $Id: treehit.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../../common/main.h"
#include "treectrl.h"
#include "treeid.h"
#include "treeitem.h"
#include "treehit.h"

using namespace wxjs;
using namespace wxjs::gui;

/***
 * <file>control/treehit</file>
 * <module>gui</module>
 * <class name="wxTreeHitTest">
 *  Helper class for returning information from wxTreeCtrl @wxTreeCtrl#hitTest.
 *  This class is specific for wxJavaScript. It doesn't exist in wxWindows.
 * </class>
 */
WXJS_INIT_CLASS(TreeHitTest, "wxTreeHitTest", 0)

/***
 * <properties>
 *  <property name="item" type="Integer" readonly="Y">
 *   Get the item.
 *  </property>
 *  <property name="flags" type="Integer" readonly="Y">
 *   Get the flags. These flags give details about the position and the tree control item.
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(TreeHitTest)
    WXJS_READONLY_PROPERTY(P_ITEM, "item")
    WXJS_READONLY_PROPERTY(P_FLAGS, "flags")
WXJS_END_PROPERTY_MAP()

bool TreeHitTest::GetProperty(wxTreeHitTest *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    switch (id)
    {
    case P_ITEM:
        *vp = TreeItemId::CreateObject(cx, new wxTreeItemId(p->GetItem()));
        break;
    case P_FLAGS:
        *vp = ToJS(cx, p->GetFlags());
        break;
    }
    return true;
}

/***
 * <constants>
 *  <type name="flags">
 *   <constant name="ABOVE">Above the client area.</constant>
 *   <constant name="BELOW">Below the client area.</constant>
 *   <constant name="NOWHERE">In the client area but below the last item.</constant>
 *   <constant name="ONITEMBUTTON">On the button associated with an item.</constant>
 *   <constant name="ONITEMICON">On the bitmap associated with an item.</constant>
 *   <constant name="ONITEMINDENT">In the indentation associated with an item.</constant>
 *   <constant name="ONITEMLABEL">On the label (string) associated with an item.</constant>
 *   <constant name="ONITEMRIGHT">In the area to the right of an item.</constant>
 *   <constant name="ONITEMSTATEICON">On the state icon for a tree view item that is in a user-defined state.</constant>
 *   <constant name="TOLEFT">To the left of the client area.</constant>
 *   <constant name="TORIGHT">To the right of the client area.</constant>
 *   <constant name="ONITEM">(ONITEMICON | ONITEMLABEL)</constant>
 *  </type>
 * </constants>
 */
WXJS_BEGIN_CONSTANT_MAP(TreeHitTest)
    WXJS_CONSTANT(wxTREE_HITTEST_, ABOVE)
    WXJS_CONSTANT(wxTREE_HITTEST_, BELOW)
    WXJS_CONSTANT(wxTREE_HITTEST_, NOWHERE)
    WXJS_CONSTANT(wxTREE_HITTEST_, ONITEMBUTTON)
    WXJS_CONSTANT(wxTREE_HITTEST_, ONITEMICON)
    WXJS_CONSTANT(wxTREE_HITTEST_, ONITEMINDENT)
    WXJS_CONSTANT(wxTREE_HITTEST_, ONITEMLABEL)
    WXJS_CONSTANT(wxTREE_HITTEST_, ONITEMRIGHT)
    WXJS_CONSTANT(wxTREE_HITTEST_, ONITEMSTATEICON)
    WXJS_CONSTANT(wxTREE_HITTEST_, TOLEFT)
    WXJS_CONSTANT(wxTREE_HITTEST_, TORIGHT)
    WXJS_CONSTANT(wxTREE_HITTEST_, ONITEMUPPERPART)
    WXJS_CONSTANT(wxTREE_HITTEST_, ONITEMLOWERPART)
    WXJS_CONSTANT(wxTREE_HITTEST_, ONITEM)
WXJS_END_CONSTANT_MAP()
