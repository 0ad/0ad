#ifndef _WXJSSettings_H
#define _WXJSSettings_H

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
