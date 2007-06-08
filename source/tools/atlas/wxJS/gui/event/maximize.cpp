#include "precompiled.h"

/*
 * wxJavaScript - maximize.cpp
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
 * $Id: maximize.cpp 598 2007-03-07 20:13:28Z fbraem $
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
 * <file>event/maximize</file>
 * <module>gui</module>
 * <class name="wxMaximizeEvent" prototype="@wxEvent">
 *	An event being sent when the frame is maximized (minimized) or restored.
 *  See @wxFrame.
 * </class>
 */
WXJS_INIT_CLASS(MaximizeEvent, "wxMaximizeEvent", 0)
