#include "precompiled.h"

/*
 * wxJavaScript - syscol.cpp
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
 * $Id: syscol.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
// jsevent.cpp
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../../common/main.h"

#include "jsevent.h"
#include "event.h"

using namespace wxjs;
using namespace wxjs::gui;

/***
 * <file>event/syscol</file>
 * <module>gui</module>
 * <class name="wxSysColourChangedEvent" prototype="@wxEvent">
 *	An event being sent when a system colour is changed.
 *  See @wxPanel.
 * </class>
 */
WXJS_INIT_CLASS(SysColourChangedEvent, "wxSysColourChangedEvent", 0)
