#include "precompiled.h"

/*
 * wxJavaScript - sizer.cpp
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
 * $Id: sizer.cpp 810 2007-07-13 20:07:05Z fbraem $
 */
// sizer.cpp

#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif

#include "../../common/main.h"
#include "../../common/apiwrap.h"
#include "../../ext/wxjs_ext.h"

#include "../control/window.h"

#include "sizer.h"
#include "size.h"

using namespace wxjs;
using namespace wxjs::gui;

/***
 * <file>misc/sizer</file>
 * <module>gui</module>
 * <class name="wxSizer">
 *	wxSizer is the prototype for all sizer objects :
 *	@wxBoxSizer, @wxFlexGridSizer, @wxGridSizer and @wxStaticBoxSizer.
 *	See also following wxWindow properties: @wxWindow#sizer, and @wxWindow#autoLayout.
 *  <br /><br />
 *  The example will show a dialog box that asks the user to enter a name and a password. 
 *  The labels, text controls and buttons are placed dynamically on the dialog using sizers.
 *  <pre><code class="whjs">
 *    // The wxSizer example
 *    wxTheApp.onInit = init;
 *   
 *    function init()
 *    {
 *      //Create a dialog box
 *
 *      var dlg = new wxDialog(null, -1, "wxSizer Example",
 *                             wxDefaultPosition, new wxSize(200, 150));
 *      // Create a wxBoxSizer for the dialog. 
 *      // The main direction of this sizer is vertical. 
 *      // This means that when an item is added to this sizer, it will be added below the previous item.
 *
 *      dlg.sizer = new wxBoxSizer(wxOrientation.VERTICAL);
 *  
 *      // Use a wxFlexGridSizer to layout the labels with their corresponding text controls.
 *      // The sizer is created with 2 rows and 2 columns. The first column is used for the label, 
 *      // while the second column is used for the text controls. The space between the rows and 
 *      // columns is set to 10.
 *
 *      var sizer1 = new wxFlexGridSizer(2, 2, 10, 10);
 *
 *      // Add the labels and text controls to sizer1. A label is centered vertically. 
 *      // A grid sizer is filled from left to right, top to bottom.
 *
 *      sizer1.add(new wxStaticText(dlg, -1, "Username"), 0, wxAlignment.CENTER_VERTICAL);
 *      sizer1.add(new wxTextCtrl(dlg, -1, "&lt;user&gt;"));
 *      sizer1.add(new wxStaticText(dlg, -1, "Password"), 0, wxAlignment.CENTER_VERTICAL);
 *      sizer1.add(new wxTextCtrl(dlg, -1, "&lt;pwd&gt;"));
 *
 *      // Add this sizer to the sizer of the dialog. The flag argument is 0 which means 
 *      // that this item is not allowed to grow in the main direction (which is vertically).
 *      // The item is centered. Above (wxDirection.TOP) and below (wxDirection.BOTTOM) the item, 
 *      // a border is created with a size of 10.
 *
 *      dlg.sizer.add(sizer1, 0, wxAlignment.CENTER | wxDirection.TOP | wxDirection.BOTTOM, 10);
 *
 *      // Create a new wxBoxSizer and assign it to sizer1. The main direction of this sizer 
 *      // is horizontally. This means that when an item is added it will be shown on the same row.
 *
 *      sizer1 = new wxBoxSizer(wxOrientation.HORIZONTAL);
 *
 *      // Add the Ok button to the sizer and make sure that it has a right border of size 10. 
 *      // This way there's a space between the ok button and the cancel button.
 *
 *      sizer1.add(new wxButton(dlg, wxId.OK, "Ok"), 0, wxDirection.RIGHT, 10);
 *
 *      // Add the Cancel button to the sizer.
 *
 *      sizer1.add(new wxButton(dlg, wxId.CANCEL, "Cancel"));
 *
 *      // Add the sizer to the sizer of the dialog. The item is centered.
 *
 *      dlg.sizer.add(sizer1, 0, wxAlignment.CENTER);
 *
 *      // Before showing the dialog, set the autoLayout property to true and call 
 *      // layout to layout the controls. When the dialog is resized, the controls
 *      // will be automaticcally resized.
 *
 *      dlg.autoLayout = true;
 *      dlg.layout();
 *      dlg.showModal();
 *      return false;
 *    }
 *  </code></pre>
 * </class>
 */
