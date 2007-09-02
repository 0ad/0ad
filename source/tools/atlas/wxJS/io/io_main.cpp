#include "precompiled.h"

/*
 * wxJavaScript - main.cpp
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
 * $Id: main.cpp 777 2007-06-24 19:42:27Z fbraem $
 */
// main.cpp
#ifdef __WXMSW__
	#include <windows.h>
    #include <wx/msw/private.h>
#endif
#include <wx/app.h>

#include "../common/main.h"
#include "../common/wxjs.h"
#include "init.h"

using namespace wxjs;
using namespace wxjs::io;

#ifdef __WXMSW__
	BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
	{
		BOOL result = TRUE;
	
		switch(fdwReason)
		{
		case DLL_PROCESS_ATTACH:
            {
			    DisableThreadLibraryCalls(hinstDLL);
			    break;
            }
		case DLL_PROCESS_DETACH:
			break;
		}    

		return result;
	}
#endif

WXJSAPI bool wxJS_InitClass(JSContext *cx, JSObject *global)
{
    return InitClass(cx, global);
}

WXJSAPI bool wxJS_InitObject(JSContext *cx, JSObject *global)
{
    return InitObject(cx, global);
}

WXJSAPI void wxJS_Destroy()
{
    Destroy();
}
