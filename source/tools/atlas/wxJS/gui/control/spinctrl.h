#ifndef _WXJSSPINCTRL_H
#define _WXJSSPINCTRL_H

#include "../../common/evtconn.h"

#include <wx/spinctrl.h>

namespace wxjs
{
	namespace gui
	{
		class SpinCtrl : public ApiWrapper<SpinCtrl, wxSpinCtrl>
		{
		public:
			static void InitClass(JSContext* cx,
								  JSObject* obj,
								  JSObject* proto);
			static bool AddProperty(wxSpinCtrl *p,
									JSContext *cx,
									JSObject *obj,
									const wxString &prop,
									jsval *vp);
			static bool DeleteProperty(wxSpinCtrl *p,
									JSContext* cx,
									JSObject* obj,
									const wxString &prop);
			static bool GetProperty(wxSpinCtrl *p,
									JSContext *cx,
									JSObject *obj,
									int id,
									jsval *vp);
			static bool SetProperty(wxSpinCtrl *p,
									JSContext *cx,
									JSObject *obj,
									int id,
									jsval *vp);

			static wxSpinCtrl* Construct(JSContext *cx,
										 JSObject *obj,
										 uintN argc,
										 jsval *argv,
										 bool constructing);
			
			WXJS_DECLARE_METHOD_MAP()
			WXJS_DECLARE_METHOD(create)
			WXJS_DECLARE_METHOD(setRange)
			WXJS_DECLARE_METHOD(setSelection)

			WXJS_DECLARE_CONSTANT_MAP()
		
			WXJS_DECLARE_PROPERTY_MAP()
			enum
			{
				P_VALUE
				, P_MIN
				, P_MAX
			};
		};

		class SpinCtrlEventHandler : public EventConnector<wxSpinCtrl>
								   , public wxEvtHandler
		{
		public:
		// Events
			void OnText(wxCommandEvent &event);
			void OnSpinCtrl(wxSpinEvent &event);
			void OnSpin(wxSpinEvent &event);
			void OnSpinUp(wxSpinEvent &event);
			void OnSpinDown(wxSpinEvent &event);
			static void InitConnectEventMap();
		private:
			static void ConnectText(wxSpinCtrl *p, bool connect);
			static void ConnectSpinCtrl(wxSpinCtrl *p, bool connect);
			static void ConnectSpin(wxSpinCtrl *p, bool connect);
			static void ConnectSpinUp(wxSpinCtrl *p, bool connect);
			static void ConnectSpinDown(wxSpinCtrl *p, bool connect);
		};
	}; // namespace gui
}; // namespace wxjs

#endif //_WXJSSPINCTRL_H
