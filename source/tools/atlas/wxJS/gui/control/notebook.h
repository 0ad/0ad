#ifndef _WXJSNOTEBOOK_H
#define _WXJSNOTEBOOK_H

#include "../../common/evtconn.h"

#include <wx/notebook.h>

namespace wxjs
{
	namespace gui
	{
		class Notebook : public ApiWrapper<Notebook, wxNotebook>
		{
		public:
			static void InitClass(JSContext *cx,
								  JSObject *obj,
								  JSObject *proto);
			static bool AddProperty(wxNotebook *p,
									JSContext *cx,
									JSObject *obj,
									const wxString &prop,
									jsval *vp);
			static bool DeleteProperty(wxNotebook *p,
									   JSContext *cx,
									   JSObject *obj,
									   const wxString &prop);
			static bool GetProperty(wxNotebook *p,
									JSContext *cx,
									JSObject *obj,
									int id,
									jsval *vp);
			static bool SetProperty(wxNotebook *p,
									JSContext *cx,
									JSObject *obj,
									int id,
									jsval *vp);

			static wxNotebook* Construct(JSContext *cx,
										 JSObject *obj,
										 uintN argc,
										 jsval *argv,
										 bool constructing);

			WXJS_DECLARE_METHOD_MAP()
			WXJS_DECLARE_METHOD(create)

			WXJS_DECLARE_CONSTANT_MAP()
		
			WXJS_DECLARE_PROPERTY_MAP()
			enum
			{
				P_ROW_COUNT
			};
		};
		class NotebookEventHandler : public EventConnector<wxNotebook>
								   , public wxEvtHandler
		{
		public:
			void OnPageChanged(wxNotebookEvent &event);
			void OnPageChanging(wxNotebookEvent &event);
			static void InitConnectEventMap();
		private:
			static void ConnectPageChanged(wxNotebook *p, bool connect);
			static void ConnectPageChanging(wxNotebook *p, bool connect);
		};
	}; // namespace gui
}; // namespace wxjs

#endif //_WXJSNOTEBOOK_H
