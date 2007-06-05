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

#include "../../common/main.h"

#include "constant.h"

using namespace wxjs;
using namespace wxjs::gui;

/***
 * <file>misc/constants</file>
 * <module>gui</module>
 * <class name="Global Constants">
 *  The following list shows all the constants that are defined on the <I>Global</I> object.
 * </class>
 */

JSConstDoubleSpec wxGlobalMap[] = 
{
    WXJS_SIMPLE_CONSTANT(wxNOT_FOUND)
    WXJS_SIMPLE_CONSTANT(wxInvalidOffset)
	{ 0 }
};

/***
 * <constants>
 *  <type name="wxDirection">
 *   <constant name="TOP" />
 *   <constant name="BOTTOM" />
 *   <constant name="LEFT" />
 *   <constant name="TOP" />
 *   <constant name="UP" />
 *   <constant name="DOWN" />
 *   <constant name="RIGHT" />
 *   <constant name="NORTH" />
 *   <constant name="SOUTH" />
 *   <constant name="WEST" />
 *   <constant name="EAST" />
 *   <constant name="ALL" />
 *   <desc>
 *    See @wxSizer, @wxBoxSizer, @wxFlexGridSizer, @wxGridSizer
 *   </desc>
 *  </type>
 */
JSConstDoubleSpec wxDirectionMap[] =
{
	WXJS_CONSTANT(wx, TOP)
	WXJS_CONSTANT(wx, BOTTOM)
	WXJS_CONSTANT(wx, LEFT)
	WXJS_CONSTANT(wx, TOP)
	WXJS_CONSTANT(wx, UP)
	WXJS_CONSTANT(wx, DOWN)
    WXJS_CONSTANT(wx, RIGHT)
    WXJS_CONSTANT(wx, NORTH)
    WXJS_CONSTANT(wx, SOUTH)
    WXJS_CONSTANT(wx, WEST)
    WXJS_CONSTANT(wx, EAST)
    WXJS_CONSTANT(wx, ALL)
	{ 0 }
};

/***
 * <type name="wxStretch">
 *  <constant name="NOT" />
 *  <constant name="SHRINK" />
 *  <constant name="GROW" />
 *  <constant name="EXPAND" />
 *  <constant name="SHAPED" />
 *  <desc>
 *   See @wxSizer, @wxBoxSizer, @wxFlexGridSizer, @wxGridSizer
 *  </desc>
 * </type>
 */
JSConstDoubleSpec wxStretchMap[] = 
{
    WXJS_CONSTANT(wxSTRETCH_, NOT)
    WXJS_CONSTANT(wx, SHRINK)
    WXJS_CONSTANT(wx, GROW)
    WXJS_CONSTANT(wx, EXPAND)
    WXJS_CONSTANT(wx, SHAPED)
	{ 0 }
};

/***
 * <type name="wxOrientation">
 *  <constant name="HORIZONTAL" />
 *  <constant name="VERTICAL" />
 *  <constant name="BOTH" />
 *  <desc>
 *   See @wxSizer, @wxBoxSizer, @wxFlexGridSizer, @wxGridSizer, 
 *   wxWindow @wxWindow#centre, wxWindow @wxWindow#centreOnParent,
 *   wxWindow @wxWindow#centreOnScreen
 *  </desc>
 * </type>
 */
JSConstDoubleSpec wxOrientationMap[] = 
{
    WXJS_CONSTANT(wx, HORIZONTAL)
	WXJS_CONSTANT(wx, VERTICAL)
	WXJS_CONSTANT(wx, BOTH)
	{ 0 }
};

/***
 * <method name="wxAlignment">
 *   <constant name="NOT" />
 *   <constant name="CENTER_HORIZONTAL" />
 *   <constant name="CENTRE_HORIZONTAL" />
 *   <constant name="LEFT" />
 *   <constant name="TOP" />
 *   <constant name="RIGHT" />
 *   <constant name="BOTTOM" />
 *   <constant name="CENTER_VERTICAL" />
 *   <constant name="CENTRE_VERTICAL" />
 *   <constant name="CENTER" />
 *   <constant name="CENTRE" />
 *   <desc>
 *    See @wxSizer, @wxBoxSizer, @wxFlexGridSizer, @wxGridSizer
 *   </desc>
 * </method>
 */
JSConstDoubleSpec wxAlignmentMap[] = 
{
    WXJS_CONSTANT(wxALIGN_, NOT)
    WXJS_CONSTANT(wxALIGN_, CENTER_HORIZONTAL)
    WXJS_CONSTANT(wxALIGN_, CENTRE_HORIZONTAL)
    WXJS_CONSTANT(wxALIGN_, LEFT)
    WXJS_CONSTANT(wxALIGN_, TOP)
    WXJS_CONSTANT(wxALIGN_, RIGHT)
    WXJS_CONSTANT(wxALIGN_, BOTTOM)
    WXJS_CONSTANT(wxALIGN_, CENTER_VERTICAL)
    WXJS_CONSTANT(wxALIGN_, CENTRE_VERTICAL)
    WXJS_CONSTANT(wxALIGN_, CENTER)
    WXJS_CONSTANT(wxALIGN_, CENTRE)
	{ 0 }
};

