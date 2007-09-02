#ifndef _WXJSSettings_H
#define _WXJSSettings_H

/*
 * wxJavaScript - settings.h
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
 * Remark: This class was donated by Philip Taylor
 *
 * $Id$
 */
namespace wxjs
{
	namespace gui
	{
		class SystemSettings : public ApiWrapper<SystemSettings, wxSystemSettings>
		{
		public:
			enum
			{
				P_SCREEN_TYPE
			};

			WXJS_DECLARE_CONSTANT_MAP()

			WXJS_DECLARE_STATIC_PROPERTY_MAP()
			static bool GetStaticProperty(JSContext *cx, int id, jsval *vp);
			static bool SetStaticProperty(JSContext *cx, int id, jsval *vp);

			WXJS_DECLARE_STATIC_METHOD_MAP()
			static JSBool getColour(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
			static JSBool getFont(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
			static JSBool getMetric(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
			static JSBool hasFeature(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
		};
	}; // namespace gui
}; // namespace wxjs

#endif //_WXJSSettings_H
