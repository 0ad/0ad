#ifndef _WXJSNotebookEvent_H
#define _WXJSNotebookEvent_H

class wxNotebookEvent;

namespace wxjs
{
	namespace gui
	{
		typedef JSEvent<wxNotebookEvent> PrivNotebookEvent;
		class NotebookEvent : public ApiWrapper<NotebookEvent, PrivNotebookEvent>
		{
		public:
			static bool GetProperty(PrivNotebookEvent *p, JSContext *cx, JSObject *obj, int id, jsval *vp);
			static bool SetProperty(PrivNotebookEvent *p, JSContext *cx, JSObject *obj, int id, jsval *vp);

			enum
			{
				P_OLD_SELECTION
				, P_SELECTION
			};

			WXJS_DECLARE_PROPERTY_MAP()
		};
	}; // namespace gui
}; // namespace wxjs

#endif //_WXJSNotebookEvent_H