/***
 * <type name="wxId">
 *   <constant name="LOWEST" />
 *   <constant name="HIGHEST" />
 *   <constant name="SEPARATOR" />
 *   <constant name="OPEN" />
 *   <constant name="CLOSE" />
 *   <constant name="NEW" />
 *   <constant name="SAVE" />
 *   <constant name="SAVEAS" />
 *   <constant name="REVERT" />
 *   <constant name="EXIT" />
 *   <constant name="UNDO" />
 *   <constant name="REDO" />
 *   <constant name="HELP" />
 *   <constant name="PRINT" />
 *   <constant name="PRINT_SETUP" />
 *   <constant name="PREVIEW" />
 *   <constant name="ABOUT" />
 *   <constant name="HELP_CONTENTS" />
 *   <constant name="HELP_COMMANDS" />
 *   <constant name="HELP_PROCEDURES" />
 *   <constant name="HELP_CONTEXT" />
 *   <constant name="CUT" />
 *   <constant name="COPY" />
 *   <constant name="PASTE" />
 *   <constant name="CLEAR" />
 *   <constant name="FIND" />
 *   <constant name="DUPLICATE" />
 *   <constant name="SELECTALL" />
 *   <constant name="FILE1" />
 *   <constant name="FILE2" />
 *   <constant name="FILE3" />
 *   <constant name="FILE4" />
 *   <constant name="FILE5" />
 *   <constant name="FILE6" />
 *   <constant name="FILE7" />
 *   <constant name="FILE8" />
 *   <constant name="FILE9" />
 *   <constant name="OK" />
 *   <constant name="CANCEL" />
 *   <constant name="APPLY" />
 *   <constant name="YES" />
 *   <constant name="NO" />
 *   <constant name="STATIC" />
 *   <constant name="FORWARD" />
 *   <constant name="BACKWARD" />
 *   <constant name="DEFAULT" />
 *   <constant name="MORE" />
 *   <constant name="SETUP" />
 *   <constant name="RESET" />
 *   <constant name="CONTEXT_HELP" />
 *   <constant name="YESTOALL" />
 *   <constant name="NOTOALL" />
 *   <constant name="ABORT" />
 *   <constant name="RETRY" />
 *   <constant name="IGNORE" />
 *  </type>
 */
JSConstDoubleSpec wxIdMap[] = 
{
    WXJS_CONSTANT(wxID_, LOWEST)
    WXJS_CONSTANT(wxID_, HIGHEST)

    WXJS_CONSTANT(wxID_, SEPARATOR)
    WXJS_CONSTANT(wxID_, OPEN)
    WXJS_CONSTANT(wxID_, CLOSE)
    WXJS_CONSTANT(wxID_, NEW)
    WXJS_CONSTANT(wxID_, SAVE)
    WXJS_CONSTANT(wxID_, SAVEAS)
    WXJS_CONSTANT(wxID_, REVERT)
    WXJS_CONSTANT(wxID_, EXIT)
    WXJS_CONSTANT(wxID_, UNDO)
    WXJS_CONSTANT(wxID_, REDO)
    WXJS_CONSTANT(wxID_, HELP)
    WXJS_CONSTANT(wxID_, PRINT)
    WXJS_CONSTANT(wxID_, PRINT_SETUP)
    WXJS_CONSTANT(wxID_, PREVIEW)
    WXJS_CONSTANT(wxID_, ABOUT)
    WXJS_CONSTANT(wxID_, HELP_CONTENTS)
    WXJS_CONSTANT(wxID_, HELP_COMMANDS)
    WXJS_CONSTANT(wxID_, HELP_PROCEDURES)
    WXJS_CONSTANT(wxID_, HELP_CONTEXT)

    WXJS_CONSTANT(wxID_, CUT)
    WXJS_CONSTANT(wxID_, COPY)
    WXJS_CONSTANT(wxID_, PASTE)
    WXJS_CONSTANT(wxID_, CLEAR)
    WXJS_CONSTANT(wxID_, FIND)
    WXJS_CONSTANT(wxID_, DUPLICATE)
    WXJS_CONSTANT(wxID_, SELECTALL)

    WXJS_CONSTANT(wxID_, FILE1)
    WXJS_CONSTANT(wxID_, FILE2)
    WXJS_CONSTANT(wxID_, FILE3)
    WXJS_CONSTANT(wxID_, FILE4)
    WXJS_CONSTANT(wxID_, FILE5)
    WXJS_CONSTANT(wxID_, FILE6)
    WXJS_CONSTANT(wxID_, FILE7)
    WXJS_CONSTANT(wxID_, FILE8)
    WXJS_CONSTANT(wxID_, FILE9)

// Standard button IDs
    WXJS_CONSTANT(wxID_, OK)
    WXJS_CONSTANT(wxID_, CANCEL)
    WXJS_CONSTANT(wxID_, APPLY)
    WXJS_CONSTANT(wxID_, YES)
    WXJS_CONSTANT(wxID_, NO)
    WXJS_CONSTANT(wxID_, STATIC)
    WXJS_CONSTANT(wxID_, FORWARD)
    WXJS_CONSTANT(wxID_, BACKWARD)
    WXJS_CONSTANT(wxID_, DEFAULT)
    WXJS_CONSTANT(wxID_, MORE)
    WXJS_CONSTANT(wxID_, SETUP)
    WXJS_CONSTANT(wxID_, RESET)
    WXJS_CONSTANT(wxID_, CONTEXT_HELP)
    WXJS_CONSTANT(wxID_, YESTOALL)
    WXJS_CONSTANT(wxID_, NOTOALL)
    WXJS_CONSTANT(wxID_, ABORT)
    WXJS_CONSTANT(wxID_, RETRY)
    WXJS_CONSTANT(wxID_, IGNORE)

    WXJS_CONSTANT(wxID_, FILEDLGG)
	{ 0 }
};

