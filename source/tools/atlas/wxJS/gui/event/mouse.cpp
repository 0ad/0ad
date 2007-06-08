#include "precompiled.h"

/*
 * wxJavaScript - mouse.cpp
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
 * $Id: mouse.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
// mouse.cpp

#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif

#include "../../common/main.h"
#include "../../common/type.h"
#include "../../common/apiwrap.h"

#include "jsevent.h"
#include "mouse.h"
#include "../misc/point.h"

using namespace wxjs;
using namespace wxjs::gui;

/***
 * <file>event/mouse</file>
 * <module>gui</module>
 * <class name="wxMouseEvent" prototype="@wxEvent">
 *	This event class contains information about mouse events.
 *  <br /><br />
 *  Note the difference between methods like @wxMouseEvent#leftDown
 *  and @wxMouseEvent#leftIsDown: the formet returns true when the event corresponds
 *  to the left mouse button click while the latter returns true if the left mouse button
 *  is currently being pressed. For example, when the user is dragging the mouse you can use 
 *  @wxMouseEvent#leftIsDown to test whether the left mouse button is (still) depressed.
 *  Also, by convention, if @wxMouseEvent#leftDown returns true, @wxMouseEvent#leftIsDown 
 *  will also return true in wxWindows whatever the underlying GUI 
 *  behaviour is (which is platform-dependent).
 *  The same applies, of course, to other mouse buttons as well.
 *  <br /><br />
 *  See @wxWindow#onMouseEvents, @wxWindow#onEnterWindow, @wxWindow#onLeaveWindow,
 *  @wxWindow#onLeftUp, @wxWindow#onLeftDown, @wxWindow#onLeftDClick,
 *  @wxWindow#onMiddleUp, @wxWindow#onMiddleDown, @wxWindow#onMiddleDClick,
 *  @wxWindow#onRightUp, @wxWindow#onRightDown, @wxWindow#onRightDClick.
 * </class>
 */
WXJS_INIT_CLASS(MouseEvent, "wxMouseEvent", 0)

