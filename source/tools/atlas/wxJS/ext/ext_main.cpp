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
 * $Id: main.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
#ifdef __WXMSW__
	#include <windows.h>
#endif
#include "../common/main.h"

#include "jsmembuf.h"
#include "wxjs_ext.h"

using namespace wxjs;
using namespace wxjs::ext;

/*
#ifdef __WXMSW__
	BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
	{
		BOOL result = TRUE;
	
		switch(fdwReason)
		{
		case DLL_PROCESS_ATTACH:
			DisableThreadLibraryCalls(hinstDLL);
			break;
		case DLL_PROCESS_DETACH:
			break;
		}    
	
		return result;
	}
#endif

WXJSAPI bool wxJS_EXTInit(JSContext *cx, JSObject *global)
{
	MemoryBuffer::JSInit(cx, global);
    return true;
}

WXJSAPI bool wxJS_EXTInitClass(JSContext *cx, JSObject *obj)
{
    return true;
}

WXJSAPI void wxJS_EXTDestroy()
{
}
*/

JSObject *wxjs::ext::CreateMemoryBuffer(JSContext *cx, void *buffer, int size)
{
	wxMemoryBuffer *membuf = new wxMemoryBuffer(size);
	membuf->AppendData(buffer, size);
	JSObject *obj = JS_NewObject(cx, MemoryBuffer::GetClass(), NULL, NULL);
	JS_SetPrivate(cx, obj, membuf);
	return obj;
}

wxMemoryBuffer* wxjs::ext::GetMemoryBuffer(JSContext *cx, JSObject *obj)
{
	return MemoryBuffer::GetPrivate(cx, obj);
}

wxMemoryBuffer* wxjs::ext::NewMemoryBuffer(void *buffer, int size)
{
	wxMemoryBuffer *membuf = new wxMemoryBuffer(size);
	membuf->AppendData(buffer, size);
	return membuf;
}