/***
 * <type name="wxKeyCode">
 *   <constant name="BACK" />
 *   <constant name="TAB" />
 *   <constant name="RETURN" />
 *   <constant name="ESCAPE" />
 *   <constant name="SPACE" />
 *   <constant name="DELETE" />
 *   <constant name="START" />
 *   <constant name="LBUTTON" />
 *   <constant name="RBUTTON" />
 *   <constant name="CANCEL" />
 *   <constant name="MBUTTON" />
 *   <constant name="CLEAR" />
 *   <constant name="SHIFT" />
 *   <constant name="ALT" />
 *   <constant name="CONTROL" />
 *   <constant name="MENU" />
 *   <constant name="PAUSE" />
 *   <constant name="CAPITAL" />
 *   <constant name="PRIOR" />
 *   <constant name="NEXT" />
 *   <constant name="END" />
 *   <constant name="HOME" />
 *   <constant name="LEFT" />
 *   <constant name="UP" />
 *   <constant name="RIGHT" />
 *   <constant name="DOWN" />
 *   <constant name="SELECT" />
 *   <constant name="PRINT" />
 *   <constant name="EXECUTE" />
 *   <constant name="SNAPSHOT" />
 *   <constant name="INSERT" />
 *   <constant name="HELP" />
 *   <constant name="NUMPAD0" />
 *   <constant name="NUMPAD1" />
 *   <constant name="NUMPAD2" />
 *   <constant name="NUMPAD3" />
 *   <constant name="NUMPAD4" />
 *   <constant name="NUMPAD5" />
 *   <constant name="NUMPAD6" />
 *   <constant name="NUMPAD7" />
 *   <constant name="NUMPAD8" />
 *   <constant name="NUMPAD9" />
 *   <constant name="MULTIPLY" />
 *   <constant name="ADD" />
 *   <constant name="SEPARATOR" />
 *   <constant name="SUBTRACT" />
 *   <constant name="DECIMAL" />
 *   <constant name="DIVIDE" />
 *   <constant name="F1" />
 *   <constant name="F2" />
 *   <constant name="F3" />
 *   <constant name="F4" />
 *   <constant name="F5" />
 *   <constant name="F6" />
 *   <constant name="F7" />
 *   <constant name="F8" />
 *   <constant name="F9" />
 *   <constant name="F10" />
 *   <constant name="F11" />
 *   <constant name="F12" />
 *   <constant name="F13" />
 *   <constant name="F14" />
 *   <constant name="F15" />
 *   <constant name="F16" />
 *   <constant name="F17" />
 *   <constant name="F18" />
 *   <constant name="F19" />
 *   <constant name="F20" />
 *   <constant name="F21" />
 *   <constant name="F22" />
 *   <constant name="F23" />
 *   <constant name="F24" />
 *   <constant name="NUMLOCK" />
 *   <constant name="SCROLL" />
 *   <constant name="PAGEUP" />
 *   <constant name="PAGEDOWN" />
 *   <constant name="NUMPAD_SPACE" />
 *   <constant name="NUMPAD_TAB" />
 *   <constant name="NUMPAD_ENTER" />
 *   <constant name="NUMPAD_F1" />
 *   <constant name="NUMPAD_F2" />
 *   <constant name="NUMPAD_F3" />
 *   <constant name="NUMPAD_F4" />
 *   <constant name="NUMPAD_HOME" />
 *   <constant name="NUMPAD_LEFT" />
 *   <constant name="NUMPAD_UP" />
 *   <constant name="NUMPAD_RIGHT" />
 *   <constant name="NUMPAD_DOWN" />
 *   <constant name="NUMPAD_PRIOR" />
 *   <constant name="NUMPAD_PAGEUP" />
 *   <constant name="NUMPAD_NEXT" />
 *   <constant name="NUMPAD_PAGEDOWN" />
 *   <constant name="NUMPAD_END" />
 *   <constant name="NUMPAD_BEGIN" />
 *   <constant name="NUMPAD_INSERT" />
 *   <constant name="NUMPAD_DELETE" />
 *   <constant name="NUMPAD_EQUAL" />
 *   <constant name="NUMPAD_MULTIPLY" />
 *   <constant name="NUMPAD_ADD" />
 *   <constant name="NUMPAD_SEPARATOR" />
 *   <constant name="NUMPAD_SUBTRACT" />
 *   <constant name="NUMPAD_DECIMAL" />
 *   <constant name="NUMPAD_DIVIDE" />
 *   <desc>
 *    See @wxKeyEvent
 *   </desc>
 * </type>
 * @endif 
 */
