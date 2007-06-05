#include "precompiled.h"

/*
 * wxJavaScript - treeid.cpp
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
 * $Id: treeid.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <wx/treectrl.h>
/**
 * @if JS
 *  @page wxTreeItemId wxTreeItemId
 *  @since version 0.6
 * @endif
 */

#include "../../common/main.h"

#include "treeid.h"

using namespace wxjs;
using namespace wxjs::gui;

/***
 * <file>control/treeid</file>
 * <module>gui</module>
 * <class name="wxTreeItemId">
 *  wxTreeItemId identifies an element of the tree.
 *  See @wxTreeCtrl and @wxTreeEvent
 * </class>
 */
WXJS_INIT_CLASS(TreeItemId, "wxTreeItemId", 0)

/***
 * <properties>
 *  <property name="ok" type="Boolean" readonly="Y">
 *   Returns true when the item is valid.
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(TreeItemId)
    WXJS_READONLY_PROPERTY(P_OK, "ok")
WXJS_END_PROPERTY_MAP()

bool TreeItemId::GetProperty(wxTreeItemId *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    if ( id == P_OK )
    {
        *vp = ToJS(cx, p->IsOk());
    }
    return true;
}
