#include "precompiled.h"

/*
 * wxJavaScript - treeitem.cpp
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
 * $Id: treeitem.cpp 784 2007-06-25 18:34:22Z fbraem $
 */
// wxJSTreeItem.cpp

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <wx/treectrl.h>

#include "../../common/main.h"

#include "treectrl.h"
#include "treeid.h"
#include "treeitem.h"

#include "../misc/colour.h"
#include "../misc/font.h"

using namespace wxjs;
using namespace wxjs::gui;

/***
 * <file>control/treeitem</file>
 * <module>gui</module>
 * <class name="wxTreeItem">
 *  wxTreeItem is not available in wxWidgets. It's main purpose is to concentrate
 *  item specific properties and methods in this class which will make it easier to work with
 *  tree items.
 *  See @wxTreeCtrl, @wxTreeItemId and @wxTreeCtrl#getItem
 * </class>
 */
WXJS_INIT_CLASS(TreeItem, "wxTreeItem", 0)

/***
 * <properties>
 *  <property name="allChildrenCount" type="Integer" readonly="Y">
 *   Gets the number of child items. It returns the total number
 *   of descendants.
 *  </property>
 *  <property name="backgroundColour" type="@wxColour">
 *   Get/Set the background colour of the item.
 *  </property>
 *  <property name="bold" type="Boolean">
 *   Get/Set the item text in bold.
 *  </property>
 *  <property name="childrenCount" type="Integer" readonly="Y">
 *   Gets the number of child items. Only the children of one level is returned.
 *  </property>
 *  <property name="data" type="Any">
 *   Get/Set the associated data of the item.
 *  </property>
 *  <property name="expanded" type="Boolean">
 *   Returns true when the item is expanded. When set to true
 *   the items is expanded, collapsed when set to false.
 *  </property>
 *  <property name="font" type="@wxFont">
 *   Get/Set the font of the item.
 *  </property>
 *  <property name="hasChildren" type="Boolean">
 *   Returns true when the item has children. When set
 *   to true you can force the appearance of the button next to the item.
 *   This is useful to allow the user to expand the items which don't have 
 *   any children now, but instead adding them only when needed, thus minimizing 
 *   memory usage and loading time.
 *  </property>
 *  <property name="lastChild" type="@wxTreeItem" readonly="Y">
 *   Get the last child item. Returns null when the item has no childs.
 *  </property>
 *  <property name="nextSibling" type="@wxTreeItem" readonly="Y">
 *   Get the next sibling item. Returns null when there are no items left.
 *  </property>
 *  <property name="parent" type="@wxTreeItem" readonly="Y">
 *   Get the parent item. Returns null when this item is the root item.
 *  </property>
 *  <property name="prevSibling" type="@wxTreeItem" readonly="Y">
 *   Get the previous sibling item. Returns null when there are no items left.
 *  </property>
 *  <property name="selected" type="Boolean">
 *   Returns true when the item is selected. When set to true
 *   the items is selected, unselected when set to false.
 *  </property>
 *  <property name="text" type="String">
 *   Get/Set the item text.
 *  </property>
 *  <property name="textColour" type="@wxColour">
 *  Get/Set the colour of the text.
 *  </property>
 *  <property name="visible" type="Boolean">
 *   Get/Set the visibility of the item. Setting visible to false
 *   doesn't do anything.
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(TreeItem)
    WXJS_PROPERTY(P_BOLD, "bold")
    WXJS_PROPERTY(P_DATA, "data")
    WXJS_PROPERTY(P_TEXT_COLOUR, "textColour")
    WXJS_PROPERTY(P_TEXT, "text")
    WXJS_PROPERTY(P_FONT, "font")
    WXJS_PROPERTY(P_HAS_CHILDREN, "hasChildren")
    WXJS_PROPERTY(P_VISIBLE, "visible")
    WXJS_PROPERTY(P_EXPANDED, "expanded")
    WXJS_PROPERTY(P_SELECTED, "selected")
    WXJS_READONLY_PROPERTY(P_CHILDREN_COUNT, "childrenCount")
    WXJS_READONLY_PROPERTY(P_ALL_CHILDREN_COUNT, "allChildrenCount")
    WXJS_READONLY_PROPERTY(P_PARENT, "parent")
    WXJS_READONLY_PROPERTY(P_LAST_CHILD, "lastChild")
    WXJS_READONLY_PROPERTY(P_PREV_SIBLING, "prevSibling")
    WXJS_READONLY_PROPERTY(P_NEXT_SIBLING, "nextSibling")
WXJS_END_PROPERTY_MAP()

bool TreeItem::GetProperty(wxTreeItemId *p,
                           JSContext *cx,
                           JSObject *obj,
                           int id,
                           jsval *vp)
{
    JSObject *objParent = JS_GetParent(cx, obj);
    wxTreeCtrl *tree = TreeCtrl::GetPrivate(cx, objParent);
    if ( tree == NULL )
        return false;

    switch (id)
    {
    case P_TEXT:
        *vp = ToJS(cx, tree->GetItemText(*p));
        break;
    case P_DATA:
        {
            ObjectTreeData *data = (ObjectTreeData*) tree->GetItemData(*p);
            *vp = data->GetJSVal();
            break;
        }
    case P_TEXT_COLOUR:
        *vp = Colour::CreateObject(cx, new wxColour(tree->GetItemTextColour(*p)));
        break;
    case P_BACKGROUND_COLOUR:
        *vp = Colour::CreateObject(cx, new wxColour(tree->GetItemBackgroundColour(*p)));
        break;
    case P_FONT:
        *vp = Font::CreateObject(cx, new wxFont(tree->GetItemFont(*p)));
        break;
    case P_HAS_CHILDREN:
        *vp = ToJS(cx, tree->GetChildrenCount(*p, FALSE) != 0);
        break;
    case P_BOLD:
        *vp = ToJS(cx, tree->IsBold(*p));
        break;
    case P_VISIBLE:
        *vp = ToJS(cx, tree->IsVisible(*p));
        break;
    case P_EXPANDED:
        *vp = ToJS(cx, tree->IsExpanded(*p));
        break;
    case P_SELECTED:
        *vp = ToJS(cx, tree->IsSelected(*p));
        break;
    case P_CHILDREN_COUNT:
        *vp = ToJS(cx, tree->GetChildrenCount(*p, false));
        break;
    case P_ALL_CHILDREN_COUNT:
        *vp = ToJS(cx, tree->GetChildrenCount(*p));
        break;
    case P_PARENT:
        {
            wxTreeItemId parentId = tree->GetItemParent(*p);
            if ( parentId.IsOk() )
                *vp = TreeItem::CreateObject(cx, new wxTreeItemId(parentId));
            break;
        }
    case P_LAST_CHILD:
        {
            wxTreeItemId childId = tree->GetLastChild(*p);
            if ( childId.IsOk() )
                *vp = TreeItem::CreateObject(cx, new wxTreeItemId(childId));
            break;
        }
    case P_PREV_SIBLING:
        {
            wxTreeItemId prevId = tree->GetPrevSibling(*p);
            if ( prevId.IsOk() )
                *vp = TreeItem::CreateObject(cx, new wxTreeItemId(prevId));
            break;
        }
    case P_NEXT_SIBLING:
        {
            wxTreeItemId nextId = tree->GetNextSibling(*p);
            if ( nextId.IsOk() )
                *vp = TreeItem::CreateObject(cx, new wxTreeItemId(nextId));
            break;
        }
    }
    return true;
}

bool TreeItem::SetProperty(wxTreeItemId *p,
                           JSContext *cx,
                           JSObject *obj,
                           int id,
                           jsval *vp)
{
    JSObject *objParent = JS_GetParent(cx, obj);
    wxTreeCtrl *tree = TreeCtrl::GetPrivate(cx, objParent);
    if ( tree == NULL )
        return false;

    switch (id)
    {
    case P_TEXT:
        {
            wxString text;
            FromJS(cx, *vp, text);
            tree->SetItemText(*p, text);
            break;
        }
    case P_DATA:
        tree->SetItemData(*p, new ObjectTreeData(cx, *vp));
        break;
    case P_TEXT_COLOUR:
        {
            wxColour *colour = Colour::GetPrivate(cx, *vp);
            if ( colour != NULL )
                tree->SetItemTextColour(*p, *colour);
        }
        break;
    case P_BACKGROUND_COLOUR:
        {
            wxColour *colour = Colour::GetPrivate(cx, *vp);
            if ( colour != NULL )
                tree->SetItemBackgroundColour(*p, *colour);
        }
        break;
    case P_FONT:
        {
            wxFont *font = Font::GetPrivate(cx, *vp);
            if ( font != NULL )
                tree->SetItemFont(*p, *font);
        }
        break;
    case P_HAS_CHILDREN:
        {
            bool children;
            if ( FromJS(cx, *vp, children) )
                tree->SetItemHasChildren(*p, children);
        }
        break;
    case P_BOLD:
        {
            bool bold;
            if ( FromJS(cx, *vp, bold) )
                tree->SetItemBold(*p, bold);
        }
        break;
    case P_EXPANDED:
        {
            bool expand;
            if ( FromJS(cx, *vp, expand) )
            {
                if ( expand )
                    tree->Expand(*p);
                else
                    tree->Collapse(*p);
            }
        }
        break;
    case P_VISIBLE:
        {
            bool visible;
            if (    FromJS(cx, *vp, visible) 
                 && visible )
            {
               tree->EnsureVisible(*p);
            }
        }
        break;
    case P_SELECTED:
        {
            bool select;
            if ( FromJS(cx, *vp, select) )
            {
                if ( select )
                    tree->SelectItem(*p);
                else
                    tree->Unselect();
            }
        }
        break;
    }
    return true;
}

WXJS_BEGIN_METHOD_MAP(TreeItem)
    WXJS_METHOD("getImage", getImage, 1)
    WXJS_METHOD("setImage", setImage, 2)
    WXJS_METHOD("getFirstChild", getFirstChild, 1)
    WXJS_METHOD("getNextChild", getNextChild, 1)
WXJS_END_METHOD_MAP()

/***
 * <method name="getImage">
 *  <function returns="Integer">
 *   <arg name="Which" type="Integer">
 *    A @wxTreeItemIcon constant.
 *   </arg>
 *  </function>
 *  <desc>
 *   Gets the specified item image.
 *   See @wxTreeCtrl @wxTreeCtrl#getItemImage method
 *  </desc>
 * </method>
 */
