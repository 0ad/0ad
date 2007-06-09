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
 * $Id$
 */
// main.cpp
#include <wx/setup.h>

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif
#include <wx/intl.h>

#ifdef __WXMSW__
	#include <wx/msw/private.h>
#endif

#include <js/jsapi.h>
#include "../common/wxjs.h"
#include "../common/main.h"
 
#include "init.h"

/*
#if defined( __WXMSW__)
	void WXDLLEXPORT wxEntryCleanup();
	extern "C" 
	{
	  WXJSAPI BOOL APIENTRY DllMain(HINSTANCE, DWORD, LPVOID); 
	}
	
	WXJSAPI BOOL APIENTRY DllMain(HINSTANCE hinstDLL, 
                                  DWORD fdwReason, 
                                  LPVOID WXUNUSED(lpvReserved))
	{
		BOOL result = TRUE;
	
		switch(fdwReason)
		{
		case DLL_PROCESS_ATTACH:
			{
				wxSetInstance(hinstDLL);
				DisableThreadLibraryCalls(hinstDLL);
    		    int app_argc = 0;
	    	    char **app_argv = NULL;
		        wxEntryStart(app_argc, app_argv);
				break;
			}
		case DLL_PROCESS_DETACH:
            wxEntryCleanup();
			break;
		}    
	
		return result;
	}
#endif

WXJSAPI bool wxJS_InitClass(JSContext *cx, JSObject *global)
{
    return wxjs::gui::InitClass(cx, global);
}

WXJSAPI bool wxJS_InitObject(JSContext *cx, JSObject *obj)
{
    return wxjs::gui::InitObject(cx, obj);
}

WXJSAPI void wxJS_Destroy()
{
  wxjs::gui::Destroy();
}
*/