/***
 * <properties>
 *	<property name="altDown" type="Boolean" readonly="Y">
 *	 Returns true when the alt key is down at the time of the key event.
 *  </property>
 *	<property name="button" type="Integer" readonly="Y">
 *	 Get the button which is changing state
 *  </property>
 *	<property name="controlDown" type="Boolean" readonly="Y">
 *	 Returns true when the control key is down at the time of the event.
 *  </property>
 *	<property name="dragging" type="Boolean" readonly="Y">
 *	 Returns true when this is a dragging event.
 *  </property>
 *	<property name="entering" type="Boolean" readonly="Y">
 *	 Returns true when the mouse was entering the window.
 *	 See @wxMouseEvent#leaving
 *  </property>
 *	<property name="leaving" type="Boolean" readonly="Y">
 *	 Returns true when the mouse was leaving the window.
 *	 See @wxMouseEvent#entering.
 *  </property>
 *	<property name="leftDClick" type="Boolean" readonly="Y">
 *	 Returns true when the event is a left double click event (wxEVT_LEFT_DCLICK).
 *  </property>
 *	<property name="leftDown" type="Boolean" readonly="Y">
 *	 Returns true when the event is a left down event (wxEVT_LEFT_DOWN).
 *  </property>
 *	<property name="leftIsDown" type="Boolean" readonly="Y">
 *	 Returns true if the left mouse button is currently down, independent of the current event type.
 *	 Please notice that it is not the same as @wxMouseEvent#leftDown which returns true
 *	 if the left mouse button was just pressed. Rather, it describes the state of the mouse button 
 *	 before the event happened.
 *  </property>
 *	<property name="linesPerAction" type="Integer" readonly="Y">
 *	 Returns the configured number of lines (or whatever)
 *	 to be scrolled per wheel action. Defaults to 1.
 *  </property>
 *	<property name="metaDown" type="Boolean" readonly="Y">
 *	 Returns true when the meta key was down at the time of the event.
 *  </property>
 *	<property name="middleDClick" type="Boolean" readonly="Y">
 *	 Returns true when the event is a middle double click event (wxEVT_MIDDLE_DCLICK).
 *  </property>
 *	<property name="middleDown" type="Boolean" readonly="Y">
 *	 Returns true when the event is a middle down event (wxEVT_MIDDLE_DOWN).
 *  </property>
 *	<property name="middleIsDown" type="Boolean" readonly="Y">
 *	 Returns true if the middle mouse button is currently down, independent of the current event type.
 *  </property>
 *	<property name="moving" type="Boolean" readonly="Y">
 *	 Returns true when this was a moving event (wxEVT_MOTION)
 *  </property>
 *	<property name="position" type="@wxPoint" readonly="Y">
 *	 Returns the physical mouse position in pixels.
 *  </property>
 *	<property name="rightDClick" type="Boolean" readonly="Y">
 *	 Returns true when the event is a right double click event (wxEVT_RIGHT_DCLICK).
 *  </property>
 *	<property name="rightDown" type="Boolean" readonly="Y">
 *	 Returns true when the event is a right down event (wxEVT_RIGHT_DOWN).
 *  </property>
 *	<property name="rightIsDown" type="Boolean" readonly="Y">
 *	 Returns true if the right mouse button is currently down, independent of the current event type.
 *  </property>
 *	<property name="shiftDown" type="Boolean" readonly="Y">
 *	 Returns true when the shift key was down at the time of the event.
 *  </property>
 *	<property name="wheelDelta" type="Integer" readonly="Y">
 *	 Get wheel delta, normally 120.  This is the threshold for action to be
 *	 taken, and one such action (for example, scrolling one increment)
 *	 should occur for each delta.
 *  </property>
 *	<property name="wheelRotation" type="Integer" readonly="Y">
 *	 Get wheel rotation, positive or negative indicates direction of
 *	 rotation.	Current devices all send an event when rotation is equal to
 *	 +/-WheelDelta, but this allows for finer resolution devices to be
 *	 created in the future.  Because of this you shouldn't assume that one
 *	 event is equal to 1 line or whatever, but you should be able to either
 *	 do partial line scrolling or wait until +/-WheelDelta rotation values
 *	 have been accumulated before scrolling.
 *  </property>
 *	<property name="x" type="Integer" readonly="Y">
 *	 Returns the x-coordinate of the physical mouse position.
 *	 See @wxMouseEvent#position and @wxMouseEvent#y
 *  </property>
 *	<property name="y" type="Integer" readonly="Y">
 *	 Returns the y-coordinate of the physical mouse position.
 *	 See @wxMouseEvent#position and @wxMouseEvent#x
 *  </property>
 * </properties>
 */

WXJS_BEGIN_PROPERTY_MAP(MouseEvent)
  WXJS_READONLY_PROPERTY(P_ALTDOWN, "altDown")
  WXJS_READONLY_PROPERTY(P_CONTROLDOWN, "controlDown")
  WXJS_READONLY_PROPERTY(P_DRAGGING, "dragging")
  WXJS_READONLY_PROPERTY(P_ENTERING, "entering")
  WXJS_READONLY_PROPERTY(P_POSITION, "position")
  WXJS_READONLY_PROPERTY(P_LINES_PER_ACTION, "linesPerAction")
  WXJS_READONLY_PROPERTY(P_BUTTON, "button")
  WXJS_READONLY_PROPERTY(P_METADOWN, "metaDown")
  WXJS_READONLY_PROPERTY(P_SHIFTDOWN, "shiftDown")
  WXJS_READONLY_PROPERTY(P_LEFT_DOWN, "leftDown")
  WXJS_READONLY_PROPERTY(P_MIDDLE_DOWN, "middleDown")
  WXJS_READONLY_PROPERTY(P_RIGHT_DOWN, "rightDown")
  WXJS_READONLY_PROPERTY(P_LEFT_UP, "leftUp")
  WXJS_READONLY_PROPERTY(P_MIDDLE_UP, "middleUp")
  WXJS_READONLY_PROPERTY(P_RIGHT_UP, "rightUp")
  WXJS_READONLY_PROPERTY(P_LEFT_DCLICK, "leftDClick")
  WXJS_READONLY_PROPERTY(P_MIDDLE_DCLICK, "middleDClick")
  WXJS_READONLY_PROPERTY(P_RIGHT_DCLICK, "rightDClick")
  WXJS_READONLY_PROPERTY(P_LEFT_IS_DOWN, "leftIsDown")
  WXJS_READONLY_PROPERTY(P_MIDDLE_IS_DOWN, "middleIsDown")
  WXJS_READONLY_PROPERTY(P_RIGHT_IS_DOWN, "rightIsDown")
  WXJS_READONLY_PROPERTY(P_MOVING, "moving")
  WXJS_READONLY_PROPERTY(P_LEAVING, "leaving")
  WXJS_READONLY_PROPERTY(P_X, "x")
  WXJS_READONLY_PROPERTY(P_Y, "y")
  WXJS_READONLY_PROPERTY(P_WHEELROTATION, "wheelRotation")
  WXJS_READONLY_PROPERTY(P_WHEELDELTA, "wheelDelta")