JSBool TreeItem::getImage(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxTreeItemId *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    JSObject *objParent = JS_GetParent(cx, obj);
    wxTreeCtrl *tree = TreeCtrl::GetPrivate(cx, objParent);
    if ( tree == NULL )
        return JS_FALSE;

    int which;
    if ( FromJS(cx, argv[0], which) )
    {
        *rval = ToJS(cx, tree->GetItemImage(*p, (wxTreeItemIcon) which));
        return JS_TRUE;
    }
   
    return JS_FALSE;
}

/***
 * <method name="setImage">
 *  <function>
 *   <arg name="Image" type="Integer">
 *    Index of the image in the image list
 *   </arg>
 *   <arg name="Which" type="Integer">
 *    A @wxTreeItemIcon constant.
 *   </arg>
 *  </function>
 *  <desc>
 *   Sets the specified item image.
 *   See @wxTreeCtrl @wxTreeCtrl#setItemImage method
 *  </desc>
 * </method>
 */
JSBool TreeItem::setImage(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxTreeItemId *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    wxTreeCtrl *tree = TreeCtrl::GetPrivate(cx, JS_GetParent(cx, obj));
    if ( tree == NULL )
        return JS_FALSE;

    int image;
    int which;
    if (    FromJS(cx, argv[0], image)
         && FromJS(cx, argv[1], which) )
    {
        tree->SetItemImage(*p, image, (wxTreeItemIcon) which);
        return JS_TRUE;
    }
   
    return JS_FALSE;
}