WXJS_INIT_CLASS(Sizer, "wxSizer", 0)

/***
 * <properties>
 *	<property name="minSize" type="@wxSize">
 *	 Get/Set the minimal size of the sizer.
 *	 See @wxSizer#setMinSize
 *  </property>
 *  <property name="position" type="@wxPoint" readonly="Y">
 *	 Gets the position of the sizer.
 *  </property>
 *  <property name="size" type="@wxSize" readonly="Y">
 *	 Gets the current size of the sizer.
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(Sizer)
  WXJS_PROPERTY(P_MIN_SIZE, "minSize")
  WXJS_READONLY_PROPERTY(P_POSITION, "position")
  WXJS_READONLY_PROPERTY(P_SIZE, "size")
WXJS_END_PROPERTY_MAP()

WXJS_BEGIN_METHOD_MAP(Sizer)
  WXJS_METHOD("add", add, 1)
  WXJS_METHOD("layout", layout, 0)
  WXJS_METHOD("prepend", prepend, 5)
  WXJS_METHOD("remove", remove, 1)
  WXJS_METHOD("setDimension", setDimension, 4)
  WXJS_METHOD("setMinSize", setMinSize, 4)
  WXJS_METHOD("setItemMinSize", setItemMinSize, 3)
  WXJS_METHOD("clear", clear, 1)
WXJS_END_METHOD_MAP()

bool Sizer::GetProperty(wxSizer *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
	switch(id)
	{
	case P_MIN_SIZE:
		*vp = Size::CreateObject(cx, new wxSize(p->GetMinSize()));
		break;
	case P_SIZE:
		*vp = Size::CreateObject(cx, new wxSize(p->GetSize()));
		break;
	case P_POSITION:
      *vp = wxjs::ext::CreatePoint(cx, p->GetPosition());
		break;
	}
	return true;
}

bool Sizer::SetProperty(wxSizer *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
	if ( id == P_MIN_SIZE )
	{
		wxSize *size = Size::GetPrivate(cx, *vp);
		if ( size != NULL )
		{
			p->SetMinSize(*size);
		}
	}
	return true;
}

/***
 * <method name="add">
 *	<function>
 *   <arg name="Window" type="@wxWindow">
 *	  An object with a @wxWindow as its prototype.
 *   </arg>
 *	 <arg name="Option" type="Integer" default="0">
 *	  Option is used together with @wxBoxSizer. 0 means that the size of the control
 *	  is not allowed to change in the main orientation of the sizer. 1 means that the 
 *	  size of the control may grow or shrink in the main orientation of the sizer
 *   </arg>
 *   <arg name="Flag" type="Integer" default="0">
 *	  This parameter is used to set a number of flags. One main behaviour of a flag is to
 *	  set a border around the window. Another behaviour is to determine the child window's 
 *	  behaviour when the size of the sizer changes. You can use the constants of @wxDirection
 *	  and @wxStretch.
 *   </arg>
 *   <arg name="Border" type="Integer" default="0">
 *	  The border width. Use this when you used a border(@wxDirection) flag.
 *   </arg>
 *  </function>
 *	<function>
 *   <arg name="Sizer" type="wxSizer">
 *	  An object with a @wxWindow as its prototype.
 *	  An object which prototype is wxSizer.
 *   </arg>
 *	 <arg name="Option" type="Integer" default="0">
 *	  Option is used together with @wxBoxSizer. 0 means that the size of the control
 *	  is not allowed to change in the main orientation of the sizer. 1 means that the 
 *	  size of the control may grow or shrink in the main orientation of the sizer
 *   </arg>
 *   <arg name="Flag" type="Integer" default="0">
 *	  This parameter is used to set a number of flags. One main behaviour of a flag is to
 *	  set a border around the window. Another behaviour is to determine the child window's 
 *	  behaviour when the size of the sizer changes. You can use the constants of @wxDirection
 *	  and @wxStretch.
 *   </arg>
 *   <arg name="Border" type="Integer" default="0">
 *	  The border width. Use this when you used a border(@wxDirection) flag.
 *   </arg>
 *  </function>
 *  <function>
 *   <arg name="Width" type="Integer">
 *	  The width of the spacer.
 *   </arg>
 *   <arg name="Height" type="Integer">
 *	  The height of the spacer.
 *   </arg>
 *	 <arg name="Option" type="Integer" default="0">
 *	  Option is used together with @wxBoxSizer. 0 means that the size of the control
 *	  is not allowed to change in the main orientation of the sizer. 1 means that the 
 *	  size of the control may grow or shrink in the main orientation of the sizer
 *   </arg>
 *   <arg name="Flag" type="Integer" default="0">
 *	  This parameter is used to set a number of flags. One main behaviour of a flag is to
 *	  set a border around the window. Another behaviour is to determine the child window's 
 *	  behaviour when the size of the sizer changes. You can use the constants of @wxDirection
 *	  and @wxStretch.
 *   </arg>
 *   <arg name="Border" type="Integer" default="0">
 *	  The border width. Use this when you used a border(@wxDirection) flag.
 *   </arg>
 *  </function>
 *  <desc>
 *	 Adds a window, another sizer or a spacer to the sizer.
 *  </desc>
 * </method>
 */