JSConstDoubleSpec wxKeyCodeMap[] = 
{
  WXJS_CONSTANT(WXK_, BACK)
  WXJS_CONSTANT(WXK_, TAB)
  WXJS_CONSTANT(WXK_, RETURN)
  WXJS_CONSTANT(WXK_, ESCAPE)
  WXJS_CONSTANT(WXK_, SPACE)
  WXJS_CONSTANT(WXK_, DELETE)
  WXJS_CONSTANT(WXK_, START)
  WXJS_CONSTANT(WXK_, LBUTTON)
  WXJS_CONSTANT(WXK_, RBUTTON)
  WXJS_CONSTANT(WXK_, CANCEL)
  WXJS_CONSTANT(WXK_, MBUTTON)
  WXJS_CONSTANT(WXK_, CLEAR)
  WXJS_CONSTANT(WXK_, SHIFT)
  WXJS_CONSTANT(WXK_, ALT)
  WXJS_CONSTANT(WXK_, CONTROL)
  WXJS_CONSTANT(WXK_, MENU)
  WXJS_CONSTANT(WXK_, PAUSE)
  WXJS_CONSTANT(WXK_, CAPITAL)
  WXJS_CONSTANT(WXK_, PRIOR)
  WXJS_CONSTANT(WXK_, NEXT)
  WXJS_CONSTANT(WXK_, END)
  WXJS_CONSTANT(WXK_, HOME)
  WXJS_CONSTANT(WXK_, LEFT)
  WXJS_CONSTANT(WXK_, UP)
  WXJS_CONSTANT(WXK_, RIGHT)
  WXJS_CONSTANT(WXK_, DOWN)
  WXJS_CONSTANT(WXK_, SELECT)
  WXJS_CONSTANT(WXK_, PRINT)
  WXJS_CONSTANT(WXK_, EXECUTE)
  WXJS_CONSTANT(WXK_, SNAPSHOT)
  WXJS_CONSTANT(WXK_, INSERT)
  WXJS_CONSTANT(WXK_, HELP)
  WXJS_CONSTANT(WXK_, NUMPAD0)
  WXJS_CONSTANT(WXK_, NUMPAD1)
  WXJS_CONSTANT(WXK_, NUMPAD2)
  WXJS_CONSTANT(WXK_, NUMPAD3)
  WXJS_CONSTANT(WXK_, NUMPAD4)
  WXJS_CONSTANT(WXK_, NUMPAD5)
  WXJS_CONSTANT(WXK_, NUMPAD6)
  WXJS_CONSTANT(WXK_, NUMPAD7)
  WXJS_CONSTANT(WXK_, NUMPAD8)
  WXJS_CONSTANT(WXK_, NUMPAD9)
  WXJS_CONSTANT(WXK_, MULTIPLY)
  WXJS_CONSTANT(WXK_, ADD)
  WXJS_CONSTANT(WXK_, SEPARATOR)
  WXJS_CONSTANT(WXK_, SUBTRACT)
  WXJS_CONSTANT(WXK_, DECIMAL)
  WXJS_CONSTANT(WXK_, DIVIDE)
  WXJS_CONSTANT(WXK_, F1)
  WXJS_CONSTANT(WXK_, F2)
  WXJS_CONSTANT(WXK_, F3)
  WXJS_CONSTANT(WXK_, F4)
  WXJS_CONSTANT(WXK_, F5)
  WXJS_CONSTANT(WXK_, F6)
  WXJS_CONSTANT(WXK_, F7)
  WXJS_CONSTANT(WXK_, F8)
  WXJS_CONSTANT(WXK_, F9)
  WXJS_CONSTANT(WXK_, F10)
  WXJS_CONSTANT(WXK_, F11)
  WXJS_CONSTANT(WXK_, F12)
  WXJS_CONSTANT(WXK_, F13)
  WXJS_CONSTANT(WXK_, F14)
  WXJS_CONSTANT(WXK_, F15)
  WXJS_CONSTANT(WXK_, F16)
  WXJS_CONSTANT(WXK_, F17)
  WXJS_CONSTANT(WXK_, F18)
  WXJS_CONSTANT(WXK_, F19)
  WXJS_CONSTANT(WXK_, F20)
  WXJS_CONSTANT(WXK_, F21)
  WXJS_CONSTANT(WXK_, F22)
  WXJS_CONSTANT(WXK_, F23)
  WXJS_CONSTANT(WXK_, F24)
  WXJS_CONSTANT(WXK_, NUMLOCK)
  WXJS_CONSTANT(WXK_, SCROLL)
  WXJS_CONSTANT(WXK_, PAGEUP)
  WXJS_CONSTANT(WXK_, PAGEDOWN)
  WXJS_CONSTANT(WXK_, NUMPAD_SPACE)
  WXJS_CONSTANT(WXK_, NUMPAD_TAB)
  WXJS_CONSTANT(WXK_, NUMPAD_ENTER)
  WXJS_CONSTANT(WXK_, NUMPAD_F1)
  WXJS_CONSTANT(WXK_, NUMPAD_F2)
  WXJS_CONSTANT(WXK_, NUMPAD_F3)
  WXJS_CONSTANT(WXK_, NUMPAD_F4)
  WXJS_CONSTANT(WXK_, NUMPAD_HOME)
  WXJS_CONSTANT(WXK_, NUMPAD_LEFT)
  WXJS_CONSTANT(WXK_, NUMPAD_UP)
  WXJS_CONSTANT(WXK_, NUMPAD_RIGHT)
  WXJS_CONSTANT(WXK_, NUMPAD_DOWN)
  WXJS_CONSTANT(WXK_, NUMPAD_PRIOR)
  WXJS_CONSTANT(WXK_, NUMPAD_PAGEUP)
  WXJS_CONSTANT(WXK_, NUMPAD_NEXT)
  WXJS_CONSTANT(WXK_, NUMPAD_PAGEDOWN)
  WXJS_CONSTANT(WXK_, NUMPAD_END)
  WXJS_CONSTANT(WXK_, NUMPAD_BEGIN)
  WXJS_CONSTANT(WXK_, NUMPAD_INSERT)
  WXJS_CONSTANT(WXK_, NUMPAD_DELETE)
  WXJS_CONSTANT(WXK_, NUMPAD_EQUAL)
  WXJS_CONSTANT(WXK_, NUMPAD_MULTIPLY)
  WXJS_CONSTANT(WXK_, NUMPAD_ADD)
  WXJS_CONSTANT(WXK_, NUMPAD_SEPARATOR)
  WXJS_CONSTANT(WXK_, NUMPAD_SUBTRACT)
  WXJS_CONSTANT(WXK_, NUMPAD_DECIMAL)
  WXJS_CONSTANT(WXK_, NUMPAD_DIVIDE)
  { 0 }
};

