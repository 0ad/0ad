#ifndef _WXJSSpinEvent_H
#define _WXJSSpinEvent_H

#include <wx/spinctrl.h>

namespace wxjs
{
	namespace gui
	{
		typedef JSEvent<wxSpinEvent> PrivSpinEvent;
		class SpinEvent : public ApiWrapper<SpinEvent, PrivSpinEvent>
		{
		public:
			static bool GetProperty(PrivSpinEvent *p, JSContext *cx, JSObject *obj, int id, jsval *vp);
			static bool SetProperty(PrivSpinEvent *p, JSContext *cx, JSObject *obj, int id, jsval *vp);

			enum
			{
				P_POSITION
			};

			WXJS_DECLARE_PROPERTY_MAP()
		};
	}; // namespace gui
}; // namespace wxjs

#endif //_WXJSSpinEvent_H
