#include "precompiled.h"

/*
 * wxJavaScript - helpbtn.cpp
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
 * $Id: helpbtn.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../../common/main.h"

#include "../event/evthand.h"
#include "../event/jsevent.h"
#include "../event/command.h"

#include "../misc/size.h"
#include "../misc/point.h"
#include "helpbtn.h"
#include "../misc/bitmap.h"
#include "window.h"

using namespace wxjs;
using namespace wxjs::gui;

/***
 * <file>control/helpbtn</file>
 * <module>gui</module>
 * <class name="wxContextHelpButton" prototype="@wxButton">
 *  Instances of this class may be used to add a question mark button that when pressed,
 *  puts the application into context-help mode. It does this by creating a @wxContextHelp
 *  object which itself generates a @wxHelpEvent event when the user clicks on a window.
 * </class>
 */
WXJS_INIT_CLASS(ContextHelpButton, "wxContextHelpButton", 1)

ContextHelpButton::ContextHelpButton(wxWindow *parent, JSContext *cx, JSObject *obj)
	: wxContextHelpButton(parent)
	, Object(obj, cx)
{
	PushEventHandler(new EventHandler(this));
}

ContextHelpButton::~ContextHelpButton()
{
	PopEventHandler(true);
}

/***
 * <ctor>
 *  <function>
 *   <arg name="Parent" type="@wxWindow">
 *    The parent of wxContextHelpButton.
 *   </arg>
 *  </function>
 *  <desc>
 *   Constructs a new wxContextHelpButton object.
 *  </desc>
 * </ctor>
 */
wxContextHelpButton* ContextHelpButton::Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing)
{
    wxWindow *parent = Window::GetPrivate(cx, argv[0]);
	if ( parent == NULL )
		return NULL;

    Object *wxjsParent = dynamic_cast<Object *>(parent);
    JS_SetParent(cx, obj, wxjsParent->GetObject());

	ContextHelpButton *p = new ContextHelpButton(parent, cx, obj);
	return p;
}
