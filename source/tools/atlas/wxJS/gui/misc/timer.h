#ifndef _WXJSTimer_H
#define _WXJSTimer_H

namespace wxjs
{
	namespace gui
	{
		class Timer : public wxTimer
		            , public ApiWrapper<Timer, wxTimer>
		{
		public:
			static bool GetProperty(wxTimer *p, JSContext *cx, JSObject *obj, int id, jsval *vp);
			static bool SetProperty(wxTimer *p, JSContext *cx, JSObject *obj, int id, jsval *vp);
			static wxTimer *Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing);
			enum
			{
				P_INTERVAL
				, P_ONESHOT
				, P_RUNNING
			};

			static JSBool start(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
			static JSBool stop(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

			virtual void Notify();
			void OnWindowDestroy(wxWindowDestroyEvent &event);

			WXJS_DECLARE_PROPERTY_MAP()
			WXJS_DECLARE_METHOD_MAP()
		};
	}; // namespace gui
}; // namespace wxjs

#endif //_WXJSTimer_H