JSBool Sizer::add(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxSizer *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

	int option = 0;
	int flag = 0;
	int border = 0;

    if ( Window::HasPrototype(cx, argv[0]) )
    {
        wxWindow *win = Window::GetPrivate(cx, argv[0], false);
        switch(argc)
        {
        case 4:
            if ( ! FromJS(cx, argv[3], border) )
                return JS_FALSE;
        case 3:
            if ( ! FromJS(cx, argv[2], flag) )
                return JS_FALSE;
        case 2:
            if ( ! FromJS(cx, argv[1], option) )
                return JS_FALSE;
        }
        p->Add(win, option, flag, border);
    }
    else if (HasPrototype(cx, argv[0]) )
    {
        wxSizer *sizer = GetPrivate(cx, argv[0], false);
        switch(argc)
        {
        case 4:
            if ( ! FromJS(cx, argv[3], border) )
                return JS_FALSE;
        case 3:
            if ( ! FromJS(cx, argv[2], flag) )
                return JS_FALSE;
        case 2:
            if ( ! FromJS(cx, argv[1], option) )
                return JS_FALSE;
        }
        p->Add(sizer, option, flag, border);
        
        // Protect the sizer
        JavaScriptClientData *data 
          = dynamic_cast<JavaScriptClientData*>(sizer->GetClientObject());
        if ( data != NULL )
        {
          data->Protect(true);
          data->SetOwner(false);
        }
    }
    else
    {
        if ( argc < 2 )
            return JS_FALSE;

        switch(argc)
        {
        case 5:
            if ( ! FromJS(cx, argv[3], border) )
                return JS_FALSE;
        case 4:
            if ( ! FromJS(cx, argv[2], flag) )
                return JS_FALSE;
        case 3:
            if ( ! FromJS(cx, argv[1], option) )
                return JS_FALSE;
        }

        int height;
        if ( ! FromJS(cx, argv[1], height) )
            return JS_FALSE;

        int width;
        if ( ! FromJS(cx, argv[0], width) )
            return JS_FALSE;

        p->Add(width, height, option, flag, border);
    }

    return JS_TRUE;
}

/***
 * <method name="layout">
 *  <function />
 *  <desc>
 *	 Call this to force layout of the children.
 *  </desc>
 * </method> 
 */
JSBool Sizer::layout(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxSizer *p = (wxSizer *) GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;
	
	p->Layout();

	return JS_TRUE;
}

