#include "precompiled.h"

/*
 * wxJavaScript - fontlist.cpp
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
 * $Id: fontlist.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
// fontlist.cpp
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../../common/main.h"

#include "font.h"
#include "fontlist.h"

using namespace wxjs;
using namespace wxjs::gui;

/***
 * <file>misc/fontlist</file>
 * <module>gui</module>
 * <class name="wxFontList">
 *  A font list is a list containing all fonts which have been created. 
 *  There is only one instance of this class: wxTheFontList. Use this object 
 *  to search for a previously created font of the desired type and create it 
 *  if not already found. In some windowing systems, the font may be a scarce resource, 
 *  so it is best to reuse old resources if possible. 
 *  When an application finishes, all fonts will be deleted and their resources freed, 
 *  eliminating the possibility of 'memory leaks'.
 *  See @wxFont, @wxFontEncoding and @wxFontEnumerator
 * </class>
 */
WXJS_INIT_CLASS(FontList, "wxFontList", 0)

// To avoid delete of wxTheFontList
void FontList::Destruct(JSContext *cx, wxFontList *p)
{
}

WXJS_BEGIN_METHOD_MAP(FontList)
  WXJS_METHOD("findOrCreateFont", findOrCreate, 7)
WXJS_END_METHOD_MAP()

/***
 * <method name="findOrCreateFont">
 *  <function returns="@wxFont">
 *   <arg name="PointSize" type="Integer" />
 *   <arg name="Family" type="Integer" />
 *   <arg name="Style" type="Integer" />
 *   <arg name="Weight" type="Integer" />
 *   <arg name="Underline" type="Boolean" default="false" />
 *   <arg name="FaceName" type="String" default="" />
 *   <arg name="Encoding" type="@wxFontEncoding" default="wxFontEncoding.DEFAULT" /> 
 *  </function>
 *  <desc>
 *   Finds a font of the given specification, or creates one and adds it to the list.
 *   The following code shows an example:
 *   <code class="whjs">
 *    var font = wxTheFontList.findOrCreateFont(10, wxFont.DEFAULT, wxFont.NORMAL, wxFont.NORMAL);
 *   </code>
 *  </desc>
 * </method>
 */
JSBool FontList::findOrCreate(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	int point;
	int family;
	int style;
	int weight;
	bool underline = false;
	JSString *faceName = NULL;
	int encoding = wxFONTENCODING_DEFAULT;

	if ( JS_ConvertArguments(cx, argc, argv, "iiii/bSi", &point, &family, &style,
							 &weight, &underline, &faceName, &encoding) == JS_TRUE )
	{
		wxString face;
		if ( faceName == NULL )
		{
			face = wxEmptyString;
		}
		else
		{
			face = (wxChar *) JS_GetStringChars(faceName);
		}
        *rval = Font::CreateObject(cx, wxTheFontList->FindOrCreateFont(point, family, style,
													  weight, underline, face,
                                                      (wxFontEncoding) encoding));
		return JS_TRUE;
	}
	else
		return JS_FALSE;
}
