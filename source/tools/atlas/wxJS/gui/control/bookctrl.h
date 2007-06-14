#ifndef _WXJSBOOKCTRL_H
#define _WXJSBOOKCTRL_H

#include "../../common/evtconn.h"

#include <wx/bookctrl.h>

namespace wxjs
{
	namespace gui
	{
		class BookCtrlBase : public ApiWrapper<BookCtrlBase, wxBookCtrlBase>
		{
		public:
			static bool GetProperty(wxBookCtrlBase *p,
									JSContext *cx,
									JSObject *obj,
									int id,
									jsval *vp);
			static bool SetProperty(wxBookCtrlBase *p,
									JSContext *cx,
									JSObject *obj,
									int id,
									jsval *vp);

			WXJS_DECLARE_METHOD_MAP()
			WXJS_DECLARE_METHOD(addPage)
			WXJS_DECLARE_METHOD(deleteAllPages)
			WXJS_DECLARE_METHOD(getPage)

			WXJS_DECLARE_CONSTANT_MAP()
		
			WXJS_DECLARE_PROPERTY_MAP()
			enum
			{
				P_PAGE_COUNT
			};
		};
	}; // namespace gui
}; // namespace wxjs

#endif //_WXJSBOOKCTRL_H