/***
 * <method name="add">
 *	<function>
 *   <arg name="Window" type="@wxWindow">
 *	  An object with a @wxWindow as its prototype.
 *   </arg>
 *	 <arg name="Option" type="Integer" default="0">
 *	  Option is used together with @wxBoxSizer. 0 means that the size of the control
 *	  is not allowed to change in the main orientation of the sizer. 1 means that the 
 *	  size of the control may grow or shrink in the main orientation of the sizer
 *   </arg>
 *   <arg name="Flag" type="Integer" default="0">
 *	  This parameter is used to set a number of flags. One main behaviour of a flag is to
 *	  set a border around the window. Another behaviour is to determine the child window's 
 *	  behaviour when the size of the sizer changes. You can use the constants of @wxDirection
 *	  and @wxStretch.
 *   </arg>
 *   <arg name="Border" type="Integer" default="0">
 *	  The border width. Use this when you used a border(@wxDirection) flag.
 *   </arg>
 *  </function>
 *	<function>
 *   <arg name="Sizer" type="wxSizer">
 *	  An object with a @wxWindow as its prototype.
 *	  An object which prototype is wxSizer.
 *   </arg>
 *	 <arg name="Option" type="Integer" default="0">
 *	  Option is used together with @wxBoxSizer. 0 means that the size of the control
 *	  is not allowed to change in the main orientation of the sizer. 1 means that the 
 *	  size of the control may grow or shrink in the main orientation of the sizer
 *   </arg>
 *   <arg name="Flag" type="Integer" default="0">
 *	  This parameter is used to set a number of flags. One main behaviour of a flag is to
 *	  set a border around the window. Another behaviour is to determine the child window's 
 *	  behaviour when the size of the sizer changes. You can use the constants of @wxDirection
 *	  and @wxStretch.
 *   </arg>
 *   <arg name="Border" type="Integer" default="0">
 *	  The border width. Use this when you used a border(@wxDirection) flag.
 *   </arg>
 *  </function>
 *  <function>
 *   <arg name="Width" type="Integer">
 *	  The width of the spacer.
 *   </arg>
 *   <arg name="Height" type="Integer">
 *	  The height of the spacer.
 *   </arg>
 *	 <arg name="Option" type="Integer" default="0">
 *	  Option is used together with @wxBoxSizer. 0 means that the size of the control
 *	  is not allowed to change in the main orientation of the sizer. 1 means that the 
 *	  size of the control may grow or shrink in the main orientation of the sizer
 *   </arg>
 *   <arg name="Flag" type="Integer" default="0">
 *	  This parameter is used to set a number of flags. One main behaviour of a flag is to
 *	  set a border around the window. Another behaviour is to determine the child window's 
 *	  behaviour when the size of the sizer changes. You can use the constants of @wxDirection
 *	  and @wxStretch.
 *   </arg>
 *   <arg name="Border" type="Integer" default="0">
 *	  The border width. Use this when you used a border(@wxDirection) flag.
 *   </arg>
 *  </function>
 *  <desc>
 *	 Prepends a window, another sizer or a spacer to the beginning of the list of items 
 *	 owned by this sizer. See @wxSizer#add.
 *  </desc>
 * </method>
 */
JSBool Sizer::prepend(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxSizer *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

	int option = 0;
	int flag = 0;
	int border = 0;

    if ( Window::HasPrototype(cx, argv[0]) )
    {
        wxWindow *win = Window::GetPrivate(cx, argv[0], false);
        switch(argc)
        {
        case 4:
            if ( ! FromJS(cx, argv[3], border) )
                return JS_FALSE;
        case 3:
            if ( ! FromJS(cx, argv[2], flag) )
                return JS_FALSE;
        case 2:
            if ( ! FromJS(cx, argv[1], option) )
                return JS_FALSE;
        }
        p->Prepend(win, option, flag, border);
    }
    else if (HasPrototype(cx, argv[0]) )
    {
        wxSizer *sizer = GetPrivate(cx, argv[0], false);
        switch(argc)
        {
        case 4:
            if ( ! FromJS(cx, argv[3], border) )
                return JS_FALSE;
        case 3:
            if ( ! FromJS(cx, argv[2], flag) )
                return JS_FALSE;
        case 2:
            if ( ! FromJS(cx, argv[1], option) )
                return JS_FALSE;
        }
        p->Prepend(sizer, option, flag, border);
    }
    else
    {
        if ( argc < 2 )
            return JS_FALSE;

        switch(argc)
        {
        case 5:
            if ( ! FromJS(cx, argv[3], border) )
                return JS_FALSE;
        case 4:
            if ( ! FromJS(cx, argv[2], flag) )
                return JS_FALSE;
        case 3:
            if ( ! FromJS(cx, argv[1], option) )
                return JS_FALSE;
        }

        int height;
        if ( ! FromJS(cx, argv[1], height) )
            return JS_FALSE;

        int width;
        if ( ! FromJS(cx, argv[0], width) )
            return JS_FALSE;

        p->Prepend(width, height, option, flag, border);
    }

    return JS_TRUE;
}

