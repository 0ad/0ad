#include "precompiled.h"

/*
 * wxJavaScript - constant.cpp
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
 * $Id: constant.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
// constant.cpp

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif
#include <wx/calctrl.h>
#include <wx/dir.h>
#include <wx/stream.h>

#include "../common/main.h"

#include "constant.h"

using namespace wxjs;

/*** 
 * <file>constant</file>
 * <module>io</module>
 * <class name="Global Constants">
 *  The following list shows all the constants that are defined on the <I>Global</I> object.
 *  <ul><li>wxNOT_FOUND</li></ul>
 *  Used by several <I>find</I> methods to indicate that nothing is found.
 * </class>
 */
extern JSConstDoubleSpec wxGlobalMap[];

/***
 * <constants>
 *  <type name="wxStreamError">
 *   <constant name="NO_ERROR">
 *    No error occurred.
 *   </constant>
 *   <constant name="EOF">
 *    An End-OF-File occurred.
 *   </constant>
 *   <constant name="WRITE_ERROR">
 *    An error occurred in the last write operation.
 *   </constant>
 *   <constant name="READ_ERROR">
 *    An error occurred in the last read operation.
 *   </constant>
 *   <desc>See @wxStreamBase</desc>
 *  </type>
 */
JSConstDoubleSpec wxStreamErrorMap[] = 
{
    WXJS_CONSTANT(wxSTREAM_, NO_ERROR)
    WXJS_CONSTANT(wxSTREAM_, EOF)
    WXJS_CONSTANT(wxSTREAM_, WRITE_ERROR)
    WXJS_CONSTANT(wxSTREAM_, READ_ERROR)
	{ 0 }
};

/***
 * <type name="wxSeekMode">
 *  <constant name="FromCurrent" />
 *  <constant name="FromStart" />
 *  <constant name="FromEnd" />
 *  <desc>See @wxInputStream#seekI and @wxFile#seek</desc>
 * </type>
 */
JSConstDoubleSpec wxSeekModeMap[] = 
{
	WXJS_CONSTANT(wx, FromCurrent)
	WXJS_CONSTANT(wx, FromStart)
	WXJS_CONSTANT(wx, FromEnd)
	{ 0 }
};

/***
 * <type name="wxExecFlag">
 *  <constant name="FromCurrent" />
 *  <constant name="ASYNC">execute the process asynchronously</constant>
 *  <constant name="SYNC">execute it synchronously, i.e. wait until it finishes</constant>
 *  <constant name="NOHIDE">under Windows, don't hide the child even if it's IO is redirected (this
 *   is done by default)</constant>
 *  <constant name="MAKE_GROUP_LEADER">
 *   under Unix, if the process is the group leader then passing true to @wxProcess#kill
 *   kills all children as well as pid</constant>
 *  <constant name="NODISABLE">
 *    by default synchronous execution disables all program windows to avoid
 *    that the user interacts with the program while the child process is
 *    running, you can use this flag to prevent this from happening</constant>
 *  <desc>See @wxExecute and @wxProcess</desc>
 * </type>
 * </constants>
 */
JSConstDoubleSpec wxExecFlagsMap[] = 
{
	WXJS_CONSTANT(wxEXEC_, ASYNC)
	WXJS_CONSTANT(wxEXEC_, SYNC)
	WXJS_CONSTANT(wxEXEC_, NOHIDE)
	WXJS_CONSTANT(wxEXEC_, MAKE_GROUP_LEADER)
	WXJS_CONSTANT(wxEXEC_, NODISABLE)
	{ 0 }
};

void io::InitConstants(JSContext *cx, JSObject *obj)
{
	// Define the global constants
	JS_DefineConstDoubles(cx, obj, wxGlobalMap);

    JSObject *constObj = JS_DefineObject(cx, obj, "wxStreamError", 
							             NULL, NULL,
							             JSPROP_READONLY | JSPROP_PERMANENT);
	JS_DefineConstDoubles(cx, constObj, wxStreamErrorMap);

    constObj = JS_DefineObject(cx, obj, "wxSeekMode", 
							   NULL, NULL,
							   JSPROP_READONLY | JSPROP_PERMANENT);
	JS_DefineConstDoubles(cx, constObj, wxSeekModeMap);

    constObj = JS_DefineObject(cx, obj, "wxExecFlag", 
							   NULL, NULL,
							   JSPROP_READONLY | JSPROP_PERMANENT);
    JS_DefineConstDoubles(cx, constObj, wxExecFlagsMap);
}