WXJS_END_PROPERTY_MAP()

bool MouseEvent::GetProperty(PrivMouseEvent *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    wxMouseEvent *event = (wxMouseEvent*) p->GetEvent();
    switch (id) 
	{
		case P_ALTDOWN:
			*vp = ToJS(cx, event->AltDown());
			break;
		case P_CONTROLDOWN:
			*vp = ToJS(cx, event->ControlDown());
			break;
		case P_DRAGGING:
			*vp = ToJS(cx, event->Dragging());
			break;
		case P_ENTERING:
			*vp = ToJS(cx, event->Entering());
			break;
		case P_POSITION:
			*vp = Point::CreateObject(cx, new wxPoint(event->GetPosition()));
			break;
		case P_LINES_PER_ACTION:
			*vp = ToJS(cx, event->GetLinesPerAction());
			break;
		case P_BUTTON:
			*vp = ToJS(cx, event->GetButton());
			break;
		case P_METADOWN:
			*vp = ToJS(cx, event->MetaDown());
			break;
		case P_SHIFTDOWN:
			*vp = ToJS(cx, event->ShiftDown());
			break;
		case P_LEFT_DOWN:
			*vp = ToJS(cx, event->LeftDown());
			break;
		case P_MIDDLE_DOWN:
			*vp = ToJS(cx, event->MiddleDown());
			break;
		case P_RIGHT_DOWN:
			*vp = ToJS(cx, event->RightDown());
			break;
		case P_LEFT_UP:
			*vp = ToJS(cx, event->LeftUp());
			break;
		case P_MIDDLE_UP:
			*vp = ToJS(cx, event->MiddleUp());
			break;
		case P_RIGHT_UP:
			*vp = ToJS(cx, event->RightUp());
			break;
		case P_LEFT_DCLICK:
			*vp = ToJS(cx, event->LeftDClick());
			break;
		case P_MIDDLE_DCLICK:
			*vp = ToJS(cx, event->MiddleDClick());
			break;
		case P_RIGHT_DCLICK:
			*vp = ToJS(cx, event->RightDClick());
			break;
		case P_LEFT_IS_DOWN:
			*vp = ToJS(cx, event->LeftIsDown());
			break;
		case P_MIDDLE_IS_DOWN:
			*vp = ToJS(cx, event->MiddleIsDown());
			break;
		case P_RIGHT_IS_DOWN:
			*vp = ToJS(cx, event->RightIsDown());
			break;
		case P_MOVING:
			*vp = ToJS(cx, event->Moving());
			break;
		case P_LEAVING:
			*vp = ToJS(cx, event->Leaving());
			break;
		case P_X:
			*vp = ToJS(cx, event->GetX());
			break;
		case P_Y:
			*vp = ToJS(cx, event->GetY());
			break;
		case P_WHEELROTATION:
			*vp = ToJS(cx, event->GetWheelRotation());
			break;
		case P_WHEELDELTA:
			*vp = ToJS(cx, event->GetWheelDelta());
			break;
	}
	return true;
}

