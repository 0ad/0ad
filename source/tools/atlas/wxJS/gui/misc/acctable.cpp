#include "precompiled.h"

/*
 * wxJavaScript - acctable.cpp
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
 * $Id: acctable.cpp 782 2007-06-24 20:23:23Z fbraem $
 */
// acctable.cpp

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../../common/main.h"
#include "../../common/apiwrap.h"

#include "acctable.h"
#include "accentry.h"

using namespace wxjs;
using namespace wxjs::gui;

/***
 * <file>misc/acctable</file>
 * <module>gui</module>
 * <class name="wxAcceleratorTable">
 *  An accelerator table allows the application to specify a table 
 *  of keyboard shortcuts for menus or other commands. On Windows,
 *  menu or button commands are supported; on GTK,
 *  only menu commands are supported.
 *  The following example adds an accelerator to a frame. When CTRL-F1 is pressed
 *  the menu action associated with id 1 is executed.
 *  <pre><code class="whjs">
 *    wxTheApp.onInit = init;
 *
 *    function init()
 *    {
 *      var frame = new wxFrame(null, -1, "Accelerator Test");
 *      var menuBar = new wxMenuBar();
 *      var menu = new wxMenu();
 *      var idInfoAbout = 1;
 *
 *      menu = new wxMenu();
 *      menu.append(idInfoAbout, "About", infoAboutMenu);
 *      menuBar.append(menu, "Info");
 *
 *      frame.menuBar = menuBar;
 *
 *      var entries = new Array();
 *      entries[0] = new wxAcceleratorEntry(wxAcceleratorEntry.CTRL,
 *                                          wxKeyCode.F1,
 *                                          idInfoAbout);
 *      frame.acceleratorTable = new wxAcceleratorTable(entries);
 *
 *      menu.getItem(idInfoAbout).accel = entries[0];
 *
 *      topWindow = frame;
 *      frame.visible = true;
 *
 *      return true;
 *    }
 *
 *    function infoAboutMenu(event)
 *    {
 *      wxMessageBox("Accelerator Test Version 1.0");
 *    }
 *   </code></pre>
 * </class>
 */
WXJS_INIT_CLASS(AcceleratorTable, "wxAcceleratorTable", 1)

/***
 * <properties>
 *  <property name="ok" type="Boolean" readonly="Y">
 *   Returns true when the accelerator table is valid
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(AcceleratorTable)
  WXJS_READONLY_PROPERTY(P_OK, "ok")
WXJS_END_PROPERTY_MAP()

bool AcceleratorTable::GetProperty(wxAcceleratorTable *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
	if ( id == P_OK )
	{
		*vp = ToJS(cx, p->Ok());
    }
    return true;
}

/***
 * <ctor>
 *  <function>
 *   <arg name="Entries" type="Array">
 *    An array of @wxAcceleratorEntry objects
 *   </arg>
 *  </function>
 *  <desc>
 *   Constructs a new wxAcceleratorTable object.
 *  </desc>
 * </ctor>
 */
wxAcceleratorTable *AcceleratorTable::Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing)
{
	if ( JSVAL_IS_OBJECT(argv[0]) )
	{
		JSObject *objEntries = JSVAL_TO_OBJECT(argv[0]);
		if ( JS_IsArrayObject(cx, objEntries) )
		{
			jsuint length = 0;
			JS_GetArrayLength(cx, objEntries, &length);
			int n = length;
			wxAcceleratorEntry **items = new wxAcceleratorEntry*[n];
			for(jsint i =0; i < n; i++)
			{
				jsval element;
				JS_GetElement(cx, objEntries, i, &element);
				items[i] = AcceleratorEntry::GetPrivate(cx, element);
			}

			wxAcceleratorTable *p = new wxAcceleratorTable(n, *items);

			delete[] items;

			return p;
		}
	}

	return NULL;
}