/***
 * <type name="wxCalendarDateBorder">
 *   <constant name="NONE" />
 *   <constant name="SQUARE" />
 *   <constant name="ROUND" />
 *   <desc>
 *    See @wxCalendarDateAttr, @wxCalendarCtrl
 *   </desc>
 * </type>
 */
JSConstDoubleSpec wxCalendarDateBorderMap[] = 
{
    WXJS_CONSTANT(wxCAL_BORDER_, NONE)
    WXJS_CONSTANT(wxCAL_BORDER_, SQUARE)
    WXJS_CONSTANT(wxCAL_BORDER_, ROUND)
	{ 0 }
};

/***
 * <type name="wxFontEncoding">
 *   <constant name="SYSTEM">system default</constant>
 *   <constant name="DEFAULT">current default encoding</constant>
 *   <constant name="ISO8859_1">West European (Latin1)</constant>
 *   <constant name="ISO8859_2">Central and East European (Latin2)</constant>
 *   <constant name="ISO8859_3">Esperanto (Latin3)</constant>
 *   <constant name="ISO8859_4">Baltic (old) (Latin4)</constant>
 *   <constant name="ISO8859_5">Cyrillic</constant>
 *   <constant name="ISO8859_6">Arabic</constant>
 *   <constant name="ISO8859_7">Greek</constant>
 *   <constant name="ISO8859_8">Hebrew</constant>
 *   <constant name="ISO8859_9">Turkish (Latin5)</constant>
 *   <constant name="ISO8859_10">Variation of Latin4 (Latin6)</constant>
 *   <constant name="ISO8859_11">Thai</constant>
 *   <constant name="ISO8859_12">Doesn't exist currently, but put it
 *                       here anyhow to make all ISO8859
 *                       consecutive numbers</constant>
 *   <constant name="ISO8859_13">Baltic (Latin7)</constant>
 *   <constant name="ISO8859_14">Latin8</constant>
 *   <constant name="ISO8859_15">Latin9 (a.k.a. Latin0, includes euro)</constant>
 *   <constant name="ISO8859_MAX" />
 *   <constant name="KOI8">We don't support any of KOI8 variants</constant>
 *   <constant name="ALTERNATIVE">same as MS-DOS CP866</constant>
 *   <constant name="BULGARIAN">used under Linux in Bulgaria</constant>
 *   <constant name="CP437">original MS-DOS codepage</constant>
 *   <constant name="CP850">CP437 merged with Latin1</constant>
 *   <constant name="CP852">CP437 merged with Latin2</constant>
 *   <constant name="CP855">another cyrillic encoding</constant>
 *   <constant name="CP866">and another one and for Windows</constant>
 *   <constant name="CP874">WinThai</constant>
 *   <constant name="CP932">Japanese (shift-JIS)</constant>
 *   <constant name="CP936">Chinese simplified (GB)</constant>
 *   <constant name="CP949">Korean (Hangul charset)</constant>
 *   <constant name="CP950">Chinese (traditional - Big5)</constant>
 *   <constant name="CP1250">WinLatin2</constant>
 *   <constant name="CP1251">WinCyrillic</constant>
 *   <constant name="CP1252">WinLatin1</constant>
 *   <constant name="CP1253">WinGreek (8859-7)</constant>
 *   <constant name="CP1254">WinTurkish</constant>
 *   <constant name="CP1255">WinHebrew</constant>
 *   <constant name="CP1256">WinArabic</constant>
 *   <constant name="CP1257">WinBaltic (same as Latin 7)</constant>
 *   <constant name="CP12_MAX" />
 *   <constant name="UTF7">UTF-7 Unicode encoding</constant>
 *   <constant name="UTF8">UTF-8 Unicode encoding</constant>
 *   <constant name="UNICODE">Unicode - currently used only by wxEncodingConverter class</constant>
 *   <constant name="MAX" />
 * </type>
 */