/***
 * <method name="remove">
 *	<function>
 *   <arg name="Index" type="Integer">
 *	  The index of the item to remove from the sizer.
 *   </arg>
 *  </function>
 *  <function>
 *   <arg name="Window" type="@wxWindow">
 *	  Window to remove from the sizer.
 *   </arg>
 *  </function>
 *  <function>
 *   <arg name="Sizer" type="wxSizer">
 *	  Sizer to remove from this sizer.
 *   </arg>
 *  </function>
 *  <desc>
 *	 Removes the item from the sizer. Call @wxSizer#layout to update the screen.
 *  </desc>
 * </method>
 */
JSBool Sizer::remove(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxSizer *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    if ( Window::HasPrototype(cx, argv[0]) )
    {
        wxWindow *win = Window::GetPrivate(cx, argv[0], false);
        p->Remove(win);
    }
    else if ( HasPrototype(cx, argv[0]) )
    {
        wxSizer *sizer = GetPrivate(cx, argv[0], false);
        p->Remove(sizer);
    }
    else
    {
        int idx;
        if ( ! FromJS(cx, argv[0], idx) )
            return JS_FALSE;
        p->Remove(idx);
    }

    return JS_TRUE;
}

/***
 * <method name="setDimension">
 *	<function>
 *   <arg name="x" type="Integer" />
 *   <arg name="y" type="Integer" />
 *   <arg name="width" type="Integer" />
 *   <arg name="height" type="Integer" />
 *  </function>
 *  <desc>
 *	 Call this to force the sizer to take the given dimension and thus force the items 
 *	 owned by the sizer to resize themselves according to the rules defined by the parameters 
 *	 in the @wxSizer#add and @wxSizer#prepend methods.
 *  </desc>
 * </method>
 */
JSBool Sizer::setDimension(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxSizer *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

	int x;
	int y;
	int width;
	int height;

    if (    FromJS(cx, argv[0], x)
         && FromJS(cx, argv[1], y)
         && FromJS(cx, argv[2], width)
         && FromJS(cx, argv[3], height) )
    {
    	p->SetDimension(x, y, width, height);
        return JS_TRUE;
    }

    return JS_FALSE;
}

/***
 * <method name="setMinSize">
 *	<function>
 *   <arg name="Width" type="Integer" />
 *   <arg name="Height" type="Integer" />
 *  </function>
 *  <desc>
 *	 Sets the minimal size of the sizer.
 *	 See @wxSizer#minSize.
 *  </desc>
 * </method>
 */
JSBool Sizer::setMinSize(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxSizer *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    int width;
    int height;

	if (	FromJS(cx, argv[0], width) 
		 && FromJS(cx, argv[1], height) )
	{
    	p->SetMinSize(width, height);
    	return JS_TRUE;
    }
    return JS_FALSE;
}

/***
 * <method name="setItemMinSize">
 *	<function>
 *	 <arg name="Window" type="@wxWindow" />
 *   <arg name="Width" type="Integer" />
 *   <arg name="Height" type="Integer" />
 *  </function>
 *	<function>
 *	 <arg name="Sizer" type="wxSizer" />
 *   <arg name="Width" type="Integer" />
 *   <arg name="Height" type="Integer" />
 *  </function>
 *	<function>
 *	 <arg name="Item" type="Integer" />
 *   <arg name="Width" type="Integer" />
 *   <arg name="Height" type="Integer" />
 *  </function>
 *  <desc>
 *	 Sets an item minimal size.
 *  </desc>
 * </method>
 */
JSBool Sizer::setItemMinSize(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxSizer *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

	int width = 0;
	int height = 0;

    if (    FromJS(cx, argv[1], width)
         && FromJS(cx, argv[2], height) )
    {
        if ( Window::HasPrototype(cx, argv[0]) )
        {
            p->SetItemMinSize(Window::GetPrivate(cx, argv[0], false), width, height);
        }
        else if ( HasPrototype(cx, argv[1]) )
        {
            p->SetItemMinSize(GetPrivate(cx, argv[0], false), width, height);
        }
        else
        {
            int idx;
            if ( FromJS(cx, argv[0], idx) )
                p->SetItemMinSize(idx, width, height);
            else
                return JS_FALSE;
        }
    }
    else
        return JS_FALSE;
    
    return JS_TRUE;
}

JSBool Sizer::clear(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxSizer *p = (wxSizer *) GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;
    bool delete_windows = false;

    if (FromJS(cx, argv[0], delete_windows))
        p->Clear(delete_windows);
    else
        return JS_FALSE;

    return JS_TRUE;
}