WXJS_BEGIN_METHOD_MAP(MouseEvent)
  WXJS_METHOD("button", button, 1)
  WXJS_METHOD("buttonDClick", buttonDClick, 1)
  WXJS_METHOD("buttonDown", buttonDown, 1)
  WXJS_METHOD("buttonUp", buttonUp, 1)
//	Waiting for wxDC
//	WXJS_METHOD("getLogicalPosition", getLogicalPosition, 1)
WXJS_END_METHOD_MAP()

/***
 * <method name="button">
 *	<function returns="Boolean">
 *	 <arg name="Number" type="Integer">
 *	  The mouse button.  
 *   </arg>
 *  </function>
 *  <desc>
 *	 Returns true if the identified mouse button is changing state.
 *	 Valid values of button are 1, 2 or 3 for left, middle and right buttons respectively.
 *  </desc>
 * </method>
 */ 
JSBool MouseEvent::button(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	PrivMouseEvent *p = (PrivMouseEvent*) JS_GetPrivate(cx, obj);
	wxASSERT_MSG(p != NULL, wxT("No private data associated with wxMouseEvent"));

	int button;
	if ( FromJS(cx, argv[0], button) )
	{
		*rval = ToJS(cx, p->GetEvent()->Button(button));
		return JS_TRUE;
	}
	else
	{
		return JS_FALSE;
	}
}

/***
 * <method name="buttonDClick">
 *	<function returns="Boolean">
 *	 <arg name="Number" type="Integer">
 *	  The mouse button.  
 *   </arg>
 *  </function>
 *  <desc>
 *	 If the argument is omitted, this returns true if the event was a mouse double click event. 
 *	 Otherwise the argument specifies which double click event was generated (1, 2 or 3 for left, middle and right buttons respectively).
 *  </desc>
 * </method>
 */
JSBool MouseEvent::buttonDClick(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	PrivMouseEvent *p = (PrivMouseEvent*) JS_GetPrivate(cx, obj);
	wxASSERT_MSG(p != NULL, wxT("No private data associated with wxMouseEvent"));

	int button = -1;
	FromJS(cx, argv[0], button);
	*rval = ToJS(cx, p->GetEvent()->ButtonDClick(button));

	return JS_TRUE;
}

/***
 * <method name="buttonDown">
 *	<function returns="Boolean">
 *	 <arg name="Number" type="Integer">
 *	  The mouse button.  
 *   </arg>
 *  </function>
 *  <desc>
 *	 If the argument is omitted, this returns true if the event was a mouse button down event. 
 *	 Otherwise the argument specifies which button-down event was generated (1, 2 or 3 for left, middle and right buttons respectively).
 *  </desc>
 * </method>
 */
JSBool MouseEvent::buttonDown(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	PrivMouseEvent *p = (PrivMouseEvent*) JS_GetPrivate(cx, obj);
	wxASSERT_MSG(p != NULL, wxT("No private data associated with wxMouseEvent"));

	int button = -1;
	FromJS(cx, argv[0], button);
	*rval = ToJS(cx, p->GetEvent()->ButtonDown(button));

	return JS_TRUE;
}

/***
 * <method name="buttonUp">
 *	<function returns="Boolean">
 *	 <arg name="Number" type="Integer">
 *	  The mouse button.  
 *   </arg>
 *  </function>
 *  <desc>
 *	 If the argument is omitted, this returns true if the event was a mouse button up event.
 *	 Otherwise the argument specifies which button-up event was generated (1, 2 or 3 for left, middle and right buttons respectively).
 *  </desc>
 * </method>
 */
JSBool MouseEvent::buttonUp(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	PrivMouseEvent *p = (PrivMouseEvent*) JS_GetPrivate(cx, obj);
	wxASSERT_MSG(p != NULL, wxT("No private data associated with wxMouseEvent"));

	int button = -1;
	FromJS(cx, argv[0], button);
	*rval = ToJS(cx, p->GetEvent()->ButtonUp(button));

	return JS_TRUE;
}