JSConstDoubleSpec wxFontEncodingMap[] = 
{
    WXJS_CONSTANT(wxFONTENCODING_, SYSTEM)
    WXJS_CONSTANT(wxFONTENCODING_, DEFAULT)
    WXJS_CONSTANT(wxFONTENCODING_, ISO8859_1)
    WXJS_CONSTANT(wxFONTENCODING_, ISO8859_2)
    WXJS_CONSTANT(wxFONTENCODING_, ISO8859_3)
    WXJS_CONSTANT(wxFONTENCODING_, ISO8859_4)
    WXJS_CONSTANT(wxFONTENCODING_, ISO8859_5)
    WXJS_CONSTANT(wxFONTENCODING_, ISO8859_6)
    WXJS_CONSTANT(wxFONTENCODING_, ISO8859_7)
    WXJS_CONSTANT(wxFONTENCODING_, ISO8859_8)
    WXJS_CONSTANT(wxFONTENCODING_, ISO8859_9)
    WXJS_CONSTANT(wxFONTENCODING_, ISO8859_10)
    WXJS_CONSTANT(wxFONTENCODING_, ISO8859_11)
    WXJS_CONSTANT(wxFONTENCODING_, ISO8859_12)
    WXJS_CONSTANT(wxFONTENCODING_, ISO8859_13)
    WXJS_CONSTANT(wxFONTENCODING_, ISO8859_14)
    WXJS_CONSTANT(wxFONTENCODING_, ISO8859_15)
    WXJS_CONSTANT(wxFONTENCODING_, ISO8859_MAX)
    WXJS_CONSTANT(wxFONTENCODING_, KOI8)
    WXJS_CONSTANT(wxFONTENCODING_, ALTERNATIVE)
    WXJS_CONSTANT(wxFONTENCODING_, BULGARIAN)
    WXJS_CONSTANT(wxFONTENCODING_, CP437)
    WXJS_CONSTANT(wxFONTENCODING_, CP850)
    WXJS_CONSTANT(wxFONTENCODING_, CP852)
    WXJS_CONSTANT(wxFONTENCODING_, CP855)
    WXJS_CONSTANT(wxFONTENCODING_, CP866)
    WXJS_CONSTANT(wxFONTENCODING_, CP874)
    WXJS_CONSTANT(wxFONTENCODING_, CP932)
    WXJS_CONSTANT(wxFONTENCODING_, CP936)
    WXJS_CONSTANT(wxFONTENCODING_, CP949)
    WXJS_CONSTANT(wxFONTENCODING_, CP950)
    WXJS_CONSTANT(wxFONTENCODING_, CP1250)
    WXJS_CONSTANT(wxFONTENCODING_, CP1251)
    WXJS_CONSTANT(wxFONTENCODING_, CP1252)
    WXJS_CONSTANT(wxFONTENCODING_, CP1253)
    WXJS_CONSTANT(wxFONTENCODING_, CP1254)
    WXJS_CONSTANT(wxFONTENCODING_, CP1255)
    WXJS_CONSTANT(wxFONTENCODING_, CP1256)
    WXJS_CONSTANT(wxFONTENCODING_, CP1257)
    WXJS_CONSTANT(wxFONTENCODING_, CP12_MAX)
    WXJS_CONSTANT(wxFONTENCODING_, UTF7)
    WXJS_CONSTANT(wxFONTENCODING_, UTF8)
    WXJS_CONSTANT(wxFONTENCODING_, UNICODE)
    WXJS_CONSTANT(wxFONTENCODING_, MAX)
	{ 0 }
};

/***
 * <type name="wxBitmapType">
 *  <desc>
 *   The validity of these flags depends on the platform and wxWidgets configuration. 
 *   If all possible wxWidgets settings are used, the Windows platform supports BMP file,
 *   BMP resource, XPM data, and XPM. Under wxGTK, the available formats are BMP file, 
 *   XPM data, XPM file, and PNG file. Under wxMotif, the available formats are XBM data,
 *   XBM file, XPM data, XPM file.
 *  </desc>
 *   <constant name="INVALID" />
 *   <constant name="BMP" />
 *   <constant name="BMP_RESOURCE" />
 *   <constant name="RESOURCE" />
 *   <constant name="ICO" />         
 *   <constant name="ICO_RESOURCE" />
 *   <constant name="CUR" />     
 *   <constant name="CUR_RESOURCE" />
 *   <constant name="XBM" />     
 *   <constant name="XBM_DATA" />
 *   <constant name="XPM" />         
 *   <constant name="XPM_DATA" />
 *   <constant name="TIF" />         
 *   <constant name="TIF_RESOURCE" />
 *   <constant name="GIF" />     
 *   <constant name="GIF_RESOURCE" />
 *   <constant name="PNG" />     
 *   <constant name="PNG_RESOURCE" />
 *   <constant name="JPEG" />     
 *   <constant name="JPEG_RESOURCE" />
 *   <constant name="PNM" />    
 *   <constant name="PNM_RESOURCE" />
 *   <constant name="PCX" />     
 *   <constant name="PCX_RESOURCE" />
 *   <constant name="PICT" />     
 *   <constant name="PICT_RESOURCE" />
 *   <constant name="ICON" />    
 *   <constant name="ICON_RESOURCE" />
 *   <constant name="MACCURSOR" />    
 *   <constant name="MACCURSOR_RESOURCE" />
 *   <constant name="ANY" />
 * </type>
 */