/***
 * <method name="getFirstChild">
 *  <function returns="@wxTreeItem">
 *   <arg name="Cookie" type="Integer">
 *    For this enumeration function you must pass in a 'cookie' parameter 
 *    which is opaque for the application but is necessary for the library 
 *    to make these functions reentrant (i.e. allow more than one enumeration 
 *    on one and the same object simultaneously). The cookie passed 
 *    to getFirstChild and @wxTreeItem#getNextChild should be the same variable.
 *   </arg>
 *  </function>
 *  <desc>
 *   Returns the first child item. An invalid item is returned when there is no child item.
 *   See @wxTreeCtrl @wxTreeCtrl#getFirstChild method
 *  </desc>
 * </method>
 */
JSBool TreeItem::getFirstChild(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxTreeItemId *p = TreeItemId::GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    wxTreeCtrl *tree = TreeCtrl::GetPrivate(cx, JS_GetParent(cx, obj));
    if ( tree == NULL )
        return JS_FALSE;

    long cookie;
    if ( FromJS(cx, argv[0], cookie) )
    {
		wxTreeItemIdValue newCookie = (wxTreeItemIdValue) cookie;
        *rval = TreeItem::CreateObject(cx, new wxTreeItemId(tree->GetFirstChild(*p, newCookie)));
        return JS_TRUE;
    }
    return JS_FALSE;

}

/***
 * <method name="getNextChild">
 *  <function returns="@wxTreeItem">
 *   <arg name="Cookie" type="Integer">
 *    For this enumeration function you must pass in a 'cookie' parameter 
 *    which is opaque for the application but is necessary for the library 
 *    to make these functions reentrant (i.e. allow more than one enumeration 
 *    on one and the same object simultaneously). The cookie passed 
 *    to @wxTreeItem @wxTreeItem#getFirstChild and getNextChild should be the same variable.
 *   </arg>
 *  </function>
 *  <desc>
 *   Returns the first child item. An invalid item is returned when there is no child item.
 *   See @wxTreeCtrl @wxTreeCtrl#getNextChild method
 *  </desc>
 * </method>
 */
JSBool TreeItem::getNextChild(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxTreeItemId *p = TreeItemId::GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    wxTreeCtrl *tree = TreeCtrl::GetPrivate(cx, JS_GetParent(cx, obj));
    if ( tree == NULL )
        return JS_FALSE;

    long cookie;
    if ( FromJS(cx, argv[0], cookie) )
    {
		wxTreeItemIdValue newCookie = (wxTreeItemIdValue) cookie;
        *rval = TreeItem::CreateObject(cx, new wxTreeItemId(tree->GetNextChild(*p, newCookie)));
        return JS_TRUE;
    }
    return JS_FALSE;

}
