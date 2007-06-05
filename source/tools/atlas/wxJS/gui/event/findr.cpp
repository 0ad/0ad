#include "precompiled.h"

/*
 * wxJavaScript - findr.cpp
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
 * $Id: findr.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
// findr.cpp

#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif

#include "../../common/main.h"
#include "../../common/type.h"

#include "jsevent.h"
#include "findr.h"

using namespace wxjs;
using namespace wxjs::gui;

/***
 * <file>event/findr</file>
 * <module>gui</module>
 * <class name="wxFindDialogEvent" prototype="@wxEvent">
 *	This event class contains the information about 
 *  all @wxFindReplaceDialog events: @wxFindReplaceDialog#onFind
 *	@wxFindReplaceDialog#onFindNext, @wxFindReplaceDialog#onFindReplace,
 *	@wxFindReplaceDialog#onFindReplaceAll, @wxFindReplaceDialog#onFindClose.
 * </class>
 */
WXJS_INIT_CLASS(FindDialogEvent, "wxFindDialogEvent", 0)

/***
 * <properties>
 *	<property name="findString" type="String" readonly="Y" />
 *  <property name="flags" type="Integer" readonly="Y" />
 *  <property name="replaceString" type="String" readonly="Y" />
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(FindDialogEvent)
	WXJS_READONLY_PROPERTY(P_FLAGS, "flags")
	WXJS_READONLY_PROPERTY(P_FINDSTRING, "findString")
	WXJS_READONLY_PROPERTY(P_REPLACESTRING, "replaceString")
	WXJS_READONLY_PROPERTY(P_DIALOG, "dialog")
WXJS_END_PROPERTY_MAP()

bool FindDialogEvent::GetProperty(PrivFindDialogEvent *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    wxFindDialogEvent *event = p->GetEvent();
	switch (id) 
	{
	case P_FLAGS:
		*vp = ToJS(cx, event->GetFlags());
		break;
	case P_FINDSTRING:
		*vp = ToJS(cx, event->GetFindString());
		break;
	case P_REPLACESTRING:
		*vp = ToJS(cx, event->GetReplaceString());
		break;
	case P_DIALOG:
		{
			Object *win = dynamic_cast<Object *>(event->GetDialog());
			*vp = win == NULL ? JSVAL_VOID 
							  : OBJECT_TO_JSVAL(win->GetObject());
			break;
		}
	}
	return true;
}