JSConstDoubleSpec wxBitmapTypeMap[] = 
{
    WXJS_CONSTANT(wxBITMAP_TYPE_, INVALID)
    WXJS_CONSTANT(wxBITMAP_TYPE_, BMP)
    WXJS_CONSTANT(wxBITMAP_TYPE_, BMP_RESOURCE)
    WXJS_CONSTANT(wxBITMAP_TYPE_, RESOURCE)
    WXJS_CONSTANT(wxBITMAP_TYPE_, ICO)
    WXJS_CONSTANT(wxBITMAP_TYPE_, ICO_RESOURCE)
    WXJS_CONSTANT(wxBITMAP_TYPE_, CUR)
    WXJS_CONSTANT(wxBITMAP_TYPE_, CUR_RESOURCE)
    WXJS_CONSTANT(wxBITMAP_TYPE_, XBM)
    WXJS_CONSTANT(wxBITMAP_TYPE_, XBM_DATA)
    WXJS_CONSTANT(wxBITMAP_TYPE_, XPM)
    WXJS_CONSTANT(wxBITMAP_TYPE_, XPM_DATA)
    WXJS_CONSTANT(wxBITMAP_TYPE_, TIF)
    WXJS_CONSTANT(wxBITMAP_TYPE_, TIF_RESOURCE)
    WXJS_CONSTANT(wxBITMAP_TYPE_, GIF)
    WXJS_CONSTANT(wxBITMAP_TYPE_, GIF_RESOURCE)
    WXJS_CONSTANT(wxBITMAP_TYPE_, PNG)
    WXJS_CONSTANT(wxBITMAP_TYPE_, PNG_RESOURCE)
    WXJS_CONSTANT(wxBITMAP_TYPE_, JPEG)
    WXJS_CONSTANT(wxBITMAP_TYPE_, JPEG_RESOURCE)
    WXJS_CONSTANT(wxBITMAP_TYPE_, PNM)
    WXJS_CONSTANT(wxBITMAP_TYPE_, PNM_RESOURCE)
    WXJS_CONSTANT(wxBITMAP_TYPE_, PCX)
    WXJS_CONSTANT(wxBITMAP_TYPE_, PCX_RESOURCE)
    WXJS_CONSTANT(wxBITMAP_TYPE_, PICT)
    WXJS_CONSTANT(wxBITMAP_TYPE_, PICT_RESOURCE)
    WXJS_CONSTANT(wxBITMAP_TYPE_, ICON)
    WXJS_CONSTANT(wxBITMAP_TYPE_, ICON_RESOURCE)
    WXJS_CONSTANT(wxBITMAP_TYPE_, MACCURSOR)
    WXJS_CONSTANT(wxBITMAP_TYPE_, MACCURSOR_RESOURCE)
    WXJS_CONSTANT(wxBITMAP_TYPE_, ANY)
	{ 0 }
};

/***
 * <type name="wxFilter">
 *   <constant name="NONE">
 *    No filtering
 *   </constant>
 *   <constant name="ASCII">
 *    Non-ASCII characters are filtered out.
 *   </constant>
 *   <constant name="ALPHA">
 *    Non-alpha characters are filtered out. 
 *   </constant>
 *   <constant name="ALPHANUMERIC">
 *    Non-alphanumeric characters are filtered out. 
 *   </constant>
 *   <constant name="NUMERIC">
 *    Non-numeric characters are filtered out. 
 *   </constant>
 *   <constant name="INCLUDE_LIST">
 *    Use an include list. The validator checks if the user input is on the list,
 *    complaining if not. 
 *   </constant>
 *   <constant name="EXCLUDE_LIST">
 *    Use an exclude list. The validator checks if the user input is on the list,
 *    complaining if it is. 
 *   </constant>
 *   <constant name="INCLUDE_CHAR_LIST">
 *    Use an include list. The validator checks if each input character is in 
 *    the list (one character per list element), complaining if not. 
 *   </constant>
 *   <constant name="EXCLUDE_CHAR_LIST">
 *    Use an include list. The validator checks if each input character is 
 *    in the list (one character per list element), complaining if it is. 
 *   </constant>
 *   <desc>
 *    See @wxTextValidator
 *   </desc>
 * </type>
 */
JSConstDoubleSpec wxFilterMap[] = 
{
    WXJS_CONSTANT(wxFILTER_, NONE)
    WXJS_CONSTANT(wxFILTER_, ASCII)
    WXJS_CONSTANT(wxFILTER_, ALPHA)
    WXJS_CONSTANT(wxFILTER_, ALPHANUMERIC)
    WXJS_CONSTANT(wxFILTER_, NUMERIC)
    WXJS_CONSTANT(wxFILTER_, INCLUDE_LIST)
    WXJS_CONSTANT(wxFILTER_, EXCLUDE_LIST)
    WXJS_CONSTANT(wxFILTER_, INCLUDE_CHAR_LIST)
    WXJS_CONSTANT(wxFILTER_, EXCLUDE_CHAR_LIST)
	{ 0 }
};

/***
 * <type name="wxItemKind">
 *   <constant name="SEPARATOR" />
 *   <constant name="NORMAL" />
 *   <constant name="CHECK" />
 *   <constant name="RADIO" />
 *   <constant name="MAX" />
 *   <desc>
 *    See @wxMenuBar, @wxMenu, @wxToolBar
 *   </desc>
 *  </type>
 * </constants>
 */
JSConstDoubleSpec wxItemKindMap[] = 
{
	WXJS_CONSTANT(wxITEM_, SEPARATOR)
	WXJS_CONSTANT(wxITEM_, NORMAL)
	WXJS_CONSTANT(wxITEM_, CHECK)
	WXJS_CONSTANT(wxITEM_, RADIO)
	WXJS_CONSTANT(wxITEM_, MAX)
	{ 0 }
};

JSConstDoubleSpec wxBorderMap[] = 
{
	WXJS_CONSTANT(wxBORDER_, DEFAULT)
	WXJS_CONSTANT(wxBORDER_, NONE)
	WXJS_CONSTANT(wxBORDER_, STATIC)
	WXJS_CONSTANT(wxBORDER_, SIMPLE)
	WXJS_CONSTANT(wxBORDER_, RAISED)
	WXJS_CONSTANT(wxBORDER_, SUNKEN)
	WXJS_CONSTANT(wxBORDER_, DOUBLE)
	WXJS_CONSTANT(wxBORDER_, MASK)
	{ 0 }
};

void wxjs::gui::InitGuiConstants(JSContext *cx, JSObject *obj)
{
	// Define the global constants

	JS_DefineConstDoubles(cx, obj, wxGlobalMap);

	// Create all the separate constant objects.

	JSObject *constObj = JS_DefineObject(cx, obj, "wxDirection", 
										 NULL, NULL,
										 JSPROP_READONLY | JSPROP_PERMANENT);
	JS_DefineConstDoubles(cx, constObj, wxDirectionMap);

	constObj = JS_DefineObject(cx, obj, "wxStretch", 
										NULL, NULL,
										JSPROP_READONLY | JSPROP_PERMANENT);
	JS_DefineConstDoubles(cx, constObj, wxStretchMap);

	constObj = JS_DefineObject(cx, obj, "wxOrientation", 
										NULL, NULL,
										JSPROP_READONLY | JSPROP_PERMANENT);
	JS_DefineConstDoubles(cx, constObj, wxOrientationMap);

	constObj = JS_DefineObject(cx, obj, "wxAlignment", 
										NULL, NULL,
										JSPROP_READONLY | JSPROP_PERMANENT);
	JS_DefineConstDoubles(cx, constObj, wxAlignmentMap);

	constObj = JS_DefineObject(cx, obj, "wxId", 
										NULL, NULL,
										JSPROP_READONLY | JSPROP_PERMANENT);
	JS_DefineConstDoubles(cx, constObj, wxIdMap);

	constObj = JS_DefineObject(cx, obj, "wxKeyCode", 
										NULL, NULL,
										JSPROP_READONLY | JSPROP_PERMANENT);
	JS_DefineConstDoubles(cx, constObj, wxKeyCodeMap);

	constObj = JS_DefineObject(cx, obj, "wxCalendarDateBorder", 
										NULL, NULL,
										JSPROP_READONLY | JSPROP_PERMANENT);
	JS_DefineConstDoubles(cx, constObj, wxCalendarDateBorderMap);

	constObj = JS_DefineObject(cx, obj, "wxFontEncoding", 
										NULL, NULL,
										JSPROP_READONLY | JSPROP_PERMANENT);
	JS_DefineConstDoubles(cx, constObj, wxCalendarDateBorderMap);

	constObj = JS_DefineObject(cx, obj, "wxBitmapType", 
										NULL, NULL,
										JSPROP_READONLY | JSPROP_PERMANENT);
	JS_DefineConstDoubles(cx, constObj, wxBitmapTypeMap);

    constObj = JS_DefineObject(cx, obj, "wxFilter", 
										NULL, NULL,
										JSPROP_READONLY | JSPROP_PERMANENT);

	JS_DefineConstDoubles(cx, constObj, wxFilterMap);

    constObj = JS_DefineObject(cx, obj, "wxItemKind", 
										NULL, NULL,
										JSPROP_READONLY | JSPROP_PERMANENT);
	JS_DefineConstDoubles(cx, constObj, wxItemKindMap);

    constObj = JS_DefineObject(cx, obj, "wxBorder", 
										NULL, NULL,
										JSPROP_READONLY | JSPROP_PERMANENT);
	JS_DefineConstDoubles(cx, constObj, wxBorderMap);
}